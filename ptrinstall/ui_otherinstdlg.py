# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'otherinstdlg.ui'
#
# Created: Tue Sep  1 15:10:59 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_otherinstdlg(object):
    def setupUi(self, otherinstdlg):
        otherinstdlg.setObjectName("otherinstdlg")
        otherinstdlg.resize(506, 252)
        self.buttonBox = QtGui.QDialogButtonBox(otherinstdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 10, 81, 181))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.widget = QtGui.QWidget(otherinstdlg)
        self.widget.setGeometry(QtCore.QRect(40, 20, 371, 171))
        self.widget.setObjectName("widget")
        self.verticalLayout = QtGui.QVBoxLayout(self.widget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtGui.QLabel(self.widget)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.devname = QtGui.QLineEdit(self.widget)
        self.devname.setMaxLength(29)
        self.devname.setObjectName("devname")
        self.verticalLayout.addWidget(self.devname)
        self.label_2 = QtGui.QLabel(self.widget)
        self.label_2.setObjectName("label_2")
        self.verticalLayout.addWidget(self.label_2)
        self.description = QtGui.QLineEdit(self.widget)
        self.description.setMaxLength(41)
        self.description.setObjectName("description")
        self.verticalLayout.addWidget(self.description)
        self.label.setBuddy(self.devname)
        self.label_2.setBuddy(self.description)

        self.retranslateUi(otherinstdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), otherinstdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), otherinstdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(otherinstdlg)
        otherinstdlg.setTabOrder(self.devname, self.description)
        otherinstdlg.setTabOrder(self.description, self.buttonBox)

    def retranslateUi(self, otherinstdlg):
        otherinstdlg.setWindowTitle(QtGui.QApplication.translate("otherinstdlg", "Install device 3rd party driver", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("otherinstdlg", "De&vice name or other parameter for $SPOOLDEV\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.devname.setToolTip(QtGui.QApplication.translate("otherinstdlg", "This is what will be quoted using $SPOOLDEV and appear in\n"
"screen displays as <dev> or [dev] maybe to distinguish\n"
"similar printers.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("otherinstdlg", "Please give some sort of &description for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.description.setToolTip(QtGui.QApplication.translate("otherinstdlg", "This should be a helpful description of the printer", None, QtGui.QApplication.UnicodeUTF8))

