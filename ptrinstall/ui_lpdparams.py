# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'lpdparams.ui'
#
# Created: Tue Sep  1 15:10:58 2009
#      by: PyQt4 UI code generator 4.4.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_lpdparams(object):
    def setupUi(self, lpdparams):
        lpdparams.setObjectName("lpdparams")
        lpdparams.resize(509, 322)
        self.buttonBox = QtGui.QDialogButtonBox(lpdparams)
        self.buttonBox.setGeometry(QtCore.QRect(410, 10, 81, 241))
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.layoutWidget = QtGui.QWidget(lpdparams)
        self.layoutWidget.setGeometry(QtCore.QRect(20, 10, 375, 290))
        self.layoutWidget.setObjectName("layoutWidget")
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setVerticalSpacing(11)
        self.gridLayout.setObjectName("gridLayout")
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName("label_2")
        self.gridLayout.addWidget(self.label_2, 0, 0, 1, 1)
        self.ctrlfile = QtGui.QLineEdit(self.layoutWidget)
        self.ctrlfile.setObjectName("ctrlfile")
        self.gridLayout.addWidget(self.ctrlfile, 0, 1, 1, 4)
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        self.label_3.setObjectName("label_3")
        self.gridLayout.addWidget(self.label_3, 1, 0, 1, 1)
        self.outhost = QtGui.QLineEdit(self.layoutWidget)
        self.outhost.setObjectName("outhost")
        self.gridLayout.addWidget(self.outhost, 1, 1, 1, 4)
        self.label_4 = QtGui.QLabel(self.layoutWidget)
        self.label_4.setObjectName("label_4")
        self.gridLayout.addWidget(self.label_4, 2, 0, 1, 1)
        self.outptrname = QtGui.QLineEdit(self.layoutWidget)
        self.outptrname.setObjectName("outptrname")
        self.gridLayout.addWidget(self.outptrname, 2, 1, 1, 4)
        self.nonull = QtGui.QCheckBox(self.layoutWidget)
        self.nonull.setChecked(True)
        self.nonull.setObjectName("nonull")
        self.gridLayout.addWidget(self.nonull, 3, 0, 1, 1)
        self.resvport = QtGui.QCheckBox(self.layoutWidget)
        self.resvport.setObjectName("resvport")
        self.gridLayout.addWidget(self.resvport, 3, 1, 1, 4)
        self.label_5 = QtGui.QLabel(self.layoutWidget)
        self.label_5.setObjectName("label_5")
        self.gridLayout.addWidget(self.label_5, 4, 0, 1, 1)
        self.loops = QtGui.QSpinBox(self.layoutWidget)
        self.loops.setMinimum(1)
        self.loops.setProperty("value", QtCore.QVariant(3))
        self.loops.setObjectName("loops")
        self.gridLayout.addWidget(self.loops, 4, 1, 1, 1)
        self.label_6 = QtGui.QLabel(self.layoutWidget)
        self.label_6.setObjectName("label_6")
        self.gridLayout.addWidget(self.label_6, 4, 2, 1, 1)
        self.loopwait = QtGui.QSpinBox(self.layoutWidget)
        self.loopwait.setMinimum(1)
        self.loopwait.setMaximum(300)
        self.loopwait.setObjectName("loopwait")
        self.gridLayout.addWidget(self.loopwait, 4, 3, 1, 2)
        self.label_8 = QtGui.QLabel(self.layoutWidget)
        self.label_8.setObjectName("label_8")
        self.gridLayout.addWidget(self.label_8, 5, 0, 1, 1)
        self.itimeout = QtGui.QSpinBox(self.layoutWidget)
        self.itimeout.setMinimum(1)
        self.itimeout.setMaximum(300)
        self.itimeout.setProperty("value", QtCore.QVariant(5))
        self.itimeout.setObjectName("itimeout")
        self.gridLayout.addWidget(self.itimeout, 5, 1, 1, 1)
        self.label_9 = QtGui.QLabel(self.layoutWidget)
        self.label_9.setObjectName("label_9")
        self.gridLayout.addWidget(self.label_9, 5, 2, 1, 1)
        self.otimeout = QtGui.QSpinBox(self.layoutWidget)
        self.otimeout.setMinimum(1)
        self.otimeout.setMaximum(300)
        self.otimeout.setProperty("value", QtCore.QVariant(5))
        self.otimeout.setObjectName("otimeout")
        self.gridLayout.addWidget(self.otimeout, 5, 3, 1, 2)
        self.label_10 = QtGui.QLabel(self.layoutWidget)
        self.label_10.setObjectName("label_10")
        self.gridLayout.addWidget(self.label_10, 6, 0, 1, 1)
        self.retries = QtGui.QSpinBox(self.layoutWidget)
        self.retries.setObjectName("retries")
        self.gridLayout.addWidget(self.retries, 6, 1, 1, 1)
        self.label_11 = QtGui.QLabel(self.layoutWidget)
        self.label_11.setObjectName("label_11")
        self.gridLayout.addWidget(self.label_11, 6, 2, 1, 2)
        self.linger = QtGui.QSpinBox(self.layoutWidget)
        self.linger.setMaximum(99999)
        self.linger.setSingleStep(1000)
        self.linger.setObjectName("linger")
        self.gridLayout.addWidget(self.linger, 6, 4, 1, 1)
        self.label_2.setBuddy(self.ctrlfile)
        self.label_3.setBuddy(self.outhost)
        self.label_4.setBuddy(self.outptrname)
        self.label_5.setBuddy(self.loops)
        self.label_6.setBuddy(self.loopwait)
        self.label_8.setBuddy(self.itimeout)
        self.label_9.setBuddy(self.otimeout)
        self.label_10.setBuddy(self.retries)
        self.label_11.setBuddy(self.linger)

        self.retranslateUi(lpdparams)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), lpdparams.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), lpdparams.reject)
        QtCore.QMetaObject.connectSlotsByName(lpdparams)
        lpdparams.setTabOrder(self.ctrlfile, self.outhost)
        lpdparams.setTabOrder(self.outhost, self.outptrname)
        lpdparams.setTabOrder(self.outptrname, self.nonull)
        lpdparams.setTabOrder(self.nonull, self.resvport)
        lpdparams.setTabOrder(self.resvport, self.loops)
        lpdparams.setTabOrder(self.loops, self.loopwait)
        lpdparams.setTabOrder(self.loopwait, self.itimeout)
        lpdparams.setTabOrder(self.itimeout, self.otimeout)
        lpdparams.setTabOrder(self.otimeout, self.retries)
        lpdparams.setTabOrder(self.retries, self.linger)
        lpdparams.setTabOrder(self.linger, self.buttonBox)

    def retranslateUi(self, lpdparams):
        lpdparams.setWindowTitle(QtGui.QApplication.translate("lpdparams", "LPD specific parameters", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("lpdparams", "&Control file", None, QtGui.QApplication.UnicodeUTF8))
        self.ctrlfile.setToolTip(QtGui.QApplication.translate("lpdparams", "This is the location of the control file", None, QtGui.QApplication.UnicodeUTF8))
        self.ctrlfile.setText(QtGui.QApplication.translate("lpdparams", "SPROGDIR/xtlpc-ctrl", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("lpdparams", "&Outgoing host/IP", None, QtGui.QApplication.UnicodeUTF8))
        self.outhost.setToolTip(QtGui.QApplication.translate("lpdparams", "Some network setups may require the sending host to use a specific IP address or host name", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("lpdparams", "Printer &Name", None, QtGui.QApplication.UnicodeUTF8))
        self.outptrname.setToolTip(QtGui.QApplication.translate("lpdparams", "This is the name of the printer which the LPD protocol expects (spooler printer name may not work)", None, QtGui.QApplication.UnicodeUTF8))
        self.nonull.setToolTip(QtGui.QApplication.translate("lpdparams", "Don\'t send any empty jobs", None, QtGui.QApplication.UnicodeUTF8))
        self.nonull.setText(QtGui.QApplication.translate("lpdparams", "No &null jobs", None, QtGui.QApplication.UnicodeUTF8))
        self.resvport.setToolTip(QtGui.QApplication.translate("lpdparams", "Use a reserved port", None, QtGui.QApplication.UnicodeUTF8))
        self.resvport.setText(QtGui.QApplication.translate("lpdparams", "Use &reserved port", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("lpdparams", "Conn &attempts", None, QtGui.QApplication.UnicodeUTF8))
        self.loops.setToolTip(QtGui.QApplication.translate("lpdparams", "Number of attempts to be made to connect", None, QtGui.QApplication.UnicodeUTF8))
        self.label_6.setText(QtGui.QApplication.translate("lpdparams", "&waiting", None, QtGui.QApplication.UnicodeUTF8))
        self.loopwait.setToolTip(QtGui.QApplication.translate("lpdparams", "Wait time in between connection attempts", None, QtGui.QApplication.UnicodeUTF8))
        self.loopwait.setSuffix(QtGui.QApplication.translate("lpdparams", " sec", None, QtGui.QApplication.UnicodeUTF8))
        self.label_8.setText(QtGui.QApplication.translate("lpdparams", "Input timeout", None, QtGui.QApplication.UnicodeUTF8))
        self.itimeout.setToolTip(QtGui.QApplication.translate("lpdparams", "Timeout waiting for input to complete", None, QtGui.QApplication.UnicodeUTF8))
        self.itimeout.setSuffix(QtGui.QApplication.translate("lpdparams", " sec", None, QtGui.QApplication.UnicodeUTF8))
        self.label_9.setText(QtGui.QApplication.translate("lpdparams", "&Output", None, QtGui.QApplication.UnicodeUTF8))
        self.otimeout.setToolTip(QtGui.QApplication.translate("lpdparams", "Timeout wait for output to complete", None, QtGui.QApplication.UnicodeUTF8))
        self.otimeout.setSuffix(QtGui.QApplication.translate("lpdparams", " sec", None, QtGui.QApplication.UnicodeUTF8))
        self.label_10.setText(QtGui.QApplication.translate("lpdparams", "Retries", None, QtGui.QApplication.UnicodeUTF8))
        self.retries.setToolTip(QtGui.QApplication.translate("lpdparams", "Number of times to retry if timeout encountered", None, QtGui.QApplication.UnicodeUTF8))
        self.label_11.setText(QtGui.QApplication.translate("lpdparams", "Linger", None, QtGui.QApplication.UnicodeUTF8))
        self.linger.setToolTip(QtGui.QApplication.translate("lpdparams", "Linger time", None, QtGui.QApplication.UnicodeUTF8))
        self.linger.setSuffix(QtGui.QApplication.translate("lpdparams", " ms", None, QtGui.QApplication.UnicodeUTF8))

