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
    pass

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

    def write_defs(self, outfile):
        """Write out definitions to file"""

        outfile.write("# Definitions for %s\n\n%s:\n\n" % (self.descr, self.descr))

        for k,v in dict.items(self.defs):
            outfile.write("\t%s=%s\n" % (k, v))

        outfile.write("\n")
        for a in self.attrlist:
            typ, val = self.attrs[a]
            outfile.write("\t%s %s %s\n" % (a, typ, val))
        outfile.write("\n")
        

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
        except ConfError as Err:
            raise ConfError(Err.args[0] + " in " + line)

class Conf:
    """Content of config file"""

    def __init__(self):
        self.defaults = Confdef("DEFAULTS", "Prefix", "Form", "GSPrinter", "Copies", "Title", "User", "Printer")
        self.printers = dict()
        self.plist = []
        self.loglevel = 1
        self.timeouts = 20
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
            if opt == "LOGLEVEL":
                try:
                    self.loglevel = int(arg)
                    if self.loglevel < 0 or self.loglevel > 4:
                        raise ValueError
                except ValueError:
                    raise ConfError("Invalid LOGLEVEL " + arg + " (should be int < 5)")
            elif opt == "PPDDIR":
                self.ppddir = arg
                if  len(arg) == 0:
                    raise ConfError("Invalid null PPDDIR")
                hadppd = True
            elif opt == "TIMEOUT":
                try:
                    self.timeouts = float(arg)
                    if self.timeouts <= 0:
                        raise ValueError
                except ValueError:
                    raise ConfError("Invalid TIMEOUT " + arg + " (should be >0)")

        # If ppdir was specified and not absolute, put confdir in front

        if hadppd:
            if self.ppddir[0] != '/':
                self.ppddir = os.path.join(self.conf_dir, self.ppddir)
            try:
                s = os.stat(self.ppddir)
                if not stat.S_ISDIR(s[0]):
                    raise ConfError("PPDDIR " + self.ppddir + " not directory")
            except OSError:
                raise ConfError("Invalid PPDDIR " + self.ppddir)

    def parse_conf_defaults(self, f):
        """Parse the defaults section of a config file"""
        parse_loop(f, self.defaults)

    def parse_conf_printer(self, pname, f):
        """Parse printer definition in a config file"""
        if pname in self.printers:
            raise ConfError("Already had definition of " + pname)
        self.plist.append(pname)
        pd = Confdef(pname, "GSPrinter", "Form", "Command")
        pd.defs["Command"] = ':'
        self.printers[pname] = pd
        parse_loop(f, pd)

    def parse_conf_file(self, fname):
        """Parse a config file and set up defaults"""

        # Set base directory for conf stuff to be the directory we are reading the file from
        # That might reset it later

        confd, filen = os.path.split(os.path.abspath(fname))
        self.conf_dir = confd
        self.ppddir = confd

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
            raise ConfError("Default printer " + self.defaults['Printer'] + " not set up")

    def write_config(self, fname, plist=None):
        """Write out a config file to the specified file
Optionally write printers out in the order given"""
        try:
            outfile = open(fname, "wb")
        except IOError:
            raise ConfError("Cannot open " + fname)

        outd, filen = os.path.split(os.path.abspath(fname))
     
        # Write parameters
        
        outfile.write("# CUPSPY configuration file written on %s\n\n" % time.ctime())
        outfile.write("# Parameters section\n\nPARAMS:\n\tLOGLEVEL=%d\n\tTIMEOUT=%d\n" % (self.loglevel, self.timeouts))

        # Write out ppddir as relative or not at all if it's the same as outd

        ppd = self.ppddir
        if outd != ppd:
            lout = len(outd)
            if lout < len(ppd) and outd == ppd[0:lout]  and  ppd[lout] == '/':
                ppd = ppd[lout+1:]
            outfile.write("\tPPDDIR=%s\n" % ppd)

        outfile.write("\n")
        
        # Write defaults
        
        self.defaults.write_defs(outfile)
        if not plist:
            plist = self.plist
        for p in plist:
            if p in self.printers:
                self.printers[p].write_defs(outfile)
        outfile.close()

    # The following things are enquiries on the config data
    # Not too many errors now

    def list_printers(self):
        """Get a list of printers"""
        return self.plist

    def default_printer(self):
        """Get default printer name"""
        return  self.defaults.defs['Printer']

    def set_default_printer(self, pname):
        """Set default printer"""
        self.defaults.defs['Printer'] = pname

    def print_command(self, pname, copies=1, user='root', title='No title'):
        """Get print command for printer"""

        # If specific command given (not ':') use that

        try:
            pdefs = self.printers[pname].defs
        except (KeyError, AttributeError):
            return None
        
        try:
            cmd = pdefs['Command']
        except KeyError:
            cmd = ':'

        if cmd != ':':
            return cmd

        # Put quotes round title if needed

        title = re.sub("'", "_", title)
        if re.search("\s", title):
            title = "'" + title + "'"

        # Create copies, printer, user, title args

        try:
            cmd = self.defaults.defs['Prefix']
            cpsarg = self.defaults.defs['Copies'] % copies
            titlearg = self.defaults.defs['Title'] % title
            userarg = self.defaults.defs['User'] % user
            if len(pdefs['GSPrinter']) != 0 and  pdefs['GSPrinter'] != ':':
                printerarg = self.defaults.defs['GSPrinter'] % pdefs['GSPrinter']
            else:
                printerarg = ""
            formarg = self.defaults.defs['Form'] % pdefs['Form']
        except (TypeError, KeyError, AttributeError):
            return None

        return string.join([cmd, cpsarg, titlearg, userarg, printerarg, formarg], ' ')

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

    def get_default_attribute(self, attr):
        """Get default attribute value as a list"""
        if attr not in self.defaults.attrs:
            return None
        return  re.split('\s+', self.defaults.attrs[attr][1])

    def get_attribute_value(self, pname, attr):
        """Get specified attribute value as single item"""
        if pname not in self.printers:
            return None
        atts = self.get_config(pname, [attr])
        if len(atts) == 0:
            return None
        atts = atts[0][2]
        if len(atts) == 1:
            return atts[0]
        return atts

    def set_attribute_value(self, pname, attrname, value):
        """Set specified attribute value"""
        if pname not in self.printers:
            raise ConfError("No such printer - " + pname)
        ptrdef = self.printers[pname]
        pattrs = ptrdef.attrs

        # Set value to list if it isn't

        if isinstance(value, list):
            value = string.join(map(list, str), ' ')
        
        # If something is already defined just update it otherwise
        # get type from default or throw wobbly
        
        if attrname in pattrs:
            ty, ev = pattrs[attrname]
            if ty == "IPP_TAG_TEXT":
                value = '"' + value + '"'
            pattrs[attrname] = (ty, value)
        else:
            if attrname not in self.defaults.attrs:
                raise ConfError("Unknown attribute " + attrname)
            # Need first level copy as we're overwriting value part
            ty, ev = self.defaults.attrs[attrname]
            if ty == "IPP_TAG_TEXT":
                value = '"' + value + '"'
            newatt = (ty, value)
            pattrs[attrname] = newatt
            ptrdef.attrlist.append(attrname)

    def get_param_value(self, pname, param):
        """Get specified printer parameter"""
        if pname not in self.printers:
            raise ConfError("No such printer - " + pname)
        pdefs = self.printers[pname].defs
        if param not in pdefs:
            raise ConfError("No such parameter - " + param)
        return pdefs[param]
        
    def set_param_value(self, pname, param, value):
        """Set specified printer parameter"""
        if pname not in self.printers:
            raise ConfError("No such printer - " + pname)
        pdefs = self.printers[pname].defs
        if param not in pdefs:
            raise ConfError("No such parameter - " + param)
        pdefs[param] = value
        
    def add_printer(self, pname):
        """Add a new printer to the list"""
        if pname in self.printers:
            raise ConfError("Duplicate printer " + pname)
        self.plist.append(pname)
        pd = Confdef(pname, "GSPrinter", "Form", "Command")
        pd.defs["Command"] = ':'
        self.printers[pname] = pd

    def rename_printer(self, pname, newpname):
        """Rename a printer"""
        if newpname in self.printers:
            raise ConfError("Duplicate printer name " + newpname)
        self.printers[newpname] = self.printers[pname]
        del self.printers[pname]
        self.plist[self.plist.index(pname)] = newpname
        # If it's the default printer rename that
        if self.defaults.defs['Printer'] == pname:
            self.defaults.defs['Printer'] = newpname
        
    def del_printer(self, pname):
        """Delete a printer from the list"""
        if pname not in self.printers:
            raise ConfError("Unknown printer " + pname)
        del self.printers[pname]
        try:
            del self.plist[self.plist.index(pname)]
        except:
            pass
        if self.defaults.defs['Printer'] == pname:
            self.defaults.defs['Printer'] = ""
