# Microsoft Developer Studio Project File - Name="Spqw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Spqw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Spqw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Spqw.mak" CFG="Spqw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Spqw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Spqw - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Spqw - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "../INCLUDE" /I "../spqw" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SPQW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G3 /MD /W3 /GX /O2 /Ob2 /I "../INCLUDE" /I "../spqw" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SPQW" /D XITEXT_VN=23 /D "_AFXDLL" /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\xtloglib.lib winsock.lib oldnames.lib /nologo /stack:0x4098 /subsystem:windows /machine:IX86
# ADD LINK32 wsock32.lib /nologo /stack:0x4098 /subsystem:windows /machine:IX86

!ELSEIF  "$(CFG)" == "Spqw - Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /O2 /I "../INCLUDE" /I "../spqw" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "SPQW" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /I "../INCLUDE" /I "../spqw" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "SPQW" /D XITEXT_VN=23 /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\xtloglib.lib winsock.lib oldnames.lib /nologo /stack:0x4098 /subsystem:windows /debug /machine:IX86 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /stack:0x4098 /subsystem:windows /debug /machine:IX86 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Spqw - Win32 Release"
# Name "Spqw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat"
# Begin Source File

SOURCE=.\Editfmt.cpp
# End Source File
# Begin Source File

SOURCE=.\Editsep.cpp
# End Source File
# Begin Source File

SOURCE=.\Fmtdef.cpp
# End Source File
# Begin Source File

SOURCE=.\Formdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Getregdata.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Issubset.cpp
# End Source File
# Begin Source File

SOURCE=.\Jdatadoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Jdatavie.cpp
# End Source File
# Begin Source File

SOURCE=.\Jdsearch.cpp
# End Source File
# Begin Source File

SOURCE=.\Jobdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Joblist.cpp
# End Source File
# Begin Source File

SOURCE=.\Jobview.cpp
# End Source File
# Begin Source File

SOURCE=.\Jpsearch.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginHost.cpp
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Matchrou.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Netinit.cpp
# End Source File
# Begin Source File

SOURCE=.\Netmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Netstat.cpp
# End Source File
# Begin Source File

SOURCE=.\Opdlg22.cpp
# End Source File
# Begin Source File

SOURCE=.\Pagedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Plist.cpp
# End Source File
# Begin Source File

SOURCE=.\Poptsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Prform.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Prinsize.cpp
# End Source File
# Begin Source File

SOURCE=.\Ptrcolours.cpp
# End Source File
# Begin Source File

SOURCE=.\Ptrdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Ptrview.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Qmatch.cpp
# End Source File
# Begin Source File

SOURCE=.\Restrict.cpp
# End Source File
# Begin Source File

SOURCE=.\Retndlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Rowview.cpp
# End Source File
# Begin Source File

SOURCE=.\Secdlg22.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Smstr.cpp
# End Source File
# Begin Source File

SOURCE=.\Spqw.cpp
# End Source File
# Begin Source File

SOURCE=.\Spqw.def
# End Source File
# Begin Source File

SOURCE=.\Spqw.rc
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=..\Winlib\Ulist.cpp
# End Source File
# Begin Source File

SOURCE=.\Unqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\Uperm.cpp
# End Source File
# Begin Source File

SOURCE=.\Userdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Vmdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Xtini.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Editfmt.h
# End Source File
# Begin Source File

SOURCE=.\Editsep.h
# End Source File
# Begin Source File

SOURCE=.\Fmtdef.h
# End Source File
# Begin Source File

SOURCE=.\formatcode.h
# End Source File
# Begin Source File

SOURCE=.\Formdlg.h
# End Source File
# Begin Source File

SOURCE=.\Jdatadoc.h
# End Source File
# Begin Source File

SOURCE=.\Jdatavie.h
# End Source File
# Begin Source File

SOURCE=.\Jdsearch.h
# End Source File
# Begin Source File

SOURCE=.\Jident.h
# End Source File
# Begin Source File

SOURCE=.\Jobdoc.h
# End Source File
# Begin Source File

SOURCE=.\Joblist.h
# End Source File
# Begin Source File

SOURCE=.\Jobview.h
# End Source File
# Begin Source File

SOURCE=.\Jpsearch.h
# End Source File
# Begin Source File

SOURCE=.\LoginHost.h
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\Netmsg.h
# End Source File
# Begin Source File

SOURCE=.\Netstat.h
# End Source File
# Begin Source File

SOURCE=.\Opdlg22.h
# End Source File
# Begin Source File

SOURCE=.\Pagedlg.h
# End Source File
# Begin Source File

SOURCE=.\Pident.h
# End Source File
# Begin Source File

SOURCE=.\Plist.h
# End Source File
# Begin Source File

SOURCE=.\Poptsdlg.h
# End Source File
# Begin Source File

SOURCE=.\Prform.h
# End Source File
# Begin Source File

SOURCE=.\Ptrcolours.h
# End Source File
# Begin Source File

SOURCE=.\Ptrdoc.h
# End Source File
# Begin Source File

SOURCE=.\Ptrview.h
# End Source File
# Begin Source File

SOURCE=.\Restrict.h
# End Source File
# Begin Source File

SOURCE=.\Retndlg.h
# End Source File
# Begin Source File

SOURCE=.\Rowview.h
# End Source File
# Begin Source File

SOURCE=.\Secdlg22.h
# End Source File
# Begin Source File

SOURCE=.\Spqw.h
# End Source File
# Begin Source File

SOURCE=.\Spqw.hpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.h
# End Source File
# Begin Source File

SOURCE=.\Uperm.h
# End Source File
# Begin Source File

SOURCE=.\Userdlg.h
# End Source File
# Begin Source File

SOURCE=.\Vmdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES\IDR_MAIN.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\JOBS.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\PTR.ICO
# End Source File
# Begin Source File

SOURCE=.\RES\SPQW.ICO
# End Source File
# Begin Source File

SOURCE=.\res\spqw.rc2
# End Source File
# Begin Source File

SOURCE=.\RES\VIEWJOB.ICO
# End Source File
# End Group
# End Target
# End Project
# Section Spqw : {F36E7742-6643-11D1-BA1D-00C0DF501B60}
# 	1:14:IDD_LOGINHOST1:107
# 	2:16:Resource Include:resource.h
# 	2:13:LoginHost.cpp:LoginHost.cpp
# 	2:10:ENUM: enum:enum
# 	2:11:LoginHost.h:LoginHost.h
# 	2:19:Application Include:spqw.h
# 	2:13:IDD_LOGINHOST:IDD_LOGINHOST1
# 	2:17:CLASS: CLoginHost:CLoginHost
# End Section
