@echo off
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by SPQW.HPJ. >hlp\SPQW.hm
echo. >>hlp\SPQW.hm
echo // Commands (ID_* and IDM_*) >>hlp\SPQW.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\SPQW.hm
echo. >>hlp\SPQW.hm
echo // Prompts (IDP_*) >>hlp\SPQW.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\SPQW.hm
echo. >>hlp\SPQW.hm
echo // Resources (IDR_*) >>hlp\SPQW.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\SPQW.hm
echo. >>hlp\SPQW.hm
echo // Dialogs (IDD_*) >>hlp\SPQW.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\SPQW.hm
echo. >>hlp\SPQW.hm
echo // Frame Controls (IDW_*) >>hlp\SPQW.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\SPQW.hm
REM -- Make help for Project SPQW
call hc31 SPQW.hpj
echo.
