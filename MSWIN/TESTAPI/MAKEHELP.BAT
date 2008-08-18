@echo off
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by TESTAPI.HPJ. >hlp\testapi.hm
echo. >>hlp\testapi.hm
echo // Commands (ID_* and IDM_*) >>hlp\testapi.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\testapi.hm
echo. >>hlp\testapi.hm
echo // Prompts (IDP_*) >>hlp\testapi.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\testapi.hm
echo. >>hlp\testapi.hm
echo // Resources (IDR_*) >>hlp\testapi.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\testapi.hm
echo. >>hlp\testapi.hm
echo // Dialogs (IDD_*) >>hlp\testapi.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\testapi.hm
echo. >>hlp\testapi.hm
echo // Frame Controls (IDW_*) >>hlp\testapi.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\testapi.hm
REM -- Make help for Project TESTAPI
call hc31 testapi.hpj
echo.
