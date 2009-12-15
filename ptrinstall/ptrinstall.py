#! /usr/bin/python
#
#   Copyright 2009 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys, os, os.path, stat, string, re, subprocess, time, platform, copy, socket, glob
try:
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
except ImportError:
    print "Sorry but you need PyQt4 installed to run this"
    sys.exit(20)

import ui_ptrinstall_main
import ui_newptrdlg
import ui_serialparams, ui_parparams, ui_usbparams
import ui_ptseldlg, ui_psoptdlg, ui_sethostdlg, ui_netprotodlg
import ui_netparams
import ui_lpdparams, ui_telnetparams, ui_ftpparams, ui_hpnpfparams
import ui_othernetdlg, ui_clonedlg
import ui_devinstdlg, ui_netinstdlg, ui_otherinstdlg
import ui_optsdlg

try:
    import config
except ImportError:
    print "Sorry but you must build config.py to run this"
    sys.exit(21)

# These are globals, list of printers, program locations, config data

Printerlist = dict()

def decode_dir(dir):
    """Extract shorthand from something like ${SPROGDIR-/usr/spool/progs}/xtlpc-ctrl"""
    m = re.match('\$\{(\w+):-[^}]*\}/(.*)', dir)
    if m and m.group(1) in config.Deflocs:
        return m.group(1) + '/' + m.group(2)
    return dir

def encode_dir(dir):
    """Reverse above operation"""
    m = re.match('(\w+)/(.*)', dir)
    if m and m.group(1) in config.Deflocs:
        return '${' + m.group(1) + ':-' + config.Deflocs[m.group(1)] + '}/' + m.group(2)
    return dir

def write_bool(df, name, bopt, descr):
    """Write out a boolean option to a setup or .device file"""
    df.write("# %s\n" % descr)
    if not bopt: df.write("# ")
    df.write("%s\n\n" % name)

def try_connect(h, p):
    """See if we can connect to the given host with the given port number"""
    sockfd = None
    try:
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sockfd.connect((h, p))
        return True
    except socket.error:
        return False
    finally:
        if sockfd is not None:
            sockfd.close()

class FileParseError(Exception):
    """Throw this if we have an error parsing a .device file or whatever"""
    pass

def numordefault(optdict, varname, default):
    """Get numeric value for options or give default"""
    try:
        return int(optdict[varname])
    except (TypeError, KeyError, ValueError):
        return default

def boolordefault(optdict, varname, default):
    """Get bool value for options.

Give default if no options at all defined
Give False if options defined but not set"""
    if optdict is None:
        return default
    try:
        return optdict[varname]
    except KeyError:
        return False

def parse_command(cmd):
    """Turn command string into array of command options and arguments

NB Currently we don't handle ''s"""
    cmd = string.strip(cmd)
    result = []
    for c in string.split(cmd):
        m = re.match('(-[^-])(.+)', c)
        if m:
            result.append(m.group(1))
            result.append(m.group(2))
        else:
            result.append(c)
    return result

class Spoolopts:
    """Represent spooler options"""

    def __init__(self, d = None):
        self.addcr = boolordefault(d, 'addcr', False)
        self.retain = boolordefault(d, 'retain', False)
        self.norange = boolordefault(d, 'norange', False)
        self.inclpage1 = boolordefault(d, 'inclpage1', False)
        self.onecopy = boolordefault(d, 'onecopy', False)
        self.single = boolordefault(d, 'single', False)
        self.nohdr = boolordefault(d, 'nohdr', False)
        self.forcehdr = boolordefault(d, 'forcehdr', False)
        self.hdrpercopy = boolordefault(d, 'hdrpercopy', False)

class Portopts:
    """Represent general port parameters"""

    def __init__(self, d = None):
        self.opento = numordefault(d, 'open', 30)
        self.offlineto = numordefault(d, 'offline', 300)
        self.canhang = boolordefault(d, 'canhang', False)
        self.outbuffer = numordefault(d, 'outbuffer', 1024)
        self.reopen = boolordefault(d, 'reopen', False)

    def setup_opts(self, dlg):
        """Set up fields in dialog"""
        dlg.opento.setValue(self.opento)
        dlg.offlineto.setValue(self.offlineto)
        dlg.canhang.setChecked(self.canhang)
        dlg.outbuffer.setValue(self.outbuffer)
        dlg.reopen.setChecked(self.reopen)

    def extract_opts(self, dlg):
        """Extract port options from dialog"""
        self.opento = dlg.opento.value()
        self.offlineto = dlg.offlineto.value()
        self.canhang = dlg.canhang.isChecked()
        self.outbuffer = dlg.outbuffer.value()
        self.reopen = dlg.reopen.isChecked()

    def write_opts(self, df):
        """Write out values of options to file with appropriate comments"""
        df.write("# Timeout on open before giving up\nopen %d\n\n" % self.opento)
        df.write("# Timeout on write before regarding device as offline\noffline %d\n\n" % self.offlineto)
        write_bool(df, 'canhang', self.canhang, 'Running processes attached to device hard to kill')
        df.write("# Size of output buffer in bytes\noutbuffer %d\n\n" % self.outbuffer)
        write_bool(df, 'reopen', self.reopen, 'Close and reopen device after each job')

class USBopts(Portopts):
    """Represent options for USB"""
    pass

class Parallelopts(Portopts):
    """Represent options for Parallel ports"""
    pass

class Serialopts(Portopts):
    """Represent options for serial ports"""

    def __init__(self, d = None):
        Portopts.__init__(self, d)
        self.baudrate = numordefault(d, 'baud', 9600)
        self.ixon = boolordefault(d, 'ixon', True)
        self.ixany = boolordefault(d, 'ixany', False)
        b = boolordefault(d, 'twostop', False)
        if b: self.stopbits = 2
        else: self.stopbits = 1
        self.csize = 8
        if d:
            for n in range(8,4,-1):
                b = 'cs' + str(n)
                if b in d:
                    self.csize = n
                    break
        self.parenb = boolordefault(d, 'parenb', False)
        self.parodd = boolordefault(d, 'parodd', False)
        self.clocal = boolordefault(d, 'clocal', False)
        self.onlcr = boolordefault(d, 'onlcr', False)

    def setup_opts(self, dlg):
        """Set up fields in dialog"""

        Portopts.setup_opts(self, dlg)

        for c in range(dlg.baudrate.count()):
            if int(dlg.baudrate.itemText(c)) == self.baudrate:
                dlg.baudrate.setCurrentIndex(c)
                break

        dlg.xon.setChecked(self.ixon)
        dlg.xany.setChecked(self.ixany)

        for c in range(dlg.csize.count()):
            if int(dlg.csize.itemText(c)) == self.csize:
                dlg.csize.setCurrentIndex(c)
                break

        dlg.twostop.setChecked(self.stopbits == 2)
        c = 0
        if self.parenb:
            c = 1
            if self.parodd:
                c = 2

        dlg.parity.setCurrentIndex(c)
        dlg.clocal.setChecked(self.clocal)
        dlg.onlcr.setChecked(self.onlcr)

    def extract_opts(self, dlg):
        """Extract serial options from dialog"""

        Portopts.extract_opts(self, dlg)
        self.baudrate = int(dlg.baudrate.currentText())
        self.ixon = dlg.xon.isChecked()
        self.ixany = dlg.xany.isChecked()
        self.csize = int(dlg.csize.currentText())
        self.stopbits = 1
        if dlg.twostop.isChecked():
            self.stopbits = 2
        c = dlg.parity.currentIndex()
        self.parenb = False
        self.parodd = False
        if c > 0:
            self.parenb = True
            if c > 1:
                self.parodd = True
        self.clocal = dlg.clocal.isChecked()
        self.onlcr = dlg.onlcr.isChecked()

    def write_opts(self, df):
        """Write out values of options to file with appropriate comments"""
        Portopts.write_opts(self, df)
        df.write("# Baudrate\nbaud %d\n\n" % self.baudrate)
        write_bool(df, 'ixon', self.ixon, 'Set xon/xoff')
        write_bool(df, 'ixany', self.ixany, 'Set xon/xoff with any character release')
        df.write("# Character size\ncs%d\n\n" % self.csize)
        write_bool(df, 'twostop', self.stopbits == 2, 'Stop bits')
        write_bool(df, 'parenb', self.parenb, 'Parity enabled')
        write_bool(df, 'parodd', self.parodd, 'Odd parity')
        write_bool(df, 'clocal', self.onlcr, 'Insert CR before each newline')

class Networkopts(Portopts):
    """Represent options for any network ports"""

    def __init__(self, d = None):
        Portopts.__init__(self, d)
        self.closeto = numordefault(d, 'close', 10000)
        self.postclose = numordefault(d, "postclose", 0)
        self.logerror = boolordefault(d, 'logerror', True)
        self.fberror = boolordefault(d, 'fberror', True)
        self.reopen = boolordefault(d, 'reopen', True)

    def setup_opts(self, dlg):
        """Set up fields in dialog"""
        Portopts.setup_opts(self, dlg)
        dlg.closeto.setValue(self.closeto)
        dlg.postclose.setValue(self.postclose)
        dlg.logerror.setChecked(self.logerror)
        dlg.fberror.setChecked(self.fberror)

    def extract_opts(self, dlg):
        """Extract network options from dialog"""
        Portopts.extract_opts(self, dlg)
        self.closeto = dlg.closeto.value()
        self.postclose = dlg.postclose.value()
        self.logerror = dlg.logerror.isChecked()
        self.fberror = dlg.fberror.isChecked()

    def write_opts(self, df):
        """Write out values of options to file with appropriate comments"""
        Portopts.write_opts(self, df)
        df.write("# Time to wait for close to complete\nclose %d\n\n" % self.closeto)
        df.write("# Time to wait after close\npostclose %d\n\n" % self.postclose)
        write_bool(df, 'logerror', self.logerror, 'Log error messages to system log')
        write_bool(df, 'fberror', self.fberror, 'Display error messages on printer list displays')

class LPDnet(Networkopts):
    """Represent options for LPD network"""

    def __init__(self, d = None):
        Networkopts.__init__(self, d)
        # Options are decoded/encoded in network command
        self.outhost = ""
        self.ctrlfile = "SPROGDIR/xtlpc-ctrl"
        self.lpdname = ""
        self.nonull = True
        self.resport = False
        self.loops = 3
        self.loopwait = 1
        self.itimeout = 5
        self.otimeout = 5
        self.retries = 0
        self.linger = 0

    def parse_nwcmd(self, cmd):
        """Parse xtlpc network command"""
        alist = parse_command(cmd)
        cmd = alist.pop(0)              # Catch empty lists in parse_device
        cmd = os.path.basename(cmd)
        if cmd != "xtlpc": raise FileParseError("xtlpc not found")

        # Set these up because we need options to cancel them

        self.nonull = False
        self.resport = True

        while len(alist) != 0:
            opt = alist.pop(0)
            if len(opt) != 2 or opt[0] != '-': continue
            opt = opt[1]
            if opt == 'H':
                self.hostname = alist.pop(0)
            elif opt == 'f':
                self.ctrlfile = decode_dir(alist.pop(0))
            elif opt == 'S':
                self.outhost = alist.pop(0)
            elif opt == 'P':
                self.lpdname = alist.pop(0)
            elif opt == 'N':
                self.nonull = True
            elif opt == 'U':
                self.resport = False
            elif opt == 'l':
                self.loops = int(alist.pop(0))
            elif opt == 'L':
                self.loopwait = int(alist.pop(0))
            elif opt == 'I':
                self.itimeout = int(alist.pop(0))
            elif opt == 'O':
                self.otimeout = int(alist.pop(0))
            elif opt == 'R':
                self.retries = int(alist.pop(0))
            elif opt == 's':
                self.linger = float(alist.pop(0))
            else:
                raise FileParseError("Unexpected option -" + opt)

    def write_netcmd(self, df):
        """Add corresponding network command to file"""
        df.write("# Network command for LPD protocol\n")
        cmdarr = []
        cmdarr.append('network='+ encode_dir('SPROGDIR/xtlpc'))
        cmdarr.append('-H')
        cmdarr.append('$SPOOLDEV')
        cmdarr.append('-f')
        cmdarr.append(encode_dir(self.ctrlfile))
        if self.nonull:
            cmdarr.append('-N')
        if not self.resport:
            cmdarr.append('-U')
        if len(self.outhost) != 0:
            cmdarr.append('-S')
            cmdarr.append(self.outhost)
        if len(self.lpdname) != 0:
            cmdarr.append('-P')
            cmdarr.append(self.lpdname)
        if self.loops != 3:
            cmdarr.append('-l')
            cmdarr.append(str(self.loops))
        if self.loopwait != 1:
            cmdarr.append('-L')
            cmdarr.append(str(self.loopwait))
        if self.itimeout != 5:
            cmdarr.append('-I')
            cmdarr.append(str(self.itimeout))
        if self.otimeout != 5:
            cmdarr.append('-O')
            cmdarr.append(str(self.otimeout))
        if self.retries != 0:
            cmdarr.append('-R')
            cmdarr.append(str(self.retries))
        if self.linger != 0:
            cmdarr.append('-s')
            cmdarr.append(str(self.linger))
        df.write(string.join(cmdarr, ' ') + '\n')

class Telnet(Networkopts):
    """Represent options for Telnet"""

    def __init__(self, d = None):
        Networkopts.__init__(self, d)
        # These are all set in the command line not in the options
        self.outport = 9100
        self.loops = 3
        self.loopwait = 1
        self.endsleep = 0
        self.linger = 0

    def parse_nwcmd(self, cmd):
        """Parse xtelnet network command"""
        alist = parse_command(cmd)
        cmd = alist.pop(0)              # Catch empty lists in parse_device
        cmd = os.path.basename(cmd)
        if cmd != "xtelnet": raise FileParseError("xtelnet not found")

        while len(alist) != 0:
            opt = alist.pop(0)
            if len(opt) != 2 or opt[0] != '-': continue
            opt = opt[1]

            if opt == 'h':
                self.hostname = alist.pop(0)
            elif opt == 'p':
                self.outport = alist.pop(0)
            elif opt == 'l':
                self.loops = int(alist.pop(0))
            elif opt == 'L':
                self.loopwait = int(alist.pop(0))
            elif opt == 't':
                self.endsleep = int(alist.pop(0))
            elif opt == 's':
                self.linger = float(alist.pop(0))
            else:
                raise FileParseError("Unexpected option -" + opt)

    def write_netcmd(self, df):
        """Add corresponding network command to file"""
        df.write("# Network command for Telnet protocol\n")
        cmdarr = []
        cmdarr.append('network='+ encode_dir('SPROGDIR/xtelnet'))
        cmdarr.append('-h')
        cmdarr.append('$SPOOLDEV')
        cmdarr.append('-p')
        cmdarr.append(str(self.outport))
        if self.loops != 3:
            cmdarr.append('-l')
            cmdarr.append(str(self.loops))
        if self.loopwait != 1:
            cmdarr.append('-L')
            cmdarr.append(str(self.loopwait))
        if self.linger != 0:
            cmdarr.append('-s')
            cmdarr.append(str(self.linger))
        if self.endsleep != 0:
            cmdarr.append('-t')
            cmdarr.append(str(self.endsleep))
        df.write(string.join(cmdarr, ' ') + '\n')

class FTPnet(Networkopts):
    """Represent options for FTP"""

    def __init__(self, d = None):
        Networkopts.__init__(self, d)
        self.outhost = ""
        self.controlport = "ftp"
        self.username = ""
        self.password = ""
        self.directory = ""
        self.outfile = ""
        self.textmode = False
        self.timeout = 750
        self.maintimeout = 30000

    def parse_nwcmd(self, cmd):
        """Parse xtftp network command"""
        alist = parse_command(cmd)
        cmd = alist.pop(0)              # Catch empty lists in parse_device
        cmd = os.path.basename(cmd)
        if cmd != "xtftp": raise FileParseError("xtftp not found")

        while len(alist) != 0:
            opt = alist.pop(0)
            if len(opt) != 2 or opt[0] != '-': continue
            opt = opt[1]

            if opt == 'h':
                self.hostname = alist.pop(0)
            elif opt == 'A':
                self.outhost = alist.pop(0)
            elif opt == 'p':
                self.controlport = alist.pop(0)
            elif opt == 'u':
                self.username = alist.pop(0)
            elif opt == 'w':
                self.password = alist.pop(0)
            elif opt == 'D':
                self.directory = alist.pop(0)
            elif opt == 'o':
                self.outfile = alist.pop(0)
            elif opt == 't':
                self.textmode = True
            elif opt == 'T':
                self.timeout = int(alist.pop(0))
            elif opt == 'R':
                self.maintimeout = int(alist.pop(0))
            else:
                raise FileParseError("Unexpected option -" + opt)

    def write_netcmd(self, df):
        """Add corresponding network command to file"""
        df.write("# Network command for FTP protocol\n")
        cmdarr = []
        cmdarr.append('network='+ encode_dir('SPROGDIR/xtftp'))
        cmdarr.append('-h')
        cmdarr.append('$SPOOLDEV')
        if len(self.outhost) != 0:
            cmdarr.append('-A')
            cmdarr.append(self.outhost)
        if self.controlport != 'ftp':
            cmdarr.append('-p')
            cmdarr.append(str(self.controlport))
        if len(self.username) != 0:
            cmdarr.append('-u')
            cmdarr.append(self.username)
        if len(self.password) != 0:
            cmdarr.append('-w')
            cmdarr.append(self.password)
        if len(self.directory) != 0:
            cmdarr.append('-D')
            cmdarr.append(self.directory)
        if len(self.outfile) != 0:
            cmdarr.append('-o')
            cmdarr.append(self.outfile)
        if self.textmode:
            cmdarr.append('-t')
        if self.timeout != 750:
            cmdarr.append('-T')
            cmdarr.append(str(self.timeout))
        if self.maintimeout != 30000:
            cmdarr.append('-R')
            cmdarr.append(str(self.maintimeout))
        df.write(string.join(cmdarr, ' ') + '\n')

class XTLHPnet(Networkopts):
    """Represent options for FTP"""

    def __init__(self, d = None):
        Networkopts.__init__(self, d)
        self.configfile = "SPROGDIR/xtsnmpdef"
        self.ctrlfile = "SPROGDIR/xtlhp-ctrl"
        self.outhost = ""
        self.outport = 9100
        self.snmpport = "snmp"
        self.community = "public"
        self.udptimeout = 1.0
        self.blocksize = 10240
        self.getnext = False

    def parse_nwcmd(self, cmd):
        """Parse xtlhp network command"""
        alist = parse_command(cmd)
        cmd = alist.pop(0)              # Catch empty lists in parse_device
        cmd = os.path.basename(cmd)
        if cmd != "xtlhp": raise FileParseError("xtlhp not found")

        while len(alist) != 0:
            opt = alist.pop(0)
            if len(opt) != 2 or opt[0] != '-': continue
            opt = opt[1]

            if opt == 'h':
                self.hostname = alist.pop(0)
            elif opt == 'H':
                self.outhost = alist.pop(0)
            elif opt == 'f':
                self.configfile = alist.pop(0)
            elif opt == 'c':
                self.ctrlfile = alist.pop(0)
            elif opt == 'p':
                self.outport = alist.pop(0)
            elif opt == 'c':
                self.community = alist.pop(0)
            elif opt == 'T':
                self.udptimeout = float(alist.pop(0))
            elif opt == 'S':
                self.snmpport = alist.pop(0)
            elif opt == 'b':
                self.blocksize = int(alist.pop(0))
            elif opt == 'N':
                self.getnext = True
            else:
                raise FileParseError("Unexpected option -" + opt)

    def write_netcmd(self, df):
        """Add corresponding network command to file"""
        df.write("# Network command for HPNPF protocol\n")
        cmdarr = []
        cmdarr.append('network='+ encode_dir('SPROGDIR/xtlhp'))
        cmdarr.append('-h')
        cmdarr.append('$SPOOLDEV')
        if len(self.outhost) != 0:
            cmdarr.append('-H')
            cmdarr.append(self.outhost)
        cmdarr.append('-f')
        cmdarr.append(encode_dir(self.configfile))
        cmdarr.append('-c')
        cmdarr.append(encode_dir(self.ctrlfile))
        if self.outport != 9100:
            cmdarr.append('-p')
            cmdarr.append(str(self.outport))
        if self.snmpport != 'snmp':
            cmdarr.append('-S')
            cmdarr.append(str(self.snmpport))
        if self.community != 'public':
            cmdarr.append('-c')
            cmdarr.append(self.community)
        if self.udptimeout != 1:
            cmdarr.append('-T')
            cmdarr.append(str(self.udptimeout))
        if self.blocksize != 10240:
            cmdarr.append('-b')
            cmdarr.append(str(self.blocksize))
        if self.getnext:
            cmdarr.append('-N')
        df.write(string.join(cmdarr, ' ') + '\n')

class Othernet(Networkopts):
    """Represent options for Other"""

    def __init__(self, d = None):
        Networkopts.__init__(self, d)
        self.networkcommand = ""

    def parse_nwcmd(self, cmd):
        """Parse xtlhp network command"""
        self.networkcommand = string.strip(cmd)
        if len(self.networkcommand) == 0:
            raise FileParseError("Could not parse command for Other")

    def write_netcmd(self, df):
        """Add corresponding network command to file"""
        df.write("# Network command for Third-party protocol\n")
        df.write("network=%s\n" % self.networkcommand)

class Printer:
    """Represent a printer we know about"""

    Canonports = { 'LPD' : 'LPDNET', 'REVERSE TELNET' : 'TELNET', 'TELNET-SNMP' : 'XTLHP', 'OTHER' : 'OTHER' }
    Knownports = ('SERIAL', 'PARALLEL', 'USB', 'LPDNET', 'TELNET', 'FTP', 'XTLHP')

    def __init__(self, n = ""):
        self.name = n
        self.descr = ""
        self.isnetwork = False
        self.device = ""
        self.cloneof = None
        self.emulation = ""
        self.mfrname = ""
        self.selectedtype = ""
        self.psemul = False
        self.usegs = False
        self.colour = False
        self.defcolour = False
        self.paper = ""
        self.porttype = ""
        self.portopts = None
        self.installed = False
        self.options = None
        self.isinst = False

    def make_clone(self, src):
        """Record details of a clone of existing printer"""
        self.isnetwork = src.isnetwork
        self.cloneof = src.name
        self.emulation = src.emulation
        self.porttype = src.porttype
        self.portopts = copy.deepcopy(src.portopts)
        self.options = copy.deepcopy(src.options)
        self.mfrname = src.mfrname
        self.selectedtype = src.selectedtype
        self.psemul = src.psemul
        self.usegs = src.usegs
        self.colour = src.colour
        self.defcolour = src.defcolour
        self.paper = src.paper
        self.options = copy.deepcopy(src.options)
        self.isinst = False

    def create_clone(self):
        """Actually create the clone"""
        try:
            os.symlink(self.cloneof, os.path.join(config.Locs['SPOOLPT'], self.name))
        except OSError as e:
            QMessageBox.warning(mw, "Clone error", "Could not create clone " + self.name + " " + e.args[1])

    def has_clones(self):
        """Check if printer has clones before we delete it"""

        for p in Printerlist.keys():
            if p == self: continue
            ptr = Printerlist[p]
            if ptr.cloneof == self.name: return True
        return False

    def copy_clones(self):
        """Copy updated details to clones"""
        for p in Printerlist.keys():
            if p == self: continue
            ptr = Printerlist[p]
            if ptr.cloneof == self.name:
                ptr.emulation = self.emulation
                ptr.porttype = self.porttype
                ptr.portopts = copy.deepcopy(self.portopts)
                ptr.options = copy.deepcopy(self.options)
                ptr.mfrname = self.mfrname
                ptr.selectedtype = self.selectedtype
                ptr.psemul = self.psemul
                ptr.usegs = self.usegs
                ptr.colour = self.colour
                ptr.defcolour = self.defcolour
                ptr.paper = self.paper
                ptr.options = copy.deepcopy(self.options)

    def parse_device(self, fname):
        """Parse .device file to get port type and options"""
        try:
            devf = open(fname)
        except IOError:
            return False

        ptc = re.compile('#\s*Porttype:\s*(\w+)')
        mtc = re.compile('(-?)(\w+)(?:\s+(\w+))?\s*$')
        nwc = re.compile('network=(.*)')
        dvc = re.compile('#\s*Orig device:\s*(.*)')

        opts = dict()
        porttype = ""
        device = ""
        nwcmd = ""

        for line in devf:
            line = string.rstrip(line)
            mp = ptc.search(line)
            if mp:
                porttype = string.upper(mp.group(1))
                continue
            mp = mtc.match(line)
            if mp:
                arg = mp.group(3)
                if arg is None:
                    arg = mp.group(1) == ''
                opts[mp.group(2)] = arg
                continue
            mp = nwc.match(line)
            if mp:
                nwcmd = mp.group(1)
            mp = dvc.match(line)
            if mp:
                device = mp.group(1)
                continue

        devf.close()

        if len(porttype) != 0:
            if porttype not in Printer.Knownports:
                try:
                    porttype = Printer.Canonports[porttype]
                except KeyError:
                    porttype = 'OTHER'

        elif len(nwcmd) != 0:
            for t in (('xtlpc', 'LPDNET'), ('xtftp', 'FTP'), ('xtlhp', 'XTLHP'), ('xtelnet', 'TELNET')):
                if re.search(t[0], nwcmd):
                    porttype = t[1]
                    break
            else:
                porttype = 'OTHER'
        elif 'baud' in opts or 'parenb' in opts:
            porttype = 'SERIAL'
        else:
            porttype = 'PARALLEL'       # Give up guess at something

        # Now set up the options according to the port type

        self.porttype = porttype
        self.device = device

        if porttype == 'SERIAL':
            self.portopts = Serialopts(opts)
        elif porttype == "PARALLEL":
            self.portopts = Parallelopts(opts)
        elif porttype == "USB":
            self.portopts = USBopts(opts)
        elif porttype == "LPDNET":
            self.portopts = LPDnet(opts)
            self.isnetwork = True
        elif porttype == "TELNET":
            self.portopts = Telnet(opts)
            self.isnetwork = True
        elif porttype == "FTP":
            self.portopts = FTPnet(opts)
            self.isnetwork = True
        elif porttype == "XTLHP":
            self.portopts = XTLHPnet(opts)
            self.isnetwork = True
        elif porttype == "OTHER":
            self.portopts = Othernet(opts)
            self.isnetwork = True

        if self.isnetwork:
            if len(nwcmd) == 0: return False
            try:
                self.portopts.parse_nwcmd(nwcmd)
            except (IndexError, FileParseError, ValueError):
                return False

        return True

    def parse_default(self, fname):
        """Parse the default file for relevant options"""
        try:
            deff = open(fname)
        except IOError:
            return False

        opts = dict()

        for line in deff:
            line = string.rstrip(line)
            m = re.match('#\s*Ptrtype:\s*(.*)', line)
            if m:
                self.emulation = m.group(1)
                continue
            m = re.match('#\s*Ptrname:\s*(.*)', line)
            if m:
                self.mfrname = m.group(1)
                continue
            m = re.match('#\s*Set up for\s*(.*)', line)
            if m:
                self.selectedtype = m.group(1)
                continue
            m = re.match('#\s*Postscript emulation', line)
            if m:
                self.psemul = True
                continue
            m = re.match('#\s*Using ghostscript', line)
            if m:
                self.usegs = True
                continue
            m = re.match('#\s*Paper:\s*(\w+)', line)
            if m:
                self.paper = m.group(1)
                continue
            m = re.match('#\s*Has colou?r', line)
            if m:
                self.colour = True
                continue
            m = re.match('#\s*Default colou?r', line)
            if m:
                self.defcolour = True
                continue
            m = re.match('(-?)(\w+)$', line)
            if m:
                opts[m.group(2)] = m.group(1) != '-'
                continue
        deff.close()

        self.options = Spoolopts(opts)
        return True

    def is_running(self):
        """Report if printer is running"""
        return os.system(makecommand('STATPROG', self.name, '>/dev/null', '2>&1')) == 0

    def stopit(self):
        """Stop the given printer"""
        return os.system(makecommand('HALTPROG', self.name, '>/dev/null', '2>&1')) == 0

    def performinstall(self):
        """Install the printer"""
        dev = self.device
        if self.isnetwork:
            lt = '-N'
        else:
            lt = '-L'
        if os.system(makecommand('ADDPROG', lt, '-l', self.device, '-D', "'" + self.descr + "'", self.name, ">/dev/null", "2>&1")) != 0:
            QMessageBox.warning(mw, "Install error", "Could not install " + self.name)
            return False
        self.isinst = True
        return True

    def performuninstall(self):
        """Uninstall the printer"""
        if os.system(makecommand('DELPROG', self.name, ">/dev/null", "2>&1")) != 0:
            QMessageBox.warning(mw, "Uninstall error", "Could not uninstall " + self.name)
            return False
        self.isinst = False
        return True

    def write_device(self, dpath):
        """Write the .device file for the printer"""
        df = open(os.path.join(dpath, '.device'), 'wb')
        #os.fchown(df.fileno(), config.Spooluid, config.Spoolgid)
        t = time.localtime()
        df.write('# Device file created %.2d/%.2d/%d %.2d:%.2d:%.2d\n' % (t.tm_mday, t.tm_mon, t.tm_year, t.tm_hour, t.tm_min, t.tm_sec))
        df.write('# Porttype: %s\n' % string.capitalize(self.porttype))
        if len(self.device) != 0:
            df.write('# Orig device: %s\n' % self.device)
        df.write("""#
# This file contains parameters for the above interface.
# It is read in ahead of any "setup file" such as "default"
# and should contain interface-only features.
#
# Please do not edit this file directly without changing "Porttype" above
# to "other".

""")
        self.portopts.write_opts(df)
        if self.isnetwork:
            self.portopts.write_netcmd(df)
        df.close()

    def out_splurge(self, outfile, pfile):
        """Output pfile with standard changes to outfile"""
        infile = '../Psetups/' + pfile + '.pi'
        try:
            inf = open(infile)
        except IOError:
            return
        spd = '${SPROGDIR-' + config.Deflocs['SPROGDIR'] + '}'
        for l in inf:
            l = re.sub('SPROGDIR', spd, l)
            l = re.sub('\\bGS\\b', config.Programs['GS'], l)
            l = re.sub('\\bIJS\\b', config.Programs['HPIJS'], l)
            l = re.sub('\\bGSSIZE\\b', self.paper, l)
            l = re.sub('\\bMODEL\\b', self.selectedtype, l)
            outfile.write(l)
        inf.close()

    def write_default(self, dpath):
        """Write the default file for the printer"""
        df = open(os.path.join(dpath, 'default'), 'wb')
        #os.fchown(df.fileno(), config.Spooluid, config.Spoolgid)
        df.write("# Ptrtype: %s\n" % self.emulation)
        df.write('# This is the "default" setup file for %s.\n' % self.name)
        df.write("""# It is used to handle forms for which there isn't a specific setup file given
# with the name of the formtype (most of the time).
# If you make changes to the file other than with this program
# please change the printer type above to "Custom"
# so that your changes don't get wiped out.
""")
        if len(self.mfrname) != 0: df.write("# Ptrname: %s\n" % self.mfrname)
        if len(self.selectedtype) != 0: df.write("# Set up for %s\n" % self.selectedtype)
        if self.psemul: df.write("# Postscript emulation\n")
        if self.usegs: df.write("# Using ghostscript\n")
        if len(self.paper) != 0: df.write("# Paper: %s\n" % self.paper)
        if self.colour: df.write("# Has colour\n")
        if self.defcolour: df.write("# Default colour\n")
        df.write("\n# Various spooler options\n\n")
        write_bool(df, 'nohdr', self.options.nohdr, 'Force off all header pages')
        write_bool(df, 'forcehdr', self.options.forcehdr, 'Force on all header pages')
        write_bool(df, 'hdrpercopy', self.options.hdrpercopy, 'Always prefix header to each copy of multiple copies')
        write_bool(df, 'retain', self.options.retain, 'Always prefix header to each copy of multiple copies')
        write_bool(df, 'norange', self.options.norange, 'Disregard job page ranges')
        write_bool(df, 'inclpage1', self.options.inclpage1, 'Always include page 1 in multiple page ranges')
        write_bool(df, 'single', self.options.single, 'Single-shot print mode')
        write_bool(df, 'addcr', self.options.addcr, 'Prefix CRs to LFs on output')
        write_bool(df, 'onecopy', self.options.onecopy, 'Only print one copy of multiple copies - multiple copies handled in driver')
        if self.emulation == 'Epson':
            self.out_splurge(df, 'Epson')
        elif self.emulation == 'PCL':
            self.out_splurge(df, 'PCL1')
            if self.usegs:
                f = "PCL2"
                if self.colour:
                    if not self.defcolour: f += 'dbw'
                else:
                    f += 'bw'
                self.out_splurge(df, f)
            elif self.psemul:
                self.out_splurge(df, 'PCLPS')
            else:
                self.out_splurge(df, 'PCLnoPS')
        elif self.emulation == 'IBM':
            self.out_splurge(df, 'IBM')
        df.close()

    def write_files(self):
        """Write out the generated .device and default files"""
        dpath = os.path.join(config.Locs['SPOOLPT'], self.name)
        if not os.path.isdir(dpath):
            try:
                os.mkdir(dpath)
                #os.chown(dpath, config.Spooluid, config.Spoolgid)
            except OSError:
                QMessageBox.warning(mw, "Printer directory", "Could not make directory for " + self.name)
                return
        self.write_device(dpath)
        self.write_default(dpath)

    def deletedef(self):
        """Delete all traces of printer def"""
        dpath = os.path.join(config.Locs['SPOOLPT'], self.name)
        try:
            if os.path.islink(dpath):
                os.unlink(dpath)
            else:
                if os.system('rm -rf ' + dpath) != 0:
                    raise OSError(13, 'Could not delete')
        except OSError as e:
            QMessageBox.warning(mw, "Could not delete", "Could not delete definition for " + self.name)
            return False
        return True

def makecommand(p, *s):
    """Make a command for execution out of list or tuple"""
    prog = config.Programs[p]
    if len(s) != 0:
        prog += ' ' + string.join(s, ' ')
    return prog

def isrunning():
    """Return true if the spooler is running"""
    return os.system(makecommand('LISTPROG', '>/dev/null', '2>&1')) == 0

def snmpgetptr(host):
    """Get printer name via SNMP if possible"""
    sp = subprocess.Popen(makecommand('GETSNMP', '-N', '-h', host, '1.3'), shell=True, bufsize=1024, stdout=subprocess.PIPE)
    result = sp.stdout.readline()
    sp.stdout.close()
    if sp.wait() != 0:
        return None
    return  string.strip(result)

def list_defptrs():
    """Get list of defined printers"""

    global Printerlist

    clones = []


    try:
        ents = os.listdir(config.Locs['SPOOLPT'])
    except OSError:
        return

    for file in ents:
        if re.match('[-.]', file):
            continue

        path = os.path.join(config.Locs['SPOOLPT'], file)
        ptr = Printer(file)

        if os.path.islink(path):
            linkto = os.path.basename(os.readlink(path))
            ptr.cloneof = linkto
            clones.append(ptr)
        else:
            deffile = os.path.join(path, 'default')
            if not os.path.isfile(deffile):
                continue
            devfile = os.path.join(path, '.device')
            if os.path.isfile(devfile):
                if not ptr.parse_device(devfile):
                    continue
            else:
                if not ptr.parse_device(deffile):
                    continue
            if not ptr.parse_default(deffile):
                continue
            Printerlist[ptr.name] = ptr

    for c in clones:
        cof = c.cloneof
        if cof in Printerlist:
            # Just in case of clone of clones
            cto = Printerlist[cof]
            while cto.cloneof is not None:
                cto = Printerlist[cto.cloneof]
            c.make_clone(cto)
            Printerlist[c.name] = c

def list_online_ptrs():
    """Fix list of printers to note the ones we have actually installed

This version is for when the scheduler is running"""

    global Printerlist, mw

    lp = subprocess.Popen(makecommand('LISTPROG', '-N', '-l', '-F', '%p:%d:%e'), shell=True, bufsize=1024, stdout=subprocess.PIPE)
    for line in lp.stdout:
        try:
            ptrname, dev, descr = re.split('\s*:\s*', line)
        except ValueError:
            continue

        if ptrname not in Printerlist:
            QMessageBox.warning(mw, "Printer def error", "Could not find installed printer " + ptrname + " in definitions")
            continue

        ptr = Printerlist[ptrname]
        m = re.match("<(.*)>", dev)
        if m:
            dev = m.group(1)
            if not ptr.isnetwork:
                QMessageBox.warning(mw, "Printer def error", ptrname + " not defined as network but installed as such")
        elif ptr.isnetwork:
            QMessageBox.warning(mw, "Printer def error", ptrname + " defined as network but not installed as such")
        ptr.device = dev
        ptr.descr = descr
        ptr.isinst = True

    lp.stdout.close()
    if lp.wait() != 0:
        QMessageBox.warning(mw, "Printer def error", "Unexpected error from list program")

def list_offline_ptrs():
    """Fix list of printers to note the ones we have actually installed

This version is for when the scheduler is NOT running"""

    global Printerlist, mw

    tmpfile = "/tmp/ptri." + str(os.getpid())
    ptrfile = os.path.join(config.Locs['SPOOLDIR'], 'spshed_pfile')
    t = subprocess.Popen(makecommand('CPLIST', ptrfile, tmpfile), shell=True)
    if t.wait() != 0:
        QMessageBox.warning(mw, "Printer def error", "Unexpected error from offline list program")
        return
    try:
        tf = open(tmpfile)
    except IOError:
        QMessageBox.warning(mw, "Printer def error", "Cannot open tmp file")
        return
    mre = re.compile("gspl-padd\s+-\w\s+-(\w)\s+-\w\s+[-A-Pa-p]+\s+-l\s+'(.*?)'\s+-D\s+'(.*?)'\s+(\w+)\s+'.*'")
    for line in tf:
        l = string.rstring(line)
        m = mre.match(l)
        if m:
            net = m.group(1) == 'N'
            dev = m.group(2)
            descr = m.group(3)
            ptrname = m.group(4)
            if ptrname not in Printerlist:
                QMessageBox.warning(mw, "Printer def error", "Could not find installed printer " + ptrname + " in definitions")
                continue
            ptr = Printerlist[ptrname]
            if net != ptr.isnetwork:
                if ptr.isnetwork:
                    QMessageBox.warning(mw, "Printer def error", ptrname + " not defined as network but installed as such")
                else:
                    QMessageBox.warning(mw, "Printer def error", ptrname + " not defined as network but installed as such")
            ptr.device = dev
            ptr.descr = descr
            ptr.isinst = True
    tf.close()
    os.unlink(tmpfile)

class Cptseldlg(QDialog, ui_ptseldlg.Ui_ptseldlg):

    def __init__(self, parent = None):
        super(Cptseldlg, self).__init__(parent)
        self.setupUi(self)

class Cpsoptdlg(QDialog, ui_psoptdlg.Ui_psoptdlg):

    def __init__(self, parent = None):
        super(Cpsoptdlg, self).__init__(parent)
        self.setupUi(self)

class Csethostdlg(QDialog, ui_sethostdlg.Ui_sethostdlg):

    def __init__(self, parent = None):
        super(Csethostdlg, self).__init__(parent)
        self.setupUi(self)

class Cnetprotodlg(QDialog, ui_netprotodlg.Ui_netprotodlg):

    def __init__(self, parent = None):
        super(Cnetprotodlg, self).__init__(parent)
        self.setupUi(self)

class Cnewptrdlg(QDialog, ui_newptrdlg.Ui_newptrdlg):

    def __init__(self, parent = None):
        super(Cnewptrdlg, self).__init__(parent)
        self.setupUi(self)

class Cserialoptsdlg(QDialog, ui_serialparams.Ui_serialparams):

    def __init__(self, parent = None):
        super(Cserialoptsdlg, self).__init__(parent)
        self.setupUi(self)

class Cparalleloptsdlg(QDialog, ui_parparams.Ui_parparams):

    def __init__(self, parent = None):
        super(Cparalleloptsdlg, self).__init__(parent)
        self.setupUi(self)

class CUSBoptsdlg(QDialog, ui_usbparams.Ui_usbparams):

    def __init__(self, parent = None):
        super(CUSBoptsdlg, self).__init__(parent)
        self.setupUi(self)

class Cnetoptsdlg(QDialog, ui_netparams.Ui_netparams):

    def __init__(self, parent = None):
        super(Cnetoptsdlg, self).__init__(parent)
        self.setupUi(self)

class CLPDoptsdlg(QDialog, ui_lpdparams.Ui_lpdparams):

    def __init__(self, parent = None):
        super(CLPDoptsdlg, self).__init__(parent)
        self.setupUi(self)

class Ctelnetoptsdlg(QDialog, ui_telnetparams.Ui_telnetparams):

    def __init__(self, parent = None):
        super(Ctelnetoptsdlg, self).__init__(parent)
        self.setupUi(self)

class CFTPoptsdlg(QDialog, ui_ftpparams.Ui_ftpparams):

    def __init__(self, parent = None):
        super(CFTPoptsdlg, self).__init__(parent)
        self.setupUi(self)

class CHPNPFoptsdlg(QDialog, ui_hpnpfparams.Ui_hpnpfparams):

    def __init__(self, parent = None):
        super(CHPNPFoptsdlg, self).__init__(parent)
        self.setupUi(self)

class Cothernetdlg(QDialog, ui_othernetdlg.Ui_othernetdlg):

    def __init__(self, parent = None):
        super(Cothernetdlg, self).__init__(parent)
        self.setupUi(self)

class Cclonedlg(QDialog, ui_clonedlg.Ui_clonedlg):

    def __init__(self, parent = None):
        super(Cclonedlg, self).__init__(parent)
        self.setupUi(self)

class Cinstotherdlg(QDialog, ui_otherinstdlg.Ui_otherinstdlg):

    def __init__(self, parent = None):
        super(Cinstotherdlg, self).__init__(parent)
        self.setupUi(self)

class Cinstnetdlg(QDialog, ui_netinstdlg.Ui_netinstdlg):

    def __init__(self, parent = None):
        super(Cinstnetdlg, self).__init__(parent)
        self.setupUi(self)

class Cinstdevdlg(QDialog, ui_devinstdlg.Ui_devinstdlg):

    def __init__(self, parent = None):
        super(Cinstdevdlg, self).__init__(parent)
        self.setupUi(self)

class Cspooloptsdlg(QDialog, ui_optsdlg.Ui_optsdlg):

    def __init__(self, parent = None):
        super(Cspooloptsdlg, self).__init__(parent)
        self.setupUi(self)

class PtrModel(QAbstractTableModel):
    """Model for holding printer info"""

    def __init__(self):
        super(PtrModel, self).__init__()
        self.printers = Printerlist.keys()
        self.printers.sort()

    def rowCount(self, index=QModelIndex()):
        return len(self.printers)

    def columnCount(self, index=QModelIndex()):
        return 6

    def insertRows(self, position):
        self.beginInsertRows(QModelIndex(), position, position)
        self.endInsertRows()

    def removeRows(self, position):
        self.beginRemoveRows(QModelIndex(), position, position)
        self.endRemoveRows()

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or not (0 <= index.row() < len(self.printers)):
            return QVariant()
        ptrname = self.printers[index.row()]
        ptr = Printerlist[ptrname]
        column = index.column()
        if role == Qt.DisplayRole:
            if column == 0:
                return QVariant(ptrname)
            elif column == 1:
                return QVariant(ptr.descr)
            elif column == 2:
                if ptr.isnetwork:
                    return QVariant('[' + ptr.device + ']')
                else:
                    return QVariant(ptr.device)
            elif column == 3:
                return QVariant(ptr.mfrname)
            elif column == 4:
                return QVariant(ptr.selectedtype)
            elif column == 5:
                if ptr.cloneof is None:
                    return QVariant("")
                else:
                    return QVariant(ptr.cloneof)
        elif role == Qt.TextColorRole and ptr.cloneof is not None:
            return QVariant(QColor(Qt.darkGreen))
        elif role == Qt.FontRole and ptr.isinst:
            qf = QFont()
            qf.setBold(True)
            return QVariant(qf)
        return QVariant()

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.TextAlignmentRole:
            return QVariant(int(Qt.AlignLeft|Qt.AlignVCenter))
        if role != Qt.DisplayRole:
            return QVariant()
        if orientation == Qt.Horizontal:
            if section == 0:
                return QVariant("Name")
            elif section == 1:
                return QVariant("Description")
            elif section == 2:
                return QVariant("Device")
            elif section == 3:
                return QVariant("Mfr Name")
            elif section == 4:
                return QVariant("Selected type")
            elif section == 5:
                return QVariant("Clone of")
        return QVariant(int(section+1000))

class PtrInstallMainwin(QMainWindow, ui_ptrinstall_main.Ui_MainWindow):

    def __init__(self):
        super(PtrInstallMainwin, self).__init__(None)
        self.setupUi(self)
        self.model = None
        self.tableView.verticalHeader().setVisible(False)

    def getselectedptr(self, moan = True):
        """Get the selected printer"""
        indx = self.tableView.currentIndex()
        if not indx.isValid():
            if moan:
                QMessageBox.warning(self, "No printer selected", "Please select a printer")
            return None
        return Printerlist[self.model.printers[indx.row()]]

    def performinstall(self, ptr, devn, desc):
        """Perform install of printer"""
        ptr.device = str(devn)
        ptr.descr = str(desc)
        if not ptr.performinstall(): return
        indx = self.tableView.currentIndex()
        if indx.isValid():
            rown = indx.row()
            self.model.setData(self.model.index(rown, 1), QVariant(desc))
            self.model.setData(self.model.index(rown, 2), QVariant(devn))
            self.model.emit(SIGNAL("dataChanged(QModelIndex,QModelIndex)"), self.model.index(rown, 0), self.model.index(rown, 5))

    def getselected_uninst_ptr(self):
        """Get the selected printer checking it isn't installed"""
        ptr = self.getselectedptr()
        if ptr is None: return None
        if ptr.isinst:
            QMessageBox.warning(self, "Printer installed", "Printer " + ptr.name + " is installed")
            return None
        return ptr

    def getselected_uncloned(self):
        """Get the selected printer checking it isn't a clone"""
        ptr = self.getselectedptr()
        if ptr is None: return None
        if ptr.cloneof is not None:
            QMessageBox.warning(self, "Printer is a clone", "Printer is a clone of <b>" + ptr.cloneof + "</b> please edit that")
            return None
        return ptr

    def on_action_Quit_triggered(self):
        QApplication.exit(0)

    def on_action_About_ptrinstall_triggered(self, checked = None):
        if checked is None: return
        QMessageBox.about(self, "About PtrInstall",
                          """<b>PtrInstall</b> v 2
                          <p>Copyright &copy; %d Xi Software Ltd
                          <p>Python %s - Qt %s""" % (time.localtime().tm_year, platform.python_version(), QT_VERSION_STR))

    def init_ptr_list(self):
        self.model = PtrModel()
        self.tableView.setModel(self.model)
        self.resizecolumns()

    def resizecolumns(self):
        for column in range(0,6):
            self.tableView.resizeColumnToContents(column)

    def proc_serialopts(self, opts):
        """Process serial options"""
        dlg = Cserialoptsdlg()
        opts.setup_opts(dlg)
        if dlg.exec_():
            opts.extract_opts(dlg)
            return True
        return False

    def proc_parallelopts(self, opts):
        """Process parallel options"""
        dlg = Cparalleloptsdlg()
        opts.setup_opts(dlg)
        if dlg.exec_():
            opts.extract_opts(dlg)
            return True
        return False

    def proc_USBopts(self, opts):
        """Process USB options"""
        dlg = CUSBoptsdlg()
        opts.setup_opts(dlg)
        if dlg.exec_():
            opts.extract_opts(dlg)
            return True
        return False

    def proc_netopts(self, opts):
        """Process common network options"""
        dlg = Cnetoptsdlg()
        opts.setup_opts(dlg)
        if dlg.exec_():
            opts.extract_opts(dlg)
            return True
        return False

    def proc_LPDopts(self, opts):
        """Process LPD options"""
        if not self.proc_netopts(opts): return False
        dlg = CLPDoptsdlg()
        dlg.outhost.setText(opts.outhost)
        dlg.ctrlfile.setText(opts.ctrlfile)
        dlg.outptrname.setText(opts.lpdname)
        dlg.nonull.setChecked(opts.nonull)
        dlg.resvport.setChecked(opts.resport)
        dlg.loops.setValue(opts.loops)
        dlg.loopwait.setValue(opts.loopwait)
        dlg.itimeout.setValue(opts.itimeout)
        dlg.otimeout.setValue(opts.otimeout)
        dlg.retries.setValue(opts.retries)
        dlg.linger.setValue(int(opts.linger * 1000 + 0.5))
        if dlg.exec_():
            opts.outhost = str(dlg.outhost.text())
            opts.ctrlfile = str(dlg.ctrlfile.text())
            opts.lpdname = str(dlg.outptrname.text())
            opts.nonull = dlg.nonull.isChecked()
            opts.resport = dlg.resvport.isChecked()
            opts.loops = dlg.loops.value()
            opts.loopwait = dlg.loopwait.value()
            opts.itimeout = dlg.itimeout.value()
            opts.otimeout = dlg.otimeout.value()
            opts.retries = dlg.retries.value()
            opts.linger = dlg.linger.value() / 1000.0
            return True
        return False

    def proc_telnetopts(self, opts):
        """Process Telnet options"""
        if not self.proc_netopts(opts): return False
        dlg = Ctelnetoptsdlg()
        dlg.portnum.setText(str(opts.outport))
        dlg.loops.setValue(opts.loops)
        dlg.loopwait.setValue(opts.loopwait)
        dlg.endsleep.setValue(opts.endsleep)
        dlg.linger.setValue(int(opts.linger * 1000 + 0.5))
        if dlg.exec_():
            portstr = str(dlg.portnum.text())
            if re.match('\d+', portstr):
                opts.outport = int(portstr)
            else:
                opts.outport = portstr
            opts.loops = dlg.loops.value()
            opts.loopwait = dlg.loopwait.value()
            opts.endsleep = dlg.endsleep.value()
            opts.linger = dlg.linger.value() / 1000.0
            return True
        return False

    def proc_FTPopts(self, opts):
        """Process FTP options"""
        if not self.proc_netopts(opts): return False
        dlg = CFTPoptsdlg()
        dlg.outhost.setText(opts.outhost)
        dlg.portnum.setText(str(opts.controlport))
        dlg.username.setText(opts.username)
        dlg.password.setText(opts.password)
        dlg.directory.setText(opts.directory)
        dlg.filename.setText(opts.outfile)
        dlg.textmode.setChecked(opts.textmode)
        dlg.selectto.setValue(opts.timeout)
        dlg.mainto.setValue(opts.maintimeout)
        if dlg.exec_():
            portstr = str(dlg.portnum.text())
            if re.match('\d+', portstr):
                opts.controlport = int(portstr)
            else:
                opts.controlport = portstr
            opts.username = str(dlg.username.text())
            opts.password = str(dlg.password.text())
            opts.directory = str(dlg.directory.text())
            opts.outfile = str(dlg.filename.text())
            opts.textmode = dlg.textmode.isChecked()
            opts.timeout = dlg.selectto.value()
            opts.maintimeout = dlg.mainto.value()
            return True
        return False

    def proc_HPNPFopts(self, opts):
        """Process HPNPF options"""
        if not self.proc_netopts(opts): return False
        dlg = CHPNPFoptsdlg()
        dlg.dataport.setText(str(opts.outport))
        dlg.snmpport.setText(str(opts.snmpport))
        dlg.defsfile.setText(opts.configfile)
        dlg.ctrlfile.setText(opts.ctrlfile)
        dlg.community.setText(opts.community)
        dlg.timeout.setValue(int(opts.udptimeout * 1000 + 0.5))
        dlg.blocksize.setValue(opts.blocksize / 1024)
        dlg.outhost.setText(opts.outhost)
        dlg.getnext.setChecked(opts.getnext)
        if dlg.exec_():
            portstr = str(dlg.dataport.text())
            if re.match('\d+', portstr):
                opts.outport = int(portstr)
            else:
                opts.outport = portstr
            portstr = str(dlg.snmpport.text())
            if re.match('\d+', portstr):
                opts.snmpport = int(portstr)
            else:
                opts.snmpport = portstr
            opts.configfile = str(dlg.defsfile.text())
            opts.ctrlfile = str(dlg.ctrlfile.text())
            opts.community = str(dlg.community.text())
            opts.udptimeout = dlg.timeout.value() / 1000.0
            opts.blocksize = dlg.blocksize.value() * 1024
            opts.outhost = str(dlg.outhost.text())
            opts.getnext = dlg.getnext.isChecked()
            return True
        return False

    def proc_otheropts(self, opts):
        """Get details for other kinds of network devices"""
        if not self.proc_netopts(opts): return False
        dlg = Cothernetdlg()
        dlg.netcmd.setText(opts.networkcommand)
        while dlg.exec_():
            cmd = str(dlg.netcmd.text())
            if len(cmd) != 0:
                opts.networkcommand = cmd
                return True
            QMessageBox.warning(self, "Printer def error", "No command given")
        return False

    def getpstypes(self, ptr):
        """Get Postscript options"""
        dlg = Cpsoptdlg()
        if dlg.exec_():
            ptr.usegs = dlg.usegs.isChecked()
            ptr.colour = dlg.hascolour.isChecked()
            ptr.defcolour = dlg.defcolour.isChecked()
            ptr.paper = str(dlg.paper.currentText())
            return True
        return False

    def getptrtype(self, ptr, name = None):
        """Select printer type from list and set selected name and emulation"""

        if name is None: name = ptr.name
        dlg = Cptseldlg()
        dlg.ptrname.setText(name)
        try:
            pf = open('../Psetups/printers.list')
            me = re.compile('(.*?)\s*:\s*(.*)')
            for l in pf:
                line = string.rstrip(l)
                if re.match('#', line):
                    continue
                m = me.match(line)
                if m is None:
                    continue
                name = m.group(1)
                emul = m.group(2)
                item = QListWidgetItem(name)
                item.setData(Qt.UserRole, QVariant(emul))
                dlg.ptrlist.addItem(item)
            pf.close()
        except IOError:
            for ptp in (('Generic HP LaserJet', 'Ljet'), ('Generic Epson', 'Epson')):
                item = QListWidgetItem(ptp[0])
                item.setData(Qt.UserRole, QVariant(ptp[1]))
                dlg.ptrlist.addItem(item)
        ptr.usegs = False
        ptr.paper = ""
        ptr.colour = False
        ptr.defcolour = False
        while dlg.exec_():
            item = dlg.ptrlist.currentItem()
            if item is not None:
                ptr.selectedtype = str(item.text())
                ptr.emulation = str(item.data(Qt.UserRole).toString())
                if re.match('ljet', ptr.emulation, re.I):
                    ptr.emulation = 'PCL'
                    if not self.getpstypes(ptr):
                        return False
                    if ptr.usegs:
                        if ptr.colour:
                            ptr.selectedtype = 'HP Color LaserJet'
                        else:
                            ptr.selectedtype = 'HP LaserJet 6'
                return True
        return False

    def proc_netparams(self, ptr, makingnew = True):
        """Set options  pertinent to networks"""

        scanok = False
        if makingnew:
            dlg = Csethostdlg()
            while dlg.exec_():
                host = str(dlg.hostorip.text())
                if len(host) == 0:
                    QMessageBox.warning(self, "No host name", "You need to give a host name")
                    continue
                try:
                    if re.match('\d', host):
                        h = socket.inet_aton(host)
                        host = socket.inet_ntoa(h)
                    else:
                        ip = socket.gethostbyname(host)
                except (socket.error, socket.gaierror):
                    QMessageBox.warning(self, "Invalid host name", "The host name is not valid")
                    continue
                scanok = dlg.OKscan.isChecked()
                break
            else:
                return False

            ptr.device = host
        else:
            host = ptr.device 

        # Got host, see if we can scan (don't do this if we have done it once)

        if scanok:
            trysnmp = False
            possports = []
            for possport in (('LPDNET', 'LPD protocol', 515),
                             ('TELNET', 'Reverse Telnet', 9100),
                             ('FTP', 'FTP transfer', 21),
                             ('XTLHP', 'HPNPF-style', 9100)):
                if try_connect(host, possport[2]):
                    possports.append(possport)
                    if possport[2] == 9100: trysnmp = True
            if len(possports) == 0:
                QMessageBox.warning(self, "No ports", "No ports are available on the printer")
                return False
            elif len(possports) == 1:
                pp = possports[0]
                if QMessageBox.question(self,
                                        "Port available",
                                        "OK to use " + pp[1] + " port for access",
                                        QMessageBox.Yes|QMessageBox.Default, QMessageBox.No|QMessageBox.Escape) != QMessageBox.Yes: return False
                ptr.porttype = pp[0]
            else:
                dlg = Cnetprotodlg()
                dlg.hostorip.setText(host)
                for pp in possports:
                    dlg.protocol.addItem(pp[1], QVariant(pp[0]))
                dlg.protocol.setCurrentIndex(0)
                if not dlg.exec_(): return False
                ptr.porttype = str(dlg.protocol.itemData(dlg.protocol.currentIndex()).toString())
            if trysnmp:
                mfrname = snmpgetptr(host)
                if mfrname:
                    ptr.mfrname = mfrname
                    if not self.getptrtype(ptr, mfrname): return False
                    if re.match('ljet', ptr.emulation, re.I):
                        ptr.emulation = 'PCL'
                        if not self.getpstypes(ptr):
                            return False
        else:
            dlg = Cnetprotodlg()
            dlg.hostorip.setText(host)
            sel = 0
            n = 0
            for possport in (('LPDNET', 'LPD protocol'), ('TELNET', 'Reverse Telnet'), ('FTP', 'FTP transfer'), ('XTLHP', 'HPNPF-style')):
                dlg.protocol.addItem(possport[1], QVariant(possport[0]))
                if ptr.porttype == possport[0]:
                    sel = n
                n += 1
            dlg.protocol.setCurrentIndex(sel)
            if not dlg.exec_(): return False
            ptr.porttype = str(dlg.protocol.itemData(dlg.protocol.currentIndex()).toString())

        if ptr.porttype == 'LPDNET':
            if not isinstance(ptr.portopts,LPDnet):
                ptr.portopts = LPDnet()
            if not self.proc_LPDopts(ptr.portopts): return False
        elif ptr.porttype == 'TELNET':
            if not isinstance(ptr.portopts,Telnet):
                ptr.portopts = Telnet()
            if not self.proc_telnetopts(ptr.portopts): return False
        elif ptr.porttype == 'FTP':
            if not isinstance(ptr.portopts,FTPnet):
                ptr.portopts = FTPnet()
            if not self.proc_FTPopts(ptr.portopts): return False
        elif ptr.porttype == 'XTLHP':
            if not isinstance(ptr.portopts,XTLHPnet):
                ptr.portopts = XTLHPnet()
            if not self.proc_XTLHPopts(ptr.portopts): return False
        else:
                QMessageBox.warning(self, "Program error", "Confused by port type " + ptr.porttype)
                return False
        return True

    def on_action_new_triggered(self, checked = None):
        if checked is None: return
        dlg = Cnewptrdlg()
        while dlg.exec_():
            dlg.hide()
            pname = str(dlg.prtname.text())
            pending_ptr = Printer()
            if len(pname) == 0:
                QMessageBox.warning(self, "No printer name", "You need to give a printer name")
                continue
            if pname in Printerlist:
                QMessageBox.warning(self, "Bad printer name", "Printer name " + pname + " clashes with existing name")
                continue
            pending_ptr.name = pname
            pending_ptr.options = Spoolopts()
            if dlg.int_serial.isChecked():
                pending_ptr.porttype = 'SERIAL'
                pending_ptr.portopts = Serialopts()
                if not self.proc_serialopts(pending_ptr.portopts): continue
                if not self.getptrtype(pending_ptr): continue
            elif dlg.int_parallel.isChecked():
                pending_ptr.porttype = 'PARALLEL'
                pending_ptr.portopts = Parallelopts()
                if not self.proc_parallelopts(pending_ptr.portopts): continue
                if not self.getptrtype(pending_ptr): continue
            elif dlg.int_USB.isChecked():
                pending_ptr.porttype = 'USB'
                pending_ptr.portopts = USBopts()
                if not self.proc_USBopts(pending_ptr.portopts): continue
                if not self.getptrtype(pending_ptr): continue
            elif dlg.int_network.isChecked():
                pending_ptr.isnetwork = True
                if not self.proc_netparams(pending_ptr): continue
                if len(pending_ptr.emulation) == 0 and not self.getptrtype(pending_ptr): continue
                # We don't do this any more because LPD protocol multiple copies isn't reliable
                # if pending_ptr.porttype == 'LPDNET': pending_ptr.options.onecopy = True
            elif dlg.int_other.isChecked():
                pending_ptr.isnetwork = True
                pending_ptr.porttype = 'OTHER'
                pending_ptr.portopts = Othernet()
                if not self.proc_otheropts(pending_ptr.portopts): continue
                if not self.getptrtype(pending_ptr): continue
            else:
                QMessageBox.warning(self, "Not interface type", "Printer name " + pname + " has no interface specified")
            Printerlist[pname] = pending_ptr
            row = self.model.rowCount()
            self.model.printers.append(pname)
            self.model.insertRows(row)
            pending_ptr.write_files()
            return

    def on_action_clone_triggered(self, checked = None):
        if checked is None: return
        if self.model.rowCount() == 0:
            QMessageBox.warning(self, "Nothing to clone", "There are no printers to clone")
            return
        dlg = Cclonedlg()
        ptr = self.getselectedptr(False)
        sel = 0
        n = 0
        for p in self.model.printers:
            dlg.clonedptr.addItem(p)
            if p == ptr.name: sel = n
            n += 1
        dlg.clonedptr.setCurrentIndex(sel)
        while dlg.exec_():
            newptr = str(dlg.clonename.text())
            if newptr not in Printerlist:
                break
            QMessageBox.warning(self, "Name clashes", "Printer name " + newptr + " is already defined")
        else:
            return
        clof = str(dlg.clonedptr.currentText())
        clofptr = Printerlist[clof]
        while clofptr.cloneof:
            clofptr = Printerlist[clofptr.cloneof]
        cloneptr = Printer(newptr)
        cloneptr.make_clone(clofptr)
        cloneptr.create_clone()
        Printerlist[newptr] = cloneptr
        row = self.model.rowCount()
        self.model.printers.append(newptr)
        self.model.insertRows(row)

    def on_action_install_triggered(self, checked = None):
        if checked is None: return
        if not isrunning():
            QMessageBox.warning(self, "Spooler not running", "Sorry but spooler is not running - cannot install")
            return
        ptr = self.getselected_uninst_ptr()
        if not ptr: return
        if ptr.isnetwork:
            if ptr.porttype == "OTHER":
                dlg = Cinstotherdlg()
            else:
                dlg = Cinstnetdlg()
            dlg.devname.setText(ptr.device)
            dlg.description.setText(ptr.descr)
            if not dlg.exec_(): return
            if ptr.porttype != "OTHER":
                devn = str(dlg.devname.text())
                try:
                    s=socket.gethostbyname(devn)
                except socket.gaierror:
                    if QMessageBox.question(self,
                                            "Unknown address",
                                            devn + " is not a known host/IP - are you sure",
                                            QMessageBox.Yes, QMessageBox.No|QMessageBox.Escape|QMessageBox.Default) != QMessageBox.Yes: return
            self.performinstall(ptr, dlg.devname.text(), dlg.description.text())
        else:
            if ptr.porttype == "SERIAL":
                g = "tty*"
            elif ptr.porttype == "PARALLEL":
                g = "lp*"
            else:
                g = "usb/lp*"
            ld = glob.glob("/dev/" + g)
            sel = -1
            n = 0
            dlg = Cinstdevdlg()
            dlg.description.setText(ptr.descr)
            for d in ld:
                bn = d[5:]
                if ptr.device == d or ptr.device == bn: sel = n
                dlg.devname.addItem(bn)
                n += 1
            if sel >= 0:
                dlg.devname.setCurrentIndex(sel)
            elif len(ptr.device) != 0:
                dlg.devname.addItem(ptr.device)
                dlg.devname.setCurrentIndex(n)
            while dlg.exec_():
                desc = str(dlg.description.text())
                if len(desc) == 0:
                    QMessageBox.warning(self, "No description given", "Please give a description")
                    continue
                dev = str(dlg.devname.currentText())
                if len(dev) == 0:
                    QMessageBox.warning(self, "No device given", "Please set a device")
                    continue
                d = dev
                if not os.path.isabs(d):
                    d = os.path.join('/dev', d)
                try:
                    st = os.stat(d)
                    if not stat.S_ISCHR(st.st_mode):
                        raise OSError("not dev")
                except OSError:
                    if QMessageBox.question(self,
                                            "Not a device",
                                            dev + " is not a device - OK",
                                            QMessageBox.Yes|QMessageBox.Default, QMessageBox.No|QMessageBox.Escape) != QMessageBox.Yes: continue
                    self.performinstall(ptr, dev, desc)
                    return
                if st.st_uid != config.Spooluid:
                    if QMessageBox.question(self,
                                            "Not owned",
                                            dev + " is not owned by the spooler - fix",
                                            QMessageBox.Yes|QMessageBox.Default, QMessageBox.No|QMessageBox.Escape) == QMessageBox.Yes:
                        #os.chown(dev, config.Spooluid, config.Spoolgid)
                        pass
                    elif st.st_gid == config.Spoolgid:
                        if (st.st_mode & 0020) == 0:
                            if QMessageBox.question(self,
                                            "Not group writeable",
                                            dev + " is not group writeable - fix",
                                            QMessageBox.Yes|QMessageBox.Default, QMessageBox.No|QMessageBox.Escape) == QMessageBox.Yes:
                                #os.chmod(dev, (st.st_mode & 07777) | 0020)
                                pass
                    elif (st.st_mode & 0002) == 0:
                        if QMessageBox.question(self,
                                            "Not world writeable",
                                            dev + " is not world writeable - fix",
                                            QMessageBox.Yes|QMessageBox.Default, QMessageBox.No|QMessageBox.Escape) == QMessageBox.Yes:
                                #os.chmod(dev, (st.st_mode & 07777) | 0002)
                                pass
                self.performinstall(ptr, dev, desc)
                return
            else:
                return

    def on_action_uninstall_triggered(self, checked = None):
        if checked is None: return
        if not isrunning():
            QMessageBox.warning(self, "Spooler not running", "Sorry but spooler is not running - cannot uninstall")
            return
        ptr = self.getselectedptr()
        if ptr is None: return
        if not ptr.isinst:
            QMessageBox.warning(self, "Printer not installed", "Printer " + ptr.name + " is not installed")
            return None
        if ptr.is_running():
            if QMessageBox.question(self,
                                    "Printer Running",
                                    ptr.name + " is running - are you sure?",
                                    QMessageBox.Yes, QMessageBox.No|QMessageBox.Escape|QMessageBox.Default) != QMessageBox.Yes: return
            ptr.stopit()
        if not ptr.performuninstall(): return
        indx = self.tableView.currentIndex()
        if indx.isValid():
            rown = indx.row()
            self.model.emit(SIGNAL("dataChanged(QModelIndex,QModelIndex)"), self.model.index(rown, 0), self.model.index(rown, 5))

    def on_action_deldef_triggered(self, checked = None):
        if checked is None: return
        ptr = self.getselected_uninst_ptr()
        if not ptr: return
        if ptr.has_clones():
            QMessageBox.warning(self, "Printer has clones", "Printer has clones - please delete those first")
            return
        if QMessageBox.question(self,
                                "Sure about deletion",
                                "Are you sure about deleting " + ptr.name + " it won't be taking any space",
                                QMessageBox.Yes, QMessageBox.No|QMessageBox.Escape|QMessageBox.Default) != QMessageBox.Yes: return

        if not ptr.deletedef(): return
        indx = self.tableView.currentIndex()
        rown = indx.row()
        self.model.removeRows(rown)
        del Printerlist[ptr.name]
        self.model.printers = Printerlist.keys()
        self.model.printers.sort()

    def on_action_edit_triggered(self, checked = None):
        if checked is None: return
        ptr = self.getselected_uncloned()
        if not ptr: return
        if ptr.isnetwork:
            if ptr.porttype == 'OTHER':
                if not self.proc_otheropts(ptr.portopts): return
            elif not self.proc_netparams(ptr, False): return
        elif ptr.porttype == 'SERIAL':
            if not self.proc_serialopts(ptr.portopts): return
        elif ptr.porttype == 'PARALLEL':
            if not self.proc_parallelopts(ptr.portopts): return
        elif ptr.porttype == 'USB':
            if not self.proc_USBopts(ptr.portopts): return
        else:
            QMessageBox.warning(self, "Confused", "Confused about port type " + ptr.porttype)
            return
        ptr.copy_clones()
        ptr.write_files()

    def on_action_editspool_triggered(self, checked = None):
        if checked is None: return
        ptr = self.getselected_uncloned()
        if not ptr: return
        dlg = Cspooloptsdlg()
        ho = 0
        if ptr.options.nohdr:
            ho = 1
        elif ptr.options.forcehdr:
            ho = 2
        dlg.hdropt.setCurrentIndex(ho)
        dlg.hdrpercopy.setChecked(ptr.options.hdrpercopy)
        dlg.onecopy.setChecked(ptr.options.onecopy)
        dlg.ignrange.setChecked(ptr.options.norange)
        dlg.inclpage1.setChecked(ptr.options.inclpage1)
        dlg.retnjob.setChecked(ptr.options.retain)
        dlg.single.setChecked(ptr.options.single)
        if dlg.exec_():
            ho = dlg.hdropt.currentIndex()
            ptr.options.nohdr = ho == 1
            ptr.options.forcehdr = ho == 2
            ptr.options.hdrpercopy = dlg.hdrpercopy.isChecked()
            ptr.options.onecopy = dlg.onecopy.isChecked()
            ptr.options.norange = dlg.ignrange.isChecked()
            ptr.options.inclpage1 = dlg.inclpage1.isChecked()
            ptr.options.retain = dlg.retnjob.isChecked()
            ptr.options.single = dlg.single.isChecked()
            ptr.copy_clones()
            ptr.write_files()

    def on_action_setptrtype_triggered(self, checked = None):
        if checked is None: return
        ptr = self.getselected_uncloned()
        if not ptr: return
        if not self.getptrtype(ptr): return
        ptr.copy_clones()
        ptr.write_files()

################################################################################
#
#               Start the action proper here
#
################################################################################

ipath = os.path.dirname(sys.argv[0])
if len(ipath) != 0: os.chdir(ipath)

app = QApplication(sys.argv)
app.setWindowIcon(QIcon('ptrinstall.png'))
mw = PtrInstallMainwin()

if os.geteuid() != 0:
    QMessageBox.warning(mw, "Not super-user", "Sorry, but this must be run under super-user")
    sys.exit(10)

# Get existing printer definitions

list_defptrs()

# Get installed printers and merge

if isrunning():
    list_online_ptrs()
else:
    list_offline_ptrs()

mw.init_ptr_list()

mw.show()
sys.exit(app.exec_())
