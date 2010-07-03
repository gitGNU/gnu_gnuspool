# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'netinstdlg.ui'
#
# Created: Sat Jul  3 23:26:46 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_netinstdlg(object):
    def setupUi(self, netinstdlg):
        netinstdlg.setObjectName("netinstdlg")
        netinstdlg.resize(506, 265)
        self.buttonBox = QtGui.QDialogButtonBox(netinstdlg)
        self.buttonBox.setGeometry(QtCore.QRect(420, 10, 81, 181))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(netinstdlg)
        self.layoutWidget.setGeometry(QtCore.QRect(40, 20, 343, 211))
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

        self.retranslateUi(netinstdlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), netinstdlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), netinstdlg.reject)
        QtCore.QMetaObject.connectSlotsByName(netinstdlg)
        netinstdlg.setTabOrder(self.devname, self.description)
        netinstdlg.setTabOrder(self.description, self.formtype)
        netinstdlg.setTabOrder(self.formtype, self.setdef)
        netinstdlg.setTabOrder(self.setdef, self.buttonBox)

    def retranslateUi(self, netinstdlg):
        netinstdlg.setWindowTitle(QtGui.QApplication.translate("netinstdlg", "Install network server device", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("netinstdlg", "&Host name or IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.devname.setToolTip(QtGui.QApplication.translate("netinstdlg", "This is what will be quoted using $SPOOLDEV usually\n"
"the host name or IP address", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("netinstdlg", "&Description for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.description.setToolTip(QtGui.QApplication.translate("netinstdlg", "This should be a helpful description of the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("netinstdlg", "Initial &form type", None, QtGui.QApplication.UnicodeUTF8))
        self.formtype.setToolTip(QtGui.QApplication.translate("netinstdlg", "Specify the initial form type for the printer", None, QtGui.QApplication.UnicodeUTF8))
        self.setdef.setToolTip(QtGui.QApplication.translate("netinstdlg", "Check here to make this the default form type for all users", None, QtGui.QApplication.UnicodeUTF8))
        self.setdef.setText(QtGui.QApplication.translate("netinstdlg", "Set this form type as default for all users", None, QtGui.QApplication.UnicodeUTF8))

