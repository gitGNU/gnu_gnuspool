# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'sethostdlg.ui'
#
# Created: Tue Sep  1 15:10:56 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_sethostdlg(object):
    def setupUi(self, sethostdlg):
        sethostdlg.setObjectName("sethostdlg")
        sethostdlg.resize(580, 167)
        self.buttonBox = QtGui.QDialogButtonBox(sethostdlg)
        self.buttonBox.setGeometry(QtCore.QRect(490, 10, 81, 201))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.OKscan = QtGui.QCheckBox(sethostdlg)
        self.OKscan.setGeometry(QtCore.QRect(60, 90, 311, 23))
        self.OKscan.setChecked(True)
        self.OKscan.setObjectName("OKscan")
        self.widget = QtGui.QWidget(sethostdlg)
        self.widget.setGeometry(QtCore.QRect(21, 31, 391, 41))
        self.widget.setObjectName("widget")
        self.horizontalLayout = QtGui.QHBoxLayout(self.widget)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName("label")
        self.horizontalLayout.addWidget(self.label)
        self.hostorip = QtGui.QLineEdit(self.widget)
        self.hostorip.setObjectName("hostorip")
        self.horizontalLayout.addWidget(self.hostorip)
        self.label.setBuddy(self.hostorip)

        self.retranslateUi(sethostdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), sethostdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), sethostdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(sethostdlg)
        sethostdlg.setTabOrder(self.hostorip, self.OKscan)
        sethostdlg.setTabOrder(self.OKscan, self.buttonBox)

    def retranslateUi(self, sethostdlg):
        sethostdlg.setWindowTitle(QtGui.QApplication.translate("sethostdlg", "Select network host", None, QtGui.QApplication.UnicodeUTF8))
        self.OKscan.setToolTip(QtGui.QApplication.translate("sethostdlg", "Select this to see which TCP ports are open for sending data", None, QtGui.QApplication.UnicodeUTF8))
        self.OKscan.setText(QtGui.QApplication.translate("sethostdlg", "OK to &scan address for suitable open ports", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("sethostdlg", "&Host name or IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.hostorip.setToolTip(QtGui.QApplication.translate("sethostdlg", "This is the host name or IP address of the\n"
"printer server.", None, QtGui.QApplication.UnicodeUTF8))

