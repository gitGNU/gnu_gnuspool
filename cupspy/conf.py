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

import re, socket, string, copy, time, ipp, sys, os, stat

class ConfError(Exception):
    """Error report in config file"""
    def __init__(self, msg):
        self.message = msg

class ConfEOF(Exception):
    """Just exception to raise when we read EOF"""
    pass

class Confdef:
    def __init__(self, name, *istrings):
        self.descr = name
        self.attrlist = list()
        self.attrs = dict()
        self.defs = dict()
        for i in istrings:
            self.defs[i] = ''

    def setdef(self, name, value):
        """Set a default name to given value"""
        if name not in self.defs:
            raise ConfError("Unknown keyword " + name)
        self.defs[name] = value;

    def setattr(self, name, typ, value):
        """Set up attribute name/type/value"""
        if typ not in ipp.name_to_tag:
            raise ConfError("Unknown type " + typ)
        self.attrlist.append(name)
        self.attrs[name] = (typ, value)

    def check_complete(self):
        """Check we've got all the bits we should have"""
        for k,v in dict.items(self.defs):
            if len(v) == 0:
                raise ConfError("Definition of " + k + " missing in " + self.descr)

def readuncommline(f):
    """Read a non-empty line from file
Also strip comments"""
    while 1:
        result = f.readline()
        if len(result) == 0:
            raise ConfEOF
        m = re.match("([^#]*)#", result)
        if m:
            result = m.group(1)
        result = string.strip(result)
        if  len(result) != 0:
            return  result

def readfullline(f):
    """Read a line in joining lines ending with \\ to the next one"""
    result = readuncommline(f)
    while result[-1] == '\\':
        result = result[0:-1]
        r2 = readuncommline(f)
        result += r2
    return result

def procparam(line, cdef):
    """Add the parameter list to the given entry
This might be the defaults or the entry for a given printer"""
    # Split line into attribute name, type and args
    try:
        name, typ, args = re.split("\s+", line, 2)
    except ValueError:
        raise ConfError("Expecting name/type/args in ")
    cdef.setattr(name, typ, args)

def parse_loop(f, cdef):
    """Loop to read definitions from file"""
    while 1:
        # Remember where we were
        where = f.tell()
        line = readfullline(f)

        # If start of next section rewind and drop out
        if re.match("\w+\s*:$", line):
            f.seek(where)
            return

        try:
            # Def specs have name=string in
            m = re.match("(\w+)\s*=\s*(.*)", line)
            if m:
                cdef.setdef(m.group(1), m.group(2))
            else:
                procparam(line, cdef)
        except ConfError, Err:
            raise ConfError(Err.message + " in " + line)

class Conf:
    """Content of config file"""

    def __init__(self):
        self.defaults = Confdef("Defaults", "Printer", "Copies", "Title", "User")
        self.printers = dict()
        self.logfile = None
        self.loglevel = 0
        self.conf_dir = os.getcwd()
        self.ppddir = self.conf_dir

    def parse_conf_params(self, f):
        """Parse parameters section of a config file"""

        hadppd = False
        while 1:
            # Remember where we wre
            where = f.tell()
            line = readfullline(f)
            # If start of next section rewind and drop out
            if re.match("\w+\s*:$", line):
                f.seek(where)
                break
            m = re.match("(\w+)\s*=\s*(.*)", line)
            if not m:
                break
            opt = m.group(1)
            arg = m.group(2)
            if opt == "LOG":
                if len(arg) == 0:
                    raise ConfError("Invalid null LOG file name")
                self.logfile = arg
            elif opt == "LOGLEVEL":
                try:
                    self.loglevel = int(arg)
                except ValueError:
                    raise ConfError("Invalid LOGLEVEL " + arg + " (should be int)")
            elif opt == "CONFDIR":
                direc = arg
                if  len(direc) == 0:
                    raise ConfError("Invalid null CONFDIR")
                try:
                    s = os.stat(direc)
                    if not stat.S_ISDIR(s[0]):
                        raise ConfError("CONFDIR " + direc + " not directory")
                except OSError:
                    raise ConfError("Invalid CONFDIR " + direc)
            elif opt == "PPDDIR":
                self.ppddir = arg
                if  len(arg) == 0:
                    raise ConfError("Invalid null PPDDIR")
                hadppd = True

        # If logfile is specified, check it's OK and if so redirect stdout

        if self.logfile:
            if self.logfile[0] != '/':
                self.logfile = os.path.join(self.conf_dir, self.logfile)
            try:
                lf = open(self.logfile, 'a')
            except IOError:
                raise ConfError("Cannot open logfile " + self.logfile)
            sys.stdout = lf

        # If ppdir was specified and not absolute, put confdir in front

        if hadppd:
            if self.ppddir[0] != '/':
                self.ppddir = os.path.join(self.conf_dir, self.ppddir)
            try:
                s = os.stat(direc)
                if not stat.S_ISDIR(s[0]):
                    raise ConfError("PPDDIR " + direc + " not directory")
            except OSError:
                raise ConfError("Invalid PPDDIR " + direc)

    def parse_conf_defaults(self, f):
        """Parse the defaults section of a config file"""
        parse_loop(f, self.defaults)

    def parse_conf_printer(self, pname, f):
        """Parse printer definition in a config file"""
        if pname in self.printers:
            raise ConfError("Already had definition of " + pname)
        pd = Confdef(pname, "Command")
        self.printers[pname] = pd
        parse_loop(f, pd)

    def parse_conf_file(self, fname):
        """Parse a config file and set up defaults"""

        # Set base directory for conf stuff to be the directory we are reading the file from
        # That might reset it later

        confd, filen = os.path.split(os.path.abspath(fname))
        self.conf_dir = confd

        # Open the silly thing

        try:
            cfile = open(fname, 'rb')
        except IOError:
            raise ConfError("Cannot open " + fname)
        try:
            while 1:
                line = readfullline(cfile)
                m = re.match("\s*(\w+)\s*:$", line)
                if not m:
                    raise ConfError("Unrecognised section start - " + line)
                if m.group(1) == "PARAMS":
                    self.parse_conf_params(cfile)
                elif m.group(1) == "DEFAULTS":
                    self.parse_conf_defaults(cfile)
                else:
                    self.parse_conf_printer(m.group(1), cfile)
        except ConfEOF:
            pass
        cfile.close()

        # Check things make sense

        self.defaults.check_complete()
        for ptr in self.printers.values():
            ptr.check_complete()
        if self.defaults.defs['Printer'] not in self.printers:
            raise ConfError("Default printer " + self.defaults['Printer'] + " not set up");

    # The following things are enquiries on the config data
    # Not too many errors now

    def list_printers(self):
        """Get a list of printers"""
        pl = dict.keys(self.printers)
        pl.sort()
        return pl

    def default_printer(self):
        """Get default printer name"""
        return  self.defaults.defs['Printer']

    def print_command(self, pname):
        """Get print command for printer"""
        try:
            return self.printers[pname].defs['Command']
        except KeyError:
            return None

    def copies_param(self, pname=""):
        """Get parameter for copies for printer"""
        # Currently they're all the same so we ignore pname
        return  self.defaults.defs['Copies']

    def title_param(self, pname=""):
        """Get parameter for title for printer"""
        # Currently they're all the same so we ignore pname
        return  self.defaults.defs['Title']

    def user_param(self, pname=""):
        """Get parameter for user for printer"""
        # Currently they're all the same so we ignore pname
        return  self.defaults.defs['User']

    def get_attnames(self, pname):
        """Get list of applicable attributes for specified printer
in order that they are supposed to be"""
        try:
            attlist = copy.deepcopy(self.defaults.attrlist)
            for att in self.printers[pname].attrlist:
                if att not in attlist:
                    attlist.append(att)
            return attlist
        except KeyError:
            return None

    def get_config(self, pname, al):
        """Get specified configuration information for specified printer"""

        if pname not in self.printers:
            return None

        # First get Default attributes
        # We want a copy not a reference as we're going to overwrite bits

        datts = copy.deepcopy(self.defaults.attrs)
        for k,v in dict.items(self.printers[pname].attrs):
            datts[k] = v

        pendresult = []
        for att in al:
            if att in datts:
                pendresult.append((att, datts[att]))

        result = []

        # Now decode any symbolic names and argument lists

        for r in pendresult:
            name, att = r
            typestr, argstring = att
            typenum = ipp.name_to_tag[typestr]

            # Deal with "" strings as a special case - only one of them

            m = re.match('"(.*)"', argstring)
            if  m:
                res = m.group(1)
            elif re.match("NOW", argstring):
                # Simple expressions involving current time
                m = re.match("NOW(([-+])(\d+))?", argstring)
                if not m:
                    raise ConfError("Cannot decipher " + argstring)
                when = time.time()
                if m.group(1):
                    offset = int(m.group(3))
                    if m.group(2) == '-':
                        when -= offset
                    else:
                        when += offset
                res = when
            elif re.match("MAKE\w+URI", argstring):
                if argstring == "MAKELOCALURI":
                    res = "ipp://localhost:631/printers/" + pname
                elif argstring == "MAKENETURI":
                    res = "ipp://" + socket.gethostname() + ":631/printers/" + pname
                elif argstring == "MAKEDEVURI":
                    res = "hal:///org/freedesktop/Hal/devices/" + pname
                else:
                    raise ConfError("Cannot decipher " + argstring)
            elif argstring == "PRINTERNAME":
                res = pname
            else:
                res = re.split("\s+", argstring)
                if typenum == ipp.IPP_TAG_INTEGER or typenum == ipp.IPP_TAG_ENUM or typenum == ipp.IPP_TAG_BOOLEAN or typenum == ipp.IPP_TAG_DATE:
                    try:
                        res = map(int, res)
                    except ValueError:
                        raise ConfError("Invalid number for " + typestr + " in attr " + name)
                elif typenum == ipp.IPP_TAG_RESOLUTION:
                    if len(res) != 3:
                        raise ConfError("Invalid resolution " + argstring)
                    units = res.pop();
                    try:
                        res = map(int, res)
                    except ValueError:
                        raise ConfError("Invalid number for resolution " + name)
                    res.append(units);
                    res = tuple(res)
                elif typenum == ipp.IPP_TAG_RANGE:
                    if len(res) != 2:
                        raise ConfError("Invalid range " + argstring)
                    try:
                        res = map(int, res)
                    except ValueError:
                        raise ConfError("Invalid number for range " + name)
                    res = tuple(res)

            if not isinstance(res, list):
                res = [res]

            result.append((name, typenum, res))

        return result
