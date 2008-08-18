include(xres.m4)
include(stdcolours.m4)
XRES_APPLICATION(gspl-xmpq, {GSPL-XMPQ - GNUSpool Queue Viewer}, 32)

TOPLEVEL_RESOURCE(toolTipEnable, True)
TOPLEVEL_RESOURCE(toolbarPresent, True)
TOPLEVEL_RESOURCE(jtitlePresent, True)
TOPLEVEL_RESOURCE(ptitlePresent, True)
TOPLEVEL_RESOURCE(footerPresent, True)
TOPLEVEL_RESOURCE(keepJobScroll, False)
TOPLEVEL_RESOURCE(noPageCount, False)
TOPLEVEL_RESOURCE(localOnly, False)
TOPLEVEL_RESOURCE(unPrintedOnly, 0)
TOPLEVEL_RESOURCE(confirmAbort, Unprinted)
TOPLEVEL_RESOURCE(repeatTime, 500)
TOPLEVEL_RESOURCE(repeatInt, 20)
XRES_SPACE

XRES_WIDGETOFFSETS(10,10,5,5)
XRES_SPACE
XRES_FOOTER(footer)
XRES_SPACE
XRES_PANED(layout)
XRES_SPACE

XRES_TITLE(jtitle)
XRES_LIST(jlist, 12)

XRES_POPUPMENU(do-jobpop, jobpopup)
XRES_MENUITEM(Abortj, Abort/delete job)
XRES_MENUITEM(Onemore, One more copy)
XRES_MENUITEM(Formj, Set job form)
XRES_MENUITEM(View, View job text)
XRES_MENUITEM(Unqueue, Unqueue job)

XRES_TITLE(ptitle)
XRES_LIST(plist, 4)
XRES_POPUPMENU(do-ptrpop, ptrpopup)
XRES_MENUITEM(Go, Start printer)
XRES_MENUITEM(Interrupt, Interrupt printer)
XRES_MENUITEM(Restart, Restart job on printer)
XRES_MENUITEM(Abortp, Abort job on printer)
XRES_MENUITEM(Heoj, Halt at end of job)
XRES_MENUITEM(Halt, Halt at once)
XRES_MENUITEM(Formp, Set form type)
XRES_MENUITEM(Ok, Alignment OK)
XRES_MENUITEM(Nok, Rerun Alignment)

XRES_SPACE
TAB_SET(40)
XRES_MENU(menubar)
XRES_SPACE

XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Action, Action, A)
XRES_MENUHDR(Jobs, Jobs, J)
XRES_MENUHDR(Printers, Printers, P)
XRES_MENUHDR(Search, Search, S)
XRES_MENUHDR(Help, Help, H)
XRES_MENUHDR(jobmacro, Jobmacro, b)
XRES_MENUHDR(ptrmacro, Ptrmacro, r)

XRES_COMMENT(Pulldowns for "options")
XRES_MENUITEM(Viewopts, View options, V, equal, =)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Syserror, Display Error log, E)
XRES_MENUITEM(Quit, Quit, Q, C-c)

XRES_COMMENT(Pulldowns for "action")
XRES_MENUITEM(Abortj, Abort/Delete job, A, a)
XRES_MENUITEM(Onemore, One more copy, c, plus, +)
XRES_MENUITEM(Go, Start printer, S, G)
XRES_MENUITEM(Heoj, Halt printer at eoj, H, h)
XRES_MENUITEM(Halt, Kill printer, K, H)
XRES_MENUITEM(Ok, Yes - ok align, Y, y)
XRES_MENUITEM(Nok, No - redo align, N, n)

XRES_COMMENT(Pulldowns for "jobs")
XRES_MENUITEM(View, View job, V, I)
XRES_MENUITEM(Formj, Form title pri cps, F, f)
XRES_MENUITEM(Pages, Job pages, p)
XRES_MENUITEM(User, Job user and mail, u)
XRES_MENUITEM(Retain, Retention options, R)
XRES_MENUITEM(Classj, Class codes for job, c)
XRES_MENUITEM(Unqueue, Unqueue job, U)

XRES_COMMENT(Pulldowns for "printers")
TAB_SET(48)
XRES_MENUITEM(Interrupt, Interrupt printer, I, S-1, !)
XRES_MENUITEM(Abortp, Abort printer, A)
XRES_MENUITEM(Restart, Restart printer, R)
XRES_MENUITEM(Formp, Set printer form, f)
XRES_MENUITEM(Devicep, Set device name, v)
XRES_MENUITEM(Classp, Set printer class code, c)
XRES_MENUITEM(Add, Add new printer, n)
XRES_MENUITEM(Delete, Delete printer, D)

XRES_COMMENT(Pulldowns for "search")
XRES_MENUITEM(Search, Search for..., S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)

XRES_SPACE
TAB_SET(40)
XRES_MACROMENU(jobmacro, Run job command macro, Macro command)
XRES_SPACE
XRES_MACROMENU(ptrmacro, Run printer command macro, Macro command)

XRES_COMMENT(Pulldowns for "help")
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)

XRES_COMMENT(Toolbar buttons)
TAB_SET(40)
XRES_TOOLBAR(toolbar, 30, 50)
XRES_TOOLBARICON(Abortjt, Abort/Del, xmspqAbort.xpm, Delete this job - abort if being printed)
XRES_TOOLBARICON(Onemore, +1, xmspq+1.xpm, Increment the number of copies by one (reprint))
XRES_TOOLBARICON(Form, Job Form, xmspqJobForm.xpm, Change the form type or printer for this job)
XRES_TOOLBARICON(View, View job, xmspqViewJob.xpm, View the text of the current job)
XRES_TOOLBARICON(Pform, Ptr Form, xmspqPrinterForm.xpm, Change the form type on the printer)
XRES_TOOLBARICON(Go, Go Ptr, xmspqStartPrinter.xpm, Start the selected printer)
XRES_TOOLBARICON(Heoj, Halt EOJ, xmspqHaltEndOfJob.xpm, Halt the printer at the end of the current job)
XRES_TOOLBARICON(Halt, Stop now, xmspqHaltAtOnce.xpm, Halt the printer immediately - aborting the current print)
XRES_TOOLBARICON(Ok, OK align, xmspqApproveAlignment.xpm, Accept alignment for the currently-selected printer)
XRES_TOOLBARICON(Nok, Not OK, xmspqRejectAlignment.xpm, Reject alignment for the currently-selected printer)

XRES_COMMENT(Dialog titles etc for help/error/info/confirm)

XRES_STDDIALOG(help, {On line help.....})
XRES_STDDIALOG(error, {Whoops!!!})
XRES_STDDIALOG(Confirm, {Are you sure?})

XRES_COMMENT(These are used in various places)
XRES_GENERALLABEL(Setall, Set all codes)
XRES_GENERALLABEL(Clearall, Clear all codes)
XRES_GENERALLABEL(jobnotitle, Editing job number:)

XRES_COMMENT(View options dialog)
XRES_DIALOG(Viewopts, Display options)
XRES_DLGLABEL(viewtitle, {Selecting view options})
XRES_DLGLABEL(username, {Restrict jobs display to user})
XRES_DLGLABEL(uselect, {Select...})
XRES_DLGLABEL(ptrname, {Restrict display to printer})
XRES_DLGLABEL(pselect, {Select...})
XRES_DLGLABEL(tittitle, {Restrict display by titles})
XRES_DLGLABEL(allhosts, {View all hosts})
XRES_DLGLABEL(localonly, {View local host only})
XRES_DLGLABEL(confb1, {Do not confirm delete/abort jobs})
XRES_DLGLABEL(confb2, {Confirm delete/abort if not printed})
XRES_DLGLABEL(confb3, {Always confirm delete/abort})
XRES_DLGLABEL(restrp1, {All jobs})
XRES_DLGLABEL(restrp2, {Unprinted jobs})
XRES_DLGLABEL(restrp3, {Printed jobs})
XRES_DLGLABEL(jincl1, {Jobs for matching printer})
XRES_DLGLABEL(jincl2, {Jobs for printer plus null})
XRES_DLGLABEL(jincl3, {All jobs disregarding printer})
XRES_DLGLABEL(sortptrs, {Sort printer names})
XRES_DLGLABEL(jdispfmt, {Reset Job Display Fields})
XRES_DLGLABEL(pdispfmt, {Reset Printer Display Fields})
XRES_DLGLABEL(savehome, {Save formats in home directory})
XRES_DLGLABEL(savecurr, {Save formats in current directory})
XRES_DLGLABEL(Ok, {Apply})

XRES_COMMENT(User select dialog from view options)
XRES_SELDIALOG(uselect, {Select a user}, {Possible users....}, {Set to....})

XRES_COMMENT(Printer select dialog from view options)
XRES_SELDIALOG(pselect, {Select a printer}, {Possible printers....}, {Set to....})

XRES_COMMENT(Job display fields)
XRES_DIALOG(Jdisp, {Reset Job Display Fields})
XRES_DLGLABEL(Newfld, {New field})
XRES_DLGLABEL(Newsep, {New separator})
XRES_DLGLABEL(Editfld, {Edit field})
XRES_DLGLABEL(Editsep, {Edit separator})
XRES_DLGLABEL(Delete, {Delete field/sep})
XRES_DLGLIST(Jdisplist, 16)
XRES_SPACE

XRES_DIALOG(jfldedit, {Job display field})
XRES_DLGLABEL(width, {Width of field})
XRES_DLGLABEL(useleft, {Use previous field if too large})
XRES_SPACE

XRES_DIALOG(Pdisp, {Reset Printer Display Fields})
XRES_DLGLABEL(Newfld, {New field})
XRES_DLGLABEL(Newsep, {New separator})
XRES_DLGLABEL(Editfld, {Edit field})
XRES_DLGLABEL(Editsep, {Edit separator})
XRES_DLGLABEL(Delete, {Delete field/sep})
XRES_DLGLIST(Pdisplist, 8)
XRES_SPACE

XRES_DIALOG(pfldedit, {Printer display field})
XRES_DLGLABEL(width, {Width of field})
XRES_DLGLABEL(useleft, {Use previous field if too large})
XRES_DLGLABEL(skipright, {Delete following fields if too large})
XRES_SPACE

XRES_DIALOG(msave, {Select file to save in})

XRES_COMMENT(Job form dialog)
XRES_DIALOG(Jform, {Edit form, header, priority, copies})
XRES_DLGLABEL(form, {Form    })
XRES_DLGLABEL(fselect, {Select forms})
XRES_DLGLABEL(Header, {Title   })
XRES_DLGLABEL(supph, {Suppress header page})
XRES_DLGLABEL(ptrname, {Printer })
XRES_DLGLABEL(pselect, {Select printers})
XRES_DLGLABEL(Priority, {Priority})
XRES_DLGLABEL(Copies, {Copies  })

XRES_COMMENT(Form select dialog from job options)
XRES_SELDIALOG(fselect, {Select a form}, {Possible forms....}, {Set form to....})

XRES_COMMENT(Page select dialog from job options)
XRES_DIALOG(Jpage, Select pages)
XRES_DLGLABEL(Spage, {Starting page})
XRES_DLGLABEL(Epage, {End page})
XRES_DLGLABEL(Hpage, {"Halted at" page})
XRES_DLGLABEL(allpages, {Print all pages})
XRES_DLGLABEL(oddpages, {Print odd pages})
XRES_DLGLABEL(evenpages, {Print even pages})
XRES_DLGLABEL(swapoe, {Swap printing odd/even pages})
XRES_DLGLABEL(Flags, {Post/proc flags})

XRES_COMMENT(Mail/write options)
XRES_DIALOG(Juser, {Select posting user and mail/write})
XRES_DLGLABEL(username, {User to post output to})
XRES_DLGLABEL(uselect, {Select a user})
XRES_DLGLABEL(mail, {Mail completion result})
XRES_DLGLABEL(write, {Write message of completion result})
XRES_DLGLABEL(mattn, {Mail attention messages})
XRES_DLGLABEL(wattn, {Write attention messages})

XRES_COMMENT(Retention)
XRES_DIALOG(Jret, {Select job retention options})
XRES_DLGLABEL(createdon, {Job created on})
XRES_DLGLABEL(retain, {Retain on queue after printing})
XRES_DLGLABEL(printed, {Job has been printed})
XRES_DLGLABEL(pdeltit, {Hours after which job deleted if printed    })
XRES_DLGLABEL(npdeltit, {Hours after which job deleted if NOT printed})
XRES_DLGLABEL(hold, {Hold job for printing until....})

XRES_COMMENT(Job class codes)
XRES_DIALOG(Jclass, Set job class codes)
XRES_DLGLABEL(loconly, {Print locally only})

XRES_INSERT(Unqueue job)
XRES_DIALOG(Junqueue, Unqueue job)
XRES_DLGLABEL(dselect, {Choose a directory})
XRES_DLGLABEL(copyonly, {Copy out only, no delete})
XRES_DLGLABEL(Cmdfile, {Shell script file name          })
XRES_DLGLABEL(Jobfile, {Job file name - copy of job data})

XRES_SPACE
XRES_STARTDLG(dselb, Select directory to save in)

XRES_COMMENT(Printer setting, form type)
XRES_SELDIALOG(pforms, {Set printer form type....}, {Possible form types....}, {Set to....})
XRES_SPACE

XRES_DIALOG(Pdev, {Set device name/description})
XRES_DLGLABEL(ptrnametitle, {Printer in question})
XRES_DLGLABEL(Dev, {Device})
XRES_DLGLABEL(Descr, {Description})
XRES_DLGLABEL(netfilt, {Network device})
XRES_DLGLABEL(dselect, {Select a device})

XRES_COMMENT(Printer class codes)
XRES_DIALOG(Pclass, {Set printer class codes})
XRES_DLGLABEL(ptrnametitle, {Printer to edit})
XRES_DLGLABEL(loconly, {Local jobs only})
XRES_DLGLABEL(min, {Minimum job size})
XRES_DLGLABEL(max, {Maximum job size})
XRES_SPACE

XRES_DIALOG(Padd, {Add new printer})
XRES_DLGLABEL(newprinter, {Adding new printer......})
XRES_DLGLABEL(ptrname, {Printer})
XRES_DLGLABEL(Dev, {Device})
XRES_DLGLABEL(Form, {Form  })
XRES_DLGLABEL(pselect, {Select a name})
XRES_DLGLABEL(netfilt, {Use network filter})
XRES_DLGLABEL(dselect, {Select device})
XRES_DLGLABEL(fselect, {Select form})
XRES_DLGLABEL(loconly, {Print local jobs only})

XRES_COMMENT(Search stuff)
XRES_DIALOG(Search, {Search for text})
XRES_DLGLABEL(lookfor, {Searching for...})
XRES_DLGLABEL(jobs, {Jobs})
XRES_DLGLABEL(ptrs, {Printers})
XRES_DLGLABEL(title, {Job title})
XRES_DLGLABEL(form, {Form type})
XRES_DLGLABEL(ptr, {Printer name})
XRES_DLGLABEL(user, {User (exact match)})
XRES_DLGLABEL(dev, {Printer device (exact match)})
XRES_DLGLABEL(forward, {Search forwards})
XRES_DLGLABEL(backward, {Search backwards})
XRES_DLGLABEL(match, {Match case})
XRES_DLGLABEL(wrap, {Wrap around})

XRES_SPACE
XRES_DIALOG(about, {About xmspq.....})

XRES_COMMENT(Titles for view / view error)
XRES_SUBAPP(xmspqview, {Display job....})
XRES_HEIGHT(vscroll, 300)
XRES_SPACE

XRES_MENU(viewmenu)
XRES_MENUHDR(Job, Job, J)
XRES_MENUHDR(Srch, Search, S)
XRES_MENUHDR(Page, Page range, P)
XRES_MENUHDR(Help, Help, H)
XRES_SPACE
TAB_SET(48)
XRES_MENUITEM(Update, {Update page range and exit}, U, Q)
XRES_MENUITEM(Exit, {Exit without update}, E, q)
XRES_SPACE

XRES_MENUITEM(Search, {Search for...}, S)
XRES_MENUITEM(Searchforw, {Search forward}, f, F3)
XRES_MENUITEM(Searchback, {Search backward}, b, F4)
XRES_SPACE

XRES_MENUITEM(Setstart, Start page to current, S, s)
XRES_MENUITEM(Setend, End page to current, E, e)
XRES_MENUITEM(Sethat, Halted page to current, H, H)
XRES_MENUITEM(Reset, Reset pages to full range, R, R)
XRES_SPACE

XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUEND
XRES_SPACE
XRES_ENDSUBAPP

TAB_SET(32)
XRES_SUBAPP(xmspqverr, {Display system error log...})

XRES_HEIGHT(vscroll, 200)
XRES_MENUSTART(viewmenu)
XRES_SPACE

TAB_SET(48)
XRES_MENUHDR(File, File, F)
XRES_MENUHDR(Help, Help, H)
XRES_SPACE

XRES_MENUITEM(Clearlog, Clear log file, C)
XRES_MENUITEM(Exit, Exit, E, q)
XRES_SPACE

XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)

XRES_ENDSUBAPP
XRES_END
