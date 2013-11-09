#! /usr/bin/python
#
# Copyright (C) 2011  Free Software Foundation
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

try:
    import argparse
    Have_argparse = True
except ImportError:
    import optparse
    Have_argparse = False
import conf
import subprocess
import sys
import os
import os.path
import re
import string
import time

def exit_msg(msg, code):
    """Exit with message to stderr and defined exit code"""
    sys.stdout = sys.stderr
    print msg
    sys.exit(code)

class possptr:
    """Represent possible printer we might want to use"""

    def __init__(self, p, f, c):
        self.form = f
        if string.find(p, ':') >= 0:
            self.host, self.ptr = string.split(p, ':', 1)
        else:
            self.host = None
            self.ptr = p
        self.comment = c

    def islocal(self):
        """True if printer is local to the computer"""
        return  self.host is None

# Kick off if need be by relocating self to script location

if len(os.path.dirname(sys.argv[0])) != 0: os.chdir(os.path.dirname(sys.argv[0]))

if Have_argparse:

    # Here is argparse version of argument parsing in case we have it available

    parser = argparse.ArgumentParser(description='Administer cupspy printer - batch style', prog='init_cupspy')
    parser.add_argument('--version', action='version', version='%(prog)s 1.0 (c) Xi Software Ltd')
    parser.add_argument('-c', '--configfile', default='cupspy.conf', help='Configuration file')
    parser.add_argument('-o', '--output-file', help='Config file to output to if not source file')
    parser.add_argument('-p', '--printer-name', help='Printer to create/delete')
    parser.add_argument('-D', '--delete-printer', action='store_true', help='Delete the printer')
    parser.add_argument('-C', '--clone-from', help='Clone unspecified attributes from this printer')
    parser.add_argument('-q', '--queue-name', help='Xi-Text Queue name to use')
    parser.add_argument('-i', '--information', help='Information for new printer')
    parser.add_argument('-f', '--form-type', help='Form type for new printer')
    parser.add_argument('-n', '--no-check', action='store_true', help='Turn off checks')
    parser.add_argument('-N', '--no-restart', action='store_true', help='Do not restart CUPSPY')
    parser.add_argument('-I', '--initialise-only', action='store_true', help='Just initialise files')
    parser.add_argument('-d', '--set-default', action='store_true', help='Set printer as default')
    adict = vars(parser.parse_args())

    initialising = adict['initialise_only']
    cloning = adict['clone_from']
    deleting = adict['delete_printer']
    confin = adict['configfile']
    confout = adict['output_file']
    nocheck = adict['no_check']
    setdef = adict['set_default']
    cupsptr = adict['printer_name']
    gs_q = adict['queue_name']
    info = adict['information']
    form = adict['form_type']
    no_restart = adict['no_restart']

else:

    # Here is optparse version of argument parsing needed for Python 2.6 or earlier
    
    parser = optparse.OptionParser("%prog [options]", version="%prog 1.0 (c) Xi Software Ltd")
    parser.add_option('-c', '--configfile', action='store', type='string', dest='configfile', default='cupspy.conf', metavar="FILE", help='Configuration file')
    parser.add_option('-o', '--output-file', action='store', type='string', dest='output_file', metavar="FILE", help='Config file to output to if not source file')
    parser.add_option('-p', '--printer-name', action='store', type='string', dest='printer_name', metavar="NAME", help='Printer to create/delete')
    parser.add_option('-D', '--delete-printer', action='store_true', dest='delete_printer', default=False, help='Delete the printer')
    parser.add_option('-C', '--clone-from', action='store', type='string', dest='clone_from', metavar="NAME", help='Clone unspecified attributes from this printer')
    parser.add_option('-q', '--queue-name', action='store', type='string', dest='queue_name', metavar="NAME", help='Xi-Text Queue name to use')
    parser.add_option('-i', '--information', action='store', type='string', dest='information', metavar="DESCRIPTION", help='Information for new printer')
    parser.add_option('-f', '--form-type', action='store', type='string', dest='form_type', metavar='FORM', help='Form type for new printer')
    parser.add_option('-n', '--no-check', action='store_true', dest='no_check', default=False, help='Turn off checks')
    parser.add_option('-N', '--no-restart', action='store_true', dest='no_restart', default=False, help='Do not restart CUPSPY')
    parser.add_option('-I', '--initialise-only', action='store_true', dest='init_only', default=False, help='Just initialise files')
    parser.add_option('-d', '--set-default', action='store_true', dest='set_default', default=False, help='Set printer as default')
    (options, args) = parser.parse_args()
    initialising = options.init_only
    cloning = options.clone_from
    deleting = options.delete_printer
    confin = options.configfile
    confout = options.output_file
    nocheck = options.no_check
    setdef = options.set_default
    cupsptr = options.printer_name
    gs_q = options.queue_name
    info = options.information
    form = options.form_type
    no_restart = options.no_restart

# If we can't see the config file and it isn't absolute and we were found using an absolute path, swap to that

if not os.path.exists(confin) and not os.path.isabs(confin):
    prog = sys.argv[0]
    if os.path.isabs(prog):
	try:
	    os.chdir(os.path.dirname(prog))
	except OSError:
	    exit_msg("Cannot change to directory of " + prog, 11)

if initialising:
    if cloning is not None or deleting:
	exit_msg("Confused about action - initialise + other op?", 2)
    if confout is None: confout = confin
    if not nocheck and os.path.exists(confout):
	exit_msg("Will not overwrite existing " + confout, 23)
    try:
	configuration = conf.Conf()
	configuration.setup_defaults()
	configuration.write_config(confout)
	sys.exit(0)
    except conf.ConfError as err:
	sys.exit(err.args[0], err.args[1])

if cloning is not None and deleting:
    exit_msg("Confused about action - clone + delete?", 2)

if cupsptr is None:
    if gs_q is None or not deleting:
	exit_msg("Must give printer name", 1)

# If no Xi-Text printer name given, error, unless we are just setting the default

if gs_q is None:
    if setdef:
	if confout is None: confout = confin
	configuration = conf.Conf()
	try:
	    configuration.parse_conf_file(confin, False)
	    configuration.set_default_printer(cupsptr)
	    configuration.write_config(confout)
	    sys.exit(0)
	except conf.ConfError as err:
	    exit_msg(err.args[0], err.args[1])
    elif not deleting:
	exit_msg("Must give queue name", 1)

# Get ourselves a list of existing printers that Xi-Text knows about.

plist = []
pdict = dict()
nullout = open('/dev/null', 'w')
try:
    pl = subprocess.Popen(['splist', '-F', '%p|||%f|||%e'], stdout=subprocess.PIPE, stderr=nullout)
    for lin in pl.stdout:
        lptr, lform, lcomm = string.split(lin, '|||')
        pp = possptr(string.strip(lptr), string.strip(lform), string.strip(lcomm))
        plist.append(pp)
        pdict[pp.ptr] = pp
    ecode = pl.wait()
    if ecode != 0:
	exit_msg("Warning: splist exited with non-zero exit code " + str(ecode), 102)
except OSError:
    exit_msg("***Warning: could not run splist", 101)

# If not cloning and we haven't got some bits, try to deduce as
# much as possible
# (arg parse stuff checks we've got the printer name)

if cloning is None:

    # If no printer specified, but there is only one printer to
    # specify, don't ask stupid questions

    if gs_q is None:
        if not deleting and len(plist) != 1: exit_msg("No Xi-Text print queue specified", 3)
        gs_q = plist[0].ptr

    # If we don't have info but we can get it from description here, get it

    if info is None:
        if gs_q not in pdict: exit_msg("No printer information given", 4)
        info = pdict[gs_q].comment

    # If form isn't given try to get it from printer

    if form is None:
        if gs_q not in pdict: exit_msg("No Xi-Text form type given", 5)
        form = pdict[gs_q].form
        m = re.search('(.*?)\..*', form)
        if m:
            paper = m.group(1)
        else:
            paper = form
        form = paper + '.ps'

# Protest if the specified printer name is unknown

if not deleting and not nocheck and gs_q is not None and gs_q not in pdict:
    exit_msg("Unknown Xi-Text printer " + gs_q, 5)

# If there isn't a config file already it is a mistake to clone or delete

configuration = conf.Conf()

if os.path.exists(confin):
    if not os.path.isfile(confin):
	exit_msg("***Warning:" + conffile + ' exists but is not a file', 10)

    # Now parse the thing, bombing with appropriate errors if not parseable

    try:
        configuration.parse_conf_file(confin, False)
    except conf.ConfError as err:
	exit_msg(err.args[0], err.args[1])

else:

    # Case were there is no config file

    if cloning is not None or deleting:
	exit_msg("No config file " + confin + " to operate on", 6)
    if confout is None: confout = confin

    # Initialise the default settings

    configuration.setup_defaults()

# OK now do required action

try:
    if deleting:
	# Deleting case - either CUPSPY printer or specify Xi-Text printer
	# to delete all CUPSPY ones using it.
	if cupsptr is not None:
	    configuration.del_printer(cupsptr)
	else:
	    plist = configuration.list_printers()
	    dlist = []
	    for p in plist:
		if configuration.get_param_value(p, 'gsprinter') == gs_q:
		    dlist.append(p)
	    for p in dlist:
		configuration.del_printer(p)
        if configuration.default_printer() == "":
            pl = configuration.list_printers()
            if len(pl) != 0:
                configuration.set_default_printer(pl[0])
    elif cloning is not None:
        configuration.clone_printer(cloning, cupsptr, queue=gs_q, info=info, form=form)
        if configuration.default_printer() == "" or setdef:
            configuration.set_default_printer(cupsptr)
    else:
        configuration.add_printer(cupsptr)
        configuration.set_param_value(cupsptr, 'printer-info', info)
        configuration.set_param_value(cupsptr, 'form', form)
        configuration.set_param_value(cupsptr, 'gsprinter', gs_q)
        configuration.set_param_value(cupsptr, 'document-format-default', 'application/postscript')
        if configuration.default_printer() == "" or setdef:
            configuration.set_default_printer(cupsptr)

    configuration.write_config(confout)

    if not no_restart:
        argsc = ['./cups.py']
        if confout is not None:
            argsc.append(confout)
        subprocess.call(argsc, shell=True, stdout=nullout, stderr=nullout)

except conf.ConfError as err:
    exit_msg(err.args[0], err.args[1])