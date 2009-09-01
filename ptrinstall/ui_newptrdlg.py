# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'newptrdlg.ui'
#
# Created: Tue Sep  1 15:10:56 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_newptrdlg(object):
    def setupUi(self, newptrdlg):
        newptrdlg.setObjectName("newptrdlg")
        newptrdlg.resize(361, 300)
        self.buttonBox = QtGui.QDialogButtonBox(newptrdlg)
        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 301, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.groupBox = QtGui.QGroupBox(newptrdlg)
        self.groupBox.setGeometry(QtCore.QRect(20, 80, 161, 131))
        self.groupBox.setObjectName("groupBox")
        self.int_serial = QtGui.QRadioButton(self.groupBox)
        self.int_serial.setGeometry(QtCore.QRect(10, 30, 141, 23))
        self.int_serial.setObjectName("int_serial")
        self.int_USB = QtGui.QRadioButton(self.groupBox)
        self.int_USB.setGeometry(QtCore.QRect(10, 70, 108, 23))
        self.int_USB.setObjectName("int_USB")
        self.int_other = QtGui.QRadioButton(self.groupBox)
        self.int_other.setGeometry(QtCore.QRect(10, 110, 108, 23))
        self.int_other.setObjectName("int_other")
        self.int_parallel = QtGui.QRadioButton(self.groupBox)
        self.int_parallel.setGeometry(QtCore.QRect(10, 50, 108, 23))
        self.int_parallel.setObjectName("int_parallel")
        self.int_network = QtGui.QRadioButton(self.groupBox)
        self.int_network.setGeometry(QtCore.QRect(10, 90, 108, 23))
        self.int_network.setChecked(True)
        self.int_network.setObjectName("int_network")
        self.layoutWidget = QtGui.QWidget(newptrdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(20, 20, 250, 41))
        self.layoutWidget.setObjectName("layoutWidget")
        self.horizontalLayout = QtGui.QHBoxLayout(self.layoutWidget)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName("label")
        self.horizontalLayout.addWidget(self.label)
        self.prtname = QtGui.QLineEdit(self.layoutWidget)
        self.prtname.setMaxLength(28)
        self.prtname.setObjectName("prtname")
        self.horizontalLayout.addWidget(self.prtname)
        self.label.setBuddy(self.prtname)

        self.retranslateUi(newptrdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), newptrdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), newptrdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(newptrdlg)
        newptrdlg.setTabOrder(self.prtname, self.int_serial)
        newptrdlg.setTabOrder(self.int_serial, self.int_parallel)
        newptrdlg.setTabOrder(self.int_parallel, self.int_USB)
        newptrdlg.setTabOrder(self.int_USB, self.int_network)
        newptrdlg.setTabOrder(self.int_network, self.int_other)
        newptrdlg.setTabOrder(self.int_other, self.buttonBox)

    def retranslateUi(self, newptrdlg):
        newptrdlg.setWindowTitle(QtGui.QApplication.translate("newptrdlg", "Define printer type", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("newptrdlg", "Interface type", None, QtGui.QApplication.UnicodeUTF8))
        self.int_serial.setToolTip(QtGui.QApplication.translate("newptrdlg", "Select this if the printer has a serial (TTY-style) interface", None, QtGui.QApplication.UnicodeUTF8))
        self.int_serial.setText(QtGui.QApplication.translate("newptrdlg", "&Serial", None, QtGui.QApplication.UnicodeUTF8))
        self.int_USB.setToolTip(QtGui.QApplication.translate("newptrdlg", "Select this for a USB printer", None, QtGui.QApplication.UnicodeUTF8))
        self.int_USB.setText(QtGui.QApplication.translate("newptrdlg", "&USB", None, QtGui.QApplication.UnicodeUTF8))
        self.int_other.setToolTip(QtGui.QApplication.translate("newptrdlg", "Select this if the printer uses some third-party interface program", None, QtGui.QApplication.UnicodeUTF8))
        self.int_other.setText(QtGui.QApplication.translate("newptrdlg", "&Other", None, QtGui.QApplication.UnicodeUTF8))
        self.int_parallel.setText(QtGui.QApplication.translate("newptrdlg", "&Parallel", None, QtGui.QApplication.UnicodeUTF8))
        self.int_network.setToolTip(QtGui.QApplication.translate("newptrdlg", "Select this if the printer has a network (terminal server) card", None, QtGui.QApplication.UnicodeUTF8))
        self.int_network.setText(QtGui.QApplication.translate("newptrdlg", "&Network", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("newptrdlg", "Printer &Name", None, QtGui.QApplication.UnicodeUTF8))
        self.prtname.setToolTip(QtGui.QApplication.translate("newptrdlg", "Enter the name by which this printer will be known to the spooler", None, QtGui.QApplication.UnicodeUTF8))

