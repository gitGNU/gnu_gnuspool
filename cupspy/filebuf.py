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

import select
import socket
import syslog

class filebufEOF(Exception):
    """Throw me on EOF"""
    pass

class filebuf:
    """Buffer up file/socket for efficient reading"""

    def __init__(self, f, tout = 20):
	"""Initialise with file-type argument"""
	self.sock = f
	self.buffer = ""
	self.ptr = 0
	self.expected = 0
	self.timeout = tout
	f.setblocking(0)
        self.pollobj = select.poll()
        self.pollobj.register(f, select.POLLIN|select.POLLERR|select.POLLHUP)

    def set_expected(self, l):
	"""Set expected value from HTTP header

Remember to adjust for anything we already read"""
	self.expected = l + self.ptr - len(self.buffer)

    def readbuf(self, length):
	"""Read a bufferful up to length

We are set to non-blocking so we wait for something to arrive first"""
        try:
            pollres = self.pollobj.poll(int(self.timeout * 1000))
            if len(pollres) == 0:
                syslog.syslog(syslog.LOG_INFO, "Timeout on poll")
                raise filebufEOF()
            pollfd, pollev = pollres[0]
            if pollev == select.POLLIN: return self.sock.recv(length)
            syslog.syslog(syslog.LOG_INFO, "Error on poll")
            raise filebufEOF()
        except socket.error:
            syslog.syslog(syslog.LOG_INFO, "Exception on select")
            raise filebufEOF()

    def want(self, length):
	"""Say we want a string of length given from buffer"""
	lenav = len(self.buffer) - self.ptr
	while  lenav < length:
	    getl = 1024
	    self.buffer = self.buffer[self.ptr:]
	    self.ptr = 0
	    nb = self.readbuf(getl)
	    lng = len(nb)
	    if lng == 0:
		raise filebufEOF()
	    lenav += lng
	    self.expected -= len(nb)
	    self.buffer += nb

    def get(self, length):
	"""Get a string of length given from the file"""
	self.want(length)
	result = self.buffer[self.ptr:self.ptr+length]
	self.ptr += length
	return result

    def peek(self, length):
	"""Peek at the next string of length given from the file"""
	self.want(length)
	return	self.buffer[self.ptr:self.ptr+length]

    def skip(self, length):
	"""Skip over the next string of length given from the file"""
	self.ptr += length

    def getch(self):
	"""Get the ord value of the next character"""
	return ord(self.get(1))

    def peekch(self):
	"""Peek at the ord value of the next character"""
	return ord(self.peek(1))

    def getrem(self):
	"""Get some of the remaining stuff we are expecting"""
	if  self.ptr > 0 and len(self.buffer) -	 self.ptr > 0:
	    ret = self.buffer[self.ptr:]
	    self.ptr = 0
	    return  ret
	l = self.expected
	if  l <= 0:
	    return ""
	if  l > 1024:
	    l = 1024
	ret = self.readbuf(l)
	self.expected -= len(ret)
	return ret

    def readline(self):
	"""Emulate readline function"""
	result = ""
	try:
	    while 1:
		ch = self.get(1)
		result += ch
		if ch == '\n': return result
	except filebufEOF:
	    return None

    def write(self, str):
	"""Write string to file"""
	self.sock.sendall(str)

class httpfilebuf(filebuf):
    """Class for managing HTTP chunked mode"""

    def __init__(self, f, tout = 20):
	filebuf.__init__(self, f, tout)
	self.chunked = False
	self.chunkbuf = ""
	self.chunkptr = 0

    def set_chunked(self):
	"""Set the buffering to be chunked"""
	self.chunked = True
	self.chunkbuf = ""
	self.chunkptr = 0

    def read_numline(self):
	"""Read line probably containing chunk size"""
	result = ""
	while 1:
	    ch = filebuf.get(self, 1)
	    result += ch
	    if ch == '\n': break
	return result.rstrip()

    def chunksize(self):
	"""Read size of next chunk"""
	lin = self.read_numline()
	if len(lin) == 0: return None
	try:
	    return int(lin, 16)
	except ValueError:
	    raise filebufEOF()

    def wantchunk(self, length):
	"""As for want but with chunks"""

	if self.chunkptr + length < len(self.chunkbuf):
	    return

	self.chunkbuf = self.chunkbuf[self.chunkptr:]
	self.chunkptr = 0

	while length > len(self.chunkbuf):
	    # Get size of next chunk
	    nxt_sz = self.chunksize()
	    # We're not expecting end of chunks at this point
	    if nxt_sz == 0:
		raise filebufEOF()
	    self.chunkbuf += filebuf.get(self, nxt_sz)
	    # Expect chunk to be followed by empty line
	    if self.chunksize() is not None:
		raise filebufEOF()
    
    def get(self, length):
	"""Get a string of given length.

Possibly grab it from the chunked buffer"""

	if not self.chunked:
	    return filebuf.get(self, length)
	self.wantchunk(length)
	result = self.chunkbuf[self.chunkptr:self.chunkptr+length]
	self.chunkptr += length;
	return result

    def peek(self, length):
	"""Peek at the next string of length given from the file

Possibly grab it from the chunked buffer"""

	if not self.chunked:
	    return filebuf.peek(self, length)
	self.wantchunk(length)
	return self.chunkbuf[self.chunkptr:self.chunkptr+length]
	
    def getrem(self):
	"""Get some of the remaining stuff we are expecting

Possibly grab it from the chunked buffer"""

	if not self.chunked:
	    return filebuf.getrem(self)

	# Get anything remaining
	
	if self.chunkptr < len(self.chunkbuf):
	    result = self.chunkbuf[self.chunkptr:]
	    self.chunkptr = 0
	    self.chunkbuf = ""
	    return result

	# Read the next chunk
	# If at end, turn off chunking

        nxt_sz = self.chunksize()
        if nxt_sz == 0:
            self.chunked = False
            return ""

        result = filebuf.get(self, nxt_sz)
        
        # Expect chunk to be followed by empty line

        if self.chunksize() is not None:
            raise filebufEOF()

        return result
    
# Simulate that in an string

class stringbuf:
    """Buffer up string to look like filebuf"""

    def __init__(self, f, l):
        """Initialise with file-type argument and expected length"""
        self.buffer = f
        self.ptr = 0

    def want(self, length):
        """Say we want a string of length given from buffer"""
        if len(self.buffer) - self.ptr < length:
            raise filebufEOF()

    def get(self, length):
        """Get a string of length given from the file"""
        self.want(length)
        result = self.buffer[self.ptr:self.ptr+length]
        self.ptr += length;
        return  result

    def peek(self, length):
        """Peek at the next string of length given from the file"""
        self.want(length)
        return  self.buffer[self.ptr:self.ptr+length]

    def skip(self, length):
        """Skip over the next string of length given from the file"""
        self.ptr += length

    def getch(self):
        """Get the ord value of the next character"""
        return ord(self.get(1))

    def peekch(self):
        """Peek at the ord value of the next character"""
        return ord(self.peek(1))

    def getrem(self):
        """Get some of the remaining stuff we are expecting"""
        ret = self.buffer[self.ptr:]
        self.ptr = len(self.buffer)
        return  ret;

    def readline(self):
        """Emulate readline function"""
        result = ""
        try:
            while 1:
                ch = self.getch()
                result += ch
                if ch == '\n': return result
        except filebufEOF:
            return None
