#
#   Copyright 2008 Free Software Foundation, Inc.
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
#
#  Makefile for help files living in BASE/src/helpmsg
#  Don't forget the versions in "src/helpmsg" are only stubs which "helpparse"
#  extracts the real stuff from the message file and puts in the build directory
#

SHELL		=	/bin/sh
prefix		=	/usr/local
exec_prefix	=	${prefix}
HELPFILEDIR	=	${prefix}/share/gnuspool/help
RM		=	rm -f
INSTALL		=	/usr/bin/install -c
INSTALL_DATA	=	${INSTALL} -m 644
HELPFILES	=	int-config rest.help splpd.help xmspq.help xmspuser.help
SCREENHELP	=	spq.xterm spuser.xterm spq.wy60 spuser.wy60 spq.dumb spuser.dumb

# Build variants of screen help suitable for common terminals
# Link xterm version of screen help files to spq.help and spuser.help
# as most people use xterms these days

all:
	./Maketerms.sh

install:	all
	$(INSTALL_DATA) $(HELPFILES) $(SCREENHELP) $(HELPFILEDIR)
	cd $(HELPFILEDIR); ln -f spq.xterm spq.help; ln -f spuser.xterm spuser.help

install-strip:	all
	$(INSTALL_DATA) $(HELPFILES) $(SCREENHELP) $(HELPFILEDIR)
	cd $(HELPFILEDIR); ln -f spq.xterm spq.help; ln -f spuser.xterm spuser.help
	
clean:
	$(RM) *.xterm *.nxterm *.vt100 *.vt220 *.vt320 *.wy60

distclean:	clean
	$(RM) Makefile
