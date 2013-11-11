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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# Originally written by John Collins <jmc@xisl.com>.

import filebuf
import ipp
import conf
import subprocess
import sys
import os
import pwd
import stat
import socket
import select
import threading
import re
import time
import signal
import string
import syslog
import errno
import attrib_tools

# Get us a job id in case gspl-pr doesn't give one

Next_jobid = os.getpid()
Jobid_lock = threading.Lock()

Var_run = "/var/run/cups"
Procid_file = "cupsd.pid"

def exit_msg(msg, code):
    """Exit with message to stderr and defined exit code"""
    sys.stdout = sys.stderr
    print msg
    sys.exit(code)

# Thread-safe get job ID

def get_next_jobid():
    """Get next job id"""
    global Next_jobid
    Jobid_lock.acquire()
    result = Next_jobid
    Next_jobid += 1
    Jobid_lock.release()
    return result

Pending_jobs = dict()

class pend_job:
    """Hold details of pending job"""

    def __init__(self, p, u, t, c):
        global Pending_jobs
        self.jobnum = get_next_jobid()
        self.pname = p
        self.uname = u
        self.title = t
        self.copies = c
        Pending_jobs[self.jobnum] = self

# Set up default attribute values

default_attr_values = {'attributes-charset': 'utf-8',
                       'attributes-natural-language': 'en-gb',
                       'copies': 1,
                       'job-name': 'No Title'}

# Parse config file
# Use argument if one given, otherwise look for "cupspy.conf"
# We are of course doing this from the main path so any exit codes will
# be returned.

try:
    Config_data = conf.Conf()
    Cfile = 'cupspy.conf'
    if len(sys.argv) > 1:
        Cfile = sys.argv[1]

    # If the config file doesnt exist, see if we can find where
    # it lives from the path to me (usually /etc/cups/cupspy)
    # This is to avoid having to chdir before we enter to suit
    # startup programs

    if not os.path.exists(Cfile) and not os.path.isabs(Cfile):
        prog = sys.argv[0]
        if os.path.isabs(prog):
            try:
                os.chdir(os.path.dirname(prog))
            except OSError:
                exit_msg("Cannot change to directory of " + prog, 11)

    # Now parse file

    Config_data.parse_conf_file(Cfile)

except conf.ConfError as msg:
    exit_msg(msg.args[0], msg.args[1])

Printer_list = Config_data.list_printers()
Log_level = Config_data.log_level()

# Now for the meat of the stuff

class CupsError(Exception):
    def __init__(self, code, msg):
        Exception.__init__(self, msg)
        self.codename = code

# Util routines

def copy_or_default(req, name, blank_def = False):
    """Copy field from request or supply default"""
    val = req.find_attribute(name)
    if val:
        res = val.get_value()
        if blank_def:
            res = res.strip()
            if len(res) == 0: res = default_attr_values[name]
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
    lmsg = "No-op OK response to op %s" % ipp.op_to_name[req.statuscode]
    syslog.syslog(syslog.LOG_NOTICE, lmsg)
    response = ipp.ipp(request=req)
    response.setrespcode(ipp.IPP_OK, req.id)
    response.pushvalue(opgrp_hdr(req))
    return  response.generate()

# Report status message back to caller

def ipp_status(req, code, msg):
    """Deliver a status message back to sender"""
    lmsg = "Op code %s returning result %s - %s" % (ipp.op_to_name[req.statuscode], code, msg)
    syslog.syslog(syslog.LOG_WARNING, lmsg)
    response = ipp.ipp(request=req)
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

    if pname not in Printer_list:
        raise CupsError('IPP_NOT_FOUND', 'The printer ' + pname + ' was not found')
    return pname

def get_req_user_name(req):
    """Extract requesting user name"""
    user = req.find_attribute('requesting-user-name')
    if not user:
        raise CupsError('IPP_BAD_REQUEST', 'No requesting user')
    user = user.get_value().lower()
    try:
        user = conf.Usermap[user]
    except KeyError:
        try:
            pwd.getpwnam(user)
        except KeyError:
            return Config_data.default_user()
    return user

# The following operations are "done" by just returning OK
# We might want to expand them to log that they have been attempted

def validate_job(req):
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
    return ipp_status(req, 'IPP_OPERATION_NOT_SUPPORTED', 'IPP_PRIVATE not supported!')
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
# Get notifications seems to be required in some cases
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

# Get printers gets us a list of printers.

def get_printers(req):
    """Get the list of printers and requested attributes"""

    # Get the list of requested attributes from the request
    req_al = req.find_attribute('requested-attributes')

    try:
        # Just get the requested attributes strings
        if req_al:
            req_al = attrib_tools.expand_multi_attributes(req_al)

        # Now get the list of available printers
        ptr_list = Config_data.list_printers()

        # Set up response

        response = ipp.ipp(request=req)
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

    response = ipp.ipp(request=req)
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

def get_attributes_for_ptr(req, pname):
    """Get attributes for specified printer"""

    # Get the list of requested attributes from the request
    req_al = req.find_attribute('requested-attributes')
    if req_al:
        req_al = attrib_tools.expand_multi_attributes(req_al)
    else:
        # No attributes list in request.
        # We behave as if there was just one attribute group specified, 'all'
        req_al = attrib_tools.expand_attributes(["all"])
    try:
        return  get_ptr_attributes(req, pname, req_al)
    except:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'Error in get_printer_attributes()')


def get_printer_attributes(req):
    """Get_printer_attributes request"""
    try:
        pname = get_pname_from_uri(req)
    except CupsError as err:
        return  ipp_status(req, err.codename, err.args[0])
    return get_attributes_for_ptr(req, pname)

def get_default(req):
    """Get default printer and attributes"""
    try:
        pname = Config_data.default_printer()
        if len(pname) == 0:
            return ipp_ok(req)
        return get_attributes_for_ptr(req, pname)
    except:
        return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'Error in get_default()')

def created_job_ok(req, jobnum):
    """Return response that document was sent OK"""

    response = ipp.ipp(request=req)
    response.setrespcode(ipp.IPP_OK, req.id)
    opgrp = opgrp_hdr(req)
    msg = ipp.ippvalue(ipp.IPP_TAG_TEXT)
    msg.setnamevalues('status-message', 'successful-ok')
    opgrp.setvalue(msg)
    response.pushvalue(opgrp)

    jobgrp = ipp.ippgroup(ipp.IPP_TAG_JOB)
    idval = ipp.ippvalue(ipp.IPP_TAG_INTEGER)
    idval.setnamevalues('job-id', jobnum)
    jobgrp.setvalue(idval)
    iduri = "ipp://%s:631/jobs/%d" % (Config_data.serverip(), jobnum)
    idval = ipp.ippvalue(ipp.IPP_TAG_URI)
    idval.setnamevalues('job-uri', iduri)
    jobgrp.setvalue(idval)
    jobstat = ipp.ippvalue(ipp.IPP_TAG_ENUM)
    jobstat.setnamevalues('job-state', ipp.IPP_JOB_PROCESSING)
    jobgrp.setvalue(jobstat)
    response.pushvalue(jobgrp)
    return response.generate()

class print_err(Exception):
    """Throw me if print error happens"""

    def __init__(self, c, ste, *args):
        Exception.__init__(self, *args)
        self.code = c
        self.stderr = ste

def process_print(req, pname, copies, title, user):
    """Perform the operation to do the print and return the job id.

Throw a print_error if something goes wrong"""

    command = Config_data.print_command(pname, copies=copies, title=title, user=user)
    if command is None:
        raise print_err('No print command for ' + pname)

    outf = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    pl = outf.poll()
    if pl:
        (sto, ste) = outf.communicate()
        raise print_err(pl, ste)

    # Send the data

    sti = outf.stdin

    while 1:
        # If we get some kind of reception error do the best we can
        try:
            b = req.filb.getrem()
        except:
            syslog.syslog(syslog.LOG_DEBUG, "Error reading in job")
            outf.terminate()
            (sto, ste) = outf.communicate()
            raise print_err(256, 'Input error on socket')

        # End of data is signified by zero length
        # Other errors raise filebufEOF

        if  len(b) == 0:
            break
        try:
            sti.write(b)
        except IOError as err:
            if err.args[0] == errno.EPIPE:
                raise print_err(err.args[0], "Is spooler running")
            else:
                msg = "Pipe IO Error: " + err.args[1] + "(" + err.args[0] + ")"
                raise print_err(err.args[0], msg)

        # Check it's still running

        pl = outf.poll()
        if  pl:
            (sto, ste) = outf.communicate()
            raise print_err(pl, ste)

    # Check we didn't get error message

    (sto, ste) = outf.communicate()
    pl = outf.wait()
    if pl:
        raise print_err(pl, ste)

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

    return jobid

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
    except CupsError as err:
        return  ipp_status(req, err.codename, err.args[0])

    title = copy_or_default(req, 'job-name', True)
    copies = copy_or_default(req, 'copies')

    try:
        jobid = process_print(req, pname, copies, title, uname)
    except print_err as err:
        return print_error(req, pname, err.code, err.stderr)
    return created_job_ok(req, jobid)

def extract_jobid(reqop):
    """Extract job id from a request (send-document or cancel job)"""
    # Spec says it might be given as a URI or as a job number (and printer name which we don't need)
    jobid = reqop.find_attribute('job-id')
    if jobid:
        jobid = jobid.get_value()
    else:
        juri = reqop.find_attribute('job-uri')
        if not juri:
            raise CupsError('IPP_BAD_REQUEST', 'Missing required job-id or job-uri')
        m = re.match("ipp://.*/.*/(\d+)", juri.get_value())
        if not m:
            raise CupsError('IPP_BAD_REQUEST', 'Invalid format job-uri ' + juri)
        jobid = m.group(1)
    # Convert jobid to numeric and look up
    try:
        jobid = int(jobid)
    except ValueError:
        raise CupsError('IPP_BAD_REQUEST', 'Invalid non-integer jobid ' + jobid)
    return jobid

# We believe that these don't happen

def print_uri(req):
    """Dont do print_uri as it doesnt seem to be used"""
    return  ipp_status(req, 'IPP_INTERNAL_ERROR', 'print_uri not implemented')

def create_job(req):
    """Create job routine now has to be implemented"""
    # Get printer name
    try:
        pname = get_pname_from_uri(req)
        uname = get_req_user_name(req)
    except CupsError as err:
        return  ipp_status(req, err.codename, err.args[0])

    pj = pend_job(pname, uname, copy_or_default(req, 'job-name', True), copy_or_default(req, 'copies'))
    return created_job_ok(req, pj.jobnum)

def send_document(req):
    """Send document to pending job created by create job"""

    # Find out the job number this is associated with

    reqop = req.values[0]
    jobid = extract_jobid(reqop)
    if jobid not in Pending_jobs:
        raise CupsError('IPP_NOT_FOUND', 'Job id not found in send-doc ' + str(jobid))
    pj = Pending_jobs[jobid]

    # If last document set, delete from pending

    lastdoc = reqop.find_attribute('last-document')
    if not lastdoc:
        raise CupsError('IPP_BAD_REQUEST', 'No last-doc attribute')
    if lastdoc.get_value():
        del Pending_jobs[jobid]
    try:
        process_print(req, pj.pname, pj.copies, pj.title, pj.uname)
    except print_err as err:
        return print_error(req, pj.pname, err.code, err.stderr)
    return sent_doc_ok(req, jobid)

def cancel_job(req):
    """Cancel job - dont worry if not pending"""
    reqop = req.values[0]
    jobid = extract_jobid(reqop)
    if jobid in Pending_jobs:
        del Pending_jobs[jobid]
    return ipp_ok(req)

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

    hadka = False
    kato = 60

    # First read up to end of request

    while 1:
        line = f.readline()
        if line is None:
            break
        line = line.rstrip()
        if len(line) == 0:
            break
        m = re.match('Connection:\s+Keep-Alive', l, re.I)
        if m:
            hadka = True
            continue

        m = re.match('Keep-Alive:\s+timeout=(\d+)', l, re.I)
        if m:
            kato = int(m.group(1))
            continue

    # Try to parse the starting line

    m = re.match("GET\s+/printers/(.+)\s+HTTP", l)
    if not m:
        msg = "Get request no match in %s" % l
        syslog.syslog(syslog.LOG_ERR, msg)
        return None

    # Try to open the file

    pf = None
    for ptr in (m.group(1), Config_data.default_printer() + ".ppd", "Default.ppd"):
        try:
            fn = os.path.join(Config_data.ppddir(), ptr)
            pf = open(fn, "rb")
            if Log_level > 2:
                msg = "Opened .ppd file %s" % fn
                syslog.syslog(syslog.LOG_INFO, msg)
            break
        except:
            pass

    if pf:
        st = os.fstat(pf.fileno())
        tim = time.ctime(st[stat.ST_MTIME])
        fsize = st[stat.ST_SIZE]
    else:
        tim = time.time()
        fsize = 0
        msg = "Get request file not found for %s" % m.group(1)
        syslog.syslog(syslog.LOG_ERR, msg)

    f.write("HTTP/1.1 200 OK\n")
    f.write("Date: %s\n" % time.ctime())
    f.write("Server: %s\n" % Config_data.parameters.servername)
    if hadka:
        f.write("Connection: Keep-Alive\n")
        f.write("Keep-Alive: timeout=%d\n" % kato)
    f.write("Content-Language: en_US\n")
    f.write("Content-Type: application/vnd.cups-ppd\n")
    f.write("Last-Modified: %s\n" % tim)
    f.write("Content-Length: %d\n\n" % fsize)

    if pf:
        while 1:
            buf = pf.read(1024)
            lb = len(buf)
            if lb == 0:
                break
            f.write(buf)
        pf.close()

    if hadka: return kato
    return None

def process_post(f):
    """Process an HTTP POST request"""

    ipplength=0
    exp = False
    chnk = False
    hadka = False
    kato = 60

    # Read rest of header

    while 1:
        line = f.readline()
        if line is None:
            return None

        l = line.rstrip()
        if  len(l) == 0:
            break

        m = re.match('Content-Length:\s*(\d+)', l, re.I)
        if m:
            ipplength=int(m.group(1))
            continue

        m = re.match('Expect:', l, re.I)
        if m:
            exp = True
            continue

        m = re.match('Transfer-Encoding:\s+chunked', l, re.I)
        if m:
            chnk = True
            continue

        m = re.match('Connection:\s+Keep-Alive', l, re.I)
        if m:
            hadka = True
            continue

        m = re.match('Keep-Alive:\s+timeout=(\d+)', l, re.I)
        if m:
            kato = int(m.group(1))
            continue

    # If we've had a request to request more, satisfy it

    if exp:
        f.write("HTTP/1.1 100 Continue\r\n\r\n")

    if chnk:
        f.set_chunked()
    else:
        syslog.syslog(syslog.LOG_DEBUG, "Expecting %d bytes" % ipplength)
        if ipplength == 0:
            syslog.syslog(syslog.LOG_ERR, "No POST data")
            return None
        f.set_expected(ipplength)

    # Parse IPP request

    ipr = ipp.ipp(f)
    try:
        ipr.parse()
        if Log_level > 3:
            ipr.display()
    except ipp.IppError as err:
        syslog.syslog(syslog.LOG_DEBUG, "Had IPP error: %s" % err.args[0])
        return None

    iprcode = ipr.statuscode

    try:
        iprname = ipp.op_to_name[iprcode]
        syslog.syslog(syslog.LOG_INFO, "Request operation %s" % iprname)
        func = functab[iprname]
    except KeyError:
        return None

    # Actually do the business and return an IPP string (filebuf is a structure member)

    resp = func(ipr)

    if  Log_level > 2:
        ipresp = ipp.ipp(filebuf.stringbuf(resp, 0))
        ipresp.parse()
        syslog.syslog(syslog.LOG_INFO, "Reply sent %s" % ipp.resp_to_name[ipresp.statuscode])

    # Now decorate the response

    f.write("HTTP/1.1 200 OK\r\n")
    f.write("Date: %s\n" % time.ctime())
    f.write("Server: %s\r\n" % Config_data.parameters.servername)
    if hadka:
        f.write("Connection: Keep-Alive\r\n")
        f.write("Keep-Alive: timeout=%d\r\n" % kato)
    f.write("Content-Language: en_US\r\n")
    f.write("Content-Type: application/ipp\r\n")
    f.write("Content-Length: %d\r\n\r\n" % len(resp))
    f.write(resp)
    if hadka: return kato
    return None

def parsefd(fd):
    """Parse incoming requests with HTTP header"""

    while 1:

        # Get first line which should be an HTTP request

        try:
            line = fd.readline()
            if line is None:
                return
            l = line.strip()
            # Ignore blank lines
            if len(l) == 0: continue

            m = re.match("(GET|PUT|POST|DELETE|TRACE|OPTIONS|HEAD)\s", l)
            if m:
                op = m.group(1)
                if op == "GET": kato = process_get(fd, l)
                elif op == "POST": kato = process_post(fd)
                else:
                    syslog.syslog(syslog.LOG_ERR, "Dont know how to handle %s http ops" % op)
                    return
                if kato is None: return
            else:
                syslog.syslog(syslog.LOG_ERR, "No match for HTTP op %s" % l.translate(None, string.join([chr(x) for x in range(0,256) if x<32 or x>126],'')))
                return
        except (socket.error, filebuf.filebufEOF):
            syslog.syslog(syslog.LOG_DEBUG, "Had EOF on socket")
            return

# Do the business in a thread

def cups_proc(conn, addr):
    """Process a connection and handle request"""
    fr = addr[0]
    syslog.syslog(syslog.LOG_INFO, "Connection from address %s" % fr)
    fd = filebuf.httpfilebuf(conn, Config_data.timeout_value())
    parsefd(fd)
    conn.close()
    syslog.syslog(syslog.LOG_INFO, "Closed connection from %s" % fr)

# Meat of the thing

def cups_server():
    """Accept connections on socket from people wanting to print stuff"""
    s = None
    port = socket.getservbyname('ipp', 'tcp')

    # Try to support ipv6 and ipv4

    socklist = []
    poss_socks = socket.getaddrinfo(None, port, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE)
    act_socks = []

    # First find AF_INET6 if it's there
    # Unlike the advertising it might be anywhere
    # Then find AF_INET

    for afam in (socket.AF_INET6, socket.AF_INET):
        for p in poss_socks:
            af, socktype, proto, canonname, sa = p
            if af == afam:
                act_socks.append(p)
                break

    for p in act_socks:
        af, socktype, proto, canonname, sa = p
        try:
            s = socket.socket(af, socktype, proto)

        except socket.error as msg:
            continue

        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            if af == socket.AF_INET6:
                s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
            s.bind(sa)
            s.listen(5)

        except socket.error as msg:
            if msg.args[0] == errno.EACCES:
                exit_msg("Cannot access socket - no permission", 10)

            # We may get EADDRINUSE from IPV4 if we've just set up IPV6 which we ignore
            if msg.args[0] == errno.EADDRINUSE and len(socklist) == 0 and af != socket.AF_INET:
                exit_msg("Socket in use - is CUPS running?", 11)

            s.close()
            continue

        socklist.append(s)

    # If no success, abort
    if len(socklist) == 0:
        exit_msg("Sorry cannot start - unable to allocate socket?", 12)

    while 1:
        rlist,wlist,xlist = select.select(socklist,(),())
        for s in rlist:
            connaddr = s.accept()
            th = threading.Thread(target=cups_proc, args=connaddr)
            th.daemon = True
            th.start()

def setup_pid():
    """Setup pid entry in /var/run/cups/cupsd.pid

Kill anything that's there already"""
    file = Var_run + "/" + Procid_file
    try:
        st = os.stat(file)
        if not stat.S_ISREG(st[stat.ST_MODE]):
           exit_msg("Unknown type file " + file, 20)
        f = open(file, "rb")
        pidstr = f.read(100)
        f.close()
        pid = int(pidstr)
        os.kill(pid, signal.SIGTERM)
        time.sleep(5)
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        os.remove(file)
    except (OSError, IOError, ValueError):
        pass

    # Create directory if needed
    try:
        os.mkdir(Var_run, 0755)
    except OSError as err:
        if err.args[0] != errno.EEXIST:
            exit_msg("Cannot create " + Var_run + " " + err.args[1], 21)
    try:
        f = open(file, "wb")
        f.write(str(os.getpid()) + "\n")
        f.close()
    except IOError as err:
        exit_msg("Cannot create " + file + " - " + err.args[1], 22)

def remove_pid(signum, frame):
    """Remove pid entry in /var/run/cups/cupsd.pid"""
    try:
        os.remove(Var_run + '/' + Procid_file)
    except OSError:
        # Ignore errors
        pass
    exit_msg("CUPSPY terminated by signal", 200)

# Run the stuff

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGQUIT, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    if os.fork() != 0:
        os._exit(0)
    syslog.openlog('cupspy', syslog.LOG_NDELAY|syslog.LOG_PID, syslog.LOG_LPR)
    syslog.setlogmask(syslog.LOG_UPTO(Log_level + syslog.LOG_ERR))
    syslog.syslog(syslog.LOG_INFO, "Starting")
    setup_pid()
    signal.signal(signal.SIGTERM, remove_pid)
    cups_server()
