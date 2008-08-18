# Microsoft Developer Studio Project File - Name="Sprserv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Sprserv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Sprserv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Sprserv.mak" CFG="Sprserv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sprserv - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Sprserv - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Sprserv - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "../INCLUDE" /I "../sprserv" /I "../sprsetw" /I "../spqw" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SPRSERV" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /G3 /MT /W3 /GX /O2 /Ob2 /I "../INCLUDE" /I "../sprserv" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SPRSERV" /D XITEXT_VN=23 /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\xtloglib.lib winsock.lib oldnames.lib /nologo /stack:0x2800 /subsystem:windows /machine:IX86
# ADD LINK32 wsock32.lib /nologo /stack:0x2800 /subsystem:windows /machine:IX86

!ELSEIF  "$(CFG)" == "Sprserv - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../INCLUDE" /I "../sprserv" /I "../sprsetw" /I "../spqw" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "SPRSERV" /FR /Yu"STDAFX.H" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../INCLUDE" /I "../sprserv" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "SPRSERV" /D XITEXT_VN=23 /FR /Yu"STDAFX.H" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\xtloglib.lib winsock.lib oldnames.lib /nologo /stack:0x2800 /subsystem:windows /debug /machine:IX86 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /stack:0x2800 /subsystem:windows /debug /machine:IX86 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Sprserv - Win32 Release"
# Name "Sprserv - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat"
# Begin Source File

SOURCE=..\Winlib\Delimtr.cpp
# End Source File
# Begin Source File

SOURCE=.\Dfdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Formdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Getregdata.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Hex_disp.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Hextoi.cpp
# End Source File
# Begin Source File

SOURCE=.\Hmsg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Issubset.cpp
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

SOURCE=.\Mfdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Monfile.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Netinit.cpp
# End Source File
# Begin Source File

SOURCE=.\Pagedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Parsecmd.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Qmatch.cpp
# End Source File
# Begin Source File

SOURCE=.\Refreshconn.cpp
# End Source File
# Begin Source File

SOURCE=.\Retnabsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Retndlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Secdlg22.cpp
# End Source File
# Begin Source File

SOURCE=.\Sizelim.cpp
# End Source File
# Begin Source File

SOURCE=.\Sprsedoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Sprserv.cpp
# End Source File
# Begin Source File

SOURCE=.\Sprserv.def
# End Source File
# Begin Source File

SOURCE=.\Sprserv.rc
# End Source File
# Begin Source File

SOURCE=.\Sprsevw.cpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"
# End Source File
# Begin Source File

SOURCE=.\Submitj.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Ulist.cpp
# End Source File
# Begin Source File

SOURCE=.\Userdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Winlib\Xtini.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Dfdlg.h
# End Source File
# Begin Source File

SOURCE=.\Formdlg.h
# End Source File
# Begin Source File

SOURCE=.\Hmsg.h
# End Source File
# Begin Source File

SOURCE=.\LoginHost.h
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\Mfdlg.h
# End Source File
# Begin Source File

SOURCE=.\Monfile.h
# End Source File
# Begin Source File

SOURCE=.\Pagedlg.h
# End Source File
# Begin Source File

SOURCE=.\Refreshconn.h
# End Source File
# Begin Source File

SOURCE=.\Retnabsdlg.h
# End Source File
# Begin Source File

SOURCE=.\Retndlg.h
# End Source File
# Begin Source File

SOURCE=.\Secdlg22.h
# End Source File
# Begin Source File

SOURCE=.\Sizelim.h
# End Source File
# Begin Source File

SOURCE=.\Sprsedoc.h
# End Source File
# Begin Source File

SOURCE=.\Sprserv.h
# End Source File
# Begin Source File

SOURCE=.\Sprserv.hpp
# End Source File
# Begin Source File

SOURCE=.\Sprsevw.h
# End Source File
# Begin Source File

SOURCE=.\Stdafx.h
# End Source File
# Begin Source File

SOURCE=.\Userdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Res\noconn.ico
# End Source File
# Begin Source File

SOURCE=.\RES\SPRSERV.ICO
# End Source File
# Begin Source File

SOURCE=.\res\sprserv.rc2
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLBAR.BMP
# End Source File
# End Group
# End Target
# End Project
# Section Sprserv : {F36E7742-6643-11D1-BA1D-00C0DF501B60}
# 	1:13:IDD_LOGINHOST:132
# 	2:16:Resource Include:resource.h
# 	2:13:LoginHost.cpp:LoginHost.cpp
# 	2:10:ENUM: enum:enum
# 	2:11:LoginHost.h:LoginHost.h
# 	2:19:Application Include:sprserv.h
# 	2:13:IDD_LOGINHOST:IDD_LOGINHOST
# 	2:17:CLASS: CLoginHost:CLoginHost
# End Section
