
!! *******START OF RESOURCES for xmspq
xmspq.title:			XMSPQ - GNUSpool Queue Viewer

xmspq.toolTipEnable:		True
xmspq.toolbarPresent:		True
xmspq.jtitlePresent:		True
xmspq.ptitlePresent:		True
xmspq.footerPresent:		True
xmspq.keepJobScroll:		False
xmspq.noPageCount:		False
xmspq.localOnly:		False
xmspq.unPrintedOnly:		0
xmspq.confirmAbort:		Unprinted
xmspq.repeatTime:		500
xmspq.repeatInt:		20

xmspq*leftOffset:		10
xmspq*rightOffset:		10
xmspq*topOffset:		5
xmspq*bottomOffset:		5

xmspq*footer.foreground:	navyblue
xmspq*footer.background:	#FEFCD6
xmspq*footer.labelString:	XMSPQ Copyright Free Software Foundation INC 2008

xmspq*layout.foreground:	red
xmspq*layout.background:	white

xmspq*jtitle.foreground:	#800000
xmspq*jtitle.background:	white
xmspq*jtitle.alignment:		alignment_beginning
xmspq*jlist.foreground:		black
xmspq*jlist.background:		#FAFAFF
xmspq*jlist.visibleItemCount:	12
xmspq*jlist.translations:	#override\
				<Btn3Down>:	do-jobpop()\n
xmspq*jobpopup*foreground:	navyblue
xmspq*jobpopup*background:	#FEFCD6
xmspq*jobpopup*borderColor:	#FEFCD6
xmspq*jobpopup*Abortj.labelString:	Abort/delete job
xmspq*jobpopup*Onemore.labelString:	One more copy
xmspq*jobpopup*Formj.labelString:	Set job form
xmspq*jobpopup*View.labelString:	View job text
xmspq*jobpopup*Unqueue.labelString:	Unqueue job
xmspq*ptitle.foreground:		#800000
xmspq*ptitle.background:		white
xmspq*ptitle.alignment:			alignment_beginning
xmspq*plist.foreground:			black
xmspq*plist.background:			#FAFAFF
xmspq*plist.visibleItemCount:		4
xmspq*plist.translations:		#override\
				<Btn3Down>:	do-ptrpop()\n
xmspq*ptrpopup*foreground:		navyblue
xmspq*ptrpopup*background:		#FEFCD6
xmspq*ptrpopup*borderColor:		#FEFCD6
xmspq*ptrpopup*Go.labelString:		Start printer
xmspq*ptrpopup*Interrupt.labelString:	Interrupt printer
xmspq*ptrpopup*Restart.labelString:	Restart job on printer
xmspq*ptrpopup*Abortp.labelString:	Abort job on printer
xmspq*ptrpopup*Heoj.labelString:	Halt at end of job
xmspq*ptrpopup*Halt.labelString:	Halt at once
xmspq*ptrpopup*Formp.labelString:	Set form type
xmspq*ptrpopup*Ok.labelString:		Alignment OK
xmspq*ptrpopup*Nok.labelString:		Rerun Alignment

xmspq*menubar*foreground:		navyblue
xmspq*menubar*background:		#FEFCD6
xmspq*menubar*borderColor:		#FEFCD6

xmspq*menubar.Options.labelString:	Options
xmspq*menubar.Options.mnemonic:		O
xmspq*menubar.Action.labelString:	Action
xmspq*menubar.Action.mnemonic:		A
xmspq*menubar.Jobs.labelString:		Jobs
xmspq*menubar.Jobs.mnemonic:		J
xmspq*menubar.Printers.labelString:	Printers
xmspq*menubar.Printers.mnemonic:	P
xmspq*menubar.Search.labelString:	Search
xmspq*menubar.Search.mnemonic:		S
xmspq*menubar.Help.labelString:		Help
xmspq*menubar.Help.mnemonic:		H
xmspq*menubar.jobmacro.labelString:	Jobmacro
xmspq*menubar.jobmacro.mnemonic:	b
xmspq*menubar.ptrmacro.labelString:	Ptrmacro
xmspq*menubar.ptrmacro.mnemonic:	r

!! Pulldowns for "options"
xmspq*menubar*Viewopts.labelString:	View options
xmspq*menubar*Viewopts.mnemonic:	V
xmspq*menubar*Viewopts.accelerator:	<Key>equal
xmspq*menubar*Viewopts.acceleratorText:	=
xmspq*menubar*Saveopts.labelString:	Save options
xmspq*menubar*Saveopts.mnemonic:	S
xmspq*menubar*Syserror.labelString:	Display Error log
xmspq*menubar*Syserror.mnemonic:	E
xmspq*menubar*Quit.labelString:		Quit
xmspq*menubar*Quit.mnemonic:		Q
xmspq*menubar*Quit.accelerator:		Ctrl<Key>C
xmspq*menubar*Quit.acceleratorText:	Ctrl+C

!! Pulldowns for "action"
xmspq*menubar*Abortj.labelString:	Abort/Delete job
xmspq*menubar*Abortj.mnemonic:		A
xmspq*menubar*Abortj.accelerator:	<Key>A
xmspq*menubar*Abortj.acceleratorText:	a
xmspq*menubar*Onemore.labelString:	One more copy
xmspq*menubar*Onemore.mnemonic:		c
xmspq*menubar*Onemore.accelerator:	<Key>plus
xmspq*menubar*Onemore.acceleratorText:	+
xmspq*menubar*Go.labelString:		Start printer
xmspq*menubar*Go.mnemonic:		S
xmspq*menubar*Go.accelerator:		Shift<Key>G
xmspq*menubar*Go.acceleratorText:	Shift+G
xmspq*menubar*Heoj.labelString:		Halt printer at eoj
xmspq*menubar*Heoj.mnemonic:		H
xmspq*menubar*Heoj.accelerator:		<Key>H
xmspq*menubar*Heoj.acceleratorText:	h
xmspq*menubar*Halt.labelString:		Kill printer
xmspq*menubar*Halt.mnemonic:		K
xmspq*menubar*Halt.accelerator:		Shift<Key>H
xmspq*menubar*Halt.acceleratorText:	Shift+H
xmspq*menubar*Ok.labelString:		Yes - ok align
xmspq*menubar*Ok.mnemonic:		Y
xmspq*menubar*Ok.accelerator:		<Key>Y
xmspq*menubar*Ok.acceleratorText:	y
xmspq*menubar*Nok.labelString:		No - redo align
xmspq*menubar*Nok.mnemonic:		N
xmspq*menubar*Nok.accelerator:		<Key>N
xmspq*menubar*Nok.acceleratorText:	n

!! Pulldowns for "jobs"
xmspq*menubar*View.labelString:		View job
xmspq*menubar*View.mnemonic:		V
xmspq*menubar*View.accelerator:		Shift<Key>I
xmspq*menubar*View.acceleratorText:	Shift+I
xmspq*menubar*Formj.labelString:	Form title pri cps
xmspq*menubar*Formj.mnemonic:		F
xmspq*menubar*Formj.accelerator:	<Key>F
xmspq*menubar*Formj.acceleratorText:	f
xmspq*menubar*Pages.labelString:	Job pages
xmspq*menubar*Pages.mnemonic:		p
xmspq*menubar*User.labelString:		Job user and mail
xmspq*menubar*User.mnemonic:		u
xmspq*menubar*Retain.labelString:	Retention options
xmspq*menubar*Retain.mnemonic:		R
xmspq*menubar*Classj.labelString:	Class codes for job
xmspq*menubar*Classj.mnemonic:		c
xmspq*menubar*Unqueue.labelString:	Unqueue job
xmspq*menubar*Unqueue.mnemonic:		U

!! Pulldowns for "printers"
xmspq*menubar*Interrupt.labelString:		Interrupt printer
xmspq*menubar*Interrupt.mnemonic:		I
xmspq*menubar*Interrupt.accelerator:		Shift<Key>1
xmspq*menubar*Interrupt.acceleratorText:	!
xmspq*menubar*Abortp.labelString:		Abort printer
xmspq*menubar*Abortp.mnemonic:			A
xmspq*menubar*Restart.labelString:		Restart printer
xmspq*menubar*Restart.mnemonic:			R
xmspq*menubar*Formp.labelString:		Set printer form
xmspq*menubar*Formp.mnemonic:			f
xmspq*menubar*Devicep.labelString:		Set device name
xmspq*menubar*Devicep.mnemonic:			v
xmspq*menubar*Classp.labelString:		Set printer class code
xmspq*menubar*Classp.mnemonic:			c
xmspq*menubar*Add.labelString:			Add new printer
xmspq*menubar*Add.mnemonic:			n
xmspq*menubar*Delete.labelString:		Delete printer
xmspq*menubar*Delete.mnemonic:			D

!! Pulldowns for "search"
xmspq*menubar*Search.labelString:		Search for...
xmspq*menubar*Search.mnemonic:			S
xmspq*menubar*Searchforw.labelString:		Search forward
xmspq*menubar*Searchforw.mnemonic:		f
xmspq*menubar*Searchforw.accelerator:		<Key>F3
xmspq*menubar*Searchforw.acceleratorText:	F3
xmspq*menubar*Searchback.labelString:		Search backward
xmspq*menubar*Searchback.mnemonic:		b
xmspq*menubar*Searchback.accelerator:		<Key>F4
xmspq*menubar*Searchback.acceleratorText:	F4

xmspq*jobmacro*macro0.labelString:	Run job command macro
xmspq*jobmacro*macro1.labelString:	Macro command 1
xmspq*jobmacro*macro2.labelString:	Macro command 2
xmspq*jobmacro*macro3.labelString:	Macro command 3
xmspq*jobmacro*macro4.labelString:	Macro command 4
xmspq*jobmacro*macro5.labelString:	Macro command 5
xmspq*jobmacro*macro6.labelString:	Macro command 6
xmspq*jobmacro*macro7.labelString:	Macro command 7
xmspq*jobmacro*macro8.labelString:	Macro command 8
xmspq*jobmacro*macro9.labelString:	Macro command 9

xmspq*ptrmacro*macro0.labelString:	Run printer command macro
xmspq*ptrmacro*macro1.labelString:	Macro command 1
xmspq*ptrmacro*macro2.labelString:	Macro command 2
xmspq*ptrmacro*macro3.labelString:	Macro command 3
xmspq*ptrmacro*macro4.labelString:	Macro command 4
xmspq*ptrmacro*macro5.labelString:	Macro command 5
xmspq*ptrmacro*macro6.labelString:	Macro command 6
xmspq*ptrmacro*macro7.labelString:	Macro command 7
xmspq*ptrmacro*macro8.labelString:	Macro command 8
xmspq*ptrmacro*macro9.labelString:	Macro command 9

!! Pulldowns for "help"
xmspq*menubar*Help.labelString:		Help
xmspq*menubar*Help.mnemonic:		H
xmspq*menubar*Help.accelerator:		<Key>F1
xmspq*menubar*Help.acceleratorText:	F1
xmspq*menubar*Helpon.labelString:	Help on
xmspq*menubar*Helpon.mnemonic:		o
xmspq*menubar*Helpon.accelerator:	Shift<Key>F1
xmspq*menubar*Helpon.acceleratorText:	Shift+F1
xmspq*menubar*About.labelString:	About
xmspq*menubar*About.mnemonic:		A

!! Toolbar buttons
xmspq*toolbar*foreground:		navyblue
xmspq*toolbar*background:		#FEFCD6
xmspq*toolbar.paneMinimum:		30
xmspq*toolbar.paneMaximum:		30
xmspq*toolbar*Abortjt.labelString:	Abort/Del
xmspq*toolbar*Abortjt.toolTipString:	Delete this job - abort if being printed
xmspq*toolbar*Onemore.labelString:	+1
xmspq*toolbar*Onemore.toolTipString:	Increment the number of copies by one (reprint)
xmspq*toolbar*Form.labelString:		Job Form
xmspq*toolbar*Form.toolTipString:	Change the form type or printer for this job
xmspq*toolbar*View.labelString:		View job
xmspq*toolbar*View.toolTipString:	View the text of the current job
xmspq*toolbar*Pform.labelString:	Ptr Form
xmspq*toolbar*Pform.toolTipString:	Change the form type on the printer
xmspq*toolbar*Go.labelString:		Go Ptr
xmspq*toolbar*Go.toolTipString:		Start the selected printer
xmspq*toolbar*Heoj.labelString:		Halt EOJ
xmspq*toolbar*Heoj.toolTipString:	Halt the printer at the end of the current job
xmspq*toolbar*Halt.labelString:		Stop now
xmspq*toolbar*Halt.toolTipString:	Halt the printer immediately - aborting the current print
xmspq*toolbar*Ok.labelString:		OK align
xmspq*toolbar*Ok.toolTipString:		Accept alignment for the currently-selected printer
xmspq*toolbar*Nok.labelString:		Not OK
xmspq*toolbar*Nok.toolTipString:	Reject alignment for the currently-selected printer

!! Dialog titles etc for help/error/info/confirm
xmspq*help.dialogTitle:			On line help.....
xmspq*help*foreground:			navyblue
xmspq*help*background:			#FEFCD6
xmspq*error.dialogTitle:		Whoops!!!
xmspq*error*foreground:			white
xmspq*error*background:			#FF6464
xmspq*Confirm.dialogTitle:		Are you sure?
xmspq*Confirm*foreground:		black
xmspq*Confirm*background:		limegreen

!! These are used in various places
xmspq*Setall.labelString:		Set all codes
xmspq*Clearall.labelString:		Clear all codes
xmspq*jobnotitle.labelString:		Editing job number:

!! View options dialog
xmspq*Viewopts.dialogTitle:		Display options
xmspq*Viewopts*foreground:		navyblue
xmspq*Viewopts*background:		#FEFCD6
xmspq*Viewopts*viewtitle.labelString:	Selecting view options
xmspq*Viewopts*username.labelString:	Restrict jobs display to user
xmspq*Viewopts*uselect.labelString:	Select...
xmspq*Viewopts*ptrname.labelString:	Restrict display to printer
xmspq*Viewopts*pselect.labelString:	Select...
xmspq*Viewopts*tittitle.labelString:	Restrict display by titles
xmspq*Viewopts*allhosts.labelString:	View all hosts
xmspq*Viewopts*localonly.labelString:	View local host only
xmspq*Viewopts*confb1.labelString:	Do not confirm delete/abort jobs
xmspq*Viewopts*confb2.labelString:	Confirm delete/abort if not printed
xmspq*Viewopts*confb3.labelString:	Always confirm delete/abort
xmspq*Viewopts*restrp1.labelString:	All jobs
xmspq*Viewopts*restrp2.labelString:	Unprinted jobs
xmspq*Viewopts*restrp3.labelString:	Printed jobs
xmspq*Viewopts*jincl1.labelString:	Jobs for matching printer
xmspq*Viewopts*jincl2.labelString:	Jobs for printer plus null
xmspq*Viewopts*jincl3.labelString:	All jobs disregarding printer
xmspq*Viewopts*sortptrs.labelString:	Sort printer names
xmspq*Viewopts*jdispfmt.labelString:	Reset Job Display Fields
xmspq*Viewopts*pdispfmt.labelString:	Reset Printer Display Fields
xmspq*Viewopts*savehome.labelString:	Save formats in home directory
xmspq*Viewopts*savecurr.labelString:	Save formats in current directory
xmspq*Viewopts*Ok.labelString:		Apply

!! User select dialog from view options
xmspq*uselect.dialogTitle:		Select a user
xmspq*uselect*foreground:		navyblue
xmspq*uselect*background:		#FEFCD6
xmspq*uselect.listLabelString:		Possible users....
xmspq*uselect.selectionLabelString:	Set to....

!! Printer select dialog from view options
xmspq*pselect.dialogTitle:		Select a printer
xmspq*pselect*foreground:		navyblue
xmspq*pselect*background:		#FEFCD6
xmspq*pselect.listLabelString:		Possible printers....
xmspq*pselect.selectionLabelString:	Set to....

!! Job display fields
xmspq*Jdisp.dialogTitle:		Reset Job Display Fields
xmspq*Jdisp*foreground:			navyblue
xmspq*Jdisp*background:			#FEFCD6
xmspq*Jdisp*Newfld.labelString:		New field
xmspq*Jdisp*Newsep.labelString:		New separator
xmspq*Jdisp*Editfld.labelString:	Edit field
xmspq*Jdisp*Editsep.labelString:	Edit separator
xmspq*Jdisp*Delete.labelString:		Delete field/sep
xmspq*Jdisp*Jdisplist.visibleItemCount:	16

xmspq*jfldedit.dialogTitle:		Job display field
xmspq*jfldedit*foreground:		navyblue
xmspq*jfldedit*background:		#FEFCD6
xmspq*jfldedit*width.labelString:	Width of field
xmspq*jfldedit*useleft.labelString:	Use previous field if too large

xmspq*Pdisp.dialogTitle:		Reset Printer Display Fields
xmspq*Pdisp*foreground:			navyblue
xmspq*Pdisp*background:			#FEFCD6
xmspq*Pdisp*Newfld.labelString:		New field
xmspq*Pdisp*Newsep.labelString:		New separator
xmspq*Pdisp*Editfld.labelString:	Edit field
xmspq*Pdisp*Editsep.labelString:	Edit separator
xmspq*Pdisp*Delete.labelString:		Delete field/sep
xmspq*Pdisp*Pdisplist.visibleItemCount:	8

xmspq*pfldedit.dialogTitle:		Printer display field
xmspq*pfldedit*foreground:		navyblue
xmspq*pfldedit*background:		#FEFCD6
xmspq*pfldedit*width.labelString:	Width of field
xmspq*pfldedit*useleft.labelString:	Use previous field if too large
xmspq*pfldedit*skipright.labelString:	Delete following fields if too large

xmspq*msave.dialogTitle:		Select file to save in
xmspq*msave*foreground:			navyblue
xmspq*msave*background:			#FEFCD6

!! Job form dialog
xmspq*Jform.dialogTitle:		Edit form, header, priority, copies
xmspq*Jform*foreground:			navyblue
xmspq*Jform*background:			#FEFCD6
xmspq*Jform*form.labelString:		Form    
xmspq*Jform*fselect.labelString:	Select forms
xmspq*Jform*Header.labelString:		Title   
xmspq*Jform*supph.labelString:		Suppress header page
xmspq*Jform*ptrname.labelString:	Printer 
xmspq*Jform*pselect.labelString:	Select printers
xmspq*Jform*Priority.labelString:	Priority
xmspq*Jform*Copies.labelString:		Copies  

!! Form select dialog from job options
xmspq*fselect.dialogTitle:		Select a form
xmspq*fselect*foreground:		navyblue
xmspq*fselect*background:		#FEFCD6
xmspq*fselect.listLabelString:		Possible forms....
xmspq*fselect.selectionLabelString:	Set form to....

!! Page select dialog from job options
xmspq*Jpage.dialogTitle:		Select pages
xmspq*Jpage*foreground:			navyblue
xmspq*Jpage*background:			#FEFCD6
xmspq*Jpage*Spage.labelString:		Starting page
xmspq*Jpage*Epage.labelString:		End page
xmspq*Jpage*Hpage.labelString:		"Halted at" page
xmspq*Jpage*allpages.labelString:	Print all pages
xmspq*Jpage*oddpages.labelString:	Print odd pages
xmspq*Jpage*evenpages.labelString:	Print even pages
xmspq*Jpage*swapoe.labelString:		Swap printing odd/even pages
xmspq*Jpage*Flags.labelString:		Post/proc flags

!! Mail/write options
xmspq*Juser.dialogTitle:		Select posting user and mail/write
xmspq*Juser*foreground:			navyblue
xmspq*Juser*background:			#FEFCD6
xmspq*Juser*username.labelString:	User to post output to
xmspq*Juser*uselect.labelString:	Select a user
xmspq*Juser*mail.labelString:		Mail completion result
xmspq*Juser*write.labelString:		Write message of completion result
xmspq*Juser*mattn.labelString:		Mail attention messages
xmspq*Juser*wattn.labelString:		Write attention messages

!! Retention
xmspq*Jret.dialogTitle:			Select job retention options
xmspq*Jret*foreground:			navyblue
xmspq*Jret*background:			#FEFCD6
xmspq*Jret*createdon.labelString:	Job created on
xmspq*Jret*retain.labelString:		Retain on queue after printing
xmspq*Jret*printed.labelString:		Job has been printed
xmspq*Jret*pdeltit.labelString:		Hours after which job deleted if printed    
xmspq*Jret*npdeltit.labelString:	Hours after which job deleted if NOT printed
xmspq*Jret*hold.labelString:		Hold job for printing until....

!! Job class codes
xmspq*Jclass.dialogTitle:		Set job class codes
xmspq*Jclass*foreground:		navyblue
xmspq*Jclass*background:		#FEFCD6
xmspq*Jclass*loconly.labelString:	Print locally only
Unqueue jobxmspq*Junqueue.dialogTitle:		Unqueue job
xmspq*Junqueue*foreground:		navyblue
xmspq*Junqueue*background:		#FEFCD6
xmspq*Junqueue*dselect.labelString:	Choose a directory
xmspq*Junqueue*copyonly.labelString:	Copy out only, no delete
xmspq*Junqueue*Cmdfile.labelString:	Shell script file name          
xmspq*Junqueue*Jobfile.labelString:	Job file name - copy of job data

xmspq*dselb.dialogTitle:		Select directory to save in

!! Printer setting
xmspq*pforms.dialogTitle:		Set printer form type....
xmspq*pforms*foreground:		navyblue
xmspq*pforms*background:		#FEFCD6
xmspq*pforms.listLabelString:		Possible form types....
xmspq*pforms.selectionLabelString:	Set to....

xmspq*Pdev.dialogTitle:			Set device name/description
xmspq*Pdev*foreground:			navyblue
xmspq*Pdev*background:			#FEFCD6
xmspq*Pdev*ptrnametitle.labelString:	Printer in question
xmspq*Pdev*Dev.labelString:		Device
xmspq*Pdev*Descr.labelString:		Description
xmspq*Pdev*netfilt.labelString:		Network device
xmspq*Pdev*dselect.labelString:		Select a device

!! Printer class codes
xmspq*Pclass.dialogTitle:		Set printer class codes
xmspq*Pclass*foreground:		navyblue
xmspq*Pclass*background:		#FEFCD6
xmspq*Pclass*ptrnametitle.labelString:	Printer to edit
xmspq*Pclass*loconly.labelString:	Local jobs only
xmspq*Pclass*min.labelString:		Minimum job size
xmspq*Pclass*max.labelString:		Maximum job size

xmspq*Padd.dialogTitle:			Add new printer
xmspq*Padd*foreground:			navyblue
xmspq*Padd*background:			#FEFCD6
xmspq*Padd*newprinter.labelString:	Adding new printer......
xmspq*Padd*ptrname.labelString:		Printer
xmspq*Padd*Dev.labelString:		Device
xmspq*Padd*Form.labelString:		Form  
xmspq*Padd*pselect.labelString:		Select a name
xmspq*Padd*netfilt.labelString:		Use network filter
xmspq*Padd*dselect.labelString:		Select device
xmspq*Padd*fselect.labelString:		Select form
xmspq*Padd*loconly.labelString:		Print local jobs only

!! Search stuff
xmspq*Search.dialogTitle:		Search for text
xmspq*Search*foreground:		navyblue
xmspq*Search*background:		#FEFCD6
xmspq*Search*lookfor.labelString:	Searching for...
xmspq*Search*jobs.labelString:		Jobs
xmspq*Search*ptrs.labelString:		Printers
xmspq*Search*title.labelString:		Job title
xmspq*Search*form.labelString:		Form type
xmspq*Search*ptr.labelString:		Printer name
xmspq*Search*user.labelString:		User (exact match)
xmspq*Search*dev.labelString:		Printer device (exact match)
xmspq*Search*forward.labelString:	Search forwards
xmspq*Search*backward.labelString:	Search backwards
xmspq*Search*match.labelString:		Match case
xmspq*Search*wrap.labelString:		Wrap around

xmspq*about.dialogTitle:		About xmspq.....
xmspq*about*foreground:			navyblue
xmspq*about*background:			#FEFCD6

!! Titles for view / view error
xmspqview.title:			Display job....

xmspqview*vscroll.height:		300

xmspqview*viewmenu*foreground:		navyblue
xmspqview*viewmenu*background:		#FEFCD6
xmspqview*viewmenu*borderColor:		#FEFCD6
xmspqview*viewmenu.Job.labelString:	Job
xmspqview*viewmenu.Job.mnemonic:	J
xmspqview*viewmenu.Srch.labelString:	Search
xmspqview*viewmenu.Srch.mnemonic:	S
xmspqview*viewmenu.Page.labelString:	Page range
xmspqview*viewmenu.Page.mnemonic:	P
xmspqview*viewmenu.Help.labelString:	Help
xmspqview*viewmenu.Help.mnemonic:	H

xmspqview*viewmenu*Update.labelString:		Update page range and exit
xmspqview*viewmenu*Update.mnemonic:		U
xmspqview*viewmenu*Update.accelerator:		Shift<Key>Q
xmspqview*viewmenu*Update.acceleratorText:	Shift+Q
xmspqview*viewmenu*Exit.labelString:		Exit without update
xmspqview*viewmenu*Exit.mnemonic:		E
xmspqview*viewmenu*Exit.accelerator:		<Key>Q
xmspqview*viewmenu*Exit.acceleratorText:	q

xmspqview*viewmenu*Search.labelString:		Search for...
xmspqview*viewmenu*Search.mnemonic:		S
xmspqview*viewmenu*Searchforw.labelString:	Search forward
xmspqview*viewmenu*Searchforw.mnemonic:		f
xmspqview*viewmenu*Searchforw.accelerator:	<Key>F3
xmspqview*viewmenu*Searchforw.acceleratorText:	F3
xmspqview*viewmenu*Searchback.labelString:	Search backward
xmspqview*viewmenu*Searchback.mnemonic:		b
xmspqview*viewmenu*Searchback.accelerator:	<Key>F4
xmspqview*viewmenu*Searchback.acceleratorText:	F4

xmspqview*viewmenu*Setstart.labelString:	Start page to current
xmspqview*viewmenu*Setstart.mnemonic:		S
xmspqview*viewmenu*Setstart.accelerator:	<Key>S
xmspqview*viewmenu*Setstart.acceleratorText:	s
xmspqview*viewmenu*Setend.labelString:		End page to current
xmspqview*viewmenu*Setend.mnemonic:		E
xmspqview*viewmenu*Setend.accelerator:		<Key>E
xmspqview*viewmenu*Setend.acceleratorText:	e
xmspqview*viewmenu*Sethat.labelString:		Halted page to current
xmspqview*viewmenu*Sethat.mnemonic:		H
xmspqview*viewmenu*Sethat.accelerator:		Shift<Key>H
xmspqview*viewmenu*Sethat.acceleratorText:	Shift+H
xmspqview*viewmenu*Reset.labelString:		Reset pages to full range
xmspqview*viewmenu*Reset.mnemonic:		R
xmspqview*viewmenu*Reset.accelerator:		Shift<Key>R
xmspqview*viewmenu*Reset.acceleratorText:	Shift+R

xmspqview*viewmenu*Help.labelString:		Help
xmspqview*viewmenu*Help.mnemonic:		H
xmspqview*viewmenu*Help.accelerator:		<Key>F1
xmspqview*viewmenu*Help.acceleratorText:	F1
xmspqview*viewmenu*Helpon.labelString:		Help on
xmspqview*viewmenu*Helpon.mnemonic:		o
xmspqview*viewmenu*Helpon.accelerator:		Shift<Key>F1
xmspqview*viewmenu*Helpon.acceleratorText:	Shift+F1

xmspqverr.title:		Display system error log...

xmspqverr*vscroll.height:	200

xmspqverr*viewmenu.File.labelString:		File
xmspqverr*viewmenu.File.mnemonic:		F
xmspqverr*viewmenu.Help.labelString:		Help
xmspqverr*viewmenu.Help.mnemonic:		H

xmspqverr*viewmenu*Clearlog.labelString:	Clear log file
xmspqverr*viewmenu*Clearlog.mnemonic:		C
xmspqverr*viewmenu*Exit.labelString:		Exit
xmspqverr*viewmenu*Exit.mnemonic:		E
xmspqverr*viewmenu*Exit.accelerator:		<Key>Q
xmspqverr*viewmenu*Exit.acceleratorText:	q

xmspqverr*viewmenu*Help.labelString:		Help
xmspqverr*viewmenu*Help.mnemonic:		H
xmspqverr*viewmenu*Help.accelerator:		<Key>F1
xmspqverr*viewmenu*Help.acceleratorText:	F1
xmspqverr*viewmenu*Helpon.labelString:		Help on
xmspqverr*viewmenu*Helpon.mnemonic:		o
xmspqverr*viewmenu*Helpon.accelerator:		Shift<Key>F1
xmspqverr*viewmenu*Helpon.acceleratorText:	Shift+F1


!! *******END OF RESOURCES for xmspq

