# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'netprotodlg.ui'
#
# Created: Tue Sep  1 15:11:00 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_netprotodlg(object):
    def setupUi(self, netprotodlg):
        netprotodlg.setObjectName("netprotodlg")
        netprotodlg.resize(394, 185)
        self.buttonBox = QtGui.QDialogButtonBox(netprotodlg)
        self.buttonBox.setGeometry(QtCore.QRect(290, 20, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.label = QtGui.QLabel(netprotodlg)
        self.label.setGeometry(QtCore.QRect(40, 30, 171, 18))
        self.label.setObjectName("label")
        self.hostorip = QtGui.QLineEdit(netprotodlg)
        self.hostorip.setGeometry(QtCore.QRect(40, 60, 161, 26))
        self.hostorip.setReadOnly(True)
        self.hostorip.setObjectName("hostorip")
        self.protocol = QtGui.QComboBox(netprotodlg)
        self.protocol.setGeometry(QtCore.QRect(40, 110, 201, 27))
        self.protocol.setObjectName("protocol")

        self.retranslateUi(netprotodlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), netprotodlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), netprotodlg.reject)
        QtCore.QMetaObject.connectSlotsByName(netprotodlg)
        netprotodlg.setTabOrder(self.hostorip, self.protocol)
        netprotodlg.setTabOrder(self.protocol, self.buttonBox)

    def retranslateUi(self, netprotodlg):
        netprotodlg.setWindowTitle(QtGui.QApplication.translate("netprotodlg", "Select network protocol", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("netprotodlg", "Protocol to use to talk to", None, QtGui.QApplication.UnicodeUTF8))
        self.protocol.setToolTip(QtGui.QApplication.translate("netprotodlg", "Select here the protocol to use to speak to the printer", None, QtGui.QApplication.UnicodeUTF8))

