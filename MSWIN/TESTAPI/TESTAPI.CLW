; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CLoginAs
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "testapi.h"
VbxHeaderFile=testapi.h
VbxImplementationFile=testapi.cpp
LastPage=0

ClassCount=6
Class1=CTestapiApp
Class2=CTestapiDoc
Class3=CTestapiView
Class4=CMainFrame

ResourceCount=3
Resource1=IDR_MAINFRAME
Resource2=IDD_ABOUTBOX
Class5=CAboutDlg
Resource3=IDD_LOGIN
Class6=CLoginAs

[CLS:CTestapiApp]
Type=0
HeaderFile=testapi.h
ImplementationFile=testapi.cpp
Filter=N
LastObject=CTestapiApp

[CLS:CTestapiDoc]
Type=0
HeaderFile=testadoc.h
ImplementationFile=testadoc.cpp
Filter=N

[CLS:CTestapiView]
Type=0
HeaderFile=testavw.h
ImplementationFile=testavw.cpp
Filter=C

[CLS:CMainFrame]
Type=0
HeaderFile=mainfrm.h
ImplementationFile=mainfrm.cpp
Filter=T
LastObject=ID_SPLURGE_CREATEJOB

[CLS:CAboutDlg]
Type=0
HeaderFile=testapi.cpp
ImplementationFile=testapi.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_MRU_FILE1
Command6=ID_APP_EXIT
Command7=ID_EDIT_UNDO
Command8=ID_EDIT_CUT
Command9=ID_EDIT_COPY
Command10=ID_EDIT_PASTE
Command11=ID_SPLURGE_CREATEJOB
Command12=ID_VIEW_TOOLBAR
Command13=ID_VIEW_STATUS_BAR
Command14=ID_HELP_INDEX
Command15=ID_HELP_USING
Command16=ID_APP_ABOUT
CommandCount=16

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_EDIT_COPY
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=ID_NEXT_PANE
Command13=ID_PREV_PANE
Command14=ID_CONTEXT_HELP
Command15=ID_HELP
CommandCount=15

[DLG:IDD_LOGIN]
Type=1
Class=CLoginAs
ControlCount=8
Control1=IDC_STATIC,static,1342308352
Control2=IDC_USERNAME,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_MACHINE,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_PASSWD,edit,1350631584
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816

[CLS:CLoginAs]
Type=0
HeaderFile=LoginAs.h
ImplementationFile=LoginAs.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CLoginAs

