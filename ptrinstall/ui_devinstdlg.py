# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'devinstdlg.ui'
#
# Created: Tue Sep  1 15:10:59 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_devinstdlg(object):
    def setupUi(self, devinstdlg):
        devinstdlg.setObjectName("devinstdlg")
        devinstdlg.resize(506, 252)
        self.buttonBox = QtGui.QDialogButtonBox(devinstdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 10, 81, 181))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(devinstdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(40, 20, 343, 171))
        self.layoutWidget.setObjectName("layoutWidget")
        self.verticalLayout = QtGui.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.devname = QtGui.QComboBox(self.layoutWidget)
        self.devname.setEditable(True)
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

        self.retranslateUi(devinstdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), devinstdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), devinstdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(devinstdlg)
        devinstdlg.setTabOrder(self.devname, self.description)
        devinstdlg.setTabOrder(self.description, self.buttonBox)

    def retranslateUi(self, devinstdlg):
        devinstdlg.setWindowTitle(QtGui.QApplication.translate("devinstdlg", "Install printer on device", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("devinstdlg", "De&vice name", None, QtGui.QApplication.UnicodeUTF8))
        self.devname.setToolTip(QtGui.QApplication.translate("devinstdlg", "This is the device to be used by the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("devinstdlg", "&Description for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.description.setToolTip(QtGui.QApplication.translate("devinstdlg", "This should be a helpful description of the printer", None, QtGui.QApplication.UnicodeUTF8))

