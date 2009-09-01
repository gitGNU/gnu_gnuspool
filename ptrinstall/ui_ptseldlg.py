# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ptseldlg.ui'
#
# Created: Tue Sep  1 15:10:55 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_ptseldlg(object):
    def setupUi(self, ptseldlg):
        ptseldlg.setObjectName("ptseldlg")
        ptseldlg.resize(530, 371)
        self.buttonBox = QtGui.QDialogButtonBox(ptseldlg)
        self.buttonBox.setGeometry(QtCore.QRect(430, 10, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.ptrlist = QtGui.QListWidget(ptseldlg)
        self.ptrlist.setGeometry(QtCore.QRect(0, 100, 421, 271))
        self.ptrlist.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.ptrlist.setObjectName("ptrlist")
        self.label = QtGui.QLabel(ptseldlg)
        self.label.setGeometry(QtCore.QRect(10, 10, 391, 61))
        self.label.setObjectName("label")
        self.label_2 = QtGui.QLabel(ptseldlg)
        self.label_2.setGeometry(QtCore.QRect(10, 70, 101, 18))
        self.label_2.setObjectName("label_2")
        self.ptrname = QtGui.QLineEdit(ptseldlg)
        self.ptrname.setGeometry(QtCore.QRect(120, 70, 301, 26))
        self.ptrname.setReadOnly(True)
        self.ptrname.setObjectName("ptrname")

        self.retranslateUi(ptseldlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), ptseldlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), ptseldlg.reject)
        QtCore.QMetaObject.connectSlotsByName(ptseldlg)
        ptseldlg.setTabOrder(self.ptrname, self.ptrlist)
        ptseldlg.setTabOrder(self.ptrlist, self.buttonBox)

    def retranslateUi(self, ptseldlg):
        ptseldlg.setWindowTitle(QtGui.QApplication.translate("ptseldlg", "Select printer type", None, QtGui.QApplication.UnicodeUTF8))
        self.ptrlist.setToolTip(QtGui.QApplication.translate("ptseldlg", "Select the nearest printer to your printer type here", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("ptseldlg", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:\'Sans\'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Please select your printer from this list if possible.</p>\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">If unavailable please select the nearest to it or</p>\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-style:italic;\">Generic Laserjet</span> etc</p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("ptseldlg", "Printer Name:", None, QtGui.QApplication.UnicodeUTF8))

