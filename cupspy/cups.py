#! /usr/bin/python
#
# Copyright (C) 2009  Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# Originally written by John Collins <jmc@xisl.com>.

import filebuf, ipp, conf, subprocess, sys, os, stat, socket, threading, re, time, signal, syslog, errno

# Get us a job id in case gspl-pr doesn't give one

Next_jobid = os.getpid()
Jobid_lock = threading.Lock()

# Thread-safe get job ID

def get_next_jobid():
    """Get next job id"""
    Jobid_lock.acquire()
    result = Next_jobid
    Next_jobid += 1
    Jobid_lock.release()
    return result

# Set up default attribute values

default_attr_values = {'attributes-charset': 'utf-8',
                       'attributes-natural-language': 'en-gb',
                       'copies': 1,
                       'job-name': 'No Title'}

# List of attributes to include in description

printer_description_atts = ["charset-configured",
                            "charset-supported",
                            "color-supported",
                            "compression-supported",
                            "document-format-default",
                            "document-format-supported",
                            "generated-natural-language-supported",
                            "ipp-versions-supported",
                            "job-impressions-supported",
                            "job-k-octets-supported",
                            "job-media-sheets-supported",
                            "multiple-document-jobs-supported",
                            "multiple-operation-time-out",
                            "natural-language-configured",
                            "notify-attributes-supported",
                            "notify-lease-duration-default",
                            "notify-lease-duration-supported",
                            "notify-max-events-supported",
                            "notify-events-default",
                            "notify-events-supported",
                            "notify-pull-method-supported",
                            "notify-schemes-supported",
                            "operations-supported",
                            "pages-per-minute",
                            "pages-per-minute-color",
                            "pdl-override-supported",
                            "printer-alert",
                            "printer-alert-description",
                            "printer-current-time",
                            "printer-driver-installer",
                            "printer-info",
                            "printer-is-accepting-jobs",
                            "printer-location",
                            "printer-make-and-model",
                            "printer-message-from-operator",
                            "printer-more-info",
                            "printer-more-info-manufacturer",
                            "printer-name",
                            "printer-state",
                            "printer-state-message",
                            "printer-state-reasons",
                            "printer-up-time",
                            "printer-uri-supported",
                            "queued-job-count",
                            "reference-uri-schemes-supported",
                            "uri-authentication-supported",
                            "uri-security-supported"]

# List of attributes if none asked

printer_default_atts = ["copies-default",
                        "document-format-default",
                        "finishings-default",
                        "job-hold-until-default",
                        "job-priority-default",
                        "job-sheets-default",
                        "media-default",
                        "number-up-default",
                        "orientation-requested-default",
                        "sides-default"]

# Parse config file
# Use argument if one given, otherwise look for "cupspy.conf"
try:
    Config_data = conf.Conf()
    Cfile = 'cupspy.conf'
    if len(sys.argv) > 1:
        Cfile = sys.argv[1]
    Config_data.parse_conf_file(Cfile)
except conf.ConfError, msg:
    sys.exit(msg.args[0])

# Now for the meat of the stuff

class CupsError(Exception):
    def __init__(self, code, msg):
        Exception.__init__(self, msg)
        self.codename = code

# Util routines

def copy_or_default(req, name):
    """Copy field from request or supply default"""
    val = req.find_attribute(name)
    if val:
        res = val.get_value()
    else:
        res = default_attr_values[name]
    return  res

def set_copy_or_default(req, val, name):
    """Initialise variable with copy or default"""
    val.setnamevalues(name, copy_or_default(req, name))
    return  val

def opgrp_hdr(req):
    """Return a OPERATION group with standard header fields set mostly from request"""
    opgrp = ipp.ippgroup(ipp.IPP_TAG_OPERATION)
    charset = ipp.ippvalue(ipp.IPP_TAG_CHARSET)
    opgrp.setvalue(set_copy_or_default(req, charset, 'attributes-charset'))
    lang = ipp.ippvalue(ipp.IPP_TAG_LANGUAGE)
    opgrp.setvalue(set_copy_or_default(req, lang, 'attributes-natural-language'))
    return opgrp

# Report op done OK (mostly after we haven't done anything)

def ipp_ok(req):
    """Report that everything went OK with IPP request
(Even if nothing was done)"""
    syslog.syslog(syslog.LOG_NOTICE, "No-op OK response to op %s" % ipp.op_to_name[req.statuscode])
    response = ipp.ipp()
    response.setrespcode(ipp.IPP_OK, req.id)
    response.pushvalue(opgrp_hdr(req))
    return  response.generate()

# Report status message back to caller

def ipp_status(req, code, msg):
    """Deliver a status message back to sender"""
    syslog.syslog(syslog.LOG_WARNING, "Op code %s returning result %s - %s" % (ipp.op_to_name[req.statuscode], code, msg))
    response = ipp.ipp()
    response.setrespcode(code, req.id)  # If code is a string it gets converted
    opgrp = opgrp_hdr(req)
    msgv = ipp.ippvalue(ipp.IPP_TAG_TEXT)
    msgv.setnamevalues('status-message', msg)
    opgrp.setvalue(msgv)
    response.pushvalue(opgrp)
    return  response.generate()

# Grab various fields from requests

def get_pname_from_uri(req):
    """Extract printer name from uri"""

    pname = req.find_attribute('printer-uri')
    if not pname:
        raise CupsError('IPP_BAD_REQUEST', 'Missing required printer-uri')

    # Get the actual string
    # Strip off the URI stuff to get the printer name

    pname = pname.get_value()
    m = re.match(".*/(.*)", pname)
    if not m:
        raise CupsError('IPP_BAD_REQUEST', 'Cannot determine printer name from URI')
    pname = m.group(1)

    if pname not in Config_data.printers:
        raise CupsError('IPP_NOT_FOUND', 'The printer ' + pname + ' was not found')
    return pname

def get_req_user_name(req):
    """Extract requesting user name"""

    user = req.find_attribute('requesting-user-name')
    if not user:
	raise CupsError('IPP_BAD_REQUEST', 'No requesting user')
    user = user.get_value()
    return user

# The following operations are "done" by just returning OK
# We might want to expand them to log that they have been attempted

def validate_job(req):
    return ipp_ok(req)
def cancel_job(req):
    return ipp_ok(req)
def get_jobs(req):
    return ipp_ok(req)
def hold_job(req):
    return ipp_ok(req)
def release_job(req):
    return ipp_ok(req)
def restart_job(req):
    return ipp_ok(req)
def pause_printer(req):
    return ipp_ok(req)
def resume_printer(req):
    return ipp_ok(req)
def purge_jobs(req):
    return ipp_ok(req)
def set_printer_attributes(req):
    return ipp_ok(req)
def set_job_attributes(req):
    return ipp_ok(req)
def create_printer_subscription(req):
    return  ipp_ok(req)
def create_job_subscription(req):
    return  ipp_ok(req)
def renew_subscription(req):
    return  ipp_ok(req)
def cancel_subscription(req):
    return  ipp_ok(req)
def enable_printer(req):
    return  ipp_ok(req)
def disable_printer(req):
    return  ipp_ok(req)
def pause_printer_after_current_job(req):
    return  ipp_ok(req)
def hold_new_jobs(req):
    return  ipp_ok(req)
def release_held_new_jobs(req):
    return  ipp_ok(req)
def deactivate_printer(req):
    return ipp_ok(req)
def activate_printer(req):
    return ipp_ok(req)
def restart_printer(req):
    return ipp_ok(req)
def shutdown_printer(req):
    return ipp_ok(req)
def startup_printer(req):
    return ipp_ok(req)
def reprocess_job(req):
    return ipp_ok(req)
def cancel_current_job(req):
    return ipp_ok(req)
def suspend_current_job(req):
    return ipp_ok(req)
def resume_job(req):
    return ipp_ok(req)
def promote_job(req):
    return ipp_ok(req)
def schedule_job_after(req):
    return ipp_ok(req)
def private(req):
    return ipp_ok(req)
def add_modify_printer(req):
    return ipp_ok(req)
def delete_printer(req):
    return ipp_ok(req)
def get_classes(req):
    return ipp_ok(req)
def add_modify_class(req):
    return ipp_ok(req)
def delete_class(req):
    return ipp_ok(req)
def accept_jobs(req):
    return ipp_ok(req)
def reject_jobs(req):
    return ipp_ok(req)
def set_default(req):
    return ipp_ok(req)
def move_job(req):
    return ipp_ok(req)
def authenticate_job(req):
    return ipp_ok(req)
def get_notifications(req):
    return ipp_ok(req)

# The next operations we return some kind of error message
# to tell the client we didn't do it.

def send_uri(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'send uri not implemented')
def get_job_attributes(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'get job attributes uri not implemented')
def get_printer_supported_values(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'get printer supported values not implemented')
def get_subscription_attributes(req):
    return  ipp_status(req, 'IPP_NOT_FOUND', 'Invalid notify-subscription-id')
def get_subscriptions(req):
    return  ipp_status(req, 'IPP_NOT_FOUND', 'No subscriptions found')
def send_notifications(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'send notifications not implemented')
def get_print_support_files(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'get print support files not implemented')
def get_devices(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'cups-driverd failed to execute')
def get_ppds(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'cups-driverd failed to execute')
def get_ppd(req):
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'cups-driverd failed to execute')

# This is where we actually have to do something for our money.
# First lets do the things with printers

################################################################################

def expand_multi_attributes(val):
    """Expand multiple attribute names

Currently these are printer-description and printer-defaults"""

    reslist = []
    for v in val.value:
        n = v[1]
        if n == "printer-description":
            for item in printer_description_atts:
                reslist.append(item)
        elif n == "printer-defaults":
            for item in printer_default_atts:
                reslist.append(item)
        else:
            reslist.append(n)
    return reslist

# Get printers gets us a list of printers.

def get_printers(req):
    """Get the list of printers and requested attributes"""

    # Get the list of requested attributes from the request
    req_al = req.find_attribute('requested-attributes')
    if not req_al:
        return  ipp_status(req, 'IPP_BAD_REQUEST', 'No requested-attributes')

    try:
        # Just get the requested attributes strings
        req_al = expand_multi_attributes(req_al)

        # Now get the list of available printers
        ptr_list = Config_data.list_printers()

        # Set up response

        response = ipp.ipp()
        response.setrespcode(ipp.IPP_OK, req.id)
        response.pushvalue(opgrp_hdr(req))

        for ptr in ptr_list:
            ptrgrp = ipp.ippgroup(ipp.IPP_TAG_PRINTER)
            ptratts = Config_data.get_config(ptr, req_al)
            for attitem in ptratts:
                name, typenum, attlist = attitem
                val = ipp.ippvalue(typenum)
                val.setnamevalues(name, attlist)
                ptrgrp.setvalue(val)
            response.pushvalue(ptrgrp)
        return  response.generate()

    except ValueError:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'Error in get_printers()')

################################################################################

def get_ptr_attributes(req, pname, al):
    """Get given attributes for given printer"""

    # Set up response

    response = ipp.ipp()
    response.setrespcode(ipp.IPP_OK, req.id)
    response.pushvalue(opgrp_hdr(req))

    # Printer group stuff

    ptrgrp = ipp.ippgroup(ipp.IPP_TAG_PRINTER)
    ptratts = Config_data.get_config(pname, al)

    # Add each attribute

    for attitem in ptratts:
        name, typenum, attlist = attitem
        val = ipp.ippvalue(typenum)
        val.setnamevalues(name, attlist)
        ptrgrp.setvalue(val)

    response.pushvalue(ptrgrp)
    return  response.generate()

def get_printer_attributes(req):
    """Get_printer_attributes request"""
    try:
        pname = get_pname_from_uri(req)
    except CupsError, err:
        return  ipp_status(req, err.codename, err.args[0])

    # Get the list of requested attributes from the request
    req_al = req.find_attribute('requested-attributes')
    if not req_al:
        return  ipp_status(req, 'IPP_BAD_REQUEST', 'No requested-attributes')

    try:
        return  get_ptr_attributes(req, pname, expand_multi_attributes(req_al))
    except:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'Error in get_printers()')

def get_default(req):
    """Get default printer and attributes"""
    try:
        pname = Config_data.default_printer()
        return  get_ptr_attributes(req, pname, Config_data.get_attnames(pname))
    except:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'Error in get_default()')

def print_error(req, pname, code, ste):
    """Generate return code from print"""

    msg = "%s gave exit code %d" % (pname, code)
    if len(ste) != 0:
            msg += " - " + ste
    return ipp_status(req, 'IPP_NOT_POSSIBLE', msg)

def print_job(req):
    """Actually print a job"""

    # Get printer name
    try:
        pname = get_pname_from_uri(req)
	uname = get_req_user_name(req)
    except CupsError, err:
        return  ipp_status(req, err.codename, err.args[0])

    title = copy_or_default(req, 'job-name')
    copies = copy_or_default(req, 'copies')

    # We are assuming that the content-length field gave us the whole lot

    command = Config_data.print_command(pname, copies=copies, title=title, user=uname)
    if command is None:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'No print command for ' + pname)

    # Now do the business

    outf = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stderr=subprocess.PIPE)

    # See if something went wrong at that point

    pl = outf.poll()
    if pl:
        (sto, ste) = outf.communicate()
        return print_error(req, pname, pl, ste)

    # Send the data

    sti = outf.stdin
    while 1:
        # If we get some kind of reception error do the best we can
        try:
            b = req.filb.getrem()
        except:
            outf.terminate()
            (sto, ste) = outf.communicate()
            raise

        # End of data is signified by zero length
        # Other errors raise filebufEOF

        if  len(b) == 0:
            break
        try:
            sti.write(b)
        except IOError, err:
            if err.args[0] == errno.EPIPE:
                return ipp_status(req, "IPP_NOT_POSSIBLE", "Is GNUspool running")
            else:
                msg = "Pipe IO Error: " + err.args[1] + "(" + err.args[0] + ")"
                return ipp_status(req, "IPP_NOT_POSSIBLE", msg)

        # Check it's still running

        pl = outf.poll()
        if  pl:
            (sto, ste) = outf.communicate()
            return print_error(req, pname, pl, ste)

    # Check we didn't get error message

    (sto, ste) = outf.communicate()
    pl = outf.wait()
    if pl:
        return print_error(req, pname, pl, ste)

    # Look for job id on std error and if present use it
    # Otherwise generate

    if len(ste) != 0:
        m = re.search("(\d+)", ste)
        if m:
            jobid = int(m.group(1))
        else:
            jobid = get_next_jobid()
    else:
        jobid = get_next_jobid()

    # Now generate OK response

    response = ipp.ipp()
    response.setrespcode(ipp.IPP_OK, req.id)
    opgrp = opgrp_hdr(req)
    msg = ipp.ippvalue(ipp.IPP_TAG_TEXT)
    msg.setnamevalues('status-message', 'successful-ok')
    opgrp.setvalue(msg)
    response.pushvalue(opgrp)

    jobgrp = ipp.ippgroup(ipp.IPP_TAG_JOB)
    idval = ipp.ippvalue(ipp.IPP_TAG_INTEGER)
    idval.setnamevalues('job-id', jobid)
    jobgrp.setvalue(idval)
    iduri = "ipp://%s/%s/%d" % (socket.gethostname(), pname, jobid)
    idval = ipp.ippvalue(ipp.IPP_TAG_URI)
    idval.setnamevalues('job-uri', iduri)
    jobgrp.setvalue(idval)
    jobstat = ipp.ippvalue(ipp.IPP_TAG_ENUM)
    jobstat.setnamevalues('job-state', ipp.IPP_JOB_PENDING)
    jobgrp.setvalue(jobstat)
    response.pushvalue(jobgrp)
    return response.generate()

# We believe that these don't happen

def print_uri(req):
    """Dont do print_uri as it doesnt seem to be used"""
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'print_uri not implemented')

def create_job(req):
    """Dont do create_job as it doesn't seem to be used"""
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'create_job not implemented')

def send_document(req):
    """Dont do send_document as it doesn't seem to be used"""
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'send_document not implemented')

# Lookup for operations

functab = dict(IPP_PRINT_JOB=print_job,
               IPP_PRINT_URI=print_uri,
               IPP_VALIDATE_JOB=validate_job,
               IPP_CREATE_JOB=create_job,
               IPP_SEND_DOCUMENT=send_document,
               IPP_SEND_URI=send_uri,
               IPP_CANCEL_JOB=cancel_job,
               IPP_GET_JOB_ATTRIBUTES=get_job_attributes,
               IPP_GET_JOBS=get_jobs,
               IPP_GET_PRINTER_ATTRIBUTES=get_printer_attributes,
               IPP_HOLD_JOB=hold_job,
               IPP_RELEASE_JOB=release_job,
               IPP_RESTART_JOB=restart_job,
               IPP_PAUSE_PRINTER=pause_printer,
               IPP_RESUME_PRINTER=resume_printer,
               IPP_PURGE_JOBS=purge_jobs,
               IPP_SET_PRINTER_ATTRIBUTES=set_printer_attributes,
               IPP_SET_JOB_ATTRIBUTES=set_job_attributes,
               IPP_GET_PRINTER_SUPPORTED_VALUES=get_printer_supported_values,
               IPP_CREATE_PRINTER_SUBSCRIPTION=create_printer_subscription,
               IPP_CREATE_JOB_SUBSCRIPTION=create_job_subscription,
               IPP_GET_SUBSCRIPTION_ATTRIBUTES=get_subscription_attributes,
               IPP_GET_SUBSCRIPTIONS=get_subscriptions,
               IPP_RENEW_SUBSCRIPTION=renew_subscription,
               IPP_CANCEL_SUBSCRIPTION=cancel_subscription,
               IPP_GET_NOTIFICATIONS=get_notifications,
               IPP_SEND_NOTIFICATIONS=send_notifications,
               IPP_GET_PRINT_SUPPORT_FILES=get_print_support_files,
               IPP_ENABLE_PRINTER=enable_printer,
               IPP_DISABLE_PRINTER=disable_printer,
               IPP_PAUSE_PRINTER_AFTER_CURRENT_JOB=pause_printer_after_current_job,
               IPP_HOLD_NEW_JOBS=hold_new_jobs,
               IPP_RELEASE_HELD_NEW_JOBS=release_held_new_jobs,
               IPP_DEACTIVATE_PRINTER=deactivate_printer,
               IPP_ACTIVATE_PRINTER=activate_printer,
               IPP_RESTART_PRINTER=restart_printer,
               IPP_SHUTDOWN_PRINTER=shutdown_printer,
               IPP_STARTUP_PRINTER=startup_printer,
               IPP_REPROCESS_JOB=reprocess_job,
               IPP_CANCEL_CURRENT_JOB=cancel_current_job,
               IPP_SUSPEND_CURRENT_JOB=suspend_current_job,
               IPP_RESUME_JOB=resume_job,
               IPP_PROMOTE_JOB=promote_job,
               IPP_SCHEDULE_JOB_AFTER=schedule_job_after,
               IPP_PRIVATE=private,
               CUPS_GET_DEFAULT=get_default,
               CUPS_GET_PRINTERS=get_printers,
               CUPS_ADD_MODIFY_PRINTER=add_modify_printer,
               CUPS_DELETE_PRINTER=delete_printer,
               CUPS_GET_CLASSES=get_classes,
               CUPS_ADD_MODIFY_CLASS=add_modify_class,
               CUPS_DELETE_CLASS=delete_class,
               CUPS_ACCEPT_JOBS=accept_jobs,
               CUPS_REJECT_JOBS=reject_jobs,
               CUPS_SET_DEFAULT=set_default,
               CUPS_GET_DEVICES=get_devices,
               CUPS_GET_PPDS=get_ppds,
               CUPS_MOVE_JOB=move_job,
               CUPS_AUTHENTICATE_JOB=authenticate_job,
               CUPS_GET_PPD=get_ppd)

# Process a get request which is usually for a PPD

def process_get(f, l):
    """Process a http get request"""
    m = re.match("GET\s+/printers/(.+)\s+HTTP", l)
    if not m:
        syslog.syslog(syslog.LOG_ERR, "Get request no match in %s" % l)
        return
    pf = None
    for ptr in (m.group(1), Config_data.default_printer() + ".ppd", "Default.ppd"):
        try:
            fn = os.path.join(Config_data.ppddir, ptr)
            pf = open(fn, "rb")
            if Config_data.loglevel > 2:
                syslog.syslog(syslog.LOG_INFO, "Opened .ppd file %s" % fn)
            break
        except:
            pass

    if not pf:
        syslog.syslog(syslog.LOG_ERR, "Get request file not found for %s" % m.group(1))
        return

    st = os.fstat(pf.fileno())
    f.write("HTTP/1.1 200 OK\n")
    f.write("Date: %s\n" % time.ctime())
    f.write("Server: CUPS/1.2\n")
    f.write("Connection: Keep-Alive\n")
    f.write("Keep-Alive: timeout=60\n")
    f.write("Content-Language: en_US\n")
    f.write("Content-Type: application/vnd.cups-ppd\n")
    f.write("Last-Modified: %s\n" % time.ctime(st[stat.ST_MTIME]))
    f.write("Content-Length: %d\n\n" % st[stat.ST_SIZE])
    f.flush()
    ffno = f.fileno()
    fsize = 0
    while 1:
        buf = pf.read(1024)
        lb = len(buf)
        if lb == 0:
            break
        fsize = fsize + lb
        # Do write taking into account everything doesn't get
        # written at once
        while lb > 0:
            lw = os.write(ffno, buf)
            buf = buf[lw:]
            lb -= lw
    pf.close()
    if Config_data.loglevel > 2:
        syslog.syslog(syslog.LOG_INFO, ".ppd file was %d bytes long" % fsize)
    

def process_post(f):
    """Process an HTTP POST request"""
    ipplength=0
    while 1:
        line = f.readline()
        l = line.rstrip()
        if  len(l) == 0:
            break
        m = re.search('Content-Length:\s*(\d+)', l, re.I)
        if m:
            ipplength=int(m.group(1))
    if ipplength==0:
        return None
    return  filebuf.filebuf(f, ipplength)

def parsefd(f):
    """Parse an incoming request with HTTP header"""
    # Get first line which should be either HTTP request or just the start of a header

    try:
        line = f.readline()
        l = line.strip()
        if len(l) == 0:
            return None
        m = re.match("(GET|PUT|POST|DELETE|TRACE|OPTIONS|HEAD)\s", l)
        if m:
            op = m.group(1)
            if op == "GET":
                process_get(f, l)
                return None
            elif op == "POST":
                return process_post(f)
            else:
                print "Dont know how to handle " + op + " http ops"
        else:
            print "No match for HTTP op"
    except (socket.error, filebuf.filebufEOF):
        return None
    return None

def parsefile(fname):
    """Parse a file with an IPP in"""
    try:
        f = open(fname, 'rb')
    except IOError:
        return None
    res = parsefd(f)
    f.close()
    return  res

# Do the business in a thread

def cups_proc(conn, addr):
    """Process a connection and handle request"""
    f = conn.makefile("r+")
    conn.close()

    # parsefd returns None if anything other than an IPP request,
    # in which case it returns a filebuf

    fb = parsefd(f)
    if not fb:
        f.close()
        return

    # Parse IPP request
    ipr = ipp.ipp(fb)
    try:
        ipr.parse()
    except ipp.IppError, mm:
        f.close()
        return

    iprcode = ipr.statuscode

    try:
        iprname = ipp.op_to_name[iprcode]
        syslog.syslog(syslog.LOG_INFO, "Request operation %s" % iprname)
        func = functab[iprname]
    except KeyError:
        f.close()
        return

    # Actually do the business and return an IPP string (filebuf is a structure member)

    resp = func(ipr)
    if  Config_data.loglevel > 2:
        ipresp = ipp.ipp(filebuf.stringbuf(resp, 0))
        ipresp.parse()
        syslog.syslog(syslog.LOG_INFO, "Reply sent %s" % ipp.resp_to_name[ipresp.statuscode])

    # Now decorate the response

    f.write("HTTP/1.1 200 OK\r\n")
    f.write("Date: %s\n" % time.ctime())
    f.write("Server: CUPS/1.2\r\n")
    f.write("Connection: Keep-Alive\r\n")
    f.write("Keep-Alive: timeout=60\r\n")
    f.write("Content-Language: en_US\r\n")
    f.write("Content-Type: application/ipp\r\n")
    f.write("Content-Length: %d\r\n\r\n" % len(resp))
    f.write(resp)
    f.close()

# Meat of the thing

def cups_server():
    """Accept connections on socket from people wanting to print stuff"""
    s = None
    port = socket.getservbyname('ipp', 'tcp')

    # Try to support ipv6 and ipv4

    for res in socket.getaddrinfo(None, port, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
        af, socktype, proto, canonname, sa = res
        try:
            s = socket.socket(af, socktype, proto)
        except socket.error, msg:
            s = None
            continue
        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind(sa)
            s.listen(5)
        except socket.error, msg:
            if msg.args[0] == errno.EACCES:
                sys.exit("Cannot access socket - no permission")
            if msg.args[0] == errno.EADDRINUSE:
                sys.exit("Socket in use - is CUPS running?")
            s.close()
            s = None
            continue
        break
    if s is None:
        sys.exit("Sorry cannot start - unable to allocate socket?")
    while 1:
        connaddr = s.accept()
        th = threading.Thread(target=cups_proc, args=connaddr)
        th.daemon = True
        th.start()

def setup_pid():
    """Setup pid entry in /var/run/cups/cupsd.pid

Kill anything that's there already"""
    dirname = "/var/run/cups"
    file = dirname + "/cupsd.pid"
    try:
        st = os.stat(file)
        if not stat.S_ISREG(st[stat.ST_MODE]):
            sys.exit("Unknown type file " + file)
        f = open(file, "rb")
        pidstr = f.read(100)
        f.close()
        os.kill(int(pidstr), signal.SIGTERM)
        time.sleep(5)
        os.remove(file)
    except (OSError, IOError, ValueError):
        pass

    # Create directory if needed
    try:
        os.mkdir(dirname, 0755)
    except OSError, err:
        if err.args[0] != errno.EEXIST:
            sys.exit("Cannot create " + dirname + " " + err.args[1])
    try:
        f = open(file, "wb")
        f.write(str(os.getpid()) + "\n")
        f.close()
    except IOError, err:
        sys.exit("Cannot create " + file + " - " + err.args[1])

# Run the stuff

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGQUIT, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    syslog.openlog('cupspy', syslog.LOG_NDELAY|syslog.LOG_PID, syslog.LOG_LPR)
    syslog.setlogmask(syslog.LOG_UPTO(Config_data.loglevel + syslog.LOG_ERR))
    syslog.syslog(syslog.LOG_INFO, "Starting")
    if os.fork() != 0:
        os._exit(0)
    setup_pid()
    cups_server()
