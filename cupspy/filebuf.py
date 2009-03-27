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

class filebufEOF(Exception):
    """Throw me on EOF"""
    pass

class filebuf:
    """Buffer up file/socket for efficient reading"""

    def __init__(self, f, l):
        """Initialise with file-type argument and expected length"""
        self.file = f
        self.buffer = ""
        self.ptr = 0
        self.expected = l

    def want(self, length):
        """Say we want a string of length given from buffer"""
        lenav = len(self.buffer) - self.ptr
        if  lenav < length:
            getl = 1024
            if getl > self.expected:
                getl = self.expected
            self.buffer = self.buffer[self.ptr:]
            self.ptr = 0
            try:
                nb = self.file.read(getl)
            except:
                raise filebufEOF()
            lenav += len(nb)
            self.expected -= len(nb)
            if  lenav < length:
                raise filebufEOF()
            self.buffer += nb

    def get(self, length):
        """Get a string of length given from the file"""
        self.want(length)
        result = self.buffer[self.ptr:self.ptr+length]
        self.ptr += length;
        return result

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
        if  self.ptr > 0 and len(self.buffer) -  self.ptr > 0:
            ret = self.buffer[self.ptr:]
            self.ptr = 0
            return  ret
        l = self.expected
        if  l <= 0:
            return ""
        if  l > 1024:
            l = 1024
        try:
            ret = self.file.read(l)
            self.expected -= len(ret)
            return  ret
        except:
            raise filebufEOF()

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
