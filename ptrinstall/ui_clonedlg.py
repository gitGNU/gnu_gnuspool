# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'clonedlg.ui'
#
# Created: Sat Jul  3 23:26:44 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_clonedlg(object):
    def setupUi(self, clonedlg):
        clonedlg.setObjectName("clonedlg")
        clonedlg.resize(467, 161)
        self.buttonBox = QtGui.QDialogButtonBox(clonedlg)
        self.buttonBox.setGeometry(QtCore.QRect(360, 30, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.widget = QtGui.QWidget(clonedlg)
        self.widget.setGeometry(QtCore.QRect(20, 30, 291, 101))
        self.widget.setObjectName("widget")
        self.gridLayout = QtGui.QGridLayout(self.widget)
        self.gridLayout.setObjectName("gridLayout")
        self.label_2 = QtGui.QLabel(self.widget)
        self.label_2.setObjectName("label_2")
        self.gridLayout.addWidget(self.label_2, 0, 0, 1, 1)
        self.clonename = QtGui.QLineEdit(self.widget)
        self.clonename.setObjectName("clonename")
        self.gridLayout.addWidget(self.clonename, 0, 1, 1, 1)
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 1, 0, 1, 1)
        self.clonedptr = QtGui.QComboBox(self.widget)
        self.clonedptr.setStatusTip("")
        self.clonedptr.setObjectName("clonedptr")
        self.gridLayout.addWidget(self.clonedptr, 1, 1, 1, 1)
        self.label_2.setBuddy(self.clonename)
        self.label.setBuddy(self.clonedptr)

        self.retranslateUi(clonedlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), clonedlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), clonedlg.reject)
        QtCore.QMetaObject.connectSlotsByName(clonedlg)
        clonedlg.setTabOrder(self.clonename, self.clonedptr)
        clonedlg.setTabOrder(self.clonedptr, self.buttonBox)

    def retranslateUi(self, clonedlg):
        clonedlg.setWindowTitle(QtGui.QApplication.translate("clonedlg", "Clone printer", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("clonedlg", "&Clone Name", None, QtGui.QApplication.UnicodeUTF8))
        self.clonename.setToolTip(QtGui.QApplication.translate("clonedlg", "This is the name of the clone", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("clonedlg", "&Printer to clone", None, QtGui.QApplication.UnicodeUTF8))
        self.clonedptr.setToolTip(QtGui.QApplication.translate("clonedlg", "This is the printer to be cloned", None, QtGui.QApplication.UnicodeUTF8))

