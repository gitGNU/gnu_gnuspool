# Copyright (C) 2009  Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# Originally written by John Collins <jmc@xisl.com>.

# import copy, sys

import socket
import os
import os.path
import pwd
import time
import re
import string
import ConfigParser
import printeratts

# Map of user names

Usermap = dict()

def getrootuser():
    """Get root user name being particular"""
    try:
        p = pwd.getpwuid(0)
	return	p.pw_name
    except KeyError:
	return 'root'

class ConfError(Exception):
    """Error report in config file"""
    pass

class confConfig(ConfigParser.ConfigParser):
    """Add extra conditional functionality to ConfigParser"""

    def getcheck(self, section, option):
	"""Get option and raise appropriate error"""
	try:
            return self.get(section, option)
	except ConfigParser.NoSectionError as e:
            raise ConfError("Config file error - no section " + e.section, 100)
	except ConfigParser.NoOptionError as e:
            raise ConfError("Config file error - no option " + e.option, 101)
	except ConfigParser.InterpolationMissingOptionError as e:
            raise ConfError("Config file error - interpolation problem with " + e.reference, 102)

    def getvalue(self, section, option, lookup = None):
        """Get option and attempt to make it a list etc if possible"""
        try:
            v = self.get(section, option, 0, lookup)
        except (ConfigParser.NoSectionError, ConfigParser.NoOptionError, ConfigParser.InterpolationMissingOptionError):
            raise ConfError("Config file error - no option " + option, 101)
        try:
            f = v[0]
            if '0' <= f <= '9': v = int(v)
            elif f == '(' or f == '[': v = eval(v)
        except (IndexError, ValueError, SyntaxError):
            pass
        return v

def findonpath(p):
    """Find program p on PATH environment variable"""
    for possp in string.split(os.environ['PATH'],':'):
	if len(possp) == 0 or possp[0] != '/': continue
	fp = os.path.join(possp, p)
	if os.path.isfile(fp): return fp
    return None

def finduserpath(p):
    """Find program p on USERPATH in case it's not on the PATH"""
    try:
	for m in open('/etc/Xitext-config'):
            ms = re.match('\s*USERPATH\s*=\s*(\S+)', m)
            if ms:
		fp = os.path.join(ms.group(1), p)
		if os.path.isfile(fp): return fp
    except IOError:
	pass
    return	None

class Params:
    """Parameters for system"""

    # Fields for system paramaters and default 
    fields = (('servername', 'CUPS/1.4'),
              ('loglevel', 1),
              ('timeouts', 20.0),
              ('serverip', '0.0.0.0'),
              ('ppddir', ''),
              ('formarg', '-f %s'),
              ('titlearg', '-h %s'),
              ('gsprinterarg', '-P %s'),
              ('copiesarg', '-c %d'),
              ('userarg', '-U %s'),
              ('defaultuser', 'nobody'),
              ('usermap', '/etc/xi-user.map'),
              ('prefixcommand', ''),
              ('default_ptr', None))

    def __init__(self):
        self.sectionname = "System Parameters"
        for f in Params.fields:
            name, defv = f
            setattr(self, name, defv)
	self.serverip = socket.gethostname()
	self.defaultuser = getrootuser()
        sprcmd = findonpath('gspl-pr')
	if not sprcmd:
            sprcmd = finduserpath('gspl-pr')
            if not sprcmd:
                sprcmd = 'gspl-pr'
        self.prefixcommand = sprcmd + ' --verbose -s'
	self.plist = []

    def setup_defaults(self, c):
        """Initialise local parameters section"""
	if not c.has_section(self.sectionname):
            c.add_section(self.sectionname)
        for f in Params.fields:
            name = f[0]
            v = getattr(self, name)
            if v is not None:
                c.set(self.sectionname, name, v)
	c.set(self.sectionname, 'porder', '')

    def loadconfig(self, c):
        """Load parameters from config file"""
        for f in Params.fields:
            name, defv = f
            try:
                v = c.get(self.sectionname, name)
                # Convert values to types given by default value
                if isinstance(defv,int):
                    v = int(v)
                elif isinstance(defv, float):
                    v = float(v)
                setattr(self, name, v)
            except (ConfigParser.NoOptionError, ConfigParser.NoSectionError, ValueError):
                pass
	
	if c.has_option(self.sectionname, 'porder'):
	    pord = c.get(self.sectionname, 'porder')
	    if len(pord) != 0:
		self.plist = string.split(pord, ' ')
	    else:
		self.plist = []

class Conf:
    """Content of config file"""

    tmatch = re.compile("[^\w\s.]+")

    def __init__(self):
        self.parameters = Params()
	self.confparser = confConfig()
	self.conf_dir = os.getcwd()
	self.conf_file = None

    def setup_defaults(self):
	"""Create default printer attributes the first time we start"""
	for attname, vals in printeratts.Printer_atts.iteritems():
            self.confparser.set('DEFAULT', attname, vals[1])
            self.parameters.setup_defaults(self.confparser)

    def get_printer_list(self):
        """Get list of printer sections

System sections have a space in"""
	return sorted([s for s in self.confparser.sections() if s.find(' ') < 0])

    def load_user_map(self):
	"""Load user map

Entries consist of username:External user name"""
	global Usermap
	try:
            fname = self.parameters.usermap
            if not os.path.isabs(fname):
                fname = os.path.join(self.conf_dir, fname)
            f = open(fname, 'rt')
	except IOError:
            return
	lm = re.compile("^(\w+):(.+)")
	while 1:
            l = f.readline()
            if len(l) == 0:
		break
            m = lm.match(string.rstrip(l))
            if m is None: continue
            un, wn = m.groups()
            try:
		pwd.getpwnam(un)
            except KeyError:
		continue
            Usermap[wn.lower()] = un
	f.close()

    def parse_conf_file(self, fname, checkdp = True):
        """Parse a config file"""

	r = self.confparser.read(fname)
	if len(r) == 0:
            raise ConfError("Cannot open " + fname, 2)
	self.conf_file = fname

	# Set the config directory to where we read the config file from

	confd, filen = os.path.split(os.path.abspath(fname))
	self.conf_dir = confd
	self.parameters.loadconfig(self.confparser)
        if len(self.parameters.plist) == 0:
            self.parameters.plist = self.get_printer_list()
	if self.parameters.default_ptr is None:
            if len(self.parameters.plist) > 0:
                self.parameters.default_ptr = self.parameters.plist[0]
	dp = self.parameters.default_ptr
	if checkdp and (dp is None or dp not in self.parameters.plist):
	    if dp is None:
		dp = ""
	    else:
		dp = " (" + dp + ")"
            raise ConfError("Default printer" + dp + " not set up", 150)
        self.load_user_map()

    def write_config(self, fname = None):
        """Write out a config file to the specified file"""

        if fname is None:
            fname = self.conf_file
            if fname is None:
		raise ConfError('No output file given', 3)

	try:
            outfile = open(fname, "wb")
	except IOError:
            raise ConfError("Cannot open " + fname, 4)

	outd, filen = os.path.split(os.path.abspath(fname))

	# If outd is different from conf_dir, update it.
	# Also possibly change ppd directory if that is a subdirectory

	if outd != self.conf_dir:
            ppd = self.parameters.ppddir
            if len(ppd) != 0:
		if os.path.isabs(ppd):
                    rp = os.path.relpath(ppd, outd)
                    if rp[0:2] != '..':
                        self.parameters.ppddir = rp
            self.conf_dir = outd

	# If we have a printer order, write it

	self.confparser.set(self.parameters.sectionname, 'porder', string.join(self.parameters.plist, ' '))

	# Write parameters

	outfile.write("# CUPSPY configuration file written on %s\n\n" % time.ctime())
	self.confparser.write(outfile)
	outfile.close()

	# The following things are enquiries on the config data
	# Not too many errors now

    def list_printers(self):
	"""Get a list of printers

All our parameters have spaces in"""
	pl = self.parameters.plist
	if len(pl) == 0:
            pl = self.get_printer_list()
	return pl

    def set_printer_list(self, plist):
        """Set order of printers"""
	elist = self.get_printer_list()
	if len(set(elist) ^ set(plist)) != 0:
            raise ConfError("Printer list does not accord with existing", 103)
	self.parameters.plist = plist

    def default_printer(self):
	"""Get default printer name"""
	if self.parameters.default_ptr is None: return ""
	return	self.parameters.default_ptr

    def set_default_printer(self, pname):
	"""Set default printer"""
	if pname is None or len(pname) == 0:
            pname = None
            self.confparser.remove_option(self.parameters.sectionname, 'Default_ptr')
	else:
            if pname not in self.confparser.sections():
                raise ConfError("No such printer - " + pname, 104)
            self.confparser.set(self.parameters.sectionname, 'Default_ptr', pname)
            self.parameters.default_ptr = pname

    def default_user(self):
	"""Get default user name"""
	return	self.parameters.defaultuser

    def set_default_user(self, username):
	"""Set default user name - check known"""
	try:
            pwd.getpwnam(username)
            self.parameters.defaultuser = username
	except (KeyError, TypeError):
            self.parameters.defaultuser = getrootuser()

        self.confparser.set(self.parameters.sectionname, 'Defaultuser', self.parameters.defaultuser)

    def log_level(self):
	"""Get log level"""
	return self.parameters.loglevel

    def set_log_level(self, value = 1):
	"""Set log level"""
	if value not in range(0,5):
            raise ConfError("Invalid log level", 105)
	self.parameters.loglevel = value
	self.confparser.set(self.parameters.sectionname, 'LOGLEVEL', str(value))

    def timeout_value(self):
	"""Get log level"""
	return self.parameters.timeouts

    def set_timeout_value(self, value=20):
	"""Set timeout value"""
	self.parameters.timeouts = value
	self.confparser.set(self.parameters.sectionname, 'TIMEOUT', str(value))

    def ppddir(self):
	"""Get PPD directory"""
	ret = self.parameters.ppddir
	if len(ret) == 0:
            return self.conf_dir
	if os.path.isabs(ret):
            return ret
        return os.path.join(self.conf_dir, ret)

    def set_ppddir(self, value):
	"""Set PPD directory"""
	if os.path.isabs(value):
            cp = os.path.commonprefix([self.conf_dir, value])
            if cp == self.conf_dir:
		v = value[len(cp):]
		if len(v) == 0 or v[0] == '/':
                    value = v
	self.parameters.ppddir = value
	self.confparser.set(self.parameters.sectionname, 'PPDDIR', value)

    def serverip(self):
	"""Get what we're using as the server IP"""
	return self.parameters.serverip

    def set_serverip(self, value):
	"""Set the server IP"""
        self.parameters.serverip = value
	self.confparser.set(self.parameters.sectionname, "SERVERIP", value)

    def print_command(self, pname, copies=1, user='root', title='No title'):
	"""Get print command for printer"""

	title = Conf.tmatch.sub("_", title)
	if re.search("\s", title):
            title = "'" + title + "'"

	titlearg = self.parameters.titlearg % title
	cpsarg = self.parameters.copiesarg % copies
	userarg = self.parameters.userarg % user
	try:
            formarg = self.parameters.formarg % self.confparser.getcheck(pname, "Form")
	except ConfError:
            return None
	try:
            printerarg = self.parameters.gsprinterarg % self.confparser.getcheck(pname, 'GSPrinter')
	except ConfError:
            printerarg = ""

	return string.join([self.parameters.prefixcommand,
			cpsarg, titlearg, userarg, printerarg, formarg], ' ')

    def get_config(self, pname, al):
	"""Get specified configuration information for specified printer"""

	if not self.confparser.has_section(pname):
            return None

	now = int(time.time())
	lookup = dict(PTRNAME = pname,
                      NOW = str(now),
                      SERVERIP = self.parameters.serverip,
                      STATECHANGE = str(now - 3600),
                      UPTIME = str(now - 7200))

	pendresult = []

	for att in al:
            try:
		v = self.confparser.getvalue(pname, att, lookup)
            except ConfError:
		continue
            try:
                attdescr = printeratts.Printer_atts[att]
            except KeyError:
                continue
            vtype = attdescr[0]
            # Fix resolution and ranges?
            pendresult.append((att, vtype, v))

	return pendresult

    def get_default_attribute(self, attr):
	"""Get default attribute value as a list"""
	try:
            v = self.confparser.getvalue('DEFAULT', attr)
            if not isinstance(v, list):
                v = [v]
            return v
        except ConfError:
            return None

    def get_attribute_value(self, pname, attr):
	"""Get specified attribute value as single item"""
	atts = self.get_config(pname, [attr])
	if len(atts) == 0:
            return None
	atts = atts[0][2]
	if len(atts) == 1:
            return atts[0]
	return atts

    def set_attribute_value(self, pname, attrname, value):
	"""Set specified attribute value"""
	if pname not in self.confparser.sections():
            raise ConfError("No such printer - " + pname, 104)
	self.confparser.set(pname, attrname, str(value))

    def get_param_value(self, pname, param):
	"""Get specified printer parameter"""
	if pname not in self.confparser.sections():
            raise ConfError("No such printer - " + pname, 104)
	try:
            return self.confparser.get(pname, param)
	except ConfigParser.NoOptionError:
            raise ConfError("No such parameter - " + param, 101)

    def set_param_value(self, pname, param, value):
	"""Set specified printer parameter

Always assume it to be a string"""
	if pname not in self.confparser.sections():
            raise ConfError("No such printer - " + pname, 104)
	self.confparser.set(pname, param, value)

    def add_printer(self, pname):
	"""Add a new printer to the list"""
	if self.confparser.has_section(pname):
            raise ConfError("Duplicate printer " + pname, 106)
	self.confparser.add_section(pname)
	self.parameters.plist.append(pname)

    def copy_printer(self, pname, newpname):
        """Copy a printer definition for clone or rename"""
        if not self.confparser.has_section(pname):
            raise ConfError("Unknown printer " + pname, 104)
	if self.confparser.has_section(newpname):
            raise ConfError("Duplicate printer name " + newpname, 106)
	self.confparser.add_section(newpname)
        # Cream out the ones different from defaults
        defs = self.confparser.defaults()
	for opt in self.confparser.options(pname):
            orig = self.confparser.get(pname, opt, raw=True)
            try:
                if defs[opt] == orig: continue
            except KeyError:
                pass
            self.confparser.set(newpname, opt, orig)

    def rename_printer(self, pname, newpname):
	"""Rename a printer"""
	self.copy_printer(pname, newpname)
	self.confparser.remove_section(pname)
	# If it's the default printer rename that
	if pname == self.parameters.default_ptr:
            self.parameters.default_ptr = newpname
	if pname in self.parameters.plist:
            self.parameters.plist.remove(pname)
        self.parameters.plist.append(newpname)

    def clone_printer(self, pname, newpname, queue=None, form=None, info=None):
        """Clone a printer, setting the gsprinter name"""
        self.copy_printer(pname, newpname)
        if queue is not None:
            self.confparser.set(newpname, 'gsprinter', queue)
        if form is not None:
            self.confparser.set(newpname, 'form', form)
        if info is not None:
            self.confparser.set(newpname, 'printer-info', info)
        self.parameters.plist.append(newpname)

    def del_printer(self, pname):
        """Delete a printer from the list"""
        if not self.confparser.has_section(pname):
            raise ConfError("Unknown printer " + pname, 104)
        self.confparser.remove_section(pname)
        if pname == self.parameters.default_ptr:
            self.parameters.default_ptr = ""
        if pname in self.parameters.plist:
            self.parameters.plist.remove(pname)
