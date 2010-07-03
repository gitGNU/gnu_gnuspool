# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'psoptdlg.ui'
#
# Created: Sat Jul  3 23:26:43 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_psoptdlg(object):
    def setupUi(self, psoptdlg):
        psoptdlg.setObjectName("psoptdlg")
        psoptdlg.resize(457, 300)
        self.buttonBox = QtGui.QDialogButtonBox(psoptdlg)
        self.buttonBox.setGeometry(QtCore.QRect(360, 20, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.label_2 = QtGui.QLabel(psoptdlg)
        self.label_2.setGeometry(QtCore.QRect(40, 20, 181, 18))
        self.label_2.setObjectName("label_2")
        self.widget = QtGui.QWidget(psoptdlg)
        self.widget.setGeometry(QtCore.QRect(40, 50, 301, 191))
        self.widget.setObjectName("widget")
        self.gridLayout = QtGui.QGridLayout(self.widget)
        self.gridLayout.setObjectName("gridLayout")
        self.usegs = QtGui.QCheckBox(self.widget)
        self.usegs.setChecked(True)
        self.usegs.setObjectName("usegs")
        self.gridLayout.addWidget(self.usegs, 0, 0, 1, 2)
        self.hascolour = QtGui.QCheckBox(self.widget)
        self.hascolour.setObjectName("hascolour")
        self.gridLayout.addWidget(self.hascolour, 1, 0, 1, 2)
        self.defcolour = QtGui.QCheckBox(self.widget)
        self.defcolour.setObjectName("defcolour")
        self.gridLayout.addWidget(self.defcolour, 2, 0, 1, 2)
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 3, 0, 1, 1)
        self.paper = QtGui.QComboBox(self.widget)
        self.paper.setObjectName("paper")
        self.paper.addItem("")
        self.paper.addItem("")
        self.paper.addItem("")
        self.paper.addItem("")
        self.paper.addItem("")
        self.paper.addItem("")
        self.gridLayout.addWidget(self.paper, 3, 1, 1, 1)
        self.label.setBuddy(self.paper)

        self.retranslateUi(psoptdlg)
        self.paper.setCurrentIndex(0)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), psoptdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), psoptdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(psoptdlg)
        psoptdlg.setTabOrder(self.usegs, self.hascolour)
        psoptdlg.setTabOrder(self.hascolour, self.defcolour)
        psoptdlg.setTabOrder(self.defcolour, self.paper)
        psoptdlg.setTabOrder(self.paper, self.buttonBox)

    def retranslateUi(self, psoptdlg):
        psoptdlg.setWindowTitle(QtGui.QApplication.translate("psoptdlg", "PostScript printing options", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("psoptdlg", "PostScript printing options", None, QtGui.QApplication.UnicodeUTF8))
        self.usegs.setToolTip(QtGui.QApplication.translate("psoptdlg", "Use Ghostscript to print PostScript\n"
"NB We recommend this even if printer\n"
"supports it.", None, QtGui.QApplication.UnicodeUTF8))
        self.usegs.setText(QtGui.QApplication.translate("psoptdlg", "Use &ghostscript", None, QtGui.QApplication.UnicodeUTF8))
        self.hascolour.setToolTip(QtGui.QApplication.translate("psoptdlg", "Select this if you printer can print in colour", None, QtGui.QApplication.UnicodeUTF8))
        self.hascolour.setText(QtGui.QApplication.translate("psoptdlg", "Printer can print &colour", None, QtGui.QApplication.UnicodeUTF8))
        self.defcolour.setToolTip(QtGui.QApplication.translate("psoptdlg", "Select this if you want default colour\n"
"(This is sometimes more expensive)", None, QtGui.QApplication.UnicodeUTF8))
        self.defcolour.setText(QtGui.QApplication.translate("psoptdlg", "Colour by &default", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("psoptdlg", "&Paper size", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setToolTip(QtGui.QApplication.translate("psoptdlg", "Select the type of paper to use", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(0, QtGui.QApplication.translate("psoptdlg", "a4", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(1, QtGui.QApplication.translate("psoptdlg", "11x7", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(2, QtGui.QApplication.translate("psoptdlg", "letter", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(3, QtGui.QApplication.translate("psoptdlg", "legal", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(4, QtGui.QApplication.translate("psoptdlg", "ledger", None, QtGui.QApplication.UnicodeUTF8))
        self.paper.setItemText(5, QtGui.QApplication.translate("psoptdlg", "note", None, QtGui.QApplication.UnicodeUTF8))

