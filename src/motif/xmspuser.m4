include(xres.m4)
include(stdcolours.m4)
XRES_APPLICATION(gspl-xmuser, {GSPL-XMUSER - Edit userr options}, 32)

TOPLEVEL_RESOURCE(toolTipEnable, True)
TOPLEVEL_RESOURCE(titlePresent, True)
TOPLEVEL_RESOURCE(footerPresent, True)
TOPLEVEL_RESOURCE(sortAlpha, True)
XRES_SPACE

XRES_WIDGETOFFSETS(10,10,5,5)
XRES_SPACE
XRES_TITLE(utitle, {User     def min max cps form              ptr             Class       S/Priv   })
XRES_SPACE
XRES_FOOTER(footer)
XRES_SPACE
XRES_PANED(layout)
XRES_SPACE
TAB_SET(40)

XRES_LIST(dlist, 0)
XRES_LIST(ulist, 15)
XRES_SPACE

XRES_MENU(menubar)
XRES_SPACE
XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Defaults, Defaults, D)
XRES_MENUHDR(Users, Users, U)
XRES_MENUHDR(Charges, Charges, C)
XRES_MENUHDR(Help, Help, H)
XRES_MENUHDR(usermacro, Usermacro, m)
XRES_MENUHDR(Search, Search, S)
XRES_COMMENT(Options pulldown)

TAB_SET(48)
XRES_COMMENT(Pulldowns for "options")
XRES_MENUITEM(Disporder, Display order, o, C-o)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Quit, Quit, Q, C-c)

XRES_COMMENT(Defaults pulldown)
XRES_MENUITEM(dpri, Priorities and copies, P, P)
XRES_MENUITEM(dform, Form type, F, F)
XRES_MENUITEM(dptr, Printer, n, O)
XRES_MENUITEM(dforma, Allowed form, r, R)
XRES_MENUITEM(dptra, Allowed printer, l, G)
XRES_MENUITEM(dclass, Class codes, C, C)
XRES_MENUITEM(dpriv, Privileges, v, V)
XRES_MENUITEM(defcpy, Copy to All users, A, A)

XRES_COMMENT(Users pulldown)
XRES_MENUITEM(upri, Priorities and copies, P, p)
XRES_MENUITEM(uform, Form type, F, f)
XRES_MENUITEM(uptr, Printer, n, o)
XRES_MENUITEM(uforma, Allowed form, r, r)
XRES_MENUITEM(uptra, Allowed printer, l, g)
XRES_MENUITEM(uclass, Class codes, C, c)
XRES_MENUITEM(upriv, Privileges, v, v)
XRES_MENUITEM(ucpy, Copy defaults, d, C-d)

XRES_COMMENT(Charges pulldown)
XRES_MENUITEM(Display, Display Charges, C)
XRES_MENUITEM(Zero, Zero charges for selected users, Z, Z)
XRES_MENUITEM(Zeroall, Zero charges for ALL users, Z, A-Z)
XRES_MENUITEM(Impose, Impose fee, I, I)

XRES_COMMENT(Search pulldown)
XRES_MENUITEM(Search, {Search for...}, S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)

XRES_COMMENT(Pulldowns for "help")
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)
XRES_SPACE

XRES_MACROMENU(usermacro, Run command macro, Macro command)

XRES_COMMENT(These are used in various places)
TAB_SET(32)
XRES_GENERALLABEL(Setall, Set all codes)
XRES_GENERALLABEL(Clearall, Clear all codes)
XRES_GENERALLABEL(defedit, Editing default options to apply to new users)
XRES_GENERALLABEL(useredit, Editing options for users...)
XRES_GENERALLABEL(admin, Edit system administration file)
XRES_GENERALLABEL(sstop, Stop the scheduler (and adjust network connections))
XRES_GENERALLABEL(forms, Select form types other than the default)
XRES_GENERALLABEL(changep, Change priorities of jobs once queued)
XRES_GENERALLABEL(otherj, Edit jobs belong to other users)
XRES_GENERALLABEL(otherp, Select printers other than the default)
XRES_GENERALLABEL(prinq, Access printers)
XRES_GENERALLABEL(haltgo, Stop and start printers)
XRES_GENERALLABEL(anyprio, Set any priority on the queue)
XRES_GENERALLABEL(cdefltp, Change own default priority and form type)
XRES_GENERALLABEL(adddel, Add and delete printers)
XRES_GENERALLABEL(override, {Override class codes, adding in bits})
XRES_GENERALLABEL(unqueue, Unqueue jobs)
XRES_GENERALLABEL(votherj, View (not necessarily edit) other users' jobs)
XRES_GENERALLABEL(remotej, Access remote jobs)
XRES_GENERALLABEL(remotep, Access remote printers)
XRES_GENERALLABEL(accessok, Access non-displayed parameters)
XRES_GENERALLABEL(freezeok, Save default parameters)

XRES_COMMENT(Dialog titles etc for help/error/info/confirm)
XRES_STDDIALOG(help, {On line help.....})
XRES_STDDIALOG(error, {Whoops!!!})
XRES_STDDIALOG(Confirm, {Are you sure?})

XRES_COMMENT(Display order dialog)
TAB_SET(48)
XRES_DIALOG(Disporder, Set order)
XRES_DLGLABEL(sortuid, Sort users into order of numeric userid)
XRES_DLGLABEL(sortname, Sort users into alphabetic order of name)
XRES_DLGLABEL(Ok, Apply)

XRES_COMMENT(Default priorities)
XRES_DIALOG(defpri, Set default priorities)
XRES_DLGLABEL(min, Minimum priority)
XRES_DLGLABEL(def, Default priority)
XRES_DLGLABEL(max, Maximum priority)
XRES_DLGLABEL(copies, Maximum copies at once)
XRES_DLGLABEL(cdefltp, User can reset his own default priority)

XRES_COMMENT(Default form type and printers)
XRES_SELDIALOG(dforms, {Default form type}, {Possible form types....}, {Set to....})
XRES_SELDIALOG(dformall, {Default form patterns}, {Possible form patterns....}, {Set to....})
XRES_SELDIALOG(dptrs, {Default printer}, {Possible printers....}, {Set to....})
XRES_SELDIALOG(dptrall, {Default ptr patterns}, {Possible ptr patterns....}, {Set to....})

XRES_COMMENT(Default class codes)
XRES_DIALOG(defclass, Default class codes)

XRES_COMMENT(Privileges)
XRES_DIALOG(defprivs, Default privileges)
XRES_DIALOG(uprivs, User privileges)
XRES_DLGLABEL(cdef, Set to default values)

XRES_DIALOG(upri, Set user's priorities)
XRES_DLGLABEL(min, Minimum priority)
XRES_DLGLABEL(def, Default priority)
XRES_DLGLABEL(max, Maximum priority)
XRES_DLGLABEL(copies, Maximum copies at once)
XRES_SPACE

XRES_STDDIALOG(uforms, {Default form type}, {Possible form types....}, {Set to....})
XRES_STDDIALOG(uformall, {Default form patterns}, {Possible form patterns....}, {Set to....})
XRES_STDDIALOG(uptrs, {Default printer}, {Possible printers....}, {Set to....})
XRES_STDDIALOG(uptrall, {Default ptr patterns}, {Possible ptr patterns....}, {Set to....})
XRES_SPACE

XRES_DIALOG(uclass, Users class codes)
XRES_DIALOG(chlist, {List of charges....}, 200)
XRES_SPACE

XRES_DIALOG(uimpose, Impose charges)
XRES_DLGLABEL(amount, Charge amount:)
XRES_SPACE

XRES_DIALOG(Search, {Search list for user...})
XRES_DLGLABEL(lookfor, {Search for user:})
XRES_DLGLABEL(forward, Search forward)
XRES_DLGLABEL(backward, Search backward)
XRES_DLGLABEL(match, Match case)
XRES_DLGLABEL(wrap, Wrap around)
XRES_DIALOG(about, {About xmspuser.....})
XRES_END
