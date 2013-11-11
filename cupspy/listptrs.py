#! /usr/bin/python

# Copyright (C) 2011  Free Software Foundation
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

try:
    import argparse
    Have_argparse = True
except ImportError:
    import optparse
    Have_argparse = False
import sys
import string
import conf
import os
import os.path

def max(a,b):
    """Calculate maximum"""
    if a>b: return a
    else: return b

# Set separator to space as default
sep = ' '

if Have_argparse:

    # Here is argparse version of argument parsing in case we have it available

    parser = argparse.ArgumentParser(description='List cupspy printers', prog='List_printers')
    parser.add_argument('--version', action='version', version="%(prog)s 1.0 (c) Xi Software Ltd")
    parser.add_argument('-c', '--configfile', default='cupspy.conf', help='Configuration file')
    parser.add_argument('-q', '--queue-name', help='GNUspool Queue name to restrict view for')
    parser.add_argument('-s', '--separator', help='Separator string between columns')
    parser.add_argument('-t', '--tabulate', action='store_true', help='Left-justify columns')
    adict = vars(parser.parse_args())
    confin = adict['configfile']
    qn = adict['queue_name']
    if adict['separator'] is not None: sep = adict['separator']
    tabulating = adict['tabulate']

else:

    # Here is optparse version of argument parsing needed for Python 2.6 or earlier

    parser = optparse.OptionParser("%prog [options]", version='%prog 1.0 (c) Xi Software Ltd')
    parser.add_option('-c', '--configfile', action='store', type='string', dest='configfile', default='cupspy.conf', metavar='FILE', help='Configuration file')
    parser.add_option('-q', '--queue-name', action='store', type='string', dest='queue_name', metavar='NAME', help='GNUspool Queue name to restrict view for')
    parser.add_option('-s', '--separator', action='store', type='string', dest='separator', metavar='STRING', help='Separator string between columns')
    parser.add_option('-t', '--tabulate', action='store_true', dest='tabulate', default=False, help='Left-justify columns')
    (options, args) = parser.parse_args()
    confin = options.configfile
    qn = options.queue_name
    if options.separator is not None: sep = options.separator
    tabulating = options.tabulate

# If we can't see the config file and it isn't absolute and we were found using an absolute path, swap to that

if not os.path.exists(confin) and not os.path.isabs(confin):
    prog = sys.argv[0]
    if os.path.isabs(prog):
        try:
            os.chdir(os.path.dirname(prog))
        except OSError:
            sys.stdout = sys.stderr
            print "Cannot change to directory of " + prog
            sys.exit(11)

configuration = conf.Conf()
try:
    configuration.parse_conf_file(confin)
    plist = configuration.list_printers()
    plist.sort()
    reslist = []
    for p in plist:
        gsp = configuration.get_param_value(p, 'gsprinter')
        if qn is not None and qn != gsp: continue
        form = configuration.get_param_value(p, 'form')
        inf = configuration.get_attribute_value(p, 'printer-info')
        reslist.append((p, gsp, form, inf))
    if tabulating:
        widths = [reduce(max,map(lambda x: len(x[n]), reslist)) for n in range(0,4)]
        fmts = [ '{' + str(n+1) + ':' + str(widths[n]) + '}' for n in range(0,4)]
    else:
        fmts = [ '{' + str(n+1) + '}' for n in range(0, 4) ]
    fmt = string.join(fmts, '{0}')
    for r in reslist:
        print fmt.format(sep, *r)
except conf.ConfError as err:
    # Specific case of no printers at all - just exit
    if err.args[1] == 150:
        sys.exit(0)
    sys.stdout = sys.stderr
    print err.args[0]
    sys.exit(err.args[1])
