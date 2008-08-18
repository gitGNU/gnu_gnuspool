@echo off
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by SPRSERV.HPJ. >hlp\sprserv.hm
echo. >>hlp\sprserv.hm
echo // Commands (ID_* and IDM_*) >>hlp\sprserv.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\sprserv.hm
echo. >>hlp\sprserv.hm
echo // Prompts (IDP_*) >>hlp\sprserv.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\sprserv.hm
echo. >>hlp\sprserv.hm
echo // Resources (IDR_*) >>hlp\sprserv.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\sprserv.hm
echo. >>hlp\sprserv.hm
echo // Dialogs (IDD_*) >>hlp\sprserv.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\sprserv.hm
echo. >>hlp\sprserv.hm
echo // Frame Controls (IDW_*) >>hlp\sprserv.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\sprserv.hm
REM -- Make help for Project SPRSERV
call hc31 sprserv.hpj
echo.
