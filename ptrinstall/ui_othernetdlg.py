# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'othernetdlg.ui'
#
# Created: Sat Jul  3 23:26:45 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_othernetdlg(object):
    def setupUi(self, othernetdlg):
        othernetdlg.setObjectName("othernetdlg")
        othernetdlg.resize(729, 225)
        self.buttonBox = QtGui.QDialogButtonBox(othernetdlg)
        self.buttonBox.setGeometry(QtCore.QRect(370, 160, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.label = QtGui.QLabel(othernetdlg)
        self.label.setGeometry(QtCore.QRect(20, 20, 491, 81))
        self.label.setObjectName("label")
        self.netcmd = QtGui.QLineEdit(othernetdlg)
        self.netcmd.setGeometry(QtCore.QRect(20, 110, 691, 26))
        self.netcmd.setObjectName("netcmd")

        self.retranslateUi(othernetdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), othernetdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), othernetdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(othernetdlg)

    def retranslateUi(self, othernetdlg):
        othernetdlg.setWindowTitle(QtGui.QApplication.translate("othernetdlg", "Other network device", None, QtGui.QApplication.UnicodeUTF8))
        othernetdlg.setToolTip(QtGui.QApplication.translate("othernetdlg", "This is the command to be used", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("othernetdlg", "Enter here a third-party command which will take data on standard input\n"
"and output to the printer.\n"
"\n"
"The device name string from the spool queue is available as SPOOLDEV\n"
"", None, QtGui.QApplication.UnicodeUTF8))

