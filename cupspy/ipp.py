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

import filebuf, struct, re, string, time, random

name_to_op = dict(IPP_PRINT_JOB=2,                      # Print a single file
                  IPP_PRINT_URI=3,                      # Print a single URL @private@
                  IPP_VALIDATE_JOB=4,                   # Validate job options
                  IPP_CREATE_JOB=5,                     # Create an empty print job
                  IPP_SEND_DOCUMENT=6,              	# Add a file to a job
                  IPP_SEND_URI=7,           		# Add a URL to a job @private@
                  IPP_CANCEL_JOB=8,                 	# Cancel a job
                  IPP_GET_JOB_ATTRIBUTES=9,             # Get job attributes
                  IPP_GET_JOBS=10,			# Get a list of jobs
                  IPP_GET_PRINTER_ATTRIBUTES=11,        # Get printer attributes
                  IPP_HOLD_JOB=12,                      # Hold a job for printing
                  IPP_RELEASE_JOB=13,			# Release a job for printing
                  IPP_RESTART_JOB=14,			# Reprint a job
                  IPP_PAUSE_PRINTER=16,                 # Stop a printer
                  IPP_RESUME_PRINTER=17,		# Start a printer
                  IPP_PURGE_JOBS=18,			# Cancel all jobs
                  IPP_SET_PRINTER_ATTRIBUTES=19,	# Set printer attributes @private@
                  IPP_SET_JOB_ATTRIBUTES=20,		# Set job attributes
                  IPP_GET_PRINTER_SUPPORTED_VALUES=21,	# Get supported attribute values
                  IPP_CREATE_PRINTER_SUBSCRIPTION=22,	# Create a printer subscription @since CUPS 1.2@
                  IPP_CREATE_JOB_SUBSCRIPTION=23,	# Create a job subscription @since CUPS 1.2@
                  IPP_GET_SUBSCRIPTION_ATTRIBUTES=24,	# Get subscription attributes @since CUPS 1.2@
                  IPP_GET_SUBSCRIPTIONS=25,             # Get list of subscriptions @since CUPS 1.2@
                  IPP_RENEW_SUBSCRIPTION=26,		# Renew a printer subscription @since CUPS 1.2@
                  IPP_CANCEL_SUBSCRIPTION=27,		# Cancel a subscription @since CUPS 1.2@
                  IPP_GET_NOTIFICATIONS=28,             # Get notification events @since CUPS 1.2@
                  IPP_SEND_NOTIFICATIONS=29,		# Send notification events @private@
                  IPP_GET_PRINT_SUPPORT_FILES=33,       # Get printer support files @private@
                  IPP_ENABLE_PRINTER=34,                # Start a printer
                  IPP_DISABLE_PRINTER=35,               # Stop a printer
                  IPP_PAUSE_PRINTER_AFTER_CURRENT_JOB=36,# Stop printer after the current job @private@
                  IPP_HOLD_NEW_JOBS=37,                 # Hold new jobs @private@
                  IPP_RELEASE_HELD_NEW_JOBS=38,         # Release new jobs @private@
                  IPP_DEACTIVATE_PRINTER=39,            # Stop a printer @private@
                  IPP_ACTIVATE_PRINTER=40,              # Start a printer @private@
                  IPP_RESTART_PRINTER=41,		# Restart a printer @private@
                  IPP_SHUTDOWN_PRINTER=42,		# Turn a printer off @private@
                  IPP_STARTUP_PRINTER=43,		# Turn a printer on @private@
                  IPP_REPROCESS_JOB=44,			# Reprint a job @private@
                  IPP_CANCEL_CURRENT_JOB=45,		# Cancel the current job @private@
                  IPP_SUSPEND_CURRENT_JOB=46,		# Suspend the current job @private@
                  IPP_RESUME_JOB=47,			# Resume the current job @private@
                  IPP_PROMOTE_JOB=48,			# Promote a job to print sooner @private@
                  IPP_SCHEDULE_JOB_AFTER=49,		# Schedule a job to print after another @private@
                  IPP_PRIVATE=0x4000,			# Reserved @private@
                  CUPS_GET_DEFAULT=0x4001,		# Get the default printer
                  CUPS_GET_PRINTERS=0x4002,		# Get a list of printers and/or classes
                  CUPS_ADD_MODIFY_PRINTER=0x4003,	# Add or modify a printer
                  CUPS_DELETE_PRINTER=0x4004,		# Delete a printer
                  CUPS_GET_CLASSES=0x4005,		# Get a list of classes @deprecated@
                  CUPS_ADD_MODIFY_CLASS=0x4006,		# Add or modify a class
                  CUPS_DELETE_CLASS=0x4007,		# Delete a class
                  CUPS_ACCEPT_JOBS=0x4008,		# Accept new jobs on a printer
                  CUPS_REJECT_JOBS=0x4009,		# Reject new jobs on a printer
                  CUPS_SET_DEFAULT=0x400A,		# Set the default printer
                  CUPS_GET_DEVICES=0x400B,		# Get a list of supported devices
                  CUPS_GET_PPDS=0x400C,			# Get a list of supported drivers
                  CUPS_MOVE_JOB=0x400D,			# Move a job to a different printer
                  CUPS_AUTHENTICATE_JOB=0x400E,		# Authenticate a job @since CUPS 1.2@
                  CUPS_GET_PPD=0x400E)			# Get a PPD file @since CUPS 1.3@

name_to_resp = dict(IPP_OK=0,                           # successful-ok
                    IPP_OK_SUBST=1,			# successful-ok-ignored-or-substituted-attributes
                    IPP_OK_CONFLICT=2,			# successful-ok-conflicting-attributes
                    IPP_OK_IGNORED_SUBSCRIPTIONS=3,	# successful-ok-ignored-subscriptions
                    IPP_OK_IGNORED_NOTIFICATIONS=4,	# successful-ok-ignored-notifications
                    IPP_OK_TOO_MANY_EVENTS=5,		# successful-ok-too-many-events
                    IPP_OK_BUT_CANCEL_SUBSCRIPTION=6,	# successful-ok-but-cancel-subscription
                    IPP_OK_EVENTS_COMPLETE=7,		# successful-ok-events-complete
                    IPP_REDIRECTION_OTHER_SITE=0x200,	#
                    CUPS_SEE_OTHER=0x280,               # cups-see-other
                    IPP_BAD_REQUEST=0x0400,             # client-error-bad-request
                    IPP_FORBIDDEN=0x401,		# client-error-forbidden
                    IPP_NOT_AUTHENTICATED=0x402,	# client-error-not-authenticated
                    IPP_NOT_AUTHORIZED=0x403,		# client-error-not-authorized
                    IPP_NOT_POSSIBLE=0x404,		# client-error-not-possible
                    IPP_TIMEOUT=0x405,			# client-error-timeout
                    IPP_NOT_FOUND=0x406,		# client-error-not-found
                    IPP_GONE=0x407,			# client-error-gone
                    IPP_REQUEST_ENTITY=0x408,		# client-error-request-entity-too-large
                    IPP_REQUEST_VALUE=0x409,		# client-error-request-value-too-long
                    IPP_DOCUMENT_FORMAT=0x40A,		# client-error-document-format-not-supported
                    IPP_ATTRIBUTES=0x40B,		# client-error-attributes-or-values-not-supported
                    IPP_URI_SCHEME=0x40C,		# client-error-uri-scheme-not-supported
                    IPP_CHARSET=0x40D,			# client-error-charset-not-supported
                    IPP_CONFLICT=0x40E,			# client-error-conflicting-attributes
                    IPP_COMPRESSION_NOT_SUPPORTED=0x40F,# client-error-compression-not-supported
                    IPP_COMPRESSION_ERROR=0x410,	# client-error-compression-error
                    IPP_DOCUMENT_FORMAT_ERROR=0x411,	# client-error-document-format-error
                    IPP_DOCUMENT_ACCESS_ERROR=0x412,	# client-error-document-access-error
                    IPP_ATTRIBUTES_NOT_SETTABLE=0x413,	# client-error-attributes-not-settable
                    IPP_IGNORED_ALL_SUBSCRIPTIONS=0x414,# client-error-ignored-all-subscriptions
                    IPP_TOO_MANY_SUBSCRIPTIONS=0x415,	# client-error-too-many-subscriptions
                    IPP_IGNORED_ALL_NOTIFICATIONS=0x416,# client-error-ignored-all-notifications
                    IPP_PRINT_SUPPORT_FILE_NOT_FOUND=0x417,# client-error-print-support-file-not-found
                    IPP_INTERNAL_ERROR=0x0500,		# server-error-internal-error
                    IPP_OPERATION_NOT_SUPPORTED=0x501,	# server-error-operation-not-supported
                    IPP_SERVICE_UNAVAILABLE=0x502,	# server-error-service-unavailable
                    IPP_VERSION_NOT_SUPPORTED=0x503,	# server-error-version-not-supported
                    IPP_DEVICE_ERROR=0x504,		# server-error-device-error
                    IPP_TEMPORARY_ERROR=0x505,		# server-error-temporary-error
                    IPP_NOT_ACCEPTING=0x506,		# server-error-not-accepting-jobs
                    IPP_PRINTER_BUSY=0x507,		# server-error-busy
                    IPP_ERROR_JOB_CANCELED=0x508,	# server-error-job-canceled
                    IPP_MULTIPLE_JOBS_NOT_SUPPORTED=0x509,# server-error-multiple-document-jobs-not-supported
                    IPP_PRINTER_IS_DEACTIVATED=0x50A)	# server-error-printer-is-deactivated

name_to_tag = dict(IPP_TAG_ZERO=0,                      # Zero tag - used for separators
                   IPP_TAG_OPERATION=1,			# Operation group
                   IPP_TAG_JOB=2,			# Job group
                   IPP_TAG_END=3,			# End-of-attributes
                   IPP_TAG_PRINTER=4,			# Printer group
                   IPP_TAG_UNSUPPORTED_GROUP=5,		# Unsupported attributes group
                   IPP_TAG_SUBSCRIPTION=6,		# Subscription group
                   IPP_TAG_EVENT_NOTIFICATION=7,	# Event group
                   IPP_TAG_UNSUPPORTED_VALUE=16,        # Unsupported value
                   IPP_TAG_DEFAULT=17,			# Default value
                   IPP_TAG_UNKNOWN=18,			# Unknown value
                   IPP_TAG_NOVALUE=19,			# No-value value
                   IPP_TAG_NOTSETTABLE=0x15,		# Not-settable value
                   IPP_TAG_DELETEATTR=0x16,             # Delete-attribute value
                   IPP_TAG_ADMINDEFINE=0x17,		# Admin-defined value
                   IPP_TAG_INTEGER=0x21,                # Integer value
                   IPP_TAG_BOOLEAN=0x22,                # Boolean value
                   IPP_TAG_ENUM=0x23,			# Enumeration value
                   IPP_TAG_STRING=0x30,                 # Octet string value
                   IPP_TAG_DATE=0x31,			# Date/time value
                   IPP_TAG_RESOLUTION=0x32,             # Resolution value
                   IPP_TAG_RANGE=0x33,			# Range value
                   IPP_TAG_BEGIN_COLLECTION=0x34,       # Beginning of collection value
                   IPP_TAG_TEXTLANG=0x35,               # Text-with-language value
                   IPP_TAG_NAMELANG=0x36,               # Name-with-language value
                   IPP_TAG_END_COLLECTION=0x37,		# End of collection value
                   IPP_TAG_TEXT=0x41,			# Text value
                   IPP_TAG_NAME=0x42,			# Name value
                   IPP_TAG_KEYWORD=0x44,                # Keyword value
                   IPP_TAG_URI=0x45,			# URI value
                   IPP_TAG_URISCHEME=0x46,              # URI scheme value
                   IPP_TAG_CHARSET=0x47,                # Character set value
                   IPP_TAG_LANGUAGE=0x48,               # Language value
                   IPP_TAG_MIMETYPE=0x49,               # MIME media type value
                   IPP_TAG_MEMBERNAME=0x4A,             # Collection member name value
                   IPP_TAG_MASK=0x7fffffff,             # Mask for copied attribute values
                   IPP_TAG_COPY=-0x7fffffff-1)          # Bitflag for copied attribute values

name_to_res = dict(IPP_RES_PER_INCH=3,                  # Pixels per inch
                   IPP_RES_PER_CM=4)			# Pixels per centimeter

name_to_finish = dict(IPP_FINISHINGS_NONE=3,            # No finishing
                      IPP_FINISHINGS_STAPLE=4,		# Staple (any location)
                      IPP_FINISHINGS_PUNCH=5,		# Punch (any location/count)
                      IPP_FINISHINGS_COVER=6,		# Add cover
                      IPP_FINISHINGS_BIND=7,		# Bind
                      IPP_FINISHINGS_SADDLE_STITCH=8,	# Staple interior
                      IPP_FINISHINGS_EDGE_STITCH=9,	# Stitch along any side
                      IPP_FINISHINGS_FOLD=10,		# Fold (any type)
                      IPP_FINISHINGS_TRIM=11,		# Trim (any type)
                      IPP_FINISHINGS_BALE=12,		# Bale (any type)
                      IPP_FINISHINGS_BOOKLET_MAKER=13,	# Fold to make booklet
                      IPP_FINISHINGS_JOB_OFFSET=14,	# Offset for binding (any type)
                      IPP_FINISHINGS_STAPLE_TOP_LEFT=20,# Staple top left corner
                      IPP_FINISHINGS_STAPLE_BOTTOM_LEFT=21,	# Staple bottom left corner
                      IPP_FINISHINGS_STAPLE_TOP_RIGHT=22,	# Staple top right corner
                      IPP_FINISHINGS_STAPLE_BOTTOM_RIGHT=23,	# Staple bottom right corner
                      IPP_FINISHINGS_EDGE_STITCH_LEFT=24,	# Stitch along left side
                      IPP_FINISHINGS_EDGE_STITCH_TOP=25,	# Stitch along top edge
                      IPP_FINISHINGS_EDGE_STITCH_RIGHT=26,	# Stitch along right side
                      IPP_FINISHINGS_EDGE_STITCH_BOTTOM=27,	# Stitch along bottom edge
                      IPP_FINISHINGS_STAPLE_DUAL_LEFT=28,	# Two staples on left
                      IPP_FINISHINGS_STAPLE_DUAL_TOP=29,	# Two staples on top
                      IPP_FINISHINGS_STAPLE_DUAL_RIGHT=30,	# Two staples on right
                      IPP_FINISHINGS_STAPLE_DUAL_BOTTOM=31,	# Two staples on bottom
                      IPP_FINISHINGS_BIND_LEFT=50,      # Bind on left
                      IPP_FINISHINGS_BIND_TOP=51,       # Bind on top
                      IPP_FINISHINGS_BIND_RIGHT=52,     # Bind on right
                      IPP_FINISHINGS_BIND_BOTTOM=53)    # Bind on bottom

name_to_orient = dict(IPP_PORTRAIT=3,                	# No rotation
                      IPP_LANDSCAPE=4,                       # 90 degrees counter-clockwise
                      IPP_REVERSE_LANDSCAPE=5,               # 90 degrees clockwise
                      IPP_REVERSE_PORTRAIT=6)                # 180 degrees

name_to_qual = dict(IPP_QUALITY_DRAFT=3,                  # Draft quality
                    IPP_QUALITY_NORMAL=4,                 # Normal quality
                    IPP_QUALITY_HIGH=5)			# High quality

name_to_jstate = dict(IPP_JOB_PENDING=3,                     # Job is waiting to be printed
                      IPP_JOB_HELD=4,                        # Job is held for printing
                      IPP_JOB_PROCESSING=5,                  # Job is currently printing
                      IPP_JOB_STOPPED=6,                     # Job has been stopped
                      IPP_JOB_CANCELED=7,                    # Job has been canceled
                      IPP_JOB_ABORTED=8,                     # Job has aborted due to error
                      IPP_JOB_COMPLETED=9)                   # Job has completed successfully

name_to_pstate = dict(IPP_PRINTER_IDLE=3,                    # Printer is idle
                      IPP_PRINTER_PROCESSING=4,              # Printer is working
                      IPP_PRINTER_STOPPED=5)		     # Printer is stopped

name_to_ippstate = dict(IPP_ERROR=-1,                        # An error occurred
                        IPP_IDLE=0,                          # Nothing is happening/request completed
                        IPP_HEADER=1,                        # The request header needs to be sent/received
                        IPP_ATTRIBUTE=2,                     # One or more attributes need to be sent/received
                        IPP_DATA=3)                          # IPP request data needs to be sent/received

# Set up reverse dics

def inverse_dic(dic):
    idic={}
    for n,v in dic.iteritems():
        idic[v] = n
    return idic

op_to_name=inverse_dic(name_to_op)
resp_to_name=inverse_dic(name_to_resp)
tag_to_name=inverse_dic(name_to_tag)
res_to_name=inverse_dic(name_to_res)
finish_to_name=inverse_dic(name_to_finish)
orient_to_name=inverse_dic(name_to_orient)
qual_to_name=inverse_dic(name_to_qual)
jstate_to_name=inverse_dic(name_to_jstate)
pstate_to_name=inverse_dic(name_to_pstate)
ippstate_to_name=inverse_dic(name_to_ippstate)

# Common tags

IPP_TAG_INTEGER = name_to_tag['IPP_TAG_INTEGER']
IPP_TAG_ENUM = name_to_tag['IPP_TAG_ENUM']
IPP_TAG_BOOLEAN = name_to_tag['IPP_TAG_BOOLEAN']
IPP_TAG_DATE = name_to_tag['IPP_TAG_DATE']
IPP_TAG_RESOLUTION = name_to_tag['IPP_TAG_RESOLUTION']
IPP_TAG_RANGE = name_to_tag['IPP_TAG_RANGE']
IPP_TAG_TEXTLANG = name_to_tag['IPP_TAG_TEXTLANG']
IPP_TAG_NAMELANG = name_to_tag['IPP_TAG_NAMELANG']
IPP_TAG_BEGIN_COLLECTION = name_to_tag['IPP_TAG_BEGIN_COLLECTION']
IPP_TAG_UNSUPPORTED_GROUP = name_to_tag['IPP_TAG_UNSUPPORTED_GROUP']
IPP_TAG_UNSUPPORTED_VALUE = name_to_tag['IPP_TAG_UNSUPPORTED_VALUE']
IPP_TAG_END = name_to_tag['IPP_TAG_END']

# Common tags

IPP_TAG_TEXT=0x41
IPP_TAG_URI=0x45
IPP_TAG_CHARSET=0x47
IPP_TAG_LANGUAGE=0x48

# Might as well have this

IPP_OK = 0

IPP_JOB_PENDING=3
IPP_JOB_HELD=4
IPP_JOB_PROCESSING=5

# Also have handy group names

IPP_TAG_OPERATION=1
IPP_TAG_JOB=2
IPP_TAG_PRINTER=4

class IppError(Exception):
    pass

def generate_value(name, valuetup):
    """Generate value length field and value"""
    tag = valuetup[0]
    value = valuetup[1]
    if tag == IPP_TAG_INTEGER or tag == IPP_TAG_ENUM:
        resfld = struct.pack('!I', int(value))
    elif tag == IPP_TAG_BOOLEAN:
        resfld = chr(value)
    elif tag == IPP_TAG_DATE:
        year,month,day,hours,minutes,seconds,wday,yday,isdst = time.gmtime(value)
        resfld = struct.pack('!H6Bc2B', year, month, day, hours, minutes, seconds, 0, '+', 0, 0)
    elif tag == IPP_TAG_RESOLUTION:
        resfld = struct.pack('!2Ic', *value)
    elif tag == IPP_TAG_RANGE:
        resfld = struct.pack('!2I', *value)
    elif tag == IPP_TAG_TEXTLANG or tag == IPP_TAG_NAMELANG:
        cs, t = value
        resfld = struct.pack('!H', len(cs)) + cs + struct.pack('!H', len(t)) + t
    elif tag == IPP_TAG_BEGIN_COLLECTION:
        raise IppError("Sorry cannot do collections yet")
    else:
        resfld = value
    return struct.pack('!BH', tag, len(name)) + name + struct.pack('!H', len(resfld)) + resfld

class ippvalue:
    """Representation of value field in IPP"""
    def __init__(self, tagval):
        if isinstance(tagval, str):
            try:
                self.tag = name_to_tag[tagval]
            except KeyError:
                raise IppError("Unknown tag value " + tagval)
        else:
            self.tag = tagval
        self.name = ''
        self.value = []

    def setname(self, name):
        """Set value name"""
        self.name = name

    def setvalue(self, value):
        """Set a value to given item"""
        self.value.append(value)

    def setnamevalues(self, name, values):
        """Initialise name and values"""
        self.name = name;
        if isinstance(values,str) or isinstance(values,int):
            self.value.append((self.tag, values))
        else:
            for v in values:
                self.value.append((self.tag, v))

    def display(self, indent):
        """Display/trace the value"""
        for i in range(0,indent) : print "\t",
        print "Value:", tag_to_name[self.tag]
        for i in range(0,indent) : print "\t",
        print "Name:", self.name
        for i,v in enumerate(self.value):
            for j in range(0,indent) : print "\t",
            print i, tag_to_name[v[0]], v[1]

    def generate(self):
        """Turn value into IPP string"""
        name = self.name
        result = ''
        for v in self.value:
            result += generate_value(name, v)
            name = ''
        return  result

    def find_attribute(self, name):
        """See if the attribute has the given name"""
        if  self.name == name:
            return self
        return  None

    def get_value(self, ind=0):
        """Get the actual indth value of an ippvalue"""
        return self.value[ind][1]

class ippgroup:
    """Representation of group field in IPP"""
    def __init__(self, tagval):
        self.tag = tagval
        self.values = []

    def setvalue(self, value):
        """Set a group value"""
        self.values.append(value)

    def display(self, indent):
        """Display/trace of group"""
        print "Group:", tag_to_name[self.tag]
        for i,v in enumerate(self.values):
            print "Group value",i,":"
            v.display(1)

    def generate(self):
        """Generate an IPP string for the group"""
        result = chr(self.tag)
        for v in self.values:
            result += v.generate()
        return  result

    def find_attribute(self, name):
        """Find the first attribute with the given name"""
        for v in self.values:
            fv = v.find_attribute(name)
            if  fv:
                return  fv
        return  None

class ipp:
    """Representation of IPP"""
    def __init__(self, fb = None):
        """Initialise with optional parse string"""
        self.filb = fb
        self.values = []
        self.majv = 1;
        self.minv = 1;
        self.statuscode = IPP_OK
        self.id = 0

    def setrespcode(self, code, ident = 0):
        """Set up response code - code may be string or tag number"""
        if isinstance(code, str):
            try:
                self.statuscode = name_to_resp[code]
            except KeyError:
                raise IppError("Unknown response code " + code)
        else:
            self.statuscode = code
        if  ident == 0:
            ident = random.randint(1, 1000000)
        self.id = ident

    def pushvalue(self, value):
        """Append value to structure"""
        self.values.append(value)

    def parsehdr(self):
        """Parse header of IPP string"""
        return struct.unpack('!2BHI', self.filb.get(8))

    def parsevalue(self, tag, length):
        """Parse value structure making conversions where appropriate"""
        field = self.filb.get(length)
        if tag == IPP_TAG_INTEGER or tag == IPP_TAG_ENUM:
            result = struct.unpack('!I', field)[0]
        elif tag == IPP_TAG_BOOLEAN:
            result = ord(field)
        elif tag == IPP_TAG_DATE:
            year, month, day, hours, minutes, seconds, decisec, plusmin, utchr, utcmin = struct.unpack('!H6Bc2B', field)
            t = time.mktime((year, month, day, hours, minutes, seconds, 0, 0, -1))
            if plusmin == '-':
                t += utchr * 3600 + utcmin * 60
            else:
                t -= utchr * 3600 + utcmin * 60
            result = t
        elif tag == IPP_TAG_RESOLUTION:
            result = struct.unpack('!2Ic', field)
        elif tag == IPP_TAG_RANGE:
            result = struct.unpack('!2I', field);
        elif tag == IPP_TAG_TEXTLANG or tag == IPP_TAG_NAMELANG:
            csleng = struct.unpack('!H', field[0:2])[0]
            field = field[2:]
            cs = field[0:csleng]
            field = field[csleng:]
            tleng = struct.unpack('!H', field[0:2])[0]
            field = field[2:]
            t = field[0:tleng]
            result = (cs, t)
        elif tag == IPP_TAG_BEGIN_COLLECTION:
            raise IppError("Sorry cannot do collections yet")
        else:
            result = field
        return (tag, result)

    def parseattr(self):
        """Parse attribute sequence"""
        tag = self.filb.getch()
        resval = ippvalue(tag)
        namelength = struct.unpack('!H', self.filb.get(2))[0]
        resval.setname(self.filb.get(namelength))
        valuelength = struct.unpack('!H', self.filb.get(2))[0]
        resval.setvalue(self.parsevalue(tag, valuelength))
        while self.filb.peekch() >= IPP_TAG_UNSUPPORTED_VALUE:
            newtag, namelength = struct.unpack('!BH', self.filb.peek(3))
            if namelength != 0:
                break
            self.filb.skip(3)
            valuelength = struct.unpack('!H', self.filb.get(2))[0]
            resval.setvalue(self.parsevalue(newtag, valuelength))
        return resval

    def parsegrp(self, tag):
        """Parse a group of attributes"""
        grp = ippgroup(tag)
        while 1:
            attr = self.parseattr()
            grp.setvalue(attr)
            if self.filb.peekch() < IPP_TAG_UNSUPPORTED_VALUE:
                return  grp

    def parseipp(self):
        """Parse an IPP string"""
        self.majv, self.minv, self.statuscode, self.id = self.parsehdr()
        while 1:
            if self.filb.peekch() < IPP_TAG_UNSUPPORTED_VALUE:
                # Reading groups
                grptag = self.filb.getch()
                if grptag == IPP_TAG_END:
                    break
                item = self.parsegrp(grptag)
            else:
                item = self.parseattr()
            self.values.append(item)

    def parse(self):
        """Parse an IPP structure and catch errors"""
        try:
            self.parseipp()
        except (TypeError, struct.error, filebuf.filebufEOF):
            raise IppError("Problem parsing input")

    def display(self, resp=0):
        """Display/trace an IPP structure
Argument is non-zero to specify it's a response structure"""
        try:
            if resp:
                code = resp_to_name[self.statuscode]
            else:
                code = op_to_name[self.statuscode]
        except KeyError:
            if resp:
                code = "Unknown status did you really mean resp"
            else:
                code = "Unknown code didn't you mean resp"
        print "Version=", self.majv, ".", self.minv
        print "Code=", code, "Id=", self.id
        if len(self.values) > 0:
            print "Items:"
            for v in self.values:
                v.display(0)

    def generate(self):
        """Generate an IPP string from the contents"""
        result = struct.pack('!2BHI', self.majv, self.minv, self.statuscode, self.id)
        for v in self.values:
            result += v.generate()
        return  result + chr(IPP_TAG_END)

    def find_attribute(self, name):
        """Find and return the first value field with the given name"""
        for v in self.values:
            fv = v.find_attribute(name)
            if  fv:
                return fv
        return None
