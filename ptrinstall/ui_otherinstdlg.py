# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'otherinstdlg.ui'
#
# Created: Sat Jul  3 23:26:45 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_otherinstdlg(object):
    def setupUi(self, otherinstdlg):
        otherinstdlg.setObjectName("otherinstdlg")
        otherinstdlg.resize(506, 317)
        self.buttonBox = QtGui.QDialogButtonBox(otherinstdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 10, 81, 181))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(otherinstdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(40, 20, 371, 241))
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
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        self.label_3.setObjectName("label_3")
        self.verticalLayout.addWidget(self.label_3)
        self.formtype = QtGui.QComboBox(self.layoutWidget)
        self.formtype.setEditable(True)
        self.formtype.setObjectName("formtype")
        self.verticalLayout.addWidget(self.formtype)
        self.setdef = QtGui.QCheckBox(self.layoutWidget)
        self.setdef.setObjectName("setdef")
        self.verticalLayout.addWidget(self.setdef)
        self.label.setBuddy(self.devname)
        self.label_2.setBuddy(self.description)
        self.label_3.setBuddy(self.formtype)

        self.retranslateUi(otherinstdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), otherinstdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), otherinstdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(otherinstdlg)
        otherinstdlg.setTabOrder(self.devname, self.description)
        otherinstdlg.setTabOrder(self.description, self.formtype)
        otherinstdlg.setTabOrder(self.formtype, self.setdef)
        otherinstdlg.setTabOrder(self.setdef, self.buttonBox)

    def retranslateUi(self, otherinstdlg):
        otherinstdlg.setWindowTitle(QtGui.QApplication.translate("otherinstdlg", "Install device 3rd party driver", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("otherinstdlg", "De&vice name or other parameter for $SPOOLDEV\n"
"", None, QtGui.QApplication.UnicodeUTF8))
        self.devname.setToolTip(QtGui.QApplication.translate("otherinstdlg", "This is what will be quoted using $SPOOLDEV and appear in\n"
"screen displays as <dev> or [dev] maybe to distinguish\n"
"similar printers.", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("otherinstdlg", "Please give some sort of &description for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.description.setToolTip(QtGui.QApplication.translate("otherinstdlg", "This should be a helpful description of the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("otherinstdlg", "Initial &form type", None, QtGui.QApplication.UnicodeUTF8))
        self.formtype.setToolTip(QtGui.QApplication.translate("otherinstdlg", "Specify the initial form type for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.setdef.setToolTip(QtGui.QApplication.translate("otherinstdlg", "Check here to make this the default form type for all users", None, QtGui.QApplication.UnicodeUTF8))
        self.setdef.setText(QtGui.QApplication.translate("otherinstdlg", "Set this form type as default for all users", None, QtGui.QApplication.UnicodeUTF8))

