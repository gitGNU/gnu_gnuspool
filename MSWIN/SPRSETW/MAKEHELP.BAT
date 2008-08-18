@echo off
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by SPRSETW.HPJ. >hlp\sprsetw.hm
echo. >>hlp\sprsetw.hm
echo // Commands (ID_* and IDM_*) >>hlp\sprsetw.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\sprsetw.hm
echo. >>hlp\sprsetw.hm
echo // Prompts (IDP_*) >>hlp\sprsetw.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\sprsetw.hm
echo. >>hlp\sprsetw.hm
echo // Resources (IDR_*) >>hlp\sprsetw.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\sprsetw.hm
echo. >>hlp\sprsetw.hm
echo // Dialogs (IDD_*) >>hlp\sprsetw.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\sprsetw.hm
echo. >>hlp\sprsetw.hm
echo // Frame Controls (IDW_*) >>hlp\sprsetw.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\sprsetw.hm
REM -- Make help for Project SPRSETW
call hc31 sprsetw.hpj
echo.
