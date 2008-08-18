# Microsoft Visual C++ generated build script - Do not modify

PROJ = TESTAPI
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG
R_RCDEFINES = /dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\XITEXT\TESTAPI\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = /YcSTDAFX.H
CUSEPCHFLAG = 
CPPUSEPCHFLAG = /YuSTDAFX.H
FIRSTC =             
FIRSTCPP = STDAFX.CPP  
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AL /Od /D "_DEBUG" /D "_AFXDLL" /I "\xitext\xtapi" /FR /GA /GEf /Fd"TESTAPI.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G2 /W3 /AL /O1 /D "NDEBUG" /D "_AFXDLL" /I "\xitext\xtapi" /FR /GA /GEf 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:15000 /ALIGN:16 /ONERROR:NOEXE /CO /MAP /LINE  
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:15000 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = mfc250d xtapi winsock oldnames libw llibcew commdlg.lib shell.lib 
LIBS_R_WEXE = mfc250 xtapi winsock oldnames libw llibcew commdlg.lib shell.lib 
RCFLAGS = /nologo /z
RESFLAGS = /nologo /t
RUNFLAGS = 
DEFFILE = TESTAPI.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = STDAFX.SBR \
		TESTAPI.SBR \
		MAINFRM.SBR \
		TESTADOC.SBR \
		TESTAVW.SBR


TESTAPI_RCDEP = c:\xitext\testapi\res\testapi.ico \
	c:\xitext\testapi\res\testapi.rc2


STDAFX_DEP = c:\xitext\testapi\stdafx.h


TESTAPI_DEP = c:\xitext\testapi\stdafx.h \
	c:\xitext\testapi\xtapi.h \
	c:\xitext\testapi\testapi.h \
	c:\xitext\testapi\mainfrm.h \
	c:\xitext\testapi\testadoc.h \
	c:\xitext\testapi\testavw.h


MAINFRM_DEP = c:\xitext\testapi\stdafx.h \
	c:\xitext\testapi\xtapi.h \
	c:\xitext\testapi\testapi.h \
	c:\xitext\testapi\mainfrm.h


TESTADOC_DEP = c:\xitext\testapi\stdafx.h \
	c:\xitext\testapi\xtapi.h \
	c:\xitext\testapi\testapi.h \
	c:\xitext\testapi\testadoc.h \
	c:\tcp\winsock.h


TESTAVW_DEP = c:\xitext\testapi\stdafx.h \
	c:\xitext\testapi\xtapi.h \
	c:\xitext\testapi\testapi.h \
	c:\xitext\testapi\testadoc.h \
	c:\xitext\testapi\testavw.h \
	c:\tcp\winsock.h


all:	$(PROJ).EXE $(PROJ).BSC

TESTAPI.RES:	TESTAPI.RC $(TESTAPI_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r TESTAPI.RC

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

TESTAPI.OBJ:	TESTAPI.CPP $(TESTAPI_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TESTAPI.CPP

MAINFRM.OBJ:	MAINFRM.CPP $(MAINFRM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MAINFRM.CPP

TESTADOC.OBJ:	TESTADOC.CPP $(TESTADOC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TESTADOC.CPP

TESTAVW.OBJ:	TESTAVW.CPP $(TESTAVW_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TESTAVW.CPP


$(PROJ).EXE::	TESTAPI.RES

$(PROJ).EXE::	STDAFX.OBJ TESTAPI.OBJ MAINFRM.OBJ TESTADOC.OBJ TESTAVW.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
TESTAPI.OBJ +
MAINFRM.OBJ +
TESTADOC.OBJ +
TESTAVW.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\windows\+
c:\msvc\lib\+
c:\msvc\mfc\lib\+
c:\tcp\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) TESTAPI.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	TESTAPI.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) TESTAPI.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
