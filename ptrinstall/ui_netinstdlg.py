# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'netinstdlg.ui'
#
# Created: Tue Sep  1 15:10:59 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_netinstdlg(object):
    def setupUi(self, netinstdlg):
        netinstdlg.setObjectName("netinstdlg")
        netinstdlg.resize(506, 252)
        self.buttonBox = QtGui.QDialogButtonBox(netinstdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 10, 81, 181))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(netinstdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(40, 20, 343, 171))
        self.layoutWidget.setObjectName("layoutWidget")
        self.verticalLayout = QtGui.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.devname = QtGui.QLineEdit(self.layoutWidget)
        self.devname.setMaxLength(29)
        self.devname.setObjectName("devname")
        self.verticalLayout.addWidget(self.devname)
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName("label_2")
        self.verticalLayout.addWidget(self.label_2)
        self.description = QtGui.QLineEdit(self.layoutWidget)
        self.description.setMaxLength(41)
        self.description.setObjectName("description")
        self.verticalLayout.addWidget(self.description)
        self.label.setBuddy(self.devname)
        self.label_2.setBuddy(self.description)

        self.retranslateUi(netinstdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), netinstdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), netinstdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(netinstdlg)
        netinstdlg.setTabOrder(self.devname, self.description)
        netinstdlg.setTabOrder(self.description, self.buttonBox)

    def retranslateUi(self, netinstdlg):
        netinstdlg.setWindowTitle(QtGui.QApplication.translate("netinstdlg", "Install network server device", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("netinstdlg", "&Host name or IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.devname.setToolTip(QtGui.QApplication.translate("netinstdlg", "This is what will be quoted using $SPOOLDEV usually\n"
"the host name or IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("netinstdlg", "&Description for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.description.setToolTip(QtGui.QApplication.translate("netinstdlg", "This should be a helpful description of the printer", None, QtGui.QApplication.UnicodeUTF8))

