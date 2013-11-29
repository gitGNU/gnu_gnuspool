#
#   Copyright 2013 Free Software Foundation, Inc.
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

# These are standard options for printer installation.

# Various options:
# Generally:
# Keyword, default/type, description, [-switch, program default]
# ! before default means don't prompt for it do it another way
# ! before switch means insert only if not specified

$Options = {
    PARALLEL => [
                 [ "open", 30, "Timeout on open before giving up" ],
                 [ "offline", 300, "Timeout on write before regarding device as offline" ],
                 [ "canhang", "N", "Running processes attached to device hard to kill" ],
                 [ "outbuffer", 1024, "Size of output buffer in bytes" ],
                 [ "reopen", "N", "Close and reopen device after each job" ] ],

    USB => [            # USB is a bit dodgy - so deal with as parallel for the time being
                 [ "open", 30, "Timeout on open before giving up" ],
                 [ "offline", 300, "Timeout on write before regarding device as offline" ],
                 [ "canhang", "N", "Running processes attached to device hard to kill" ],
                 [ "outbuffer", 1024, "Size of output buffer in bytes" ],
                 [ "reopen", "N", "Close and reopen device after each job" ] ],

    SERIAL => [         # Serial is a subset of the baud rates leaving out stupid slow ones
                 [ "baud", [ 9600, 1200, 1800, 2400, 4800, 9600, 19200, 38400 ], "Baudrate" ],
                 [ "ixon", "Y", "Set xon/xoff" ],
                 [ "ixany", "N", "Set xon/xoff with any character release" ],
                 [ "csize", [ 8, 5, 6, 7, 8], "Character size" ],
                 [ "stopbits", [1, 1, 2], "Stop bits" ],
                 [ "parenb", "N", "Parity enabled" ],
                 [ "parodd", "N", "Odd parity" ],
                 [ "clocal", "N", "No modem control" ],
                 [ "open", 30, "Timeout on open before giving up" ],
                 [ "offline", 300, "Timeout on write before regarding device as offline" ],
                 [ "canhang", "N", "Running processes attached to device hard to kill" ],
                 [ "outbuffer", 1024, "Size of output buffer in bytes" ],
                 [ "reopen", "N", "Close and reopen device after each job" ],
                 [ "onlcr", "N", "Insert CR before each newline" ] ],

    STDNET => [         # General network parameters
               [ "open", 30, "Timeout on open before giving up" ],
               [ "offline", 300, "Timeout on write before regarding device as offline" ],
               [ "close", 10000, "Time to wait for close to complete" ],
               [ "postclose", 1, "Time to wait after close" ],
               [ "canhang", "N", "Running processes attached to device hard to kill" ],
               [ "outbuffer", 1024, "Size of output buffer in bytes" ],
               [ "reopen", "Y", "Restart server after each job" ],
               [ "logerror", "Y", "Log error messages to system log" ],
               [ "fberror", "Y", "Display error messages on screen" ] ],

    LPDNET => [         # Specific to LPD
               [ "host", '!$SPOOLDEV', "Host address of server", 'H' ],
               [ "ctrlfile", "!SPROGDIR/xtlpc-ctrl", "Control file", 'f' ],
               [ "outip", "!h", "Outgoing host name", 'S' ],
               [ "lpdname", "!S", "Printer name for protocol", 'P' ],
               [ "nonull", "Y", "Do not send null jobs", 'N' ],
               [ "resp", "N", "Use reserved port", '!U' ],
               [ "loops", 3, "Attempts to connect", 'l', 3 ],
               [ "loopwait", 1, "Seconds to wait between connect attempts", 'L', 1 ],
               [ "itimeout", 5, "Input timeout (for response packets", 'I', 5 ],
               [ "otimeout", 5, "Output timeout (response receiving data", 'O', 5 ],
               [ "retries", 0, "Number of retries after timeouts", 'R', 0 ],
               [ "linger", 'f:0', "Linger time (may be fractional)", 's', 0 ] ],

    TELNET => [         # Specific to telnet
               [ "host", '!$SPOOLDEV', "Host name of server", 'h' ],
               [ "port", 'p:9100', "Output port", 'p', 9100 ],
               [ "loops", 3, "Attempts to connect", 'l', 3 ],
               [ "loopwait", 1, "Seconds to wait between connect attempts", 'L' ],
               [ "endsleep", 0, "Time for process to sleep at end of each job", 't', 0 ],
               [ "linger", 'f:0', "Linger time (may be fractional", 's', 0 ] ],

    FTP => [            # Specific to FTP
               [ "host", '!$SPOOLDEV', "Host name of server", 'h' ],
               [ "myhost", '!h', "IP to send from", 'A' ],
               [ "port", 'p:ftp', "Control port", 'p', "ftp" ],
               [ "username", "s:", "User name", 'u' ],
               [ "password", "s:", "Password", 'w' ],
               [ "directory", 's:', "Directory name on server", 'D' ],
               [ "outfile", "s:", "Output file on server", 'o' ],
               [ "textmode", "N", "Force text mode", 't' ],
               [ "timeout", 750, "Timeout for select (ms)", 'T', 750 ],
               [ "maintimeout", 30000, "Timeout for FTP (ms)", 'R', 30000 ] ],

    XTLHP => [          # Specific to SNMP
              [ "host", '!$SPOOLDEV', "Host name of server", 'h' ],
              [ "configfile", "!SPROGDIR/xtsnmpdef", "Config file", 'f' ],
              [ "ctrlfile", "!SPROGDIR/xtlhp-ctrl", "Control file", 'c' ],
              [ "port", 'p:9100', "Output port", 'p', 9100 ],
              [ "myhost", '!h', "Outgoing host name (sometimes different)", 'H' ],
              [ "commun", "s:public", "SNMP Community", 'C', "public"],
              [ "timeout", "f:1", "UDP timeout", 'T', 1 ],
              [ "snmpport", "p:snmp", "SNMP Port", 'S', "snmp" ],
              [ "blksize", 10240, "Block size", 'b', 10240 ],
              [ "next", "N", "Get next on SNMP var fetches", 'N' ] ],

    STDOPTS => [        # Spooler opts
               [ "addcr", "N", "Add CR before each newline (text only!)" ],
               [ "retain", "N", "Retain all jobs on queue after printing" ],
               [ "norange", "N", "Ignore page ranges" ],
               [ "inclpage1", "N", "Always print page 1 when printing ranges" ],
               [ "single", "N", "Single-job mode" ],
               [ "onecopy", "N", "Limit to one copy - handled elsewhere" ] ],

    LPDOPTS => [        # LPD variant
               [ "addcr", "N", "Add CR before each newline (text only!)" ],
               [ "retain", "N", "Retain all jobs on queue after printing" ],
               [ "norange", "N", "Ignore page ranges" ],
               [ "inclpage1", "N", "Always print page 1 when printing ranges" ],
               [ "single", "N", "Single-job mode" ],
               [ "onecopy", "N", "Limit to one copy - handled elsewhere" ] ] } ;

# Change previous way of recording port types

$Canonports = { LPD => "LPDNET", "REVERSE TELNET" => "TELNET", "TELNET-SNMP" => "XTLHP", "OTHER" => "OTHER" };

1;
