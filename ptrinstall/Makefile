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
#
# This builds the python sources from the component ui files

all:	ui_netparams.py\
	ui_ftpparams.py\
	ui_ptseldlg.py\
	ui_parparams.py\
	ui_newptrdlg.py\
	ui_sethostdlg.py\
	ui_psoptdlg.py\
	ui_serialparams.py\
	ui_hpnpfparams.py\
	ui_optsdlg.py\
	ui_clonedlg.py\
	ui_othernetdlg.py\
	ui_lpdparams.py\
	ui_ptrinstall_main.py\
	ui_otherinstdlg.py\
	ui_devinstdlg.py\
	ui_netinstdlg.py\
	ui_telnetparams.py\
	ui_netprotodlg.py\
	ui_usbparams.py

ui_netparams.py:	netparams.ui
	pyuic4 -o $@ $?

ui_ftpparams.py:	ftpparams.ui
	pyuic4 -o $@ $?

ui_ptseldlg.py:	ptseldlg.ui
	pyuic4 -o $@ $?

ui_parparams.py:	parparams.ui
	pyuic4 -o $@ $?

ui_newptrdlg.py:	newptrdlg.ui
	pyuic4 -o $@ $?

ui_sethostdlg.py:	sethostdlg.ui
	pyuic4 -o $@ $?

ui_psoptdlg.py:	psoptdlg.ui
	pyuic4 -o $@ $?

ui_serialparams.py:	serialparams.ui
	pyuic4 -o $@ $?

ui_hpnpfparams.py:	hpnpfparams.ui
	pyuic4 -o $@ $?

ui_optsdlg.py:	optsdlg.ui
	pyuic4 -o $@ $?

ui_clonedlg.py:	clonedlg.ui
	pyuic4 -o $@ $?

ui_othernetdlg.py:	othernetdlg.ui
	pyuic4 -o $@ $?

ui_lpdparams.py:	lpdparams.ui
	pyuic4 -o $@ $?

ui_ptrinstall_main.py:	ptrinstall_main.ui
	pyuic4 -o $@ $?

ui_otherinstdlg.py:	otherinstdlg.ui
	pyuic4 -o $@ $?

ui_devinstdlg.py:	devinstdlg.ui
	pyuic4 -o $@ $?

ui_netinstdlg.py:	netinstdlg.ui
	pyuic4 -o $@ $?

ui_telnetparams.py:	telnetparams.ui
	pyuic4 -o $@ $?

ui_netprotodlg.py:	netprotodlg.ui
	pyuic4 -o $@ $?

ui_usbparams.py:	usbparams.ui
	pyuic4 -o $@ $?

# Don't remove the generated files in case the guy doesn't
# have pyuic4

clean:
	#rm -f ui_*.py *.pyc
	rm *.pyc

distclean: clean
	rm -f config.py

reallyclean:
	rm -f ui_*.py
