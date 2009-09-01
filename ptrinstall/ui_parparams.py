# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'parparams.ui'
#
# Created: Tue Sep  1 15:10:56 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_parparams(object):
    def setupUi(self, parparams):
        parparams.setObjectName("parparams")
        parparams.resize(400, 240)
        self.buttonBox = QtGui.QDialogButtonBox(parparams)
        self.buttonBox.setGeometry(QtCore.QRect(290, 20, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(parparams)
        self.layoutWidget.setGeometry(QtCore.QRect(20, 19, 241, 201))
        self.layoutWidget.setObjectName("layoutWidget")
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setHorizontalSpacing(8)
        self.gridLayout.setVerticalSpacing(10)
        self.gridLayout.setObjectName("gridLayout")
        self.opento = QtGui.QSpinBox(self.layoutWidget)
        self.opento.setMaximum(65535)
        self.opento.setProperty("value", QtCore.QVariant(30))
        self.opento.setObjectName("opento")
        self.gridLayout.addWidget(self.opento, 0, 1, 1, 1)
        self.label_4 = QtGui.QLabel(self.layoutWidget)
        self.label_4.setObjectName("label_4")
        self.gridLayout.addWidget(self.label_4, 1, 0, 1, 1)
        self.offlineto = QtGui.QSpinBox(self.layoutWidget)
        self.offlineto.setMaximum(65535)
        self.offlineto.setProperty("value", QtCore.QVariant(300))
        self.offlineto.setObjectName("offlineto")
        self.gridLayout.addWidget(self.offlineto, 1, 1, 1, 1)
        self.label_5 = QtGui.QLabel(self.layoutWidget)
        self.label_5.setObjectName("label_5")
        self.gridLayout.addWidget(self.label_5, 2, 0, 1, 1)
        self.outbuffer = QtGui.QSpinBox(self.layoutWidget)
        self.outbuffer.setMinimum(1)
        self.outbuffer.setMaximum(1048576)
        self.outbuffer.setSingleStep(1024)
        self.outbuffer.setProperty("value", QtCore.QVariant(1024))
        self.outbuffer.setObjectName("outbuffer")
        self.gridLayout.addWidget(self.outbuffer, 2, 1, 1, 1)
        self.canhang = QtGui.QCheckBox(self.layoutWidget)
        self.canhang.setObjectName("canhang")
        self.gridLayout.addWidget(self.canhang, 3, 0, 1, 2)
        self.reopen = QtGui.QCheckBox(self.layoutWidget)
        self.reopen.setObjectName("reopen")
        self.gridLayout.addWidget(self.reopen, 4, 0, 1, 2)
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        self.label_3.setObjectName("label_3")
        self.gridLayout.addWidget(self.label_3, 0, 0, 1, 1)
        self.label_4.setBuddy(self.offlineto)
        self.label_5.setBuddy(self.outbuffer)
        self.label_3.setBuddy(self.opento)

        self.retranslateUi(parparams)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), parparams.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), parparams.reject)
        QtCore.QMetaObject.connectSlotsByName(parparams)
        parparams.setTabOrder(self.opento, self.offlineto)
        parparams.setTabOrder(self.offlineto, self.outbuffer)
        parparams.setTabOrder(self.outbuffer, self.canhang)
        parparams.setTabOrder(self.canhang, self.reopen)
        parparams.setTabOrder(self.reopen, self.buttonBox)

    def retranslateUi(self, parparams):
        parparams.setWindowTitle(QtGui.QApplication.translate("parparams", "Parallel port parameters", None, QtGui.QApplication.UnicodeUTF8))
        self.opento.setToolTip(QtGui.QApplication.translate("parparams", "Timeout trying to open serial port before deciding it\'s offline", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("parparams", "Offline &Timeout", None, QtGui.QApplication.UnicodeUTF8))
        self.offlineto.setToolTip(QtGui.QApplication.translate("parparams", "Timeout when trying to write to device before deciding it\'s offline", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("parparams", "&Output buffer", None, QtGui.QApplication.UnicodeUTF8))
        self.outbuffer.setToolTip(QtGui.QApplication.translate("parparams", "Output buffer size", None, QtGui.QApplication.UnicodeUTF8))
        self.canhang.setToolTip(QtGui.QApplication.translate("parparams", "Try to avoid system hangup when devices locked", None, QtGui.QApplication.UnicodeUTF8))
        self.canhang.setText(QtGui.QApplication.translate("parparams", "\"Hang\" avoidance", None, QtGui.QApplication.UnicodeUTF8))
        self.reopen.setToolTip(QtGui.QApplication.translate("parparams", "Close and reopen device after each job", None, QtGui.QApplication.UnicodeUTF8))
        self.reopen.setText(QtGui.QApplication.translate("parparams", "Reopen", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("parparams", "Open &Timeout", None, QtGui.QApplication.UnicodeUTF8))

