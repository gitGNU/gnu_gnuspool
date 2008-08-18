TAB_SET(32)
XRES_INSERT([[!! *******START OF RESOURCES for xmspuser

]])
XRES_APPLICATION("xmspuser", "XMSPUSER - Edit user options")
XRES_INSERT([[
]])
XRES_RESOURCE("titlePresent",,,,,True)
XRES_RESOURCE("footerPresent",,,,,True)
XRES_RESOURCE("sortAlpha",,,,,True)
XRES_INSERT([[
]])
XRES_WIDGETOFFSETS(10,10,5,5)
XRES_INSERT([[
]])
XRES_LABELWIDGET(utitle, red, white,
		"User     def min max cps form              ptr             Class       S/Priv   ", B)
XRES_INSERT([[
]])
XRES_LABELWIDGET("footer", "ivory", "sienna", "XMSPUSER Copyright Xi Software Ltd 2000")
XRES_INSERT([[
]])
XRES_LAYOUT("layout", "red", "white")
XRES_INSERT([[
]])
TAB_SET(40)
XRES_LISTWIDGET(dlist, black, #E8E3D9)
XRES_LISTWIDGET(ulist, black, #E8E3D9, 15)
XRES_INSERT([[
]])
XRES_MENUSTART(menubar,	black, #FEFCD6, hashFEFCD6)
XRES_INSERT([[
]])
XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Defaults, Defaults, D)
XRES_MENUHDR(Users, Users, U)
XRES_MENUHDR(Charges, Charges, C)
XRES_MENUHDR(Help, Help, H)
XRES_MENUHDR(usermacro, Usermacro, m)
XRES_MENUHDR(Search, Search, S)
XRES_INSERT([[
!! Options pulldown

]])
TAB_SET(48)
XRES_MENUITEM(Disporder, Display order, o, C-o)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Quit, Quit, Q, C-c)
XRES_INSERT([[
!! Defaults pulldown

]])
XRES_MENUITEM(dpri, Priorities and copies, P, P)
XRES_MENUITEM(dform, Form type, F, F)
XRES_MENUITEM(dptr, Printer, n, O)
XRES_MENUITEM(dforma, Allowed form, r, R)
XRES_MENUITEM(dptra, Allowed printer, l, G)
XRES_MENUITEM(dclass, Class codes, C, C)
XRES_MENUITEM(dpriv, Privileges, v, V)
XRES_MENUITEM(defcpy, Copy to All users, A, A)
XRES_INSERT([[
!! Users pulldown

]])
XRES_MENUITEM(upri, Priorities and copies, P, p)
XRES_MENUITEM(uform, Form type, F, f)
XRES_MENUITEM(uptr, Printer, n, o)
XRES_MENUITEM(uforma, Allowed form, r, r)
XRES_MENUITEM(uptra, Allowed printer, l, g)
XRES_MENUITEM(uclass, Class codes, C, c)
XRES_MENUITEM(upriv, Privileges, v, v)
XRES_MENUITEM(ucpy, Copy defaults, d, C-d)
XRES_INSERT([[
!! Charges pulldown

]])
XRES_MENUITEM(Display, Display Charges, C)
XRES_MENUITEM(Zero, Zero charges for selected users, Z, Z)
XRES_MENUITEM(Zeroall, Zero charges for ALL users, Z, A-Z)
XRES_MENUITEM(Impose, Impose fee, I, I)
XRES_INSERT([[
!! Search pulldown

]])
XRES_MENUITEM(Search, "Search for...", S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)
XRES_INSERT([[
!! Pulldowns for "help"

]])
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)
XRES_INSERT([[
]])
XRES_INSERTMACRO(macro, Run command macro, Macro command)
XRES_INSERT([[
!!	These are used in various places

]])
TAB_SET(32)
XRES_PUSHBUTTONGADGET("Setall", "Set all codes")
XRES_PUSHBUTTONGADGET("Clearall", "Clear all codes")
XRES_LABELGADGET(defedit, Editing default options to apply to new users)
XRES_LABELGADGET(useredit, "Editing options for users...")
XRES_TOGGLEBUTTONGADGET(admin, Edit system administration file)
XRES_TOGGLEBUTTONGADGET(sstop, Stop the scheduler (and adjust network connections))
XRES_TOGGLEBUTTONGADGET(forms, Select form types other than the default)
XRES_TOGGLEBUTTONGADGET(changep, Change priorities of jobs once queued)
XRES_TOGGLEBUTTONGADGET(otherj, Edit jobs belong to other users)
XRES_TOGGLEBUTTONGADGET(otherp, Select printers other than the default)
XRES_TOGGLEBUTTONGADGET(prinq, Access printers)
XRES_TOGGLEBUTTONGADGET(haltgo, Stop and start printers)
XRES_TOGGLEBUTTONGADGET(anyprio, Set any priority on the queue)
XRES_TOGGLEBUTTONGADGET(cdefltp, Change own default priority and form type)
XRES_TOGGLEBUTTONGADGET(adddel, Add and delete printers)
XRES_TOGGLEBUTTONGADGET(override, Override class codes, adding in bits)
XRES_TOGGLEBUTTONGADGET(unqueue, Unqueue jobs)
XRES_TOGGLEBUTTONGADGET(votherj, View (not necessarily edit) other users' jobs)
XRES_TOGGLEBUTTONGADGET(remotej, Access remote jobs)
XRES_TOGGLEBUTTONGADGET(remotep, Access remote printers)
XRES_TOGGLEBUTTONGADGET(accessok, Access non-displayed parameters)
XRES_TOGGLEBUTTONGADGET(freezeok, Save default parameters)

XRES_INSERT([[
!! Dialog titles etc for help/error/info/confirm

]])
XRES_STDDIALOG(help, "On line help.....")
XRES_STDDIALOG(error, "Whoops!!!", white, firebrick)
XRES_STDDIALOG(Confirm, "Are you sure?")
XRES_INSERT([[
!! Display order dialog

]])
TAB_SET(48)
XRES_DIALOG(Disporder, Set order)
XRES_TOGGLEBUTTONGADGET(sortuid, Sort users into order of numeric userid)
XRES_TOGGLEBUTTONGADGET(sortname, Sort users into alphabetic order of name)
XRES_PUSHBUTTONGADGET(Ok, Apply)
XRES_INSERT([[
!! Default priorities

]])
XRES_DIALOG(defpri, Set default priorities)
XRES_LABELGADGET(min, Minimum priority)
XRES_LABELGADGET(def, Default priority)
XRES_LABELGADGET(max, Maximum priority)
XRES_LABELGADGET(copies, Maximum copies at once)
XRES_LABELGADGET(cdefltp, User can reset his own default priority)
XRES_INSERT([[
!! Default form type and printers

]])
XRES_SELDIALOG(dforms, Default form type, "Possible form types....", "Set to....")
XRES_SELDIALOG(dformall, Default form patterns, "Possible form patterns....", "Set to....")
XRES_SELDIALOG(dptrs, Default printer, "Possible printers....", "Set to....")
XRES_SELDIALOG(dptrall, Default ptr patterns, "Possible ptr patterns....", "Set to....")
XRES_INSERT([[
!! Default class codes

]])
XRES_DIALOG(defclass, Default class codes)
XRES_INSERT([[
!! Privileges

]])
XRES_DIALOG(defprivs, Default privileges)
XRES_DIALOG(uprivs, User privileges)
XRES_PUSHBUTTONGADGET(uprivs, Set to default values)
XRES_DIALOG(upri, Set user's priorities)
XRES_LABELGADGET(min, Minimum priority)
XRES_LABELGADGET(def, Default priority)
XRES_LABELGADGET(max, Maximum priority)
XRES_LABELGADGET(copies, Maximum copies at once)
XRES_INSERT([[
]])
XRES_STDDIALOG(uforms, Default form type, "Possible form types....", "Set to....)
XRES_STDDIALOG(uformall, Default form patterns, "Possible form patterns....", "Set to....")
XRES_STDDIALOG(uptrs, Default printer, "Possible printers....", "Set to....")
XRES_STDDIALOG(uptrall, Default ptr patterns, "Possible ptr patterns....", "Set to....")
XRES_INSERT([[
]])
XRES_DIALOG(uclass, Users class codes)
XRES_DIALOG(chlist, "List of charges....", white, firebrick, 200)
XRES_INSERT([[
]])
XRES_DIALOG(uimpose, Impose charges)
XRES_LABELGADGET(amount, Charge amount:)
XRES_INSERT([[
]])
XRES_DIALOG(Search, "Search list for user...")
XRES_LABELGADGET(lookfor, Search for user:)
XRES_TOGGLEBUTTONGADGET(forward, Search forward)
XRES_TOGGLEBUTTONGADGET(backward, Search backward)
XRES_TOGGLEBUTTONGADGET(match, Match case)
XRES_TOGGLEBUTTONGADGET(wrap, Wrap around)
XRES_STDDIALOG(about, "About xmspuser.....")
XRES_INSERT([[!! *******END OF RESOURCES for xmspuser

]])
