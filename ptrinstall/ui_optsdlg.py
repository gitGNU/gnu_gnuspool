# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'optsdlg.ui'
#
# Created: Sat Jul  3 23:26:44 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_optsdlg(object):
    def setupUi(self, optsdlg):
        optsdlg.setObjectName("optsdlg")
        optsdlg.resize(473, 376)
        self.buttonBox = QtGui.QDialogButtonBox(optsdlg)
        self.buttonBox.setGeometry(QtCore.QRect(370, 20, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.widget = QtGui.QWidget(optsdlg)
        self.widget.setGeometry(QtCore.QRect(30, 20, 315, 321))
        self.widget.setObjectName("widget")
        self.gridLayout = QtGui.QGridLayout(self.widget)
        self.gridLayout.setObjectName("gridLayout")
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.hdropt = QtGui.QComboBox(self.widget)
        self.hdropt.setObjectName("hdropt")
        self.hdropt.addItem("")
        self.hdropt.addItem("")
        self.hdropt.addItem("")
        self.gridLayout.addWidget(self.hdropt, 0, 1, 1, 1)
        self.hdrpercopy = QtGui.QCheckBox(self.widget)
        self.hdrpercopy.setObjectName("hdrpercopy")
        self.gridLayout.addWidget(self.hdrpercopy, 1, 0, 1, 2)
        self.onecopy = QtGui.QCheckBox(self.widget)
        self.onecopy.setObjectName("onecopy")
        self.gridLayout.addWidget(self.onecopy, 2, 0, 1, 2)
        self.ignrange = QtGui.QCheckBox(self.widget)
        self.ignrange.setObjectName("ignrange")
        self.gridLayout.addWidget(self.ignrange, 3, 0, 1, 2)
        self.inclpage1 = QtGui.QCheckBox(self.widget)
        self.inclpage1.setObjectName("inclpage1")
        self.gridLayout.addWidget(self.inclpage1, 4, 0, 1, 1)
        self.retnjob = QtGui.QCheckBox(self.widget)
        self.retnjob.setObjectName("retnjob")
        self.gridLayout.addWidget(self.retnjob, 5, 0, 1, 1)
        self.single = QtGui.QCheckBox(self.widget)
        self.single.setObjectName("single")
        self.gridLayout.addWidget(self.single, 6, 0, 1, 1)
        self.label.setBuddy(self.hdropt)

        self.retranslateUi(optsdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), optsdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), optsdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(optsdlg)
        optsdlg.setTabOrder(self.hdropt, self.hdrpercopy)
        optsdlg.setTabOrder(self.hdrpercopy, self.onecopy)
        optsdlg.setTabOrder(self.onecopy, self.ignrange)
        optsdlg.setTabOrder(self.ignrange, self.inclpage1)
        optsdlg.setTabOrder(self.inclpage1, self.retnjob)
        optsdlg.setTabOrder(self.retnjob, self.single)
        optsdlg.setTabOrder(self.single, self.buttonBox)

    def retranslateUi(self, optsdlg):
        optsdlg.setWindowTitle(QtGui.QApplication.translate("optsdlg", "Spool options dialog", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("optsdlg", "&Header options", None, QtGui.QApplication.UnicodeUTF8))
        self.hdropt.setToolTip(QtGui.QApplication.translate("optsdlg", "Maybe override the options for banner/header page printing\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.hdropt.setItemText(0, QtGui.QApplication.translate("optsdlg", "As job specifies", None, QtGui.QApplication.UnicodeUTF8))
        self.hdropt.setItemText(1, QtGui.QApplication.translate("optsdlg", "Never print headers", None, QtGui.QApplication.UnicodeUTF8))
        self.hdropt.setItemText(2, QtGui.QApplication.translate("optsdlg", "Always print headers", None, QtGui.QApplication.UnicodeUTF8))
        self.hdrpercopy.setToolTip(QtGui.QApplication.translate("optsdlg", "When multiple consecutive copies of the same job are printed,\n"
"select this if you want the header on each", None, QtGui.QApplication.UnicodeUTF8))
        self.hdrpercopy.setText(QtGui.QApplication.translate("optsdlg", "Print header on &each copy", None, QtGui.QApplication.UnicodeUTF8))
        self.onecopy.setToolTip(QtGui.QApplication.translate("optsdlg", "Only print one copy of multiple copies.\n"
"This is advisable for LPD protocol where multiple\n"
"copies are handled differently.", None, QtGui.QApplication.UnicodeUTF8))
        self.onecopy.setText(QtGui.QApplication.translate("optsdlg", "&Only one copy", None, QtGui.QApplication.UnicodeUTF8))
        self.ignrange.setToolTip(QtGui.QApplication.translate("optsdlg", "Disregard page ranges - always print whole document", None, QtGui.QApplication.UnicodeUTF8))
        self.ignrange.setText(QtGui.QApplication.translate("optsdlg", "Ignore page &ranges", None, QtGui.QApplication.UnicodeUTF8))
        self.inclpage1.setToolTip(QtGui.QApplication.translate("optsdlg", "Always reprint page one when doing page ranges", None, QtGui.QApplication.UnicodeUTF8))
        self.inclpage1.setText(QtGui.QApplication.translate("optsdlg", "&Include page one", None, QtGui.QApplication.UnicodeUTF8))
        self.retnjob.setToolTip(QtGui.QApplication.translate("optsdlg", "Retain jobs on queue after printing", None, QtGui.QApplication.UnicodeUTF8))
        self.retnjob.setText(QtGui.QApplication.translate("optsdlg", "&Retain jobs", None, QtGui.QApplication.UnicodeUTF8))
        self.single.setToolTip(QtGui.QApplication.translate("optsdlg", "Single-shot print mode", None, QtGui.QApplication.UnicodeUTF8))
        self.single.setText(QtGui.QApplication.translate("optsdlg", "&Single-shot", None, QtGui.QApplication.UnicodeUTF8))

