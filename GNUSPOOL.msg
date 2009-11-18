#
# File for "helpparse" to generate message files from
#
#   Copyright 2008 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

######################################################################
######################################################################
##  Macros for library modules

DEFINE	LIB1 (lib) :
		calccharge.c extdefs.c getspuser.c hex_disp.c hextoi.c insertu.c
		jobshm.c look_host.c net_feed.c ptrshm.c rdpgfile.c
		readu.c produser.c;

DEFINE	INLINE_OPTS (inline) :
		o_allj.c	o_boxes.c	o_classc.c	o_copies.c	o_delim.c
		o_delimnum.c	o_dloco.c	o_dpage.c	o_dtime.c	o_extern.c
		o_flags.c       o_format.c      o_formtype.c    o_freeze.c	o_hdrs.c
		o_header.c      o_interp.c      o_jinclall.c    o_jobwait.c	o_justq.c
		o_justu.c	o_loco.c	o_mailwrt.c	o_oddeven.c	o_printer.c
		o_priority.c    o_qhost.c	o_range.c	o_retn.c        o_setclass.c
		o_tdelay.c	o_timeout.c     o_user.c	o_usort.c	o_verbose.c;

DEFINE	INLINE_QOPTS (inline) :
		o_boxes.c	o_dloco.c	o_dpage.c	o_justq.c	o_justu.c
		o_allj.c	o_classc.c	o_jinclall.c	o_psort.c;

DEFINE	INLINE_SPR (inline) :
		o_spr_expl.c    spr_adefs.c	spr_dumpo.c	spr_oproc.c	freezecode.c;

DEFINE	INLINE_JFMT (inline) :
		jfmt_class.c	jfmt_cps.c	jfmt_form.c	jfmt_hold.c	jfmt_jobno.c
		jfmt_krchd.c	jfmt_loco.c	jfmt_mail.c	jfmt_mattn.c	jfmt_nptim.c
		jfmt_oddev.c	jfmt_origh.c	jfmt_pgrch.c	jfmt_ppf.c	jfmt_prio.c
		jfmt_ptime.c	jfmt_ptr.c	jfmt_puser.c	jfmt_range.c	jfmt_retn.c
		jfmt_seq.c      jfmt_sizek.c    jfmt_stime.c	jfmt_supph.c	jfmt_szpgs.c
		jfmt_title.c 	jfmt_user.c     jfmt_wattn.c	jfmt_write.c;

DEFINE	INLINE_PFMT (inline) :
		pfmt_ab.c	pfmt_class.c	pfmt_dev.c	pfmt_form.c	pfmt_heoj.c
		pfmt_jobno.c	pfmt_limit.c	pfmt_loco.c     pfmt_maxsz.c	pfmt_minsz.c
		pfmt_msg.c	pfmt_na.c       pfmt_ostat.c    pfmt_pid.c	pfmt_ptr.c
		pfmt_shrk.c	pfmt_state.c  	pfmt_user.c;

DEFINE	LIB2 (lib) :
		disperror.c doenv.c doopts.c gprompt.c help_readl.c
		helpargs.c helphdr.c helpnstate.c helpprmpt.c helprdn.c
		helpvec.c makeovec.c makevec.c mmangle.c saveopts.c
		op_icf.c open_cfile.c optprocess.c rdoptfile.c;

DEFINE	LIB3 (lib) :
		envprocess.c ncstrncmp.c prin_gname.c prin_size.c prin_uname.c qmatch.c
		runpwd.c spath.c stracpy.c strread.c mkspid.c matchrouts.c issubset.c sortp.c;

DEFINE	LIB4 (lib) :
		getkey.c make_km.c setkey.c dohe.c whdrstr.c wgets.c wnum.c whexnum.c;

DEFINE	LIB5 (lib) :
		xfershm.c;

DEFINE	LIBSHLL (lib) :
		cgifndjb.c shutilmsg.c;

######################################################################
######################################################################
##  Which programs helps are used by

int-config (helpmsg) :
		dosspwrite spd spdinit spmdisp spexec spshed spwrite xtnetserv;

rest.help (helpmsg) :
		rspr spcharge spjobdump splist spr spstart spuchange spulist sqchange
		sqdel sqlist sqcgi sqvcgi sqdcgi sqccgi sqcrcgi spcgi spccgi sstop lpstatc mspr
		rsqcgi rsqccgi rsqcrcgi rsqdcgi rsqvcgi rspcgi rspccgi;

spq.help (helpmsg) :
		spq;

spuser.help (helpmsg) :
		spuser;

xmspq.help (helpmsg) :
		xmspq xspq;

xmspuser.help  (helpmsg) :
		xmspuser xspuser;

splpd.help (helpmsg) :
		sp.lpq;

######################################################################
######################################################################
##  Which modules make up which programs

MODULES dosspwrite =
	dosspwrite.c LIB2 LIB3 ;

MODULES spd	=
	spd.c sd_bann.c sd_fctrl.c sd_initf.c LIB1 LIB2 LIB3 ;

MODULES spdinit =
	spdinit.c sdi_gram.c LIB2 LIB3 ;

MODULES spexec	=
	spexec.c LIB2 LIB3 ;

MODULES spmdisp =
	spmdisp.c LIB2 LIB3 ;

MODULES spshed	=
	spshed.c sh_queue.c sh_plist.c sh_misc.c sh_oper.c sh_network.c sh_netfeed.c LIB2 LIB3 LIB5;

MODULES spwrite =
	spwrite.c LIB2 ;

MODULES xtnetserv =
	xtnetserv.c xtnet_ua.c xtnet_api.c LIB1 LIB2 LIB3 LIB5;

MODULES rspr =
	rspr.c rsp_net.c LIB1 LIB2 LIB3 INLINE_OPTS INLINE_SPR ;

MODULES spcharge =
	spcharge.c LIB1 LIB2 LIB3 INLINE_OPTS ;

MODULES spjobdump =
	spjobdump.c LIB1 LIB2 LIB3 INLINE_OPTS INLINE_SPR ;

MODULES splist =
	splist.c LIB1 LIB2 LIB3 INLINE_PFMT ;

MODULES spr =
	spr.c LIB1 LIB2 LIB3 INLINE_OPTS INLINE_SPR LIB5;

MODULES mspr =
	mspr.c LIB1 LIB2 LIB3 INLINE_OPTS INLINE_SPR;

MODULES spstart =
	spstart.c LIB1 LIB2 LIB3 INLINE_OPTS LIB5 LIBSHLL;

MODULES spuchange =
	spuchange.c LIB1 LIB2 LIB3 INLINE_OPTS ;

MODULES spulist =
	spulist.c LIB1 LIB2 LIB3 INLINE_OPTS ;

MODULES sqchange =
	sqchange.c LIB1 LIB2 LIB3 INLINE_OPTS LIB5 LIBSHLL;

MODULES sqdel =
	sqdel.c LIB1 LIB2 LIB3 INLINE_OPTS LIBSHLL;

MODULES sqlist =
	sqlist.c LIB1 LIB2 LIB3 INLINE_JFMT LIBSHLL;

MODULES sqcgi (loccgi) =
	sqcgi.c LIB1 LIB2 LIB3 INLINE_JFMT LIBSHLL;

MODULES rsqcgi (remcgi) =
	rsqcgi.c rcgilib.c LIB1 LIB2 LIB3 INLINE_JFMT LIBSHLL;

MODULES sqvcgi (loccgi) =
	sqvcgi.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES rsqvcgi (remcgi) =
	rsqvcgi.c rcgilib.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES sqdcgi (loccgi) =
	sqdcgi.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES rsqdcgi (remcgi) =
	rsqdcgi.c rcgilib.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES sqccgi (loccgi) =
	sqccgi.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES rsqccgi (remcgi) =
	rsqccgi.c rcgilib.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES sqcrcgi (loccgi) =
	sqcrcgi.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES rsqcrcgi (remcgi) =
	rsqcrcgi.c rcgilib.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES spcgi (loccgi) =
	spcgi.c LIB1 LIB2 LIB3 INLINE_PFMT;

MODULES rspcgi (remcgi) =
	rspcgi.c rcgilib.c LIB1 LIB2 LIB3 INLINE_PFMT;

MODULES spccgi (loccgi) =
	spccgi.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES rspccgi (remcgi) =
	rspccgi.c rcgilib.c LIB1 LIB2 LIB3 LIBSHLL;

MODULES sstop =
	sstop.c LIB1 LIB2 LIB3 INLINE_OPTS ;

MODULES spq =
	spq.c sq_jlist.c sq_plist.c sq_view.c qopts.c wtime.c
	prompt.c sq_propts.c LIB1 LIB2 LIB3 LIB4 LIB5 INLINE_JFMT INLINE_PFMT INLINE_QOPTS;

MODULES spuser =
	spuser.c spu_propts.c LIB1 LIB2 LIB3 LIB4 ;

MODULES xmspq (motif) =
	xmspq.c xmsq_cbs.c xmsq_jlist.c xmsq_jcall.c xmsq_plist.c xmsq_pcall.c xmsq_prompt.c xmsq_view.c
	LIB1 LIB2 LIB3 LIB5 INLINE_JFMT INLINE_PFMT;

MODULES xspq (gtk) =
	xspq.c xsq_cbs.c xsq_jplist.c xsq_jcall.c xsq_pcall.c
	 xsq_prompt.c xsq_view.c gtk_lib.c LIB1 LIB2 LIB3 LIB5;

MODULES xmspuser (motif) =
	xmspuser.c xmspu_cbs.c LIB1 LIB2 LIB3 ;

MODULES xspuser (gtk) =
	xspuser.c xspu_cbs.c gtk_lib.c LIB1 LIB2 LIB3 ;

MODULES lpstatc (ins) =
	lpstatc.c LIB1 LIB2 LIB3 ;

MODULES	sp.lpq (lpdint) =
	sp.lpq.c LIB1 LIB2 LIB3 ;

######################################################################
######################################################################
##  Mostly spr messages (important they come early)
##

[Messages and prompts for shell commands
]
E{Unknown post user} = 50
"Unknown user `%s' - ignoring post to this user, and posting to you
(%R) instead."

E{Priority range} ++
"Invalid priority %d1 - possible range is %d2 to %d3
(your account may provide for a smaller range)."

E{Timeout range} ++
"Invalid timeout value %d1 - range is %d2 to %d3"

E{Bad arg copies} ++
"Bad number of copies %d1 - must be between %d2 and %d3."

E{Bad page range} ++
"Bad range option ""%d0 to %d1"". Start page should be less than end."

E{Expecting terminal input} ++
"Warning: Expecting input from terminal."

E{Cannot create pipe} ++
"Cannot create pipe - %E"

E{Cannot fork} ++
"Cannot fork new process - %E"

E{Restarting scheduler} ++	"%P - restarting scheduler process."
E{Fork wait} ++			"Waiting to fork scheduler process (got error %E)"
E{Could not start scheduler} ++	"Could not start scheduler - giving up."
E{No standard input} ++		"No input from standard input - no spool job created"
E{No spool input} ++		"No spool input from %s - no job created"

E{Still could not open} ++
"Still could not open IPC Message queue err is %E
The following is from the system error logfile %s.
Please advise us of the contents:"

E{Created with title} ++	"%s - spool id %d0"
E{Created no title} ++		"Spool id %d0"
E{Invalid priority} ++		"Invalid priority %d0, must lie between %d1 and %d2"
E{Invalid form type} ++		"Invalid form type %s."
E{Form type restriction} ++	"You are only allowed to use form types matching %s."
E{Invalid copies} ++		"Invalid number of copies %d0. You are only allowed to use up to %d1."
E{No default priority} ++	"Sorry - no default priority. You must specify one (-p n)."
E{Priority range excludes} ++	"Sorry - you may not use %P"
E{Invalid ptr type} ++
"Invalid printer %s.
You are only allowed to use printers subset of %t."
E{Invalid time digit} ++
"Invalid 'n' argument '%s' - mm, hh:mm or hh:mm:ss
'n' argument gives time increment before job is run"
E{Time arg inval} ++
"Invalid 'N' argument - mm, hh:mm or hh:mm:ss
'N' argument gives earliest time to run job"
E{Time arg passed} ++		"Time specified in -N %s is not in future"
E{Invalid delim num} ++		"Invalid number of delimiters, must be at least one"
E{Invalid delim arg} ++		"Invalid escape sequence in delimiter arg %s"
E{Cannot create page file} ++	"Trouble creating page file %s - %E"
E{Spool directory full} ++	"Trouble writing output file - %E"
E{Too big warning} ++		"Size of spool file exceeds maximum limit - truncated"
E{Too big aborted} ++		"Size of spool file exceeds maximum limit - aborted"
E{Invalid odd even arg} ++	"Invalid argument to -O, should be E O A or -"
E{Unknown host name} ++		"Unknown host name `%s'"
E{Cannot find rspr} ++
"%P:Cannot locate the program ""r%P"" to run remote queue operation.
Are you sure that this has been installed?"
E{Cannot run rspr} ++
"%P:Cannot run the program ""r%P"" to run remote queue operation.
System error reported was ""%E""
Are you sure that this has been installed correctly?"
E{Send to self} ++
"The name %s you gave to send to is this machine, or another name
for it. There is no point in doing this!"
E{No host name given} ++
"No remote host specified - please use spr for submitting local jobs"

E{remote job bad copies} ++	"Remote rejected job - bad number of copies"
E{remote job bad form} ++	"Remote rejected job - bad form type"
E{remote job bad ptr} ++	"Remote rejected job - bad printer"
E{remote job past limit} ++	"Remote rejected job `%s' - past job size limit"
E{remote job bad page file} ++	"Remote rejected job - bad page delimiter file"
E{remote job bad priority} ++	"Remote rejected job - bad priority"
E{remote job invalid class code} ++	"Remote rejected job - bad (effectively zero) class code"
E{remote job file empty} ++	"Remote rejected job - file apparently empty"
E{remote job cannot create pagefile} ++	"Remote rejected job - cannot create page delimiter file"
E{remote job file full} ++	"Remote rejected job - is its spool directory file system full?"
E{remote job nomem pagefile} ++	"Remote rejected job - no memory for page delimiter table"
E{remote job message queue full} ++	"Remote rejected job - message queue is full"
E{remote job unknown client} ++	"Remote rejected job - unknown client"
E{write failure remote job} ++	"Trouble writing job to remote"
E{remote job file truncated} ++	"Outgoing Remote file truncated???"
E{Cannot bind UDP socket} ++	"Cannot bind UDP socket for remote enquiry"
E{Cannot create UDP socket} ++	"Cannot create UDP socket for remote enquiry"
E{Cannot create TCP socket} ++	"Cannot create tcp socket for remote queue"
E{No TCP protocol} ++		"Cannot determine TCP protocol number"
E{No UDP protocol} ++		"Cannot determine UDP protocol number"
E{No TCP service} ++		"Cannot determine TCP service number - looking for ""%s"""
E{No UDP service} ++		"Cannot determine UDP service number - looking for ""%s"""
E{User invalid at remote} ++	"%s is not a valid user on %t"
E{Cannot send on UDP socket} ++	"Cannot send on UDP socket to %s"
E{Cannot receive on UDP socket} ++	"Cannot receive on UDP socket from %s"
E{Trouble writing socket file} ++	"Cannot write output file to remote"
E{Cannot open print file} ++		"%P - cannot open file %s"
E{remote job lost sync} ++	"Lost TCP synchronisation during send remote job"
E{remote job lost seq} ++	"Lost data sequence during send remote job"
E{remote job truncated} ++	"Job `%s' (%d0) exceeded size limit - truncated"

E{job num not numeric} = ROUND(@, 40)	"Invalid job number `%s'. Job numbers should be given as numerics."
E{Unknown job number} ++		"Unknown job number `%s'."
E{Chngdel not yours} ++		"Job number %s does not belong to you (%R)"
E{No arguments to process} ++	"Sorry no arguments to process after errors."
E{No change form priv} ++
"Sorry but this job is using form type %t and you are not allowed
to change forms to %s"
E{No change ptr priv} ++
"Sorry but this job is using printer %t and you are not allowed
to change it to %s"

E{No change prio priv} ++
"Sorry but you are not allowed to change the priority of a job
once it has been queued."

E{Change prio out of range} ++
"Sorry but you are not allowed to change the priority to %d0.
Your permitted range is between %d1 and %d2"

E{sqchange no args} ++	"You have not specified any jobs to change!"
E{sqchange no ops} ++	"Sorry but you have not said what you want to do with the jobs"
E{sqdel not printed} ++
"The job `%s' has not been printed yet. Please specify the -y
option if you want to delete jobs which have not been printed."

E{sqdel no jobs} ++	"You have not specified any jobs to delete."
E{sqdel no remote job priv} ++	"You are not allowed to access remote jobs - sorry."
E{sqdel cannot unqueue} ++
"Sorry but you do not have ""unqueue"" privilege and therefore
cannot run %P with the -u option."

E{shell no admin file priv} ++
"Sorry but you need to have ""edit admin file"" privilege
to do the operation(s) you require."

E{sqlist no view permission local} ++
"Job %d0 does belongs to %t not you (%R) and you do
not have permission to view it."

E{sqlist no view permission remote} ++
"Job %s:%d0 does belongs to %t not you (%R) and you do
not have permission to view it."

E{sqlist no remote job priv} ++
"Sorry but you don't have permission to access remote jobs
for viewing"

E{sqlist cant view local job} ++
"Sorry but text of job %d0 cannot be fetched"

E{sqlist cant view remote job} ++
"Sorry but text of job %s:%d0 cannot be fetched"

[
Default job and command file prefixes for
unqueue options of sqdel
]

P{sqdel default job prefix} = ROUND(@, 20)	"J"
P{sqdel default cmd prefix} ++			"C"
P{sqdel default job suffix} ++			".xtj"
P{sqdel default cmd suffix} ++			".sh"

[
Spstart messages
]
E{spstart no args given} = ROUND(@, 20)		"%P - no arguments given"
E{spstart unknown printer} ++			"Unknown printer %s"

E,H{No stop start priv} ++
"You (%R) do not have permission to start or stop printers.
Please ask someone else.",
"You do not have permission in your account to start
and stop printers. You must ask someone else to do this."

E{spstart shutting down} ++		"The printer %s is currently being shut down."
E{spstart just started up} ++		"The printer %s has just been started up with a different form type (%t)."
E{spstart no match printer name} ++	"Printers match `%s' but do not also match device type `%t'."
E{No ptr select priv} ++		"You are not permitted to select printers."
E{No line given} ++
"When adding a printer you must specify the device name with
""-l ttynn""."
E{spstart printer is running} ++	"The printer %s is running - stop it before you delete it"
E{No changes given} ++
"You didn't give the changes (new device, new form) you want to
specify for the printer."
E{spstart no add priv} ++
"You (%R) do not have permission to add a new printer.
Please ask someone else."
E{spstart no change priv} ++
"You (%R) do not have permission to change printers.
Please ask someone else."
E{spstart device name clash} ++		"There is already a %s printer on line/network %t"
E{spstart delete one only} ++
"Sorry but you cannot delete more than one printer at once.
You will have to be more specific about the printer than
%s as there are %d0 of them"
E,H{No delete priv} ++
"You (%R) do not have permission to delete a printer.
Please ask someone else.",
"Your user account does not permit the addition or
deletion of printers"
E{spstart no conn/disconn priv} ++
"Sorry but you do not have permission to start/stop a connection
Please ask someone else."
E{Connop unknown host name} ++
"Unknown host name `%s' in connection command."
E{Connop is client} ++
"%s is a client (probably DOS) system it must connect to this
machine, not vice versa - sorry."
E{spstart identify printer} ++
"Sorry but you cannot do '%P' with more than one printer at once.
You will have to be more specific about the printer than
%s as there are %d0 of them"
E{spstart needs force opt} ++
"%d0 printers fit the description %s.
If you mean all of them re-run %P with the -f option.
If you only mean one, specify which with the -l option giving
the device name."
E{spstart cannot add remote printer} ++
"Sorry but you cannot add remote printers. You must rlogin to the
machine %s and do it from there."
E{spstart cannot delete remote printer} ++
"Sorry but you cannot delete remote printers. You must rlogin to the
machine %s and do it from there."

E{Spufile needs rebuild} ++
"Warning: Your user control file may be out of date
with your password file. Please consider running ""spuser -i""
or ""spuchange -R""."

E{Cannot create sequence file} = ROUND(@, 10)	"Cannot create sequence number in alt spool directory - %E"
E{Cannot find program} ++		"%P: Cannot find program %s"
E{Cannot run program} ++		"%P: Cannot run program %s"
E{Unknown alternative spool directories} ++	"%P: Cannot locate spool directory in %s"

E{sqcrcgi no file supplied} = ROUND(@, 20) "No file supplied with job create - probable CGI error"
E{sqcrcgi cannot reopen temp file} ++ "CGI error - cannot reopen temp file"
E{sqcrcgi no job data} ++	"No job data received"
E{sqcrcgi file full} ++		"Spool write failed - file system full on server?"

[
Names for permissions
]
P{Spulist default name} = 400			"DEFAULT"
P{Spulist all name} ++				"ALL"
P{Spulist lic user} ++				"L"
P{Spulist unlic user} ++			"U"

P{Priv adm} = ROUND(@, 10)			"Adm"
P{Priv stp} ++ COPY				"Stp"
P{Priv form} ++ COPY				"Form"
P{Priv othptrs} ++ COPY				"Otherp"
P{Priv cpri} ++ COPY				"Cpri"
P{Priv otherj} ++ COPY				"Otherj"
P{Priv prinq} ++ COPY				"Prinq"
P{Priv hgo} ++ COPY				"Hgo"
P{Priv anyp} ++ COPY				"Anyp"
P{Priv cdef} ++ COPY				"Cdef"
P{Priv addp} ++ COPY				"Addp"
P{Priv cover} ++ COPY				"Cover"
P{Priv unq} ++ COPY				"Unq"
P{Priv votj} ++ COPY				"Votj"
P{Priv remj} ++ COPY				"Remj"
P{Priv remp} ++ COPY				"Remp"
P{Priv access} ++ COPY				"Accessj"
P{Priv freeze} ++ COPY				"Freeze"

##  SPUCHANGE

[
Mostly for spuchange
]
E{spuchange priority error} = ROUND(@, 10)	"The priority `%s' specified is not numeric."

E{spuchange priority range} ++			"Invalid priority `%s', should be between 1 and 255."
E{spuchange copies error} ++			"The copies `%s' specified is not numeric."
E{spuchange copies range} ++			"Invalid copies `%s', should be between 1 and 255."
E{Privilege specification error} ++		"Unknown privilege code `%s'"
E{Unexpected arguments follow defaults} ++	"Unexpected arguments follow set of defaults."
E{Form type doesnt match allowed} ++		"Default form type %s doesn't fit the restriction of %t"
E{Ptr type doesnt match allowed} ++		"Default printer %s doesn't fit the restriction of %t"

E{Unknown user name ignored} ++			"Unknown user name `%s' - ignored"

E{Rebuilding spufile wait} ++			"Rebuilding user control file, please wait."
E{Finished rebuild spufile} ++			"Finished rebuild."

E{spulist nothing to do} ++			"Both header and user lines suppressed - nothing to do"
E{spuchange nothing to do} ++			"%P - Nothing to do"
E{spuchange no change default} ++		"%P - you are not allowed to change your default prio/printer/form"

##  SSTOP

[
Mostly for sstop
]
P{Confirm stop spooler} = ROUND(@, 20)	"GNUSpool is running - please confirm quit: "
E{No sstop priv} ++
"Sorry - you (%R) are not allowed to stop GNUSpool.
Please ask someone who is.
If you meant to just stop a single printer, this is the wrong
command anyway - use `spq' or `sphalt' instead."
E{sstop usage} ++
"Usage: %P
  or   %P -y     to suppress confirmation message and proceed"
E{sstop not stopped} ++			"GNUSpool not stopped."
E{Sstop waiting} ++			"Waiting as you are trying to shut down...."
E{Sstop given up} ++			"Sorry - given up!!"
E{Sstop ssuspend usage} ++		"Usage: %P [-y] [time]"
P{Confirm suspend scheduling} ++	"Please confirm that you want to suspend scheduling: "
P{Scheduling suspension time} ++	"Suspend scheduling time (seconds): "
E{Sstop ssuspend not suspended} ++	"Suspension cancelled"

##  SPCHARGE

[
Mostly for spcharge
]
E{spcharge bad amount} = ROUND(@, 20)	"%P:bad number for charge amount: %s"

E{spcharge spooler running} ++
"%P:GNUSpool is running and to avoid conflicts we only allow
reformatting of the charges file whilst it is stopped."

E{spcharge unknown user} ++		"%P:unknown user name: %s"
E{spcharge no memory} ++		"%P:Out of memory"
E{No charges file yet} ++
"Sorry but the charges file has not been created yet.
Has GNUSpool been run yet?"

E{spcharge summary format} = ROUND(@, 10)	"%s:%d1"
E{spcharge unexpected record} ++		"Unexpected charges record %d0"
E{spcharge print record format} ++		"%D4 %T4:%u0:%d1:%d2:%d3"
E{spcharge print record format host} ++		"%D4 %T4:%u0:%d1:%d2:%d3 printed on %s"
E{spcharge fee format} ++			"%D4 %T4:%u0 charged %d3"
E{spcharge feeall format} ++			"%D4 %T4:All users charged %d3"
E{spcharge consol format} ++			"%D4 %T4:%u0 consolidated to %d3"
E{spcharge zero format} ++			"%D4 %T4:%u0 zeroed"
E{spcharge zeroall format} ++			"%D4 %T4:All users zeroed"

[
Argument syntax for spr
]
A{spr explain} = 100			"?,help,explain"
A{spr toggle verbose} ++ COPY		"v,V,toggle-verbose"
A{spr set verbose} ++ COPY		"verbose"
A{spr no verbose} ++ COPY		"no-verbose"
A{spr no banner} ++ COPY		"s,no-banner"
A{spr banner} ++ COPY			"r,banner"
A{spr no messages} ++ COPY		"x,no-message"
A{spr write message} ++ COPY		"w,write-message"
A{spr mail message} ++ COPY		"m,mail-message"
A{spr no attention} ++ COPY		"b,noatt-message"
A{spr mail attention} ++ COPY		"a,mail-attention"
A{spr write attention} ++ COPY		"A,write-attention"
A{spr no retain} ++ COPY		"z,no-retain"
A{spr retain} ++ COPY			"q,retain"
A{spr local only} ++ COPY		"l,local-only"
A{spr network wide} ++ COPY		"L,network-wide"
A{spr interpolate} ++ COPY		"i,interpolate"
A{spr no interpolate} ++ COPY		"I,no-interpolate"
A{spr copies} ++ COPY			"c,copies"
A{spr header} ++ COPY			"h,header"
A{spr formtype} ++ COPY			"f,formtype"
A{spr priority} ++ COPY			"p,priority"
A{spr printer} ++ COPY			"P,printer"
A{spr post user} ++ COPY		"u,post-user"
A{spr classcode} ++ COPY		"C,classcode"
A{spr page range} ++ COPY		"R,page-range"
A{spr post proc flags} ++ COPY		"F,post-proc-flags"
A{spr printed timeout} ++ COPY		"t,printed-timeout"
A{spr not printed timeout} ++ COPY	"T,not-printed-timeout"
A{spr delay for} ++ COPY		"n,delay-for"
A{spr delay until} ++ COPY		"N,delay-until"
A{spr delimiter number} ++ COPY		"d,delimiter-number"
A{spr delimiter} ++ COPY		"D,delimiter"
A{spr odd even} ++ COPY			"O,odd-even-flags"
A{spr wait time} ++ COPY		"j,job-wait-time"
A{spr page limit} ++ COPY		"Z,job-size-limit"
A{spr host name} ++ COPY		"Q,host"
A{spr originating host} ++ COPY		"o,originating-host"
A{spr originating user} ++ COPY		"U,originating-user"
A{spr external system} ++ COPY		"E,external-system"
A{spr freeze current} ++ COPY		"freeze-current"
A{spr freeze home} ++ COPY		"freeze-home"

[
Argument syntax for sqchange
]
A{sqchange explain} = ROUND(@,20)	"?,help,explain"
A{sqchange no banner} ++ COPY		"s,no-banner"
A{sqchange banner} ++ COPY		"r,banner"
A{sqchange no messages} ++ COPY		"x,no-message"
A{sqchange write message} ++ COPY	"w,write-message"
A{sqchange mail message} ++ COPY	"m,mail-message"
A{sqchange no attention} ++ COPY	"b,noatt-message"
A{sqchange mail attention} ++ COPY	"a,mail-attention"
A{sqchange write attention} ++ COPY	"A,write-attention"
A{sqchange no retain} ++ COPY		"z,no-retain"
A{sqchange retain} ++ COPY		"q,retain"
A{sqchange local only} ++ COPY		"l,local-only"
A{sqchange network wide} ++ COPY	"L,network-wide"
A{sqchange copies} ++ COPY		"c,copies"
A{sqchange header} ++ COPY		"h,header"
A{sqchange formtype} ++ COPY		"f,formtype"
A{sqchange priority} ++ COPY		"p,priority"
A{sqchange printer} ++ COPY		"P,printer"
A{sqchange post user} ++ COPY		"u,post-user"
A{sqchange classcode} ++ COPY		"C,classcode"
A{sqchange page range} ++ COPY		"R,page-range"
A{sqchange post proc flags} ++ COPY	"F,post-proc-flags"
A{sqchange printed timeout} ++ COPY	"t,printed-timeout"
A{sqchange not printed timeout} ++ COPY	"T,not-printed-timeout"
A{sqchange delay for} ++ COPY		"n,delay-for"
A{sqchange delay until} ++ COPY		"N,delay-until"
A{sqchange odd even} ++ COPY		"O,odd-even-flags"
A{sqchange set classcode} ++ COPY	"S,set-classcode"
A{sqchange freeze current} ++ COPY	"freeze-current"
A{sqchange freeze home} ++ COPY		"freeze-home"

[
Argument syntax for sqdel
]
A{sqdel explain} = ROUND(@, 20)		"?,help,explain"
A{sqdel keep unp} ++ COPY		"n,keep-unprinted"
A{sqdel del unp} ++ COPY		"y,delete-unprinted"
A{sqdel classcode} ++ COPY		"C,classcode"
A{sqdel unqueue} ++ COPY		"u,unqueue"
A{sqdel no unqueue} ++ COPY		"e,do-not-unqueue"
A{sqdel no delete} ++ COPY		"k,do-not-delete"
A{sqdel delete} ++ COPY			"d,delete"
A{sqdel job pref} ++ COPY		"J,job-prefix"
A{sqdel cmd pref} ++ COPY		"S,command-prefix"
A{sqdel directory} ++ COPY		"D,directory"
A{sqdel freeze current} ++ COPY		"freeze-current"
A{sqdel freeze home} ++ COPY		"freeze-home"

[
Argument syntax for spstart
]
A{spstart explain} = ROUND(@, 20)	"?,help,explain"
A{spstart network} ++ COPY		"N,network-device"
A{spstart line} ++ COPY			"L,line-device"
A{spstart local} ++ COPY		"s,local-only"
A{spstart remotes} ++ COPY		"w,network-wide"
A{spstart device} ++ COPY		"l,device-name"
A{spstart classcode} ++ COPY		"C,classcode"
A{spstart description} ++ COPY		"D,description"
A{spstart newdevice} ++ COPY		"v,new-device"
A{spstart newclass} ++ COPY		"S,set-classcode"
A{spstart forceall} ++ COPY		"f,force-all"
A{spstart noforce} ++ COPY		"n,no-force"
A{spstart wait} ++ COPY			"W,wait-complete"
A{spstart exit} ++ COPY			"E,no-wait"
A{spstart freeze current} ++ COPY	"freeze-current"
A{spstart freeze home} ++ COPY		"freeze-home"

[
Argument syntax for sqlist
]
A{sqlist explain} = ROUND(@, 20)	"?,help,explain"
A{sqlist list only} ++ COPY		"q,list-only"
A{sqlist just user} ++ COPY		"u,just-user"
A{sqlist just title} ++ COPY		"t,just-title"
A{sqlist unprinted jobs} ++ COPY	"y,unprinted-jobs"
A{sqlist printed jobs} ++ COPY		"p,printed-jobs"
A{sqlist all jobs} ++ COPY		"Y,all-jobs"
A{sqlist classcode} ++ COPY		"C,classcode"
A{sqlist no page counts} ++ COPY	"e,no-page-counts"
A{sqlist page counts} ++ COPY		"E,page-counts"
A{sqlist local} ++ COPY			"l,local-only"
A{sqlist remotes} ++ COPY		"r,remotes"
A{sqlist format} ++ COPY		"F,format"
A{sqlist def format} ++ COPY		"D,default-format"
A{sqlist header} ++ COPY		"H,header"
A{sqlist no header} ++ COPY		"N,no-header"
A{sqlist include null} ++ COPY		"z,include-null"
A{sqlist no include null} ++ COPY	"Z,no-include-null"
A{sqlist include all} ++ COPY		"I,include-all"
A{sqlist view job} ++ COPY		"V,view-job"
A{sqlist no view job} ++ COPY		"n,no-view-job"
A{sqlist view pages} ++ COPY		"P,view-pages"
A{sqlist freeze current} ++ COPY	"freeze-current"
A{sqlist freeze home} ++ COPY		"freeze-home"

[
Argument syntax for splist
]
A{splist explain} = ROUND(@, 20)	"?,help,explain"
A{splist list ptr} ++ COPY		"q,list-only"
A{splist class} ++ COPY			"C,classcode"
A{splist local} ++ COPY			"l,local-only"
A{splist remotes} ++ COPY		"r,remotes"
A{splist format} ++ COPY		"F,format"
A{splist default format} ++ COPY	"D,default-format"
A{splist header} ++ COPY		"H,header"
A{splist no header} ++ COPY		"N,no-header"
A{splist unsorted} ++ COPY		"U,no-sort-ptrs"
A{splist sorted} ++ COPY		"S,sort-ptrs"
A{splist freeze current} ++ COPY	"freeze-current"
A{splist freeze home} ++ COPY		"freeze-home"

[
Argument syntax for spulist
]
A{spulist explain} = ROUND(@, 20)	"?,help,explain"
A{spulist sort user} ++ COPY		"u,user-sort"
A{spulist sort uid} ++ COPY		"n,numeric-sort"
A{spulist default line} ++ COPY		"d,default-line"
A{spulist no default line} ++ COPY	"s,no-default-line"
A{spulist user lines} ++ COPY		"U,user-lines"
A{spulist no user lines} ++ COPY	"S,no-user-lines"
A{spulist format} ++ COPY		"F,format"
A{spulist default format} ++ COPY	"D,default-format"
A{spulist header} ++ COPY		"H,header"
A{spulist no header} ++ COPY		"N,no-header"
A{spulist freeze current} ++ COPY	"freeze-current"
A{spulist freeze home} ++ COPY		"freeze-home"

[
Argument syntax for spuchange
]
A{spuchange explain} = ROUND(@, 20)	"?,help,explain"
A{spuchange set def} ++ COPY		"D,set-defaults"
A{spuchange set users} ++ COPY		"u,set-users"
A{spuchange copy def} ++ COPY		"A,copy-defaults"
A{spuchange no copy def} ++ COPY	"s,no-copy-defaults"
A{spuchange minp} ++ COPY		"l,min-priority"
A{spuchange defp} ++ COPY		"d,default-priority"
A{spuchange maxp} ++ COPY		"m,max-priority"
A{spuchange maxc} ++ COPY		"n,max-copies"
A{spuchange defform} ++ COPY		"f,default-form"
A{spuchange defptr} ++ COPY		"o,default-ptr"
A{spuchange formallow} ++ COPY		"F,form-allowed"
A{spuchange ptrallow} ++ COPY		"O,ptr-allowed"
A{spuchange privs} ++ COPY		"p,privileges"
A{spuchange class} ++ COPY		"c,class"
A{spuchange rebuild} ++ COPY		"R,rebuild-file"
A{spuchange no rebuild} ++ COPY		"N,no-rebuild"
A{spuchange dump pw} ++ COPY		"X,dump-passwd"
A{spuchange aswas pw} ++ COPY		"Y,default-passwd"
A{spuchange kill pw} ++ COPY		"Z,kill-dump-passwd"
A{spuchange freeze current} ++ COPY	"freeze-current"
A{spuchange freeze home} ++ COPY	"freeze-home"

[
Argument syntax for spcharge
]
A{spcharge explain} = ROUND(@, 20)	"?,help,explain"
A{spcharge print} ++ COPY		"p,print-users"
A{spcharge full} ++ COPY		"P,print-full"
A{spcharge zero} ++ COPY		"z,zero-charge"
A{spcharge add} ++ COPY			"c,add-charge"
A{spcharge consol} ++ COPY		"C,consolidate"
A{spcharge reset} ++ COPY		"R,reset-file"
A{spcharge cancel} ++ COPY		"K,cancel-flags"
A{spcharge freeze current} ++ COPY	"freeze-current"
A{spcharge freeze home} ++ COPY		"freeze-home"

[
Argument syntax for sp.lpq
]

A{sp.lpq explain} = 100			"?,help,explain"
A{sp.lpq longlist} ++ COPY		"l,long-list"
A{sp.lpq shortlist} ++ COPY		"s,short-list"
A{sp.lpq spechost} ++ COPY		"H,specified-host"
A{sp.lpq freeze current} ++ COPY	"freeze-current"
A{sp.lpq freeze home} ++ COPY		"freeze-home"

[
Argument syntax for sp.lprm
]

A{sp.lprm explain} = ROUND(@, 10)	"?,help,explain"
A{sp.lprm spechost} ++ COPY		"H,specified-host"
A{sp.lprm freeze current} ++ COPY	"freeze-current"
A{sp.lprm freeze home} ++ COPY		"freeze-home"

[
Usage options - spr
]
E{spr options} = 1
"%P options
-v           Toggle verbose flag (print message on std error)
-s -r        Turn off/on header
-q -z        Retain / do not retain on queue after printing
-w -m -x     Write / mail / do not write or mail message at end of job
-A -a -b     Write / mail / do not write or mail attention messages
-c n         Set number of copies to n
-p n         Set priority to n
-t hours     If printed and retained on queue delete after ""hours""
-T hours     If NOT printed delete after ""hours""
-n time      Delay for given time hh:mm
-N time      Delay until given date,time
-u user      Set user on banner
-h title     Set job title
-f form      Set form type
-i -I        Do/do not interpolate file suffixes with form type suffixes
-P printer   Set printer
-R page-page Set page range
-F flags     Set post-processing flags
-O o/e/a/b   Set to print odd, even, odd-then-even, even-then-odd pages
-C nnnn      Set class code to nnnn (hex)
-D string    Set page delimiter to ""string"" use \n etc for control codes
-d n         Set page delimiter n occurrences of the string given by -D
-j n         Submit job if no data on std input in n seconds
-Q host      Submit job directly to specified host"

[
Usage options - sqchange
]
E{sqchange options} ++
"%P options
-s -r        Turn off/on header
-q -z        Retain / do not retain on queue after printing
-w -m -x     Write / mail / do not write or mail message at end of job
-A -a -b     Write / mail / do not write or mail attention messages
-c n         Set number of copies to n
-p n         Set priority to n
-t hours     If printed and retained on queue delete after ""hours""
-T hours     If NOT printed delete after ""hours""
-n time      Delay for given time hh:mm
-N time      Delay until given date,time
-u user      Set user on banner
-h title     Set job title
-f form      Set form type
-P printer   Set printer
-R page-page Set page range
-F flags     Set post-processing flags
-O o/e/a/b   Set to print odd, even, odd-then-even, even-then-odd pages
-C nnnn      Set class code to nnnn for job selection
-S nnnn      Set job class codes to nnnn (hex)"

[
Usage options - sqdel
]
E{sqdel options} ++
"%P options
-n -y        Do not delete/do delete unprinted jobs
-C nnnn      Set class code nnnn for selection of jobs
-u           Unqueue job
-D dir -J name -S name specify directory, job name and command
             file name prefixes for unqueue
-k           Do not delete unqueued job.
-e           Cancel -u
-d           Cancel -k"

[
Usage options - sqlist
]
E{sqlist options} ++
"%P options
-q ptr       Only list jobs for given printer
-u user      Only list jobs for given user
-y -p -Y     Only unprinted, printed jobs, all jobs
-C nnnn      Set class code nnnn for selection of jobs
-e           Suppress page counts
-E           Display page counts (default)
-l           Local jobs only
-r           Remote jobs to be included (default)
-F formatstr Specify output format
-D           Restore default format
-H           Provide header
-N           No header
-z -Z -I     Include jobs with no printer, only matching or all
-V -n        View/do not view job(s)
-P arg       Specify page args to view"

[
Usage options - splist
]
E{splist options} ++
"%P options
-q ptr       Only list printers of given type
-C nnnn      Set class code nnnn (hex) for selection of printers
-l           Local printers only
-r           Remote printers to be included (default)
-F formatstr Specify output format
-D           Restore default format
-H           Provide header
-N           No header"

[
Usage options - spstart
]
E{spstart options} ++
"%P options
-?            Explain
-C            Match the classcodes
-l            Specify device name
-f            Force when pattern matching found
-n            Cancel the -f option"
[
Usage options - sphalt
]
E{spstart options} +1 COPY
"%P options
-?            Explain
-l            Specify device name
-f            Force when pattern matching found"
[
Usage options - spstop
]
E{spstart options} +2 COPY
"%P options
-?            Explain
-l            Specify device name
-f            Force when pattern matching found"
[
Usage options - spinter
]
E{spstart options} +3 COPY
"%P options
-?            Explain
-l            Specify device name
-f            Force when pattern matching found"
[
Usage options - spok
]
E{spstart options} +4 COPY
"%P options
-?            Explain
-l            Specify device name
-f            Force when pattern matching found
-n            Cancel -f option"
[
Usage options - spnok
]
E{spstart options} +5 COPY
"%P options
-?            Explain
-l            Specify device name
-f            Force when pattern matching found
-n            Cancel -f option"
[
Usage options - spadd
]
E{spstart options} +6 COPY
"%P options
-?            Explain
-D            Description associated with printer
-N            Specify network printer
-l            Specify device name  
-L            Specify line number
-s            Make available local only
-w            Make available network wide"
[
Usage options - spchange
]
E{spstart options} +7 COPY
"%P options
-?            Explain
-C            Match the classcodes
-D            Description associated with printer
-E            Cancel -W option
-f            Force when pattern matching found
-l            Specify device name
-L            Specify line number
-n            Cancel -f option
-N            Specify network printer
-s            Make available local only
-S            Set classcodes
-v            Change device name
-w            Make available network wide
-W            Wait for job completion"
[
Usage options - spdel
]
E{spstart options} +8 COPY
"%P options
-?            Explain
-l            Specify device name"
[
Usage options - spstat
]
E{spstart options} +9 COPY
"%P options
-?            Explain
-l            Specify device name"
[
Usage options - spconn
]
E{spstart options} +10 COPY
"%P options
"
[
Usage options - spdisconn
]
E{spstart options} +11 COPY
"%P options
"
[
Usage options - spulist
]
E{spulist options} ++
"%P options
-u           user sort
-n           numeric sort
-d -s        list/do not list defaults
-U -S        list/do not user lines
-F formatstr Specify output format
-D           Restore default format
-H           Provide header
-N           No header"

[
Usage options - spuchange
]
E{spuchange options} ++
"%P options
    -D      Set defaults
    -u      Set users (all or named)
    -A      copy defaults to all users
    -s      Do not copy defaults
    -l n -d n -m n Set min, default, max priority
    -n      Set max copies
    -f -o   Set default form ptr
    -F -O   Set allowable form ptr
    -p      Set privileges
    -c      Set class
    -R      Rebuild file if needed
Follow by named users or none to set all users"

[
Usage options - spcharge
]
E{spcharge options} ++
"%P options
   %P [-c charge] [-p] [-z] [ user ... ]
           -c charge
                   means add the specified charge to the named users'
                   charge record (before printing it)
           -p
                   means print the charge records for the named users
           -P
                   means print full records for the named users
           -z
                   means zero the charge records for the named users
                   (after printing it)
           -C
                   consolidate charge records
           -R
                   reset file (only whilst spooler not running)

           if no users are specified, the default is ALL users
           if no options are specified, the default is ``-p''"

[
Usage options - sp.lpq
]

E{sp.lpq options} = 1
"%P options:
           -?       explain (this message)
           -l       long list
           -s       short list (default)
	   -H host  specify host"

E{sp.lprm options} ++
"%P options:
        %P [-H host ] printer job [job ... ]"

E{sp.lpq unknown printer} ++		"%P: unknown printer %s"
E{sp.lpq no args} ++			"%P: no (single) printer argument supplied"
E{sp.lprm no args} ++			"%P: no printer and jobs arguments supplied"
E{sp.lpq cannot open ptr dir} ++	"%P: cannot open printer directory %s"
E{sp.lpq no prog} ++			"%P: cannot find internal program %s"

######################################################################
######################################################################
##  Mostly spq messages + common stuff
##

[
Default user form type if nothing else given
]
P{Default user form type} = 1000	"standard"

# Global keys

[
Keys global to all states in screen-based progs
]

K{key help} = 400		"?"
K{key refresh} ++		"^L"
K{key abort} ++			"\e"
K{key guess} ++			"\s,\t"
K{key eol} ++			"\r"
K{key halt} ++			"Q,q"
K{key cursor up} ++		"k,\kUP"
K{key cursor down} ++		"j,\kDOWN"
K{key screen up} ++		"^B"
K{key screen down} ++		"^F"
K{key half screen up} ++	"^U"
K{key half screen down} ++	"^D"
K{key top} = 416		"B,\kHOME"
K{key bottom} ++		"E"
K{key erase} = 420		"\kERASE"
K{key kill} ++			"\kKILL"
K{key save opts} = 430		"="
K{key stop} = 490		"^Z"

# State specific keys - job state

[
spq keys in job screen where we can switch to printer
]

H,S{spq job state} = 1
"?       Help
q       Quit
^L      Redraw screen
o       Printer queue

j k     down up (or arrows)
^F ^B   Page down up
^D ^U   Half page down up

A       Abort job
c       Change copies
p       Change priority
P       Change printer
f       Change form type
h       Change header
O       Change other options
I       View file
U C     Unqueue / copy
X       View system error file"

({spq job state})K{spq key abort job} = 500	"A"
({spq job state})K{spq key job other options} ++	"O"
({spq job state})K{spq key view job} ++		"I"
({spq job state})K{spq key unqueue job} ++	"U"
({spq job state})K{spq key copy job} ++		"C"
({spq job state})K{spq key other window} ++	"o"

# Attributes must be in a contiguous block

({spq job state})K{spq key job copies} = ROUND(@,10)	"c"
({spq job state})K{spq key job priority} ++	"p"
({spq job state})K{spq key job printer} ++	"P"
({spq job state})K{spq key job form type} ++	"f"
({spq job state})K{spq key job header} ++	"h"
({spq job state})K{spq key job hold time} ++	"H"
({spq job state})K{spq key job notp del} ++	"T"
({spq job state})K{spq key job pdel} ++		"t"
({spq job state})K{spq key job retain} ++	"r"
({spq job state})K{spq key job noh} ++		"s"
({spq job state})K{spq key job write} ++	"w"
({spq job state})K{spq key job mail} ++		"m"
({spq job state})K{spq key job wattn} ++	"W"
({spq job state})K{spq key job mattn} ++	"M"
({spq job state})K{spq key job loco} ++		"L"
({spq job state})K{spq key job class} ++	"K"
({spq job state})K{spq key job range} ++	"R"
({spq job state})K{spq key job hat} ++		":"
({spq job state})K{spq key job oddeven} ++	"#"
({spq job state})K{spq key job user} ++		"u"
({spq job state})K{spq key job flags} ++	"F"

({spq job state})K{key forward search} = ROUND(@, 10)	"^^"
({spq job state})K{key backward search} ++	"\\"
({spq job state})K{spq key job format} = ROUND(@, 10)	"\,"
({spq job state})K{spq key view sys err} = 590	"X"
({spq job state})K{key exec} = ROUND(@, 100)	"@"

# Job state for when we cannot access printers

[
spq keys in job screen where we CAN'T switch to printer
]

H,S{spq job state no ptr} = 2
"?       Help
q       Quit
^L      Redraw screen

j k     down up
^F ^B   Page down up
^D ^U   Half page down up

A       Abort job
c       Change copies
p       Change priority
P       Change printer
f       Change form type
h       Change header
O       Change other options
I       View file
X       View system error file"

({spq job state no ptr})K{spq key abort job}	"A"
({spq job state no ptr})K{spq key job other options}	"O"
({spq job state no ptr})K{spq key view job}	"I"
({spq job state no ptr})K{spq key unqueue job}	"U"
({spq job state no ptr})K{spq key copy job} 	"C"
({spq job state no ptr})K{spq key job copies}	"c"
({spq job state no ptr})K{spq key job priority}	"p"
({spq job state no ptr})K{spq key job printer}	"P"
({spq job state no ptr})K{spq key job form type}"f"
({spq job state no ptr})K{spq key job header}	"h"
({spq job state no ptr})K{spq key job hold time}"H"
({spq job state no ptr})K{spq key job notp del}	"T"
({spq job state no ptr})K{spq key job pdel} 	"t"
({spq job state no ptr})K{spq key job retain}	"r"
({spq job state no ptr})K{spq key job noh}	"s"
({spq job state no ptr})K{spq key job write} 	"w"
({spq job state no ptr})K{spq key job mail} 	"m"
({spq job state no ptr})K{spq key job wattn} 	"W"
({spq job state no ptr})K{spq key job mattn} 	"M"
({spq job state no ptr})K{spq key job loco} 	"L"
({spq job state no ptr})K{spq key job class} 	"K"
({spq job state no ptr})K{spq key job range} 	"R"
({spq job state no ptr})K{spq key job hat}	":"
({spq job state no ptr})K{spq key job oddeven} 	"#"
({spq job state no ptr})K{spq key job user} 	"u"
({spq job state no ptr})K{spq key job flags} 	"F"
({spq job state no ptr})K{spq key view sys err}	"X"
({spq job state no ptr})K{key forward search}	"^^"
({spq job state no ptr})K{key backward search}	"\\"
({spq job state no ptr})K{spq key job format}	"\,"
({spq job state no ptr})K{key exec}		"@"

# Printer access state

[
spq keys in printer screen
]

H,S{spq ptr cmd state} = 3
"?       Help
q       Quit
^L      Redraw screen
o       Job queue

j k     down, up

A r !   Abort, restart, interrupt job
h       halt at end of job
G H     start, stop printer
f P V   set form, printer device
N D     Add, delete printer
C       Change class code

y n     Reply to ""a/w oper""
        (n to reprint
         setup pages)
X       View system error file"

({spq ptr cmd state})K{spq key abort printer} = 500	"A"
({spq ptr cmd state})K{spq key restart printer} ++	"r"
({spq ptr cmd state})K{spq key halt printer} ++		"h"
({spq ptr cmd state})K{spq key stop printer} ++		"H"
({spq ptr cmd state})K{spq key start printer} ++	"G"
({spq ptr cmd state})K{spq key printer form} ++		"f"
({spq ptr cmd state})K{spq key printer} ++		"P"
({spq ptr cmd state})K{spq key device} ++		"V"
({spq ptr cmd state})K{spq key add printer} ++		"N"
({spq ptr cmd state})K{spq key delete printer} ++	"D"
({spq ptr cmd state})K{spq key printer class} ++	"C"
({spq ptr cmd state})K{spq key printer yes} ++		"y"
({spq ptr cmd state})K{spq key printer no} ++		"n"
({spq ptr cmd state})K{spq key interrupt printer} ++	"!"
({spq ptr cmd state})K{spq key lower limit} ++		"L"
({spq ptr cmd state})K{spq key upper limit} ++		"U"
({spq ptr cmd state})K{spq key description} ++		"c"
({spq ptr cmd state})K{spq key ptr local only} ++	"O"
({spq ptr cmd state})K{spq key ptr exported} ++		"e"
({spq ptr cmd state})K{key forward search}		"^^"
({spq ptr cmd state})K{key backward search}		"\\"
({spq ptr cmd state})K{spq key job window} = ROUND(@, 10)	"o"
({spq ptr cmd state})K{spq key printer format} = ROUND(@, 5)	"\,"
({spq ptr cmd state})K{spq key view sys err}		"X"
({spq ptr cmd state})K{key exec}			"@"

# View job state

[
View jobs
]

H,S{spq view job} = 5
"You are looking at job number %d0: `%s'
Please type:
Cursor keys - to move through file
Space - next screenful, L R left or right scroll
B E < > - beginning, end, lh, rh margin of file
s e H - set start, end, reset ""halted"" at pages
^ \ - forward, reverse search
^L - to refresh screen
q - to quit."

({spq view job})K{spq key scroll file left} = 530	"l,L,\kLEFT"
({spq view job})K{spq key scroll file right} ++		"r,R,\kRIGHT"
({spq view job})K{spq key left marg} ++			"<"
({spq view job})K{spq key right marg} ++		">"
({spq view job})K{spq key start page} ++		"s"
({spq view job})K{spq key end page} ++			"e"
({spq view job})K{spq key halted at page} ++		"H"
({spq view job})K{key forward search}			"^^"
({spq view job})K{key backward search}			"\\"

# View system error log state

[
View system error log
]

P,H,S{spq view syslog} = 6
"System error log file",
"You are looking at the system error log file
Please type:
Cursor keys - to move through file
Space - next screenful, L R left or right scroll
B E < > - beginning, end, lh, rh margin of file
^ \ - forward, reverse search
^L - to refresh screen
q - to quit."

({spq view syslog})K{spq key scroll file left}		"l,L,\kLEFT"
({spq view syslog})K{spq key scroll file right}		"r,R,\kRIGHT"
({spq view syslog})K{spq key left marg}			"<"
({spq view syslog})K{spq key right marg}		">"
({spq view syslog})K{key forward search}		"^^"
({spq view syslog})K{key backward search}		"\\"

#  Confirm bypass align

[
Confirm bypass alignment page
]

P,H,E,S{Bypass align} = 10
"Bypass alignment for printer %s? ",
"Please confirm (y or n) that you want to bypass the alignment page
for this printer (%s) which is normally printed with form type %t.",
"Please answer `y' or `n' - do you want to bypass alignment page
for printer %s form type %t"

({Bypass align})K{key yes} = 450			"y,Y,t,T"
({Bypass align})K{key no} ++				"n,N,f,F"

# Reinstate align

[
Reinstate alignment page
]

P,H,E,S{Reinstate align} = {Bypass align} + 1
"Rerun alignment for printer %s? ",
"Please confirm (y or n) that you want to reinstate the alignment page
for this printer (%s) for form type %t.",
"Please answer `y' or `n' - do you want to reinstate alignment page
for printer %s form type %t"

({Reinstate align})K{key yes}				"y,Y,t,T"
({Reinstate align})K{key no}				"n,N,f,F"

# Delete printer

[
Confirm delete printer
]

P,H,E,S{Confirm delete printer} = {Reinstate align} + 1
"Sure about deleting printer %s?",
"Please confirm (Y or N) that you want to delete printer %s.",
"Unknown command - expecting Y or N
Please confirm whether you want to delete printer %s."

({Confirm delete printer})K{key yes}			"y,Y,t,T"
({Confirm delete printer})K{key no}			"n,N,f,F"

# Confirm delete job

[
Confirm delete job
]

P,H,E,S{spq confirm delete} = {Confirm delete printer} + 1
"Job %d has not been printed. Are you sure? ",
"Please confirm (y or n) whether you want to abort
this job.",
"Please answer `y' or `n'. Do you want to abort/delete
this job."
({spq confirm delete})K{key yes}
"y,Y,t,T"
({spq confirm delete})K{key no}
"n,N,f,F"

P{Not printed sure del} ++
"Job %d has not been printed. Are you sure? "

# Boolean options in qopts screen

[
Set all sorts of yes/no options in job parameters screen
]

S{spq set boolean state} = 15

({spq set boolean state})K{key yes}			"y,Y,t,T"
({spq set boolean state})K{key no}			"n,N,f,F"
({spq set boolean state})K{key toggle} ++		"~,!"

E{spq set boolean state}
"Unknown command. Please enter Y (or T) for yes, N (or F) for no
~ to invert"

# Set hold time currently none

[
Set hold time where currently none
]

H,S{spq nohold time state} = 20
"If you do not want the job to be printed before a given
time and date type an ""S"" and adjust the time and date
displayed."

({spq nohold time state})K{spq key set hold time} = 550
"S,y,Y"

# Set hold time currently something set

[
Set hold time where currently set
]

H,S{spq hold time state} = 21
"Please specify a time to hold the job for 'U' to unset."

({spq hold time state})K{key left} = {key kill} + 1	"l,L,\kLEFT"
({spq hold time state})K{key right} ++			"r,R,\kRIGHT"
({spq hold time state})K{spq key hold unset} = 551	"U,n,N"
({spq hold time state})K{spq key increment} ++		"+"
({spq hold time state})K{spq key decrement} ++		"-"

#  Yes or no questions in save options for spq

S{spq ask yorn state} = 29

[
Set fields and widths in spq display
]

({spq ask yorn state})K{key yes}			"y,Y,t,T"
({spq ask yorn state})K{key no}				"n,N,f,F"

H,S{spq field set state} = 30
"i a         New field before/after
' \"         New separator before/after
w           Set width
c           Set code
< >         Toggle left/right flag
D           Delete
S           Set separator string"

({spq field set state})K{spq key sep before} = 500	"'"
({spq field set state})K{spq key sep after} ++		""""
({spq field set state})K{key delete field} ++		"D"
({spq field set state})K{spq key field before} ++	"i"
({spq field set state})K{spq key field after} ++	"a"
({spq field set state})K{spq key field width} ++	"w"
({spq field set state})K{spq key field code} ++		"c"
({spq field set state})K{spq key left flag} ++		"<"
({spq field set state})K{spq key sep string} ++		"S"
({spq field set state})K{spq key right flag} ++		">"

# Whexnum edit class codes

S{whexnum state} = 90

[
Set class code bits
]

({whexnum state})K{key left}				"l,L,\kLEFT"
({whexnum state})K{key right}				"r,R,\kRIGHT"
({whexnum state})K{key yes}				"y,Y,t,T,+"
({whexnum state})K{key no}				"n,N,f,F,-"
({whexnum state})K{key toggle}				"~,!"
({whexnum state})K{key all true} ++			"*"
({whexnum state})K{key all false} ++			"&"

[
Various on-line helps and errors
]
P,H{spq number copies} = 100
"Copies: ",
"Please enter the number of copies of this job to be printed.
This can be any number between %d0 and %d1. (If this job is being
printed at the moment, the current copy counts as one)."

P,H{spq enter priority} ++
"Priority: ",
"Please enter the new priority of this job, between %d0 and %d1."

P,H{spq title help} ++
"Title:",
"Please give a new title to this job."

P,H{spq form help} ++
"Form type: ",
"Please give a new form type for this job.
Possible form types may follow...."

P,H{spq printer help} ++
"Printer: ",
"Please specify the printer for this job, or blank to denote any available
printer with the right formtype.
Possible printer types may follow...."

P,H{spq range help} ++
"Page range: ",
"Please specify a range of pages as m-n, m- or -n."

H{spq hat help} ++
"Please specify a new `halted-at' page."

H{spq oddeven help} ++
"Please specify how you want to handle odd and even pages.
Set skip odd or even pages, and whether you want to swap over the
skip on alternate copies."

P,H{spq pptr help} ++
"Printer name: ",
"Please specify the printer type. Possibilities follow...."

P,H{spq device help} ++
"Device: ",
"Please give a network address/description, in <>s or
a device name, without the initial ""/dev"", such as:"

P,H{spq pform help} ++
"Form type: ",
"Please specify the initial form type:"

P,H{spq new pform help} ++
"New form type: ",
"Please specify the new form type:"

P,H{spq descr help} ++
"Description: ",
"Please give any description for the printer.
This is not taken into account anywhere but it may be helpful."

P,H{spq pclass help} ++
"Class code for %s: ",
"Please specify a new class code for the printer."

P,H{Lower job size limit} ++
"Lower job limit for %s: ",
"Please specify a new lower limit on job size (exact number of bytes) for
this printer (%s), or 0 for no limit"

P,H{Upper job size limit} ++
"Upper job limit for %s:",
"Please specify a new upper limit on job size (exact number of bytes) for
this printer (%s), or 0 for no limit"

P{spq unqueue prompt} ++	"Unqueuing job `%s' (%d)"

P,H{spq unqueue dir} ++
"Directory to write in",
"This is the directory into which the job will be
copied. It should be writable by you."

P,H{spq cmd file} ++
"Command file",
"This file will be made contain a `spr' command
to re-invoke the job at a later time."

P,H{spq job file} ++
"Job file",
"This file will contain the job standard input"

P,H{Fsearch job} ++
"Forward search job:",
"Please give a string to search for in jobs.
Title, user name, printer and form are examined.
'.' matches any character."

P,E{Rsearch job} ++
"Reverse search job:",
"No search string given, please try again."

E{Search job not found} ++
"No such job found."

P,H{Fsearch ptr} ++
"Forward search printer:",
"Please give a string to search for in printers.
Name, form type and device name are examined.
'.' matches any character."

P,E,H{Rsearch ptr} ++
"Reverse search printer:",
"No search string given, please try again.",
"You didn't give a string to search for in
the jobs or printers list. Please do so."

E{Search ptr not found} ++
"No such printer found."

######################################################################
##  Option states etc

# Printer states

[
Strings giving printer states
]
P{Printer status null} = 200		"???"
P{Printer status offline} ++ COPY	"offline"
P{Printer status error} ++ COPY		"error"
P{Printer status halted} ++ COPY	"halted"
P{Printer status startup} ++ COPY	"startup"
P{Printer status idle} ++ COPY		"idle"
P{Printer status shutdown} ++ COPY	"shutdown"
P{Printer status printing} ++ COPY	"printing"
P{Printer status awoper} ++ COPY	"a/w oper"

[
Printer state colours corresponding to above
]

P{Printer scolour null} = 7000		""
P{Printer scolour offline} ++ COPY	"red"
P{Printer scolour error} ++ COPY	"red"
P{Printer scolour halted} ++ COPY	""
P{Printer scolour startup} ++ COPY	"darkgreen"
P{Printer scolour idle} ++ COPY		"darkgreen"
P{Printer scolour shutdown} ++ COPY	"darkred"
P{Printer scolour printing} ++ COPY	"darkblue"
P{Printer scolour awoper} ++ COPY	"purple"

# Misc printer flags

[
Misc printer state strings
]

P{Printer heoj} = 210			"(heoj)"
P{Printer interrupted} ++		"AB!!"
P{Printer not aligned} ++		"(N/A)"
P{Netdev start str} ++			"<"
P{Netdev end str} ++			">"

P{xmspq start page} ++			"START PAGE"
P{xmspq end page} ++			"END PAGE"
P{xmspq hat page} ++			"\"HALTED AT\" PAGE"
P{Page eodoc} ++			"<end>"

P{Jobs more above} = ROUND(@, 100)	"-- %d more jobs above --"
P{Jobs more below} ++			"-- %d more jobs below --"
P{Ptrs more above} ++			"-- %d more printers above --"
P{Ptrs more below} ++			"-- %d more printers below --"
P{Spq local printer} ++			"[localptr]"
P{spq oe odd} ++			"Skip odd pages: "
P{spq oe even} ++			"  Skip even pages: "
P{spq oe reverse} ++			"  Reverse skip: "

P{sqcgi restr start} = ROUND(@, 20)	"<u>"
P{sqcgi restr view} ++			"View restricted"
P{sqcgi restr printers} ++		"For printer(s):"
P{sqcgi restr null} ++			"(plus no dest)"
P{sqcgi restr users} ++			"User(s):"
P{sqcgi restr unp} ++			"Unprinted only"
P{sqcgi restr p} ++			"Printed only"
P{sqcgi restr loco} ++			"Local only"
P{sqcgi restr end} ++			"</u>"

## Qopts screen
## Define where the cursor starts and where the first row comes

N{Qopt Initial prompt} = 398
N{Qopt Row start} = 399

## Define each prompt and corresponding help message

[
Prompts and helps for job parameters in spq
]

P,H{Qopt title} = 400
"Title             :",
"Please specify a new job title for the job.
This will appear on the banner page (unless suppressed) and
in any messages."

P,H{Qopt nohdr} ++
"Suppress header   :",
"Please specify (Y or N) whether the header is to be suppressed
for this job. The header may be suppressed anyway for certain form
types, or insisted upon for others."

P,H{Qopt form type} ++
"Form type         :",
"Please specify a new form type for this job. Possible form types
may follow this message..."

P,H{Qopt copies} ++
"Copies            :",
"Please specify the number of copies required for this job."

P,H{Qopt printer} ++
"Printer           :",
"Please specify the printer required for this job."

P,H{Qopt prio} ++
"Priority          :",
"Please specify the priority required for this job."

P,H{Qopt post user} ++
"Post to user      :",
"Please specify a new user for this job to be posted to.
This user will appear in big letters on the banner page."

P,H{Qopt mail msg} ++
"Mail message      :",
"Please specify (Y or N) whether you want to have messages about the
completion of this job mailed to the author."

P,H{Qopt write msg} ++
"Write message     :",
"Please specify (Y or N) whether you want to have messages about the
completion of this job written to the author's terminal."

P,H{Qopt mattn} ++
"Mail attention message  :",
"Please specify (Y or N) whether you want to have messages about the
printer on which this job is being printed mailed to the author."

P,H{Qopt wattn} ++
"Write attention message :",
"Please specify (Y or N) whether you want to have messages about the
printer on which this job is being printed written to the author's
terminal."

P,H{Qopt class} ++
"Class code        :",
"Please specify the class code for this job."

P,H{Qopt flags} ++
"P/proc flags      :",
"Please specify a value for the SPOOLFLAGS environment variable to
be passed to any postprocessor invoked when the job is printed"

P,H,N{Qopt retn} ++
"Retain after printing   :",
"Please specify (Y or N) whether you want this job to be retained on
the queue with zero copies after printing."

P,H{Qopt start page} ++
"Start page        :",
"Please specify the starting page for this job."

P,H{Qopt hat page} ++
"Last print halted at  :",
"Please specify a new value for the page marker for where the last print
was halted."

P,H{Qopt end page} ++
"End page          :",
"Please specify the final page for this job."

P,H{Qopt skip odd} ++
"Skip odd pages    :",
"Please specify (Y or N) whether you want to skip odd pages on this job"

P,H{Qopt skip even} ++
"Skip even pages   :",
"Please specify (Y or N) whether you want to skip even pages on this job"

P,H{Qopt reverse skip} ++
"Reverse skip      :",
"Please specify (Y or N) whether you want to swap the ""skip odd pages""
and ""skip even pages"" markers at the end of each copy of this job"

P,H{Qopt printed} ++
"Printed           :",
"Please specify (Y or N) whether you want to mark the job as printed.
The job will be automatically marked at the end of each copy."

P,H{Qopt del not printed} ++
"Delete time if not printed (hours)  :",
"Please specify the time in hours after which this job will be
deleted if it has NOT been printed."

P,H{Qopt del printed} ++
"Delete time if printed (hours)      :",
"Please specify the time in hours after which this job will be
deleted if it has been printed at least once."

P,H{Qopt hold time} ++
"Hold job until    :",
"Please specify a time to hold the job for, 'S' to set 'U' to unset."

P,H{Qopt local only} ++
"Local job only    :",
"This flag specifies that the job is to be regarded as ""local only""
and not to be passed on to other networked machines."

# Lay down starting points for cursor and display

[
Define initial prompt to start cursor at.
Define starting row of display
]
({Qopt Initial prompt})	N	{Qopt retn}
({Qopt Row start})	N	{Qopt title}

# Chain giving the order of prompts

[
Define order of rows displayed
]
({Qopt title})		N	{Qopt nohdr}
({Qopt nohdr})		N	{Qopt form type}
({Qopt form type})	N	{Qopt copies}
({Qopt copies})		N	{Qopt printer}
({Qopt printer})	N	{Qopt prio}
({Qopt prio})		N	{Qopt post user}
({Qopt post user})	N	{Qopt mail msg}
({Qopt mail msg})	N	{Qopt write msg}
({Qopt write msg})	N	{Qopt mattn}
({Qopt mattn})		N	{Qopt wattn}
({Qopt wattn})		N	{Qopt class}
({Qopt class})		N	{Qopt flags}
({Qopt flags})		N	{Qopt retn}
({Qopt retn})		N	{Qopt start page}
({Qopt start page})	N	{Qopt hat page}
({Qopt hat page})	N	{Qopt end page}
({Qopt end page})	N	{Qopt skip odd}
({Qopt skip odd})	N	{Qopt skip even}
({Qopt skip even})	N	{Qopt reverse skip}
({Qopt reverse skip})	N	{Qopt printed}
({Qopt printed})	N	{Qopt del not printed}
({Qopt del not printed})N	{Qopt del printed}
({Qopt del printed})	N	{Qopt hold time}
({Qopt hold time})	N	{Qopt local only}
({Qopt local only})	N	-1

[
Miscellaneous spq queue param errors
]
E{spq qopts bad state code} = ROUND({Qopt local only}, 20)
"Invalid state code number %d9 in job parameters list in
help file %F"

E{spq qopts duplicated state code} ++
"Duplicated state code %d9 referred to in %d8N%d9 in
list of job parameters order in help file %F"

E{spq job disappeared} ++
"Job number %d0 has disappeared from the queue!
It has either been printed and has been deleted, or someone else
has removed it. Please type ENTER to continue"

E{Unknown user in spq} ++
"Unknown user `%s'. Please type a known user on this system."

E{spq set zero class} ++
"Resulting job class, after applying your class of ""%s"",
would be zero. Please try again."

E{spq nohold command error} ++
"Unknown command - please type ""S"" to set a hold time"

E{spq hold command error} ++
"Unknown command - please type ""U"" to unset the hold time, digits of
time or + or - to increment or decrement the time specification"

E{spq hold time passed} ++
"The time you have given is not in the future. Please try again."

E{spq hold time past lhs} ++
"Off lhs of time specification"

E{spq hold time antique} ++
"I don't believe years less than 1990..."

E,H{spq hold time hours} ++
"Invalid digit in hours specification",
"Please specify digits of the hour to hold the job until
or increment/decrement using + and -"

E,H{spq hold time minutes} ++
"Invalid digit in minutes/seconds specification",
"Please specify digits of the minute to hold the job until
or increment/decrement using + and -"

E,H{spq hold time day of week} ++
"Unknown command please type + or - to adjust day of the week",
"Please adjust the day of the week with + or -, or move the cursor
using left/right arrow"

E,H{spq hold time day of month} ++
"Unknown command please type + or - to adjust day of the month or
explicit digits.",
"Please specify new digits of the day of the month
or increment/decrement using + and -"

E,H{spq hold time month} ++
"Unknown command please type + or - to adjust month",
"Please adjust the day of the month with + or -, or move the cursor
using left/right arrow"

E,H{spq hold time year} ++
"Unknown command please type + or - to adjust year",
"Please adjust the year using + and - or move the cursor"

# In the 2 following motif no longer has disp_str valid at help time

E,H{form type not valid} ++
"Your form type is not valid",
"Your account is limited to form types matching %t
which does not include your proposed type"

E,H{ptr type not valid} ++
"Your printer is not valid",
"Your account is limited to printer names matching %t
which does not include your proposed name"

E,H{end page less than start page} ++
"You have specified an end page less than the start page.
Please try again.",
"You have attempted to set the start of the page
range after the end of the range.
Please reset one or both."

E,H{end page less than haltat page} ++
"You have specified an end page less than the ""halted at"" page.
Please try again.",
"You have attempted to set the ""halted at"" page
after the end of the range.
Please reset either the ""halted at"" page or
the end of the range."

[
Misc displays in queue param options
]

P{spq qmsg percent complete} = ROUND(@, 10)	"%d%% complete"
P{spq qmsg end} ++				"<end>"
P{spq qmsg yes} ++				"Yes"
P{spq qmsg no} ++				"No"
P{spq qmsg nohold} ++				"<N/A>"

[
Abbreviated days of the week
]

P{Sunday} = ROUND(@, 10)		"Sun"
P{Monday} ++ COPY			"Mon"
P{Tuesday} ++ COPY			"Tue"
P{Wednesday} ++ COPY			"Wed"
P{Thursday} ++ COPY			"Thu"
P{Friday} ++ COPY			"Fri"
P{Saturday} ++ COPY			"Sat"

[
Full days of the week
]

P{Full Sunday} = ROUND(@, 10)		"Sunday"
P{Full Monday} ++ COPY			"Monday"
P{Full Tuesday} ++ COPY			"Tuesday"
P{Full Wednesday} ++ COPY		"Wednesday"
P{Full Thursday} ++ COPY		"Thursday"
P{Full Friday} ++ COPY			"Friday"
P{Full Saturday} ++ COPY		"Saturday"

[
Abbreviated months
]

P{January} = ROUND(@, 10)		"Jan"
P{February} ++ COPY			"Feb"
P{March} ++ COPY			"Mar"
P{April} ++ COPY			"Apr"
P{May} ++ COPY				"May"
P{June} ++ COPY				"Jun"
P{July} ++ COPY				"Jul"
P{August} ++ COPY			"Aug"
P{September} ++ COPY			"Sep"
P{October} ++ COPY			"Oct"
P{November} ++ COPY			"Nov"
P{December} ++ COPY			"Dec"

[
Months in full
]

P{Full January} = ROUND(@, 10)		"January"
P{Full February} ++ COPY		"February"
P{Full March} ++ COPY			"March"
P{Full April} ++ COPY			"April"
P{Full May} ++ COPY			"May"
P{Full June} ++ COPY			"June"
P{Full July} ++ COPY			"July"
P{Full August} ++ COPY			"August"
P{Full September} ++ COPY		"Sepember"
P{Full October} ++ COPY			"October"
P{Full November} ++ COPY		"November"
P{Full December} ++ COPY		"December"

[
Displays whilst viewing job in spq
]

P{Job view number form} = ROUND(@, 10)	"Job number %d: %s on form type %s"
P{Remote job view number} ++		"Job %s:%d: %s on form type %s"
P{Job view percent thru} ++		" - %d%% of the way through"
P{Job view eof} ++			"--- End of file ---"
P{Job view eop} ++			"--- End of page %d ---"
P{Job view start mkr} ++		"START"
P{Job view end mkr} ++			"END"
P{Job view haltat mkr} ++		"HALTED AT"
P{Job view fsearch} ++			"Forward search:"
P{Job view rsearch} ++			"Reverse search:"

[
Searching in spq view job
]

H,E{spq view ask search} ++
"Please enter a search string.
Case of letters is ignored. `.' represents any printing
character, except in the first position, where it represents itself.",
"Please specify a search string, consisting of printing characters
with `.' (apart from the first position) being wild."

# Prompts for program opts spq

[
Program options selection for spq
]

P,N{spq opt rptr} = 700		"Restrict to printer :"
P,N{spq opt rusr} ++		"Restrict to user    :"
P,N{spq opt rtitle} ++		"Restrict to title   :"
P,N{spq opt limitj} ++		"Limit jobs display:"
P,N{spq opt disponly} ++	"Display only      :"
P,N{spq opt loco} ++		"Local jobs/printers only    :"
P,N{spq opt sort} ++		"Sort printers     :"
P,N{spq opt class} ++		"Class code        :"
P,N{spq opt confdel} ++		"Confirm abort/delete jobs   :"
P,N{spq opt helpbox} ++		"Help messages     :"
P,N{spq opt errbox} ++		"Error messages    :"
P,N{spq opt helpclr} ++		"Clear help message:"
P,N{spq opt pagec} ++		"Page counts       :"
P,N{spq opt curs} ++		"If job moves      :"
P,N{spq opt plist} ++		"Printer list size (on entry):"
P,N{spq opt cursentry} ++	"Cursor on entry   :"
P,N{spq opt refresh} ++		"Refresh interval  :"

# Initial cursor and starting row

N{spq opt init cursor} = {spq opt rptr} - 2
N{spq opt start row} = {spq opt rptr} - 1

[
Define initial cursor location and starting row
in program options for spq
]

({spq opt init cursor})	N	{spq opt rptr}
({spq opt start row})	N	{spq opt rptr}

# Order in which prompts happen

[
Define order in which rows appear
]

({spq opt rptr})	N	{spq opt rusr}
({spq opt rusr})	N	{spq opt rtitle}
({spq opt rtitle})	N	{spq opt limitj}
({spq opt limitj})	N	{spq opt disponly}
({spq opt disponly})	N	{spq opt loco}
({spq opt loco})	N	{spq opt sort}
({spq opt sort})	N	{spq opt class}
({spq opt class})	N	{spq opt confdel}
({spq opt confdel})	N	{spq opt helpbox}
({spq opt helpbox})	N	{spq opt errbox}
({spq opt errbox})	N	{spq opt helpclr}
({spq opt helpclr})	N	{spq opt pagec}
({spq opt pagec})	N	{spq opt curs}
({spq opt curs})	N	{spq opt plist}
({spq opt plist})	N	{spq opt cursentry}
({spq opt cursentry})	N	{spq opt refresh}
({spq opt refresh})	N	-1

[
Helps and alternatives for program options in spq
]

H{spq opt rptr}*10
"If you are only interested in jobs for a given printer
put the printer name here. Alternatively leave empty
to display all printers"

H{spq opt rusr}*10
"To restrict display to jobs for a given user put the
user name here. To display all user's jobs leave empty."

H{spq opt rtitle}*10
"To restrict display to jobs with a given title put the
title pattern here. To display all jobs leave empty."

P,H{spq opt limitj}*10
"Only jobs matching printer",
"Please indicate whether you want the printer name
restriction to apply to jobs. You can just select
jobs for the given printer, those jobs plus jobs with
null printer name, or all jobs.
You are trying to save to a directory or something."
P{spq opt limitj} * 10 + 1
"Jobs matching printer plus null"
P{spq opt limitj} * 10 + 2
"All jobs regardless of printer"

P,H{spq opt disponly} * 10
"All",
"Please indicate whether you want to select all jobs,
or only those not printed, or only those printed."
P{spq opt disponly} * 10 + 1
"Not Printed"
P{spq opt disponly} * 10 + 2
"Printed"

P,H{spq opt loco} * 10
"No",
"Please indicate whether you want to display only
jobs and printers on the local machine"

P{spq opt loco} * 10 + 1
"Yes"

P,H{spq opt sort} * 10
"Unsorted",
"Please indicate whether you want the printer list
sorted or unsorted"

P{spq opt sort} * 10 + 1
"Sorted"

H{spq opt class} * 10
"Please set the class code you want to invoke
to display jobs and printers."

P,H{spq opt confdel} * 10
"Never",
"Please indicate whether you want to confirm abort delete
jobs every time, only for jobs not printed, or never."

P{spq opt confdel} * 10 + 1
"Unprinted jobs"

P{spq opt confdel} * 10 + 2
"Always"

P,H{spq opt helpbox} * 10
"Inverse video",
"Please indicate whether you want to display help messages
in inverse video (standout mode) or boxes."

P{spq opt helpbox} * 10 + 1
"Boxes"

P,H{spq opt errbox} * 10
"Inverse video",
"Please indicate whether you want to display error messages
in inverse video (standout mode) or boxes."

P{spq opt errbox} * 10 + 1
"Boxes"

P,H{spq opt helpclr} * 10
"Use next command",
"Please indicate whether you want the next keystroke afer
displaying a help message to be kept or lost"

P{spq opt helpclr} * 10 + 1
"Clear help"

P,H{spq opt pagec} * 10
"Display page counts",
"Please indicate whether page numbers in jobs should be displayed
on the main screen, or just the size."

P{spq opt pagec} * 10 + 1
"Suppress page counts"

P,H{spq opt curs} * 10
"Follow job",
"Please indicate whether you want to follow the job if it changes
position on the screen, or try to retain the display."

P{spq opt curs} * 10 + 1
"Retain display"

H{spq opt plist} * 10
"Please indicate the number of printers to display at once.
This option only applies on entry."

P,H{spq opt cursentry} * 10
"Job screen",
"Please indicate where the cursor should be on entry.
This can be the job screen, the printer screen or
`don't care'."

P{spq opt cursentry} * 10 + 1
"Printer screen"

P{spq opt cursentry} * 10 + 2
"Don't care"

H{spq opt refresh} * 10
"Please specify the polling interval between 2 and 90 seconds.
This is adjusted automatically, so this gives an initial value
only."

[
Ask about saving spq options
]

P,H{Save parameters} = 750
"Save parameters? ",
"Do you want to save the parameters just set to recover them
on entry?"

P,H{Save in current directory} ++
"Save in current directory? ",
"Do you want to save them in the current directory that they
will always take effect in this directory unless overridden
on the command line?
Your current directory is %s"

P,H{Save in home directory} ++
"Save in home directory? ",
"Do you want to save them in your home directory that they
will always take effect in this directory unless overridden
on the command line, current directory or environment?
Your home directory is %s"

P,H{Save in current home directory} ++
"Save in home (also current) directory? ",
"Do you want to save them in your home directory which is
is also your current directory?
(i.e. %s)"

E{spq options bad state code} ++
"Invalid state code number %d9 in options state code list in
help file %F"

E{spq options duplicated state code} ++
"Duplicated state code %d9 referred to in %d8N%d9 in
list of options order in help file %F"

E{spq options unknownc} ++
"Unknown command - setting screen options"

E{spq options eff zero class} ++
"Sorry but class code would give class code of 0 with your existing
class code of %s."

[
Save format codes questions
]

P,H{Save format codes} ++
"Save format codes?",
"Do you want to save the format field codes just set to
recover them on entry?"

P,H{Sure you want to overwrite} ++
"Overwrite existing file?",
"Are you sure that you want to overwrite the existing file
%s"

P,H{spq field width} ++
"Width of field",
"This is the maximum width of the field"

P,H{spq field code} ++
"Format code",
"This is the format code to assemble the display line from.
Possible codes follow:"

P,H{spq field sep} ++
"Separator value",
"This is the field to generate the separator with. Enter nothing
to just give a single space."

P,H{spq field fname} ++
"What file:",
"What file name would you like to save the new message file in."

E{spq fields unknownc} ++	"Unknown command editing format list"
E{spq fields off bottom} ++	"Off end of list of formats"
E{spq fields off top} ++	"Off beginning of list of formats"
E{spq fields none yet} ++	"No formats to edit yet - please create some"
E{spq fields is sep} ++		"Format is separator - invalid command"
E{spq fields is not sep} ++	"Format is not separator - invalid command"
E{spq fields undef code} ++	"Undefined format code"

P{Left tab field} = ROUND(@, 10)
"<"
P{Right tab field} ++
">"

[
Error messages whilst saving .gnuspool files
]

E{saveopts crashed} = ROUND(@, 10)
"Sorry saveopts crashed with signal 0x%x0"

{saveopts file error} = @ - 9
S{saveopts nodel} = 10
S{saveopts no init and del} = 11
S{saveopts cannot create} = 12
S{saveopts nomem} = 15

E{saveopts file error} + {saveopts nodel}
"Sorry cannot delete old init file from %s"
E{saveopts file error} + {saveopts no init and del}
"Sorry cannot create init file in %s
(and I've deleted your old one - sorry!!)"
E{saveopts file error} + {saveopts cannot create}
"Sorry cannot create init file in %s"
E{saveopts cannot fork} = {saveopts crashed} + 4
"Sorry cannot fork to create init file"
E{saveopts file error} + {saveopts nomem}
"No memory available creating init file."

E,H{spq help not flat file} = ROUND(@, 10)
"Your proposed saved help file is not a regular file",
"The file name you gave is not a file name but the name
of an existing directory etc."

E,H{spq help would overwrite} ++
"Your proposed saved help file would overwrite the current one.",
"You are trying to save a help file on top of the one currently
in use"

E,H{spq help file cannot open} ++
"Cannot open %s for writing.",
"I am trying to open the file %s to write the new help file
to it but I cannot open it. Maybe the permissions are wrong."

E,H{spq help file cannot create} ++
"Cannot create %s",
"I am trying to create the file %s to write the new help file
to it but I cannot do so. Maybe the permissions are wrong."

E,H{No freeze permission} ++
"No freeze permission",
"Your account is not permitted to save default options.
Sorry about that."

# Format codes

[
Default format strings for spq
]
P{Spq job default fmt} = 1990	"%3n %<6N %6u %14h %13f%5Q%5S %3c %3p %14P"
P{Spq job default fmt}+1 COPY	"%3n %<6N %6u %14h %13f%5L%5K %3c %3p %14P"
P{Spq ptr default format} ++	" %14p %10d %16f %>8s %3x %7j %7u %6w"

[
Default format strings for xmspq
]
P{xmspq job default fmt} = ROUND(@, 10)	"%3n %<6N %6u %14h %16f %4Q %4S %3c %3p %14P"
P{xmspq job default fmt}+1 COPY		"%3n %<6N %6u %14h %16f %4L %4K %3c %3p %14P"
P{xmspq ptr default format} ++		"%15p %10d %16f %>9s %3x %7j %7u %6w"

[
Display Format strings
]
P{Local only} = ROUND(@, 10)	"Localonly"
P{Fmt wattn} ++			"wattn"
P{Fmt mattn} ++			"mattn"
P{Fmt mail} ++			"mail"
P{Fmt write} ++			"write"
P{Fmt no odd} ++		"no-odd"
P{Fmt no even} ++		"no-even"
P{Fmt swap} ++			",swap"
P{Fmt local} ++			"local"
P{Fmt retain} ++		"retain"
P{Fmt nohdr} ++			"nohdr"

{job fmt title} = 2000

[
Display title abbreviations for jobs
]
P{job fmt title}+'A'-1		"Wattn"
P{job fmt title}+'C'-1		"Class"
P{job fmt title}+'D'-1		"Delim"
P{job fmt title}+'F'-1		"Post-proc"
P{job fmt title}+'G'-1		"H.at"
P{job fmt title}+'H'-1		"Hold"
P{job fmt title}+'K'-1		"Size"
P{job fmt title}+'L'-1		"Posn"
P{job fmt title}+'N'-1		"Jobno"
P{job fmt title}+'O'-1		"O/E"
P{job fmt title}+'P'-1		"Printer"
P{job fmt title}+'Q'-1		"AtPg"
P{job fmt title}+'R'-1		"Range"
P{job fmt title}+'S'-1		"Pgs"
P{job fmt title}+'T'-1		"Npdel"
P{job fmt title}+'U'-1		"Puser"
P{job fmt title}+'W'-1		"Stime"
P{job fmt title}+'a'-1		"Mattn"
P{job fmt title}+'c'-1		"Cps"
P{job fmt title}+'d'-1		"ndel"
P{job fmt title}+'f'-1		"Form"
P{job fmt title}+'h'-1		"Title"
P{job fmt title}+'l'-1		"Lonly"
P{job fmt title}+'m'-1		"Mail"
P{job fmt title}+'n'-1		"Seq"
P{job fmt title}+'o'-1		"Orig"
P{job fmt title}+'p'-1		"Pri"
P{job fmt title}+'q'-1		"Ret"
P{job fmt title}+'s'-1		"Noh"
P{job fmt title}+'t'-1		"Pdel"
P{job fmt title}+'u'-1		"User"
P{job fmt title}+'w'-1		"Wrt"

[
Display titles in full for jobs
]
P{job fmt title}+500+'A'-1	"Write Attention"
P{job fmt title}+500+'C'-1	"Class Code"
P{job fmt title}+500+'D'-1	"Delimiter"
P{job fmt title}+500+'F'-1	"Post-processing flags"
P{job fmt title}+500+'G'-1	"Halted at"
P{job fmt title}+500+'H'-1	"Hold Time"
P{job fmt title}+500+'K'-1	"Size in K"
P{job fmt title}+500+'L'-1	"Position reached (K)"
P{job fmt title}+500+'N'-1	"Job number"
P{job fmt title}+500+'O'-1	"Odd or even pages"
P{job fmt title}+500+'P'-1	"Printer"
P{job fmt title}+500+'Q'-1	"Page reached"
P{job fmt title}+500+'R'-1	"Page Range"
P{job fmt title}+500+'S'-1	"Size in Pages"
P{job fmt title}+500+'T'-1	"Delete time if not printed"
P{job fmt title}+500+'U'-1	"Post User"
P{job fmt title}+500+'W'-1	"Submission time"
P{job fmt title}+500+'a'-1	"Mail Attention"
P{job fmt title}+500+'c'-1	"Copies"
P{job fmt title}+500+'d'-1	"Number of delimiters"
P{job fmt title}+500+'f'-1	"Form type"
P{job fmt title}+500+'h'-1	"Job title"
P{job fmt title}+500+'l'-1	"Local only"
P{job fmt title}+500+'m'-1	"Mail result"
P{job fmt title}+500+'n'-1	"Number in queue"
P{job fmt title}+500+'o'-1	"Originating host"
P{job fmt title}+500+'p'-1	"Priority"
P{job fmt title}+500+'q'-1	"Retain on queue"
P{job fmt title}+500+'s'-1	"Suppress header"
P{job fmt title}+500+'t'-1	"Delete time if printed"
P{job fmt title}+500+'u'-1	"User submitting job"
P{job fmt title}+500+'w'-1	"Write result"

{Printer title} = 2100

[
Display title abbreviations for jobs
]
P{Printer title}+'a'-1		"Abort"
P{Printer title}+'c'-1		"Class"
P{Printer title}+'d'-1		"Device"
P{Printer title}+'e'-1		"Descr"
P{Printer title}+'f'-1		"Form"
P{Printer title}+'h'-1		"Heoj"
P{Printer title}+'i'-1		"Pid"
P{Printer title}+'j'-1		"Jobno"
P{Printer title}+'l'-1		"Local"
P{Printer title}+'m'-1		"Message"
P{Printer title}+'n'-1		"N/a"
P{Printer title}+'p'-1		"Printer"
P{Printer title}+'s'-1		"State:Msg"
P{Printer title}+'t'-1		"State"
P{Printer title}+'u'-1		"User"
P{Printer title}+'w'-1		"Flags"
P{Printer title}+'x'-1		"Limit"
P{Printer title}+'y'-1		"Minsize"
P{Printer title}+'z'-1		"Maxsize"

[
Display titles in full for printers
]
P{Printer title}+500+'a'-1	"Display message if abort pending"
P{Printer title}+500+'c'-1	"Class code"
P{Printer title}+500+'d'-1	"Device or <network address>"
P{Printer title}+500+'e'-1	"Printer description"
P{Printer title}+500+'f'-1	"Form type"
P{Printer title}+500+'h'-1	"Display message if halting at eoj"
P{Printer title}+500+'i'-1	"Process id"
P{Printer title}+500+'j'-1	"Job number being printed"
P{Printer title}+500+'l'-1	"Message if printer local only"
P{Printer title}+500+'m'-1	"Message in all cases"
P{Printer title}+500+'n'-1	"Display non-aligned marker"
P{Printer title}+500+'p'-1	"Printer name"
P{Printer title}+500+'s'-1	"State and message (offline/error)"
P{Printer title}+500+'t'-1	"State no message"
P{Printer title}+500+'u'-1	"User of job being printed"
P{Printer title}+500+'w'-1	"Combine abort/non-aligned message"
P{Printer title}+500+'x'-1	"Limit set, display as <>"
P{Printer title}+500+'y'-1	"Minimum job size"
P{Printer title}+500+'z'-1	"Maximum job size"

{Spulist title} = 2200

[
Display title abbreviations for user stuff
]
P{Spulist title}+'a'-1		"Form allow"
P{Spulist title}+'b'-1		"Ptr allow"
P{Spulist title}+'c'-1		"Class"
P{Spulist title}+'d'-1		"Defp"
P{Spulist title}+'f'-1		"Form"
P{Spulist title}+'l'-1		"Minp"
P{Spulist title}+'m'-1		"Maxp"
P{Spulist title}+'n'-1		"Cps"
P{Spulist title}+'o'-1		"Printer"
P{Spulist title}+'p'-1		"Priv"
P{Spulist title}+'u'-1		"User"
P{Spulist title}+'v'-1		"Lic"

# Argument names

[
Spq argument syntax
]
A{spq explain} = 200		"?,help,explain"
A{spq keepchar} ++ COPY		"H,keep-char-help"
A{spq losechar} ++ COPY		"h,lose-char-help"
A{spq cursor follow} ++ COPY	"n,follow-job"
A{spq cursor keep} ++ COPY	"s,keep-cursor"
A{spq local} ++ COPY		"l,local-only"
A{spq remotes} ++ COPY		"r,remotes"
A{spq dont care} ++ COPY	"d,dont-care"
A{spq jobs screen} ++ COPY	"j,jobs-screen"
A{spq ptr screen} ++ COPY	"p,printer-screen"
A{spq no page counts} ++ COPY	"e,no-page-counts"
A{spq page counts} ++ COPY	"E,page-counts"
A{spq confirm abort} ++ COPY	"a,confirm-abort"
A{spq no confirm abort} ++ COPY	"A,no-confirm-abort"
A{spq only queue} ++ COPY	"q,list-only"
A{spq number printers} ++ COPY	"P,number-printers"
A{spq classcode} ++ COPY	"C,classcode"
A{spq refreshtime} ++ COPY	"R,refresh-time"
A{spq help box} ++ COPY		"b,help-box"
A{spq no help box} ++ COPY	"B,no-help-box"
A{spq error box} ++ COPY	"m,error-box"
A{spq no error box} ++ COPY	"M,no-error-box"
A{spq unprinted jobs} ++ COPY	"y,unprinted-jobs"
A{spq all jobs} ++ COPY		"Y,all-jobs"
A{spq just user} ++ COPY	"u,just-user"
A{spq just title} ++ COPY	"t,just-title"
A{spq printed jobs} ++ COPY	"D,printed-jobs"
A{spq include null} ++ COPY	"z,include-null"
A{spq no include null} ++ COPY	"Z,no-include-null"
A{spq include all} ++ COPY	"I,include-all"
A{spq ptrs unsorted} ++ COPY	"U,no-sort-ptrs"
A{spq ptrs sorted} ++ COPY	"S,sort-ptrs"

[
Spq explain
]
E{spq options} = 2
"You may invoke the following options for %P

     %P -h   ignore char after help screens
     %P -H   do not ignore char after help screens
     %P -s   keep scroll position on job queue change
     %P -n   do not keep scroll position on job queue change
     %P -l   restrict attention to local jobs/printers
     %P -r   remove such restriction (default)
     %P -d   put cursor into job screen on entry unless empty
     %P -j   put cursor into job screen on entry
     %P -p   put cursor into printer screen on entry
     %P -e   do not display page counts
     %P -E   display page counts
     %P -a   require confirmation of aborts or deletion of unprinted jobs
     %P -A   do not require confirmation
     %P -q ptr       only display jobs for and/or printer specified
     %P -u user      only display for user specified
     %P -y -Y -D     only unprinted / all jobs / printed jobs
     %P -Pn  set printer screen size to n lines
     %P -Cnnnn       set class code to nnnn
     %P -Rn  set refresh time to n
     %P -b -B -m -M  boxes/no boxes help/error messages"

[
Help messages etc for xmspq
]
H{xmspq help main screen} = 1
"You are looking at the main screen of the spool queue viewer
Choose:
     Options - to select different things to view
     Action  - to perform a single action like deleting a job
               or stopping and starting printers
     Jobs    - to view and change job parameters
     Printers- to view and change printer parameters
     Search  - to find particular jobs or printers
     Help    - to view this screen"

H{xmspq view job help} ++
"You are looking at the text of a job.
Each separate page is marked off by a horizontal line.
Use the scrollbars to move through the file.
You can also choose:

     Job     - to quit back to the main menu
     Search  - to look for particular strings
     Page    - to set page ranges
     Help    - to view this screen"

H{xmspq log view menu} ++
"You are looking at the system error log file.
This file is updated when things go wrong, possibly
if problems occur on a printer on this host."

H{xmspq job list help} ++
"This is the list of jobs, or jobs visible to you.
Please ""click"" on one you want to examine or change."

H{xmspq ptr list help} ++
"This is the list of printers, or printers visible to you.
Please ""click"" on one you want to examine or change."

H{xmspq options help} ++
"Options menu - Choose from:
    View options - Select what you
                   want to see
    Save options - save configuration for
    View system error log
    Exit from %P"

H{xmspq action help} ++
"Action menu - Choose from:
    Abort and/or delete job.
    One more copy of job.

    Start printer
    Halt printer at end of job.
    Halt printer immediately
    Approve printer alignment page
    Reject printer alignment page"

H{xmspq jobs help} ++
"Job menu - Choose from:
    View job
    Set form type, title, printer,
      priority and copies.
    Set pages to be printed
    Set user to post to and/or mail options.
    Set how long job is to be retained
    Set class codes for job

    Unqueue job - save in file to resubmit
      later."

H{xmspq ptr help} ++
"Printer menu - Choose from:
    Interrupt - pause current job to let
                higher priority job print
    Abort - abort whatever is being printed
    Restart - restart from beginning

    Set form type
    Set Class code

    Add new printer
    Delete printer"

H{xmspq search help} ++
"Search menu - Choose from:
    Search - for particular string
    Search forwards - using same
      parameters as last time
    Search backwards - using same
      parameters as last time"

H{xmspq help help} ++
"Help menu - Choose from:
    Help - general information
    Help on - specific menu item
    About - display version information"

H{xmspq job macro help} ++
"Job macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with job number as parameter
    set up elsewhere in this file with codes 27000+"

H{xmspq ptr macro help} ++
"Printer macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with printer name as parameter
    set up elsewhere in this file with codes 27100+"

H{View job menu} = ROUND(@, 20)
"Job menu - Choose from:
    Update page information (as set
      whilst viewing job) and exit.
    Exit without update."

H{View search menu} ++
"Search menu - Choose from:
    Search - for particular string
    Search forwards - using same
      string as last time
    Search backwards - using same
      string as last time"

H{View page menu} ++
"Page menu - Choose from:
    Set current as starting page
    Set current as end page
    Set current as ""halted at"" page
    Reset pages to begining/end of file"

H{View help menu} ++
"Help menu - Choose from:
    Help - general information
    Help on - specific menu item"

H{Log file menu} ++
"File menu - Choose from:
    Delete error log and exit
    Exit back to main menu"

H{Log help menu} ++
"Help menu - Choose from:
    Help - general information
    Help on - specific menu item"

H{xmspq dopts help} = ROUND(@, 20)
"Use this menu to control the display of jobs or printers
which you can see. You can limit the display to particular
printers, and jobs which could in be principle be printed
on the given printers, and you can restrict the view to
a particular user or class code set.
The buttons let you scroll through lists of users and
printers if you want to save typing them in."

H{xmspq search menu help} ++
"Use this menu to set up which kind of search you want to
do for particular jobs or printers.
You can also set up whether you want the search to wrap around
and whether you want exact upper/lower-case matches of strings.
For jobs you can search for some or all of the title, the form
type, the printer name and the user. In the case of user names,
the whole of the name is matched, so it won't find ""oo"" in ""root"",
you'll have to spell it out completely as ""root"".
For printers you can search for the printer name, the device name
and the form type. Again the whole of the device name must be
specifed.
Use ""."" to signify a wild card."

H{xmspq form type menu} ++
"Use this menu to view and change the form type, the number
of copies, the priority, the title and the printer fields in
the selected job.
Use the arrows to scroll up and down the number of copies
and the priority. (If you are not allowed to change the
priority on the queue the arrows will not work)."

H{xmspq pages menu} ++
"Use this menu to set the page range you want printed
in the job, and whether you want only odd or even pages.
You can also set a flags field for the postprocessing
filter, if any.
Use the arrows to scroll up and down the page range.
You may want to view the job and set the page ranges
from there if you want."

H{xmspq user menu} ++
"Use this menu to set up the user the job is to be posted
to (i.e. the one which will appear on the banner page),
and the mail and write completion and attention messages."

H{xmspq retain dlg} ++
"Use this menu to select whether the job is to be retained
after printing, and an optional hold time.
You can also set or reset the ""has been printed"" flag.
The two times are the times in hours for which the job
will be held on the queue if it has or hasn't been printed.
After that time it will be automatically deleted.
Use the various arrow buttons to scroll these times and also
the time and date to hold up printing for."

H{xmspq class menu} ++
"Use this menu to set the class codes for the job, and
and whether it must be printed locally.
You cannot change class code bits not in your class
unless you are privileged."

H{xmspq unqueue help} ++
"Use this menu to ""unqueue"" a job - save it away in a file
for editing and reprint as needed. A command file is
created which contains a shell script with all the options
of the job you are saving."

H{Ptr class dlg} ++
"Use this menu to set the class codes for the printer, and
and whether it is to be regarded as ""local only"".
You can also set upper and lower job sizes which the
printer will take. This is in number of characters, not
pages.
You cannot change class code bits not in your class
unless you are privileged."

H{Ptr add dlg}++
"Use this menu to add details of a new printer on this
machine. Note that you must separately create a ""setup file""."

H{xmspq view search dlg} ++
"Use this menu to set up details of a search string you want
to scan the job data for."

H{Ptr pdev dlg} ++
"Use this dialog to reset the device field of a printer, or
the address of network printers and/or the network marker."

H{Please enter printer name} ++
"Please enter a printer name - you can select a possible
one if displayed by clicking on the list item."

H{Printer name or null} ++
"Please enter a printer name if required.
You don't have to put one in if you don't care about
which printer."

H{Form type selection} ++
"Please enter a form type - you can select a possible
one if displayed by clicking on the list item."

H{xmspq abort buthelp} ++
"This button aborts the selected job if printing and deletes
it from the queue. Confirmation is requested if required
(See view options)."

H{xmspq onemore buthelp} ++
"This button asks for one more copy of the selected job."

H{xmspq jform buthelp} ++
"This button lets you select the form type, printer, title,
priority and copies for the job."

H{xmspq view buthelp} ++
"This button displays the contents of the
selected job on the screen."

H{xmspq pform buthelp} ++
"This button changes the form type on
the selected printer."

H{xmspq pgo buthelp} ++
"This button starts the selected printer."

H{xmspq pheoj buthelp} ++
"This button halts the selected printer at the
end of the current job."

H{xmspq pstp buthelp} ++
"This button halts the selected printer at once."

H{xmspq pok buthelp} ++
"This button approves the alignment page on the
selected printer."

H{xmspq pnok buthelp} ++
"This button disapproves the alignment page on the
selected printer."

H{xmspq job display dlg} ++
"Use this dialog to set up the job display format.
You can add or remove fields from the display or vary the
size of existing fields."

H{new job field dlg} ++
"Use this dialog to create a field in the job display
As well as choosing the field you can set the width.
You can also set the ""backtab"" marker so if a field exceeds
the width it can backspace over the previous field."

H{new job sep dlg} ++
"Use this dialog to create a separator string in the job display
If you just accept nothing a single space will be used."

H{xmspq view opts dlg} ++
"Use this dialog to set up the variable display"

H{xmspq new field dlg} ++
"Use this dialog to create a field in the variable display
As well as choosing the field you can set the width.
You can also set the ""backtab"" marker so if a field exceeds
the width it can backspace over the previous field."

H{xmspq new sep dlg} ++
"Use this dialog to create a separator string in the variable display
If you just accept nothing a single space will be used."

H{xmspq save help file help} ++
"Please use this dialog to save the new help file
containing the new screen formats."

P,H{Sure about deleting not printed} = ROUND(@, 10)
"Job has not been printed. Are you sure?",
"This job has not been printed so you are being
asked for confirmation before it is deleted.
If you don't want to be asked again, select
""view options"" from the main menu."

P,H{Sure about deleting printed job} ++
"Sure about deleting job?",
"Please confirm whether you want to delete this job.
If you don't want to be asked again, select
""view options"" from the main menu."

P,H{xmspq bypass align} ++
"Bypass alignment for printer? ",
"Please confirm that you want to turn off
the ""non-aligned"" status on this printer
without running an alignment routine first."

P,H{xmspq reinstate align} ++
"Rerun alignment for printer? ",
"Please confirm that you want to set this
printer's status to non-aligned."

P,H{xmspq not ptd lt ptd} ++
"\"Not printed\" delete time < \"printed\" delete time?",
"It seems surprising that you want to delete the job
automatically if it HASN'T been printed sooner than
if it has, so I just thought I'd ask."

P,H{xmspq confirm delete printer} ++
"Are you sure that you want to delete this printer?",
"Please confirm that you indeed want to
delete this printer. (I always ask)."

P,H{Device does not exist} ++
"Device/file does not exist",
"You have given a device which does not exist.
This isn't necessarily wrong, but you'll have
to set it up before you go any further."

P,H{Device is block device} ++
"Device is a block device",
"You have given a device which is a so-called
""block"" device (e.g. disk drive or tape).
You should normally give a ""tty"" or ""lp"" device."

P,H{Device is flat file} ++
"Device is a \"regular\" file",
"You have given the name of a straight disk file
and print jobs will silently append to the end of
it. Are you sure that this is right?"

P,H{Device is FIFO} ++
"Device is a FIFO",
"The device name you have given is a so-called
""FIFO"" - named pipe. You MUST remember to set up
a program to ""catch"" the output from the FIFO if
you are to use this."

P,H{Device not owned} ++
"Device is not owned by spooler user",
"The device name you have given is owned by the user
name '%u8', however I think that %U can write to it.
You may want to change this at some stage to avoid
two or more processes writing to it."

P,H{Device not writeable} ++
"Device is not writeable by spooler user",
"The device name you have given is owned by the user
name '%u8' and I don't think ""%U"" can write to it.
You will need to attend to this before printing can
take place."

P,H{Owned but not writable} ++
"Device is owned but not writeable by spooler user",
"Although the device name is apparently owned by the %U
user, the permissions on it don't seem to allow writing.
Please look at this before you start trying to print on it."

P,H{Clear syserr log} ++
"Are you sure you want to clear the system error log?",
"Please confirm that you really do want to clear out the
system error log."

E,H{Null user} = 300
"You cannot specify a null user name here",
"You must send the output to some user or other.
Please fill in a user and try again."

E,H{Null printer} ++
"You cannot specify a null printer name here",
"You must give the printer a name of some kind
Please fill in a name and try again."

E,H{Null device name} ++
"You must put a device name in here.",
"You must give a device name (or distinguishing
port number etc for network printers) to allocate
to the printer. Please try again."

E,H{Null form name} ++
"You must put a form name in here.",
"You must give a form name to allocate
to the printer. Please try again."

E,H{Ptr null class} ++
"Null class code",
"You cannot have no class code bits set.
That would make the printer invisible and unusable.
Please set at least one bit."

E,H{Xmspq null class} ++
"Null class code.",
"You cannot have nothing set in the class code or
you wouldn't be able to see anything.
Please set at least one class code bit and try
again."

E,H{Rsearch ptr not found} ++
"No such printer found (searching backwards)",
"I couldn't find a printer matching your criteria
searching backwards. (Remember you may not be
able to see all of them)."

E,H{Fsearch ptr not found} ++
"No such printer found (searching forwards)",
"I couldn't find a printer matching your criteria
searching forwards. (Remember you may not be
able to see all of them)."

E,H{Rsearch job not found} ++
"No such job found (searching backwards)",
"I couldn't find a job matching your criteria
searching backwards. (Remember you may not be
able to see all of them)."

E,H{Fsearch job not found} ++
"No such job found (searching forwards)",
"I couldn't find a job matching your criteria
searching forwards. (Remember you may not be
able to see all of them)."

E,H{No ptr crits given} ++
"No printer search criteria given",
"You haven't clicked in one of the boxes to
select which criteria to search for about
printers - name, device or form.
Please do so and try again."

E,H{No job crits given} ++
"No job search criteria given",
"You haven't clicked in one of the boxes to
select which criteria to search for about
jobs - title, form, user or printer
Please do so and try again."

E,H{No previous search} ++
"No previous search string found",
"You cannot repeat the last search without
first giving a string to search for.
Please try the ""search for"" option first."

E,H{Null search string} ++
"No search string given",
"You didn't give a string to search for.
Please try again."

E,H{No job selected} ++
"You didn't select a job.",
"You must select a job by clicking the mouse
cursor over the required job before you can
carry out a job-specific operation."

E,H{No jobs to select} ++
"No jobs to select",
"There aren't any jobs to do anything with.
(Actually there may be, but you may not be
able to see them)."

E,H{Too many copies} ++
"Attempt to exceed maximum number of copies",
"You are not permitted to ask for more than
%d1 copies at a time. Sorry!"

E,H{xmspq setting zero class} ++
"Attempt to set zero class code",
"You cannot have no class code bits set.
That would make the job invisible and unprintable.
Please set at least one bit."

E,H{xmspq setting zero class ptr} ++
"Attempt to set zero class code",
"You cannot have no class code bits set.
That would make the ptr invisible and unusable.
Please set at least one bit."

E,H{xmspq no directory} ++
"No directory name given.",
"You need to specify a directory to save the
job in. Press the select directory button to
scan the directories."

E,H{Not a directory} ++
"Not a directory",
"The name you have given isn't a directory.
Please try again."

E,H{xmspq no cmd file} ++
"No command file given",
"You must specify a file to use as a command file.
This will be a shell script which when run will
recreate the job."

E,H{xmspq no job file} ++
"No job file given",
"You must specify a file to use as a job file.
This file will contain a copy of the job data."

E,H{No printer selected} ++
"You didn't select a printer.",
"You must select a printer by clicking the mouse
cursor over the required printer before you can
carry out a printer-specific operation."

E,H{No printer to select} ++
"No printers to select",
"There aren't any printers to do anything with.
(Actually there may be, but you may not be
able to see them)."

E,H{Printer is remote} ++
"Printer is remote",
"The printer which you are trying to delete
is located on the remote machine %s. You can
only add or delete printers on the machine which
they are on."

E,H{Minprint gt maxprint} ++
"Minimum print size > Maximum print size",
"You have attempted to give a minimum print size
limit which is greater than the maximum.
Please reset one or both (0 means no limit)."

E,H{Null ptr name} ++
"No printer name",
"You must give the printer some sort of name.
Please try again."

E,H{No printer list access} ++
"You do not have permission to access the printer list",
"Your user account does not provide for permission to access
the printer list. You must ask someone else to do this."

E,H{No perm change class} ++
"You cannot change printers",
"You don't have permission to change printers'
class codes - Sorry!"

E,H{No perm change dev} ++
"You cannot change device",
"You don't have permission to change the device on printers.
Sorry about that (you need add/delete privilege)."

E,H{Cannot change dev remote} ++
"Cannot change device on remote printers",
"You cannot change the device on remote printers.
If you need to do this please rlogin to the other machine and
try it from there."

E,H{View op in progress} ++
"View operation in progress",
"Sorry but you can only view one job at once
as it burns up the network/X resources etc."

E,H{Cannot obtain network job} ++
"Cannot obtain job from network",
"I am having difficulty obtaining a copy of
the job `%t:%d0' (%s). Maybe there is network
trouble."

E,H{Cannot open local job} ++
"Cannot open job file",
"I cannot open the file for job %d0.
Maybe someone just deleted it."

E,H{No mem for syslog} ++
"No memory for system error log file buffer",
"Running out of memory for the system error
log file. Maybe there is a leak somewhere??"

E,H{xmspq job not readable} ++
"Job not readable",
"This job, `%s, ' belongs to %t and not to you,
and you don't have permission to look at other
user's jobs. Sorry!"

P,H{xm save options} ++
"Write options to file",
"This will write your currently-selected options
to a resources file ~/XI for selection next time."

E,H{xmsq empty macro command} ++
"Empty macro command",
"You didn't give a command to run. If you've run this before,
you may be able to select the last command from the drop-down"

##  XMSPUSER

[
Help messages etc for xmspuser and xspuser
]
H{xmspuser main screen help} = 1
"You are looking at the main screen of the user privilege viewer
Choose:
     Options - to select different order
     Defaults- to select default parameters
     Users   - to select user parameters
     Charges - to select user changes
     Help    - to view this screen"

H{xmspuser defaults help} ++
"This shows the default options to be supplied to new users"

H{xmspuser users help} ++
"This shows the options for the listed users.
Please ""click"" on one you want to change."

H{xmspuser options help} ++
"Options menu - Choose from:
    Display order - choose from alphabetic
                    or user-id order
    Save options - save configuration
    for next time.
    Exit from xmspuser"

H{xmspuser defaults menu help} ++
"Defaults menu - sets defaults for new users
Choose from:
    Priorities - set default limits of priority
                    and copies
    Forms      - set default form type
    Class      - set default class codes
    Privileges - set default privileges

    Copy       - copy defaults to existing users"

H{xmspuser users menu help} ++
"Users menu - Select one or more users and choose from:
    Priorities - set limits of priority and copies
    Forms      - set default form type
    Class      - set class codes
    Privileges - set privileges

    Copy       - copy defaults to selected users"

H{xmspuser charges help} ++
"Charges menu - Choose from:
    Zero       - Set charges for selected users
                 to zero
    Zero all   - Set charges for all users to zero
    Impose     - Add a value to selected users'
                 current charge."

H{xmspuser search help} ++
"Search menu - Choose from:
    Search - for particular string
    Search forwards - using same
      parameters as last time
    Search backwards - using same
      parameters as last time"

H{xmspuser help help} ++
"Help menu - Choose from:
    Help - general information
    Help on - specific menu item
    About - display version information"

H{xmspuser macro help} ++
"User macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with user names as parameters
    set up elsewhere in this file with codes 27000+"

H{xmspuser action help} = ROUND(@, 10)
"This menu lets you select the order in which
user names are displayed.
You can choose from:
    Sort by order of numeric user id
    Alphabetic order of user name"

H{xmspuser system pris} ++
"This menu selects the priorities which will be
automatically assigned to new users.
Use the slides to select:
    Minimum priority
    Default priority
    Maximum priority
    Maximum number of copies the user can select
            at one time.

You can also set defaults for privileges relevant
to priorities:
    Change priority of queued job
    Set any priority on queue (bypassing limits)
    Set own default priority and form type"

H{xmspuser user pris} ++
"This menu selects the priorities for the selected users

Use the slides to select:
    Minimum priority
    Default priority
    Maximum priority
    Maximum number of copies the user can select
            at one time.

You can also set privileges relevant to priorities:
    Change priority of queued job
    Set any priority on queue (bypassing limits)
    Set own default priority and form type"

H{xmspuser system class} ++
"This menu selects the class codes which will be
automatically assigned to new users.

Use the ""Set All"" and ""Clear all"" buttons as required
but you must always set at least one class bit.

You can also set a default for a privilege relevant
to class codes:

    User can ""override"" the class by adding in bits."

H{xmspuser user class} ++
"This menu selects the class codes for the selected users

Use the ""Set All"" and ""Clear all"" buttons as required
but you must always set at least one class bit.

You can also set a privilege relevant to class codes:

    User can ""override"" the class by adding in bits."

H{xmspuser system privs} ++
"This menu selects the privileges which will be
automatically assigned to new users."

H{xmspuser user privs} ++
"This menu selects the privileges for the selected users
Use the ""Set Default"" button to set to the defaults."

H{xmspuser addfee help} ++
"Use this menu to add a constant fee to the charges
for all of the selected users."

H{xmspuser system forms} ++
"Use this menu to set the default form type which will
be automatically assigned to new users.
The list gives all known default user form types
but you are welcome to type in a completely new one if
you wish."

H{xmspuser user forms} ++
"Use this menu to set the default form type for
the selected users.
The list gives all known default user form types
but you are welcome to type in a completely new one if
you wish."

H{xmspuser system ptrs} ++
"Use this menu to set the default printer which will
be automatically assigned to new users.
The list gives all known default printers
but you are welcome to type in a completely new one if
you wish."

H{xmspuser user ptrs} ++
"Use this menu to set the default printer for
the selected users.
The list gives all known default printers
but you are welcome to type in a completely new one if
you wish."

H{xmspuser system forms allowed} ++
"Use this menu to set the default form type restriction
which will be automatically assigned to new users.
The list gives all known defaults
but you are welcome to type in a completely new one if
you wish."

H{xmspuser user forms allowed} ++
"Use this menu to set the default form type restriction
for the selected users.
The list gives all known defaults
but you are welcome to type in a completely new one if
you wish."

H{xmspuser system ptrs allowed} ++
"Use this menu to set the default printer restriction
be automatically assigned to new users.
The list gives all known defaults
but you are welcome to type in a completely new one if
you wish."

H{xmspuser user ptrs allowed} ++
"Use this menu to set the default printer restriction
for the selected users.
The list gives all known defaults
but you are welcome to type in a completely new one if
you wish."

H{xmspuser search menu help} ++
"Use this menu to set up which kind of search you want to
do for particular users.
You can also set up whether you want the search to wrap around
and whether you want exact upper/lower-case matches of strings.
Use ""."" to signify a wild card."

P,H{xmspuser zero all charges} = ROUND(@, 100)
"Zero charges for all users",
"I just thought I'd ask before zero-ing charges for
all known users, as this is a bit drastic!"

P,H{xmspuser zero charges} ++
"Zero charges for selected users",
"I just thought I'd ask before zero-ing charges for
the selected users, as this is a bit drastic!"

P,H{Xmspuser confirm rebuild} ++
"Rebuild user control file?",
"The user control file may be out of sync with the
password file, with some new users omitted or old ones
still included. Please indicate whether you want to
rebuild it. (This may take some time and will hold up
other users)."

E,H{No users selected} ++
"No users selected - please choose some",
"Please select one or more users by clicking the left mouse
button on the users, using Ctrl+left button to select more
users, then try this again."

E,H{xmspuser min gt max} ++
"Minimum priority > Maximum priority",
"You are trying to set a minimum priority greater
than the maximum. I think that this is a mistake."

E,H{xmspuser def lt min} ++
"Default priority < Minimum priority",
"You are trying to set a default priority less
than the minimum. I think that this is a mistake."

E,H{xmspuser def gt max} ++
"Default priority > Maximum priority",
"You are trying to set a default priority greater
than the maximum. I think that this is a mistake."

E,H{xmspuser null form type} ++
"Null form type",
"You aren't allowed to have empty form types
Please type in some kind of form type."

E,H{xmspuser zero class} ++
"Zero class code",
"You aren't allowed to have all the class code
bits set to zero as the jobs would be invisible and
unprintable."

E,H{xmspuser invalid charge} ++
"Invalid charge field",
"You haven't put a valid numeric charge in this field.
Please try again."

E,H{Fsearch user not found} ++
"User not found",
"No matching user was found searching forwards."

E,H{Rsearch user not found} ++
"User not found",
"No matching user was found searching backwards."

E,H{No admin file permission} ++
"Sorry - you do not have the privilege to edit the admin file
or adjust charges records.",
"You do not have any permission to edit the admin file and
adjust the charges records which you do for this operation"

E,H{xmspq app-default cannot open} ++
"Trouble opening application defaults file",
"I don't seem to be able to open your application defaults file"

E,H{xmspq app-default cannot create} ++
"Trouble creating application resource file",
"I don't seem to be able to a new application defaults file"

# Rest of the errors

[
More spq errors
]
E{spq unknown command} = 3000	"Unknown command - expecting job queue control"
E{spq job off bottom} ++	"You have gone off the end of job queue"
E{spq job off top} ++		"You have gone off the beginning of job queue"
E{spq no jobs to process} ++	"There are no jobs on the queue!"

E,H{spq job not yours} ++
"This job does not belong to you (%R).",
"This job, `%s, ' belongs to %t and not to you,
and you don't have permission to edit other
user's jobs. Sorry!"

E{spq cannot reprio} ++
"You are not allowed to change the priority of jobs on the queue.
Please leave it alone or ask someone who is."

E{spq cannot select forms} ++
"New form type does not match your limit, which is:
pattern: %s"

E{spq cannot select ptrs} ++
"New printer does not match your limit, which is:
pattern: %s"

E{spq bad range format} ++
"Bad range format, should be start-end, -end, start- or null."

E,H{spq cannot unqueue} ++
"You are not allowed to unqueue jobs",
"You do not have the required privilege to unqueue jobs"

E{spq cannot look at job} ++
"This job does not belong to you (%R)
and you can't even look at it - sorry."

E,H{spq no remote job priv} ++
"This job is a remote job and you are not permitted
to access remote jobs - sorry.",
"Your user permissions do not allow you to access
remote jobs. Sorry!"

E{spq no access privilege} ++
"Sorry but you do not have a privilege to access
other fields in the job from those displayed."

E{No space for unqueue box} ++
"There is no space on the screen to create a subwindow for job unqueue
This is probably a bug or you are running a funny terminal."

E{No subwin for unqueue} ++
"Failed to create subwindow for job unqueue - sorry"

E{No home directory for user} ++
"Unknown user directory %s"

E{Bad envirs in unq} ++
"Undefined environment variables in %s"

E,H{Not absolute path} ++
"Sorry - but an absolute pathname is required for Unqueue options.
You have supplied %s",
"You must give the directory as an ""absolute""
path name (starting with a '/') as the current
directory may change."

E{Unqueue no chdir} ++		"Cannot change to directory %s"
E{Unqueue bad file name} ++	"%s is not a valid file name (contains '/')"
E{Unqueue not flat file} ++	"%s is not a regular file"

E,H{Unqueue no fork} ++
"Cannot fork to run unqueue process",
"Could not run a subprocess to dump out the job
file as specified. Either the system is loaded
heavily or you are running out of processes
(Remember that each Xterm process, xbiff, xman,
xclock etc is a separate process)."

E,H{Cannot find unqueue} ++
"Cannot find unqueue process.
This should be located in `%s'.
Please ensure that your copy of GNUSpool is correctly installed.",
"I am looking for a special program to ""unqueue"" your job
which is normally located in %s, but I cannot find it."

E,H{Unqueue program fault} ++
"Program fault 0x%x9 in unqueue process",
"The unqueue process failed with a signal of 0x%x9.
I do not know why this happenned."

E,H{Unqueue misc error} ++
"Misc error %d1 in unqueue process for job `%s' (%d0)",
"Undefined error code %d1 given by unqueue process,
do not know why."

E,H{Unqueue spool not found} ++
"Unqueue spool file not found for job `%s' (%d0)",
"I was trying to unqueue your job but was unable to
gain access to the job file - sorry."

E,H{Unqueue dir not found} ++
"Unqueue directory not found for job `%s' (%d0)",
"I cannot change directory to target for unqueue
please check permissions involved."

E,H{Unqueue no create} ++
"Cannot create file for job `%s' (%d0)",
"Cannot create output files to unqueue jobs.
Maybe the directory cannot be written to?"

E,H{Unqueue unknown job} ++
"Unqueue process reports unknown job `%s' (%d0)??",
"Unqueue process says the job is not known.
Possibly someone got in an deleted it??"

E{spq ptr unknown cmd} ++
"Unknown command - expecting printer list control"

E{spq plist off bottom} ++
"You have gone off the end of the printer list"

E{spq plist off top} ++
"You have gone off the beginning of the printer list"

E{No printers yet} ++
"There are no printers yet - please set one up first"

E{No halt go priv} ++
"Sorry - to obey this command you need permission to stop and start
printers. Please ask someone who has this permission."

E,H{No add priv} ++
"You (%R) do not have permission to add a printer.
Please ask someone else.",
"Your user account does not permit the addition or
deletion of printers"

E,H{Printer not running} ++
"This printer is not running.",
"The printer is halted already, so
you cannot perform this operation."

E,H{Printer is running} ++
"The printer should be stopped before you can execute this command.
Please stop it first.",
"The printer should be stopped before you can execute this command.
Please stop it first."

E,H{Printer not aw oper} ++
"The printer is not awaiting confirmation - this command is not
appropriate.",
"The printer isn't waiting for alignment
pages to be approved/disapproved, so you
cannot perform this operation."

E{Cannot delete remote printers} ++
"Sorry but you cannot delete printers remotely. If you really want to do
this, you should rlogin to the remote machine and do it from there"

E,H{No remote ptr priv} ++
"Sorry but you are not permitted to access remote printers.",
"Your user account does not have the permission to
access printers other than on the local machine"

E{spq zero code error} ++
"Resulting printer class, after applying your class of %s, would be zero.
Please try again."

E{spq view job error} ++
"This job, number %d0, has just disappeared as you started
to look at it! Please type return to continue. If it still appears
on the job queue when you go back something very strange is happening.
In that case check the file ownerships, environment variables
(have you got a funny $SPOOLDIR set?) etc."

E{spq view job unknownc} ++
"Unknown command - expecting scroll command.
Type 'q' to quit back to main job queue."

E{job view at bof} ++		"You are at the beginning of the file"
E{job view at eof} ++		"You are at the end of the file"
E{job view at lhs} ++		"You are at the l.h. margin of the file"
E{job view at rhs} ++		"You are at the r.h. margin of the file"
E{spq view bad hat} ++
"This job was NOT aborted in mid-print. This command is not
appropriate. Can I suggest `s' instead to reset the job
start page?"

E,H{spq view start after end} ++
"Your start page comes after your end page. Please set the
end page.",
"You have set up a start page which comes after your end page.
Please reset one or the other so that they are either the same
or start is less than the end."

E,H{spq view hat after end} ++
"Your ""halted at"" page comes after your end page. Please set the
end page.",
"You have set up a ""halted at"" page which comes after your end page.
Please reset one or the other so that they are either the same
or ""halted at"" is less than the end."

E{spq view ss not found} ++	"No occurences found."

E,H{spq view fs not found} ++
"No occurences found (searching forwards).",
"I couldn't find your string
searching forwards."

E,H{spq view bs not found} ++
"No occurences found (searching backwards).",
"I couldn't find your string
searching backwards."

E{spq view remote job access} ++	"Unable to access job `%s' %d0 on remote machine `%t'"

E,H{spq page break memory error} ++
"Unable to allocate enough memory for page breaks in
job `%s' %d0 size $d1.",
"Not enough memory for page breaks in view job.
Your job must be very large or you are running out of
RAM."

E,H{Null job in view} ++
"Your job is zero length",
"Your job appears to be of zero length and cannot be
viewed."

E,H{No log file yet} ++
"No system error log file (yet!)",
"There is nothing in the system error log
file (for this machine) to view.
Nothing is wrong - don't worry!"

E{spq j log file unkc} ++
"Unknown command whilst looking at system error log.
Type `q' to get back to the job queue."

E{spq p log file unkc} ++
"Unknown command whilst looking at system error log.
Type `q' to get back to the printer list."

E{Log file nomem} ++
"No memory available to scan system error log file -
sorry! (It must be getting rather big)."

E{Too many printers displayed} ++
"You are asking for too many printers (%d0) to be displayed.
The maximum supported is %d1."

E{Poll parameter out of range} ++
"Your polling parameter %s is invalid.
Please give a number between %d0 and %d1."

E,H{Macro error} = ROUND(@, 10)
"Macro command %d0 is not defined.",
"There is no macro command or prompt defined with code %d0
to denote the action to be taken."

E,H{Macro fork failed} ++
"Macro command failed to fork - %E",
"Failed to do a ""fork"" to run a macro.
Your machine is running out of processes."

E,H{Macro command gave signal} ++
"Macro command failed - signal %d0",
"Your macro command failed with some signal."

E,H{Macro command error} ++
"Macro command failed - exit code %d0",
"Your macro failed, giving an exit code
of %d0. I don't know what this means!"

######################################################################
##  Mostly stuff for spuser but some of the common stuff above
##  applies

[
Spuser interactive state
]
H,S{spuser interactive state} = 1
"?             Help (this screen)
q             Quit
^L            Redraw screen

j k           down up
^F ^B ^D ^U   Page down/up, half page down/up

d l u         Set default, min, max priority
f o p n s     Set form type, ptr, privileges,
                  max copies, class code
r g           Set form and printer restrictions

D L U F P N S O R G Set system defaults.

a             Copy system default priorities to user
A             Copy system default priorities to all users

c             Set charge"

({spuser interactive state})K{spuser key user def pri} = 500	"d"
({spuser interactive state})K{spuser key sys def pri} ++	"D"
({spuser interactive state})K{spuser key user min pri} ++	"l"
({spuser interactive state})K{spuser key def min pri} ++	"L"
({spuser interactive state})K{spuser key user max pri} ++	"u"
({spuser interactive state})K{spuser key def max pri} ++	"U"
({spuser interactive state})K{spuser key def user form} ++	"f"
({spuser interactive state})K{spuser key sys def form} ++	"F"
({spuser interactive state})K{spuser key user restrict form} ++	"r"
({spuser interactive state})K{spuser key sys restrict form} ++	"R"
({spuser interactive state})K{spuser key def user ptr} ++	"o"
({spuser interactive state})K{spuser key sys def ptr} ++	"O"
({spuser interactive state})K{spuser key user restrict ptr} ++	"g"
({spuser interactive state})K{spuser key sys restrict ptr} ++	"G"
({spuser interactive state})K{spuser key user priv} ++		"p"
({spuser interactive state})K{spuser key sys priv} ++		"P"
({spuser interactive state})K{spuser key user class} ++		"s"
({spuser interactive state})K{spuser key sys class} ++		"S"
({spuser interactive state})K{spuser key user max copies} ++	"n"
({spuser interactive state})K{spuser key sys max copies} ++	"N"
({spuser interactive state})K{spuser key sys def to all} ++	"A"
({spuser interactive state})K{spuser key user charge} ++	"c,C"
({spuser interactive state})K{spuser key sys def to user} ++	"a"
({spuser interactive state})K{spuser key forward search} ++	"/"
({spuser interactive state})K{spuser key backward search} ++	"\\"

({spuser interactive state})K{spuser key exec} = 600		"@"

[
Spuser privs screen state
]
H,S{spuser privs state} = 2
"In this screen you are editing the privileges for user `%s'.
On each field you can type y, Y, t or T to allow the corresponding
privilege, n, N, f or F to disallow it, ENTER or RETURN to move
to the next line, or Q to quit back to the main screen.
Cursor up/down can be used to move up and down also.
Some privileges imply others - don't be surprised if some change
Above or below the current line.
To initialise to the default privileges, type `D'."

H{spuser def privs state} ++
"In this screen you are editing the default privileges.
These are copied to each new user subsequently created.
You will also be asked at the end whether you want to copy them
to all other users.
On each field you can type y, Y, t or T to allow the corresponding
privilege, n, N, f or F to disallow it, ENTER or RETURN to move
to the next line, or Q to quit back to the main screen.
Cursor up/down can be used to move up and down also.
Some privileges imply others - don't be surprised if some change
Above or below the current line."

({spuser privs state})K{spuser key yes}	= {key yes}	"y,Y,t,T,s,S"
({spuser privs state})K{spuser key no} = {key no}	"n,N,f,F,u,U"
({spuser privs state})K{spuser key toggle} = {key toggle}	"~,!"
({spuser privs state})K{spuser key defs} ++	"D"

[
Spuser change defaults state
]
H,S{spuser cdef state} = 4
"?  Help
q  Quit
^L Redraw screen

d  Set default priority
f  Set form type
p  Set printer type"

({spuser cdef state})K{spuser key own def form} = 500	"f,F"
({spuser cdef state})K{spuser key own def pri} ++	"d,D"
({spuser cdef state})K{spuser key own def ptr} ++	"p,P"

[
Spuser confirm rebuild
]
E,H,S{Spuser confirm rebuild} = 5
"Warning: Your user control file may be out of date
with your password file.
Do you want to rebuild it? (Y or N).
(This may take some time and may hold up other users)",
"Please confirm whether you want to rebuild
the user control file."

E{Spuser rebuilding wait} ++
"***Rebuilding user control file, please wait***"

E{Unknown command askyorn} ++
"Unknown command - please say Y or N do you want to save options"

[
Spuser save options
]
({Spuser confirm rebuild})K{spuser key yes}	"y,Y,t,T,s,S"
({Spuser confirm rebuild})K{spuser key no}	"n,N,f,F,u,U"

[
Spuser copy default classes
]
H,P,S{spuser askyorn} = 40
"Please say (Y or N) whether you want the default class code
be copied to all users INCLUDING you.",
"Copy to all users? "

({spuser askyorn})K{spuser key yes}		"y,Y"
({spuser askyorn})K{spuser key no}		"n,N"

[
Help stuff spuser main screen
]
H{spuser def pri help} = 100
"Please enter the required default priority (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{spuser sys def pri help} ++
"Please enter the required system default priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
default priority of any new user created from now on."

H{spuser min pri help} ++
"Please enter the required minimum priority (between %d0 and %d1) for
user `%s' to replace the existing value of %d2."

H{spuser sys min pri help} ++
"Please enter the required system minimum priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
minimum priority of any new user created from now on."

H{spuser max pri help} ++
"Please enter the required maximum priority (between %d0 and %d1) for
user `%s' to replace the existing value of %d2."

H{spuser sys max pri help} ++
"Please enter the required system maximum priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
maximum priority of any new user created from now on."

H{spuser max copies help} ++
"Please enter the required maximum number of copies (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{spuser sys max copies help} ++
"Please enter the required system maximum number of copies, between %d0
and %d1 to replace the existing value of %d2. This will be inserted as the
maximum number of copies for any new user created from now on."

H{spuser own def pri help} ++
"Please enter a new value for your default priority. This is currently
%d2, and can be any number between %d0 and %d1."

P,H{spuser class code help} ++
"User class code:",
"Please enter a new value for the class code for user `%s'. This is
a string of bits A to P and a to p and controls which jobs and printers the
user has access to (by default).
Type & to clear all * to set all A to P, a to p to set bit."

P,H{spuser sys class code help} ++
"Default class code:",
"Please enter a new value for the default class code.
This value is inserted by default as the class code for new users,
and (if you want) is copied to all existing users when you have finished."

H{spuser user form type help} ++
"Please specify the default form type for user `%s'"

H{spuser sys form type help} ++
"Please enter the system default form type. This will be inserted as the
default for any new user created from now on."

P,H{spuser user form patt} ++
"User form(s):",
"Please enter a form type restriction for user `%s'.
This should be a pattern, e.g. a4.* or alternatives
like a*,form[1-7]."

P,H{spuser sys form patt} ++
"System form(s):",
"Please enter a form type restriction to be given to new users.
This should be a pattern, e.g. a4.* or alternatives
like a*,form[1-7]."

P,H{spuser user ptr patt} ++
"User printer(s):",
"Please enter a printer restriction for user `%s'.
This should be a pattern, e.g. lp* or alternatives
like lp*,ptr[0-3]."

P,H{spuser sys ptr patt} ++
"System printer(s):",
"Please enter a printer restriction to be given to new users.
This should be a pattern, e.g. lp* or alternatives
like lp*,ptr[0-3]."

H{spuser user ptr help} ++
"Please specify the default printer for user `%s'"

H{spuser sys ptr help} ++
"Please enter the system default printer. This will be inserted as the
default for any new user created from now on."

H{spuser new form type help} ++
"Please enter your new default form type."

H{spuser new ptr help} ++
"Please enter your new default printer."

P{Spuser current charge} ++		"Current charge for user %s is %ld"

P,H{Fsearch user} ++
"Forward search user:",
"Please give a user name to search for.
'.' matches any character."

P,E,H{Rsearch user} ++
"Reverse search user:",
"No search string given, please try again.",
"You didn't give a string to search for."

E{Search user not found} ++
"No such user found."

[
Shell level parameters output
]
P{Cdefs params for user} ++		"Parameters for user: %s"
P{Cdefs def pri} ++			"Default priority: "
P{Cdefs def form} ++			"Default form type: "
P{Cdefs def ptr} ++			"Default printer type: "
P{Cdefs prios} ++			"Priority: min %d max %d sys default %d"
P{Cdefs formlim} ++			"Forms restricted to `%s'"
P{Cdefs ptrlim} ++			"Printers restricted to `%s'"

[
Options setting for spuser - prompts
]
P{spuser opt mode} = 700		"Default program mode       :"
P{spuser opt sort} ++			"Sort users                 :"
P{spuser opt helpbox} ++		"Help messages     :"
P{spuser opt errbox} ++			"Error messages    :"
P{spuser opt helpclr} ++		"Clear help message:"

# Initial cursor and starting row

N{spuser opt init cursor} = {spuser opt mode} - 2
N{spuser opt start row} = {spuser opt mode} - 1

[
Initial cursor and start display row for spuser opts
]
({spuser opt init cursor})	N	{spuser opt mode}
({spuser opt start row})	N	{spuser opt mode}

[Order in which prompts happen]
({spuser opt mode})		N	{spuser opt sort}
({spuser opt sort})		N	{spuser opt helpbox}
({spuser opt helpbox})		N	{spuser opt errbox}
({spuser opt errbox})		N	{spuser opt helpclr}
({spuser opt helpclr})		N	-1

P,H{spuser opt mode}*10
"Display on stdout",
"What mode to you want to have as the default
for %P?"

P{spuser opt mode} * 10 + 1		"Edit default form/prio"
P{spuser opt mode} * 10 + 2		"Edit user list"

P,H{spuser opt sort} * 10
"Numeric order of userid",
"How do you want the users to be displayed?
This only applies to full edit of user list."

P{spuser opt sort} * 10 + 1		"Alphabetic order of user name"

P,H{spuser opt helpbox} * 10
"Inverse video",
"Please indicate whether you want to display help messages
in inverse video (standout mode) or boxes."

P{spuser opt helpbox} * 10 + 1		"Boxes"

P,H{spuser opt errbox} * 10
"Inverse video",
"Please indicate whether you want to display error messages
in inverse video (standout mode) or boxes."

P{spuser opt errbox} * 10 + 1		"Boxes"

P,H{spuser opt helpclr} * 10
"Use next command",
"Please indicate whether you want the next keystroke afer
displaying a help message to be kept or lost"

P{spuser opt helpclr} * 10 + 1		"Clear help"

[
Privilege names in full
]
P,N{Priv descr edit admin file} = 1100		"Edit admin file"
P{Priv descr override class} ++			"Override class"
P{Priv descr stop spooler} ++			"Stop spooler"
P{Priv descr use other forms} ++		"Use other forms"
P{Priv descr use other ptrs} ++			"Use other printers"
P{Priv descr change priority on Q} ++		"Change priority on Q"
P{Priv descr edit other users jobs} ++		"Edit other users' jobs"
P{Priv descr select printer list} ++		"Select printer list"
P{Priv descr halt printers} ++			"Halt/restart printers"
P{Priv descr add printers} ++			"Add/delete printers"
P{Priv descr any priority Q} ++			"Set any priority on Q"
P{Priv descr own defaults} ++			"Change own default prio/form"
P{Priv descr unqueue jobs} ++			"Unqueue jobs"
P{Priv descr view other jobs} ++		"View other users' jobs"
P{Priv descr remote jobs} ++			"Edit remote jobs"
P{Priv descr remote printers} ++		"Edit remote printers"
P{Priv descr access queue} ++			"Access queue options"
P{Priv descr freeze options} ++			"Save new default options"

{Priv descr start row} = {Priv descr edit admin file} - 1

[
Select starting row for cursor of list of perms, and initial display row
]
({Priv descr start row}-1)	N	{Priv descr edit admin file}
({Priv descr start row})	N	{Priv descr edit admin file}

[
Define row order for priv descrs on spuser
]
({Priv descr edit admin file})	N	{Priv descr override class}
({Priv descr override class})	N	{Priv descr stop spooler}
({Priv descr stop spooler})	N	{Priv descr use other forms}
({Priv descr use other forms})	N	{Priv descr use other ptrs}
({Priv descr use other ptrs})	N	{Priv descr change priority on Q}
({Priv descr change priority on Q})  N	{Priv descr edit other users jobs}
({Priv descr edit other users jobs}) N	{Priv descr select printer list}
({Priv descr select printer list})   N	{Priv descr halt printers}
({Priv descr halt printers})	N	{Priv descr add printers}
({Priv descr add printers})	N	{Priv descr any priority Q}
({Priv descr any priority Q})	N	{Priv descr own defaults}
({Priv descr own defaults})	N	{Priv descr unqueue jobs}
({Priv descr unqueue jobs})	N	{Priv descr view other jobs}
({Priv descr view other jobs})	N	{Priv descr remote jobs}
({Priv descr remote jobs})	N	{Priv descr remote printers}
({Priv descr remote printers})	N	{Priv descr access queue}
({Priv descr access queue})	N	{Priv descr freeze options}
({Priv descr freeze options})	N	-1

[
Misc strings and prompts for spuser
]
H,P{Copy to everyone but you} = 1200
"Please say (Y or N) whether you want the default privileges to
be copied to everyone other than you (%R)",
"Copy to everyone else (but you)?"

P{Privileges for user} ++	"Privileges for user %s"
P{Default privileges} ++	"Default privileges"
P{User is allowed} ++		"User %s is allowed to"
P{Default is to allow} ++	"Default is to allow users to"
P{Class std} ++			""
P{Class less than} ++		"<DCl"
P{Class greater than} ++	">DCl"
P{Non std class} ++		"N/S Cl"
P{Perm std} ++			""
P{Perm less than} ++		"<DP"
P{Perm greater than} ++		">DP"
P{Non std perm} ++		"N/S P"
P{Spuser default string} ++	"DEFAULT"
P{Spuser yes} ++		"Yes"
P{Spuser no} ++			"No"
P{Spuser more above} ++		"-- %d more above --"
P{Spuser more below} ++		"-- %d more below --"

E{Spuser display values} = ROUND(@, 20)
"Default form %s form types %t
Maximum copies %d0
Priority: min %d1 max %d2 default %d3.  Current charge is %d4."

E{Spuser ptr display values} ++	"Default printer `%s' printer types `%t`"
E{Spuser display class} ++	"Class code: %s"
E{Spuser display privs} ++
"Privileges:
"
P{Spuser you may} ++		"You may"

E{spuser unknown command} = ROUND(@, 100)	"Unknown command"
E{spuser off bottom} ++				"Off end of user list"
E{spuser off top} ++				"Off beginning of user list"
E{spuser cdef unknown cmd} ++			"Unknown command (expecting `f' or `p')"
E{spuser no form type} ++			"Must give some kind of form type"
E{spuser disallowed form type} ++		"Form type not in permitted range"
E{spuser disallowed ptr type} ++		"Printer name not in permitted range"
E{spuser bad cmd setting privs} ++		"Unknown command (please type `Y' or `N')"
E{spuser privs off bottom} ++			"Off end of screen"
E{spuser privs off top} ++			"Off beginning of screen"
E{spuser already editing defs} ++		"You are editing the defaults!"
E{spuser privs missing code} ++			"Missing state code %d9 in privs order e.g. 700N702"
E{spuser privs scrambled} ++			"Jumbled state code in list of program options order"
E{spuser regexp pattern error} ++		"Invalid pattern type - unmatched [ etc"

E{spuser opts bad state code} ++
"Invalid state code number %d9 in display options order in
help file %F"

E{spuser opts duplicated state code} ++
"Duplicated state code %d9 referred to in %d8N%d9 in
list of display options order in help file %F"

E{Spuser options unknown cmd} ++
"Unknown command - setting screen options"

E{spuser options unknown command} ++
"Unknown command - please say Y or N do you want to save options"

E{Spuser no chng form priv} = ROUND(@, 20)
"Sorry - you do not have the privilege to change your form type
or priority. Please ask someone who can edit the admin file
to set this for you."

E{Spuser no admin priv} ++			"Sorry - you do not have the privilege to edit the admin file."

[
Argument defs for spuser
]
A{spuser explain} = 300		"?,help,explain"
A{spuser display} ++ COPY	"d,display"
A{spuser formprio} ++ COPY	"c,form-and-prio"
A{spuser admin} ++ COPY		"i,administer"
A{spuser sort user} ++ COPY	"u,user-sort"
A{spuser sort uid} ++ COPY	"n,numeric-sort"
A{spuser keepchar} ++ COPY	"H,keep-char-help"
A{spuser losechar} ++ COPY	"h,lose-char-help"
A{spuser help box} ++ COPY	"b,help-box"
A{spuser no help box} ++ COPY	"B,no-help-box"
A{spuser error box} ++ COPY	"m,error-box"
A{spuser no error box} ++ COPY	"M,no-error-box"

[
Spuser explain
]

E{spuser options} = 3
"You may invoke the following options for %P

     %P -d   display current privileges
     %P -c   change default form type and/or priority
     %P -i   edit admin file (if permitted)
             append -n (default) or -u to sort into numeric id
             or alphabetic id order.
     -h loses -H keeps char after help screen
     %P -b -B -m -M  boxes/no boxes help/error messages"

S{Wnum no field name} = 4

[
Strings for wnum etc
]
E{wgets string error} = 4000		"No defaults found with prefix %s"
E{wgets string too long} ++		"String too long"
E{wgets unknown command} ++		"Unknown command (expecting string)"
E{wgets invalid char} ++		"Invalid character (expecting string)"

E{wnum value too large} = ROUND(@, 10)
"Value for %s is too large. (Maximum is %d1) Please try again."
E{wnum value too small} ++
"Value for %s is too small. (Minimum is %d0) Please try again."
E{wnum unknown cmd} ++
"Unknown command (expecting numeric value for %s)"

E{wnum invalid char} ++
"Invalid character (expecting numeric value for %s)"

E{wnum value too large} + {Wnum no field name}
"Value is too large. (Maximum is %d1) Please try again."
E{wnum value too small} + {Wnum no field name}
"Value is too small. (Minimum is %d0) Please try again."
E{wnum unknown cmd} + {Wnum no field name}
"Unknown command (expecting numeric value)"
E{wnum invalid char} + {Wnum no field name}
"Invalid character (expecting numeric value)"

E{whexnum size error} = ROUND(@, 10)	"Exceeded field size for %s"
E{whexnum class zero} ++		"Sorry you are not allowed to set a zero class value for %s"
E{whexnum unknown cmd} ++		"Unknown command (expecting class letter for %s)"
E{whexnum invalid char} ++		"Invalid character (expecting class letter for %s)"

E{whexnum size error} + {Wnum no field name}	"Exceeded field size for class value"
E{whexnum class zero} + {Wnum no field name}	"Sorry you are not allowed to set a zero class value"
E{whexnum unknown cmd} + {Wnum no field name}	"Unknown command (expecting class value)"
E{whexnum invalid char} + {Wnum no field name}	"Invalid character (expecting class value)"

# Prompts for xspuser
[
Xspuser column headers
]
P{xspuser user hdr} = 1400	"User"
P{xspuser def hdr} ++ COPY	"Def"
P{xspuser min hdr} ++ COPY	"Min"
P{xspuser max hdr} ++ COPY	"Max"
P{xspuser cps hdr} ++ COPY	"Cps"
P{xspuser form hdr} ++ COPY	"Form"
P{xspuser ptr hdr} ++ COPY	"Ptr"
P{xspuser class hdr} ++ COPY	"Class"
P{xspuser priv hdr} ++ COPY	"Priv"

[
Xspuser dialog frame labels
]

P{xspuser framelab pricps} = ROUND(@, 20) "Priorities and copies"
P{xspuser framelab forms} ++		  "Form types"
P{xspuser framelab printers} ++		  "Printer types"
P{xspuser framelab ccodes} ++		  "Class codes"
P{xspuser framelab privs} ++		  "Privileges"

P{xspuser frametab pricps} = ROUND(@, 5)  "Pri/cps"
P{xspuser frametab forms} ++		  "Forms"
P{xspuser frametab printers} ++		  "Ptrs"
P{xspuser frametab ccodes} ++		  "ClassC"
P{xspuser frametab privs} ++		  "Privs"

P{xspuser deflab} = ROUND(@, 5)		  "Default parameters"
P{xspuser udlgtit} ++			  "User parameters"
P{xspuser ddlgtit} ++			  "Default parameters"
P{xspuser charges dlg} ++		  "User charges"
P{xspuser impose dlg} ++		  "Impose charges"

P{xspuser pcdlg minp} = ROUND(@, 5)	  "Minimum priority"
P{xspuser pcdlg defp} ++		  "Default priority"
P{xspuser pcdlg maxp} ++		  "Maximum priority"
P{xspuser pcdlg cps} ++			  "Max copies"

P{xspuser fmdlg deffm} = ROUND(@, 5)	  "Default formtype"
P{xspuser fmdlg allfm} ++		  "Allowed formtype"

P{xspuser ptrdlg defptr} = ROUND(@, 5)	  "Default printer type"
P{xspuser ptrdlg allptr} ++		  "Allowed printer type"

P{xspuser ccdlg setall} = ROUND(@, 5)	  "Set All"
P{xspuser ccdlg clearall} ++		  "Clear All"

P{xspuser priv setdef} = ROUND(@, 5)	  "Set to default"

P{xspuser app title} = ROUND(@, 5)	  "Set spool user perrmissions"

[
Messages for xspq
]

P{xspq app title} = ROUND(@, 20)	  "GNUSpool Queue Viewer"

P{xspq seq hdr} = ROUND(@, 10)		"Seq"
P{xspq jnum hdr} ++			"Job Num"
P{xspq juser hdr} ++			"User"
P{xspq title hdr} ++			"Title"
P{xspq jform hdr} ++			"Form"
P{xspq reached hdr} ++			"Progress"
P{xspq pages hdr} ++			"Pages"
P{xspq size hdr} ++			"Size"
P{xspq copies hdr} ++			"Cps"
P{xspq pri hdr} ++			"Pri"
P{xspq jptr hdr} ++			"Ptr"
P{xspq wattn hdr} ++			"Wattn"
P{xspq mattn hdr} ++			"Mattn"
P{xspq write hdr} ++			"Write"
P{xspq mail hdr} ++			"Mail"
P{xspq post user hdr} ++		"Post to"
P{xspq cc hdr} ++			"Ccode"
P{xspq delim hdr} ++			"Delim"
P{xspq delimnum hdr} ++			"Ndelim"
P{xspq ppflags hdr} ++			"Post proc"
P{xspq startp hdr} ++			"Start P"
P{xspq hatp hdr} ++			"Hat P"
P{xspq endp hdr} ++			"End P"
P{xspq holdt hdr} ++			"Hold time"
P{xspq oddp hdr} ++			"No odd"
P{xspq evenp hdr} ++			"No even"
P{xspq swapoe hdr} ++			"Swap o/e"
P{xspq delp hdr} ++			"Del time"
P{xspq delnp hdr} ++			"Del np time"
P{xspq subtime hdr} ++			"Submit time"
P{xspq jlocal hdr} ++			"Local"
P{xspq origh hdr} ++			"Orig host"
P{xspq retq hdr} ++			"Retain"
P{xspq supp hdr} ++			"S hdr"
P{xspq printed hdr} ++			"Printed"

P{xspq seq full descr} = ROUND(@, 10)		"Number in Queue"
P{xspq jnum full descr} ++	COPY		"Job Number"
P{xspq juser full descr} ++	COPY		"User owned by"
P{xspq title full descr} ++	COPY		"Job Title"
P{xspq jform full descr} ++	COPY		"Form type"
P{xspq reached full descr} ++	COPY		"Progress"
P{xspq pages full descr} ++	COPY		"Number of Pages"
P{xspq size full descr} ++	COPY		"Size in bytes"
P{xspq copies full descr} ++	COPY		"Copies"
P{xspq pri full descr} ++	COPY		"Priority"
P{xspq jptr full descr} ++	COPY		"Printer"
P{xspq wattn full descr} ++	COPY		"Write Attention"
P{xspq mattn full descr} ++	COPY		"Mail Attention"
P{xspq write full descr} ++	COPY		"Write message on completion"
P{xspq mail full descr} ++	COPY		"Mail message on completion"
P{xspq post user full descr} ++	COPY		"User to post to"
P{xspq cc full descr} ++	COPY		"Class code"
P{xspq delim full descr} ++	COPY		"Delimiter (not impl)"
P{xspq delimnum full descr} ++	COPY		"Number of delimiters (not impl)"
P{xspq ppflags full descr} ++	COPY		"Post processor options"
P{xspq startp full descr} ++	COPY		"Start page"
P{xspq hatp full descr} ++	COPY		"Halted at page"
P{xspq endp full descr} ++	COPY		"End Pend"
P{xspq holdt full descr} ++	COPY		"Hold job until time"
P{xspq oddp full descr} ++	COPY		"No odd pages"
P{xspq evenp full descr} ++	COPY		"No even pages"
P{xspq swapoe full descr} ++	COPY		"Swap odd/even pages"
P{xspq delp full descr} ++	COPY		"Delete time after printing"
P{xspq delnp full descr} ++	COPY		"Delete time not printed"
P{xspq subtime full descr} ++	COPY		"Job submission time"
P{xspq jlocal full descr} ++	COPY		"Local printer only"
P{xspq origh full descr} ++	COPY		"Originating host"
P{xspq retq full descr} ++	COPY		"Retain on queue printing"
P{xspq supp full descr} ++	COPY		"No header page"
P{xspq printed full descr} ++	COPY		"Job has been printed"

P{xspq ptr seq hdr} = ROUND(@, 10)	"Seq"
P{xspq pname hdr} ++			"Printer"
P{xspq dev hdr} ++			"Dev/[IP]"
P{xspq pform hdr} ++			"Form"
P{xspq statemsg hdr} ++			"State:msg"
P{xspq limit hdr} ++			"Lim"
P{xspq pjobno hdr} ++			"Job"
P{xspq puser hdr} ++			"User"
P{xspq pshriek hdr} ++			"Flags"
P{xspq ab hdr} ++			"Ab"
P{xspq pclass hdr} ++			"Ccode"
P{xspq pcomm hdr} ++			"Comment"
P{xspq heoj hdr} ++			"Heoj"
P{xspq pid hdr} ++			"Procid"
P{xspq plocal hdr} ++			"Local"
P{xspq pmsg hdr} ++			"Message"
P{xspq pna hdr} ++			"Align"
P{xspq ostate hdr} ++			"State"
P{xspq pminsize hdr} ++			"Min"
P{xspq pmaxsize hdr} ++			"Max"

P{xspq pseq full descr} = ROUND(@, 10)		"Number in list"
P{xspq pname full descr} ++	COPY		"Printer name"
P{xspq dev full descr} ++	COPY		"Device or [IP]"
P{xspq pform full descr} ++	COPY		"Form type"
P{xspq statemsg full descr} ++	COPY		"State and message"
P{xspq limit full descr} ++	COPY		"Limits set"
P{xspq pjobno full descr} ++	COPY		"Job being printed"
P{xspq puser full descr} ++	COPY		"Owner of job "
P{xspq pshriek full descr} ++	COPY		"Display flags"
P{xspq ab full descr} ++	COPY		"Abort set"
P{xspq pclass full descr} ++	COPY		"Class code"
P{xspq pcomm full descr} ++	COPY		"Printer Description"
P{xspq heoj full descr} ++	COPY		"Halt at end set"
P{xspq pid full descr} ++	COPY		"Process id"
P{xspq plocal full descr} ++	COPY		"Local printer only"
P{xspq pmsg full descr} ++	COPY		"Message from driver"
P{xspq pna full descr} ++	COPY		"Align required"
P{xspq ostate full descr} ++	COPY		"State of printer"
P{xspq pminsize full descr} ++	COPY		"Minimum job size"
P{xspq pmaxsize full descr} ++	COPY		"Maximum job size"

P{xspq jobopt dialogtit} = ROUND(@, 20)	"Job parameters"
P{xspq editing job} ++			"Editing job:"
P{xspq frametab form} ++		"Form/ptr/cps"
P{xspq frametab page} ++		"Pages"
P{xspq frametab user} ++		"User opts"
P{xspq frametab retain} ++		"Retain opts"
P{xspq frametab cc} ++			"Class codes"

P{xspq framelab jform} ++		"Form title priority copies"
P{xspq job form} ++			"Form"
P{xspq job title} ++			"Title"
P{xspq job supph} ++			"Suppress header"
P{xspq job printer} ++			"Printer"
P{xspq job copies} ++			"Copies"
P{xspq job prio} ++			"Priority"
P{xspq framelab jpage} = ROUND(@, 5)	"Select pages"
P{xspq job page names} ++		"Start/halted/end pages"
P{xspq job all pages} ++		"Print all pages"
P{xspq job odd pages} ++		"Print odd pages"
P{xspq job even pages} ++		"Print even pages"
P{xspq job swapoe} ++			"Swap odd/even pages on alternate copies"
P{xspq job postproc flags} ++		"Postprocessing options"
P{xspq framelab usermail} = ROUND(@, 5)	"User and mail options"
P{xspq post user} ++			"User to post to"
P{xspq job mail} ++			"Mail completion"
P{xspq job write} ++			"Write completion message"
P{xspq job mattn} ++			"Mail attention message"
P{xspq job wattn} ++			"Write attention message"
P{xspq framelab retain} = ROUND(@, 5)	"Retain/delete options"
P{xspq job sumitted} ++			"Job was submitted on"
P{xspq job retain} ++			"Retain on queue after printing"
P{xspq job printed} ++			"Job has been printed at least once"
P{xspq delp time} ++			"Delete job if still on queue after printing (hours)"
P{xspq delnp time} ++			"Delete job if still on queue unprinted (hours)"
P{xspq job set hold} ++			"Hold until"
P{xspq framelab ccodes} = ROUND(@, 5)	"Job class codes"
P{xspq job loconly} ++			"Local printers only"

P{xspq unqueue dialogtit} = ROUND(@,10)	"Unqueue job"
P{xspq unqueuing job} ++		"Unqueuing job"
P{xspq copy no delete} ++		"Copy to files only, no delete"
P{xspq unqueue directory} ++		"Unqueuing to directory"
P{xspq shell script} ++			"Shell script file"
P{xspq job file name} ++		"Job file name"
P{xspq unqueue cmd dlg title} ++	"Location for shell script file"
P{xspq unqueue job dlg title} ++	"Location for job data file"

P{xspg jmac dlg} ++			"Run command on job"
P{xspq jmac named} ++			"Run command with arg job:"
P{xspq jmac noname} ++			"Run command (no job)"
P{xspq jmac cmd} ++			"Command to execute"

P,H{xspq addmac to list} ++
"Add macro to list",
"Do you want to add this command as a ""macro""
to add to the macro menu"

P{xspq macname dlg} ++			"Name for macro"
P{xspq macname lab} ++			"Macro name/description"
E,H{xspq macname null} ++		"Null name",
"You must give a description (for the menu entry)"
E,H{xspq maccmd null} ++		"Null command", "You must give a shell command"

P{xspq macedit dlg} ++	"Editing macro list"
P{xspq new macro} ++			"New macro"
P{xspq del macro} ++			"Delete macro"
P{xspq edit macro} ++			"Edit macro"
P{xspq mac dlg} ++		"Editing macro"
P{xspq macdlg cmd} ++			"Shell Command"
P{xspq macdlg descr} ++			"Description"

P{xspq pdlg lab} ++		"Editing printer"

P{xspq form printer dlg} ++	"Set printer form type"
P{xspq pform lab} ++			"New form type"

P{xspq cc printer dlg} ++	"Class codes for printer"
P{xspq cc dlg minsize} ++		"Minimum job size"
P{xspq cc dlg maxsize} ++		"Maximum job size"

P{xspq add printer dlg} ++	"Add new printer"
P{xspq newprin name lab} ++		"Printer name"
P{xspq newprin dev lab} ++		"Device or IP"
P{xspq newprin form lab} ++		"Form type"
P{xspq ccdlg setall} ++			"Set All"
P{xspq ccdlg clearall} ++		"Clear All"
P{xspq newprin loconly} ++		"Local jobs only"

P{xspq pdev dlg} = ROUND(@, 10)		"Set printer device/IP"
P{xspq devip lab} ++			"Device or IP"
P{xspq ptr localhost} ++		"Host or IP not device"
P{xspq ptr comment} ++			"Description / comment"

P{xspg pmac dlg} = ROUND(@, 10)		"Run command on printer"
P{xspq pmac named} ++			"Run command with arg printer:"
P{xspq pmac noname} ++			"Run command (no printer)"
P{xspq pmac cmd} ++			"Command to execute"

P{xspq view dlg} = ROUND(@, 10)		"View job contents"
P{xspq view job} ++			"Viewing job"
P{xspq view no title} ++		"(No title)"
P{xspq view user} ++			"User:"
P{xspq view page} ++			"Page"
P{xspq view of} ++			"of"
P{xspq syserr dlg} = ROUND(@, 10)	"System error log file"

P{xspq vsearch dlg} = ROUND(@, 10)	"Search file contents"
P{xspq vsearch str} ++			"Search string"
P{xspq vsearch wrap} ++			"Wrap search"
P{xspq vsearch backw} ++		"Search backwards"

P{xspq search jplist} = ROUND(@, 10)	"Search job/printer list"
P{xspq search str} ++			"Search string"
P{xspq search jorp} ++			"Search printers not jobs"
P{xspq search title} ++			"Search titles"
P{xspq search form} ++			"Search form types"
P{xspq search ptr} ++			"Search printer names"
P{xspq search dev} ++			"Search device/host names"
P{xspq search user} ++			"Search user name"
P{xspq search backw} ++			"Search backwards"
P{xspq search wrap} ++			"Wrap around"
P{xspq search match} ++			"Match case"

P{xspq viewopt dialogtit} = ROUND(@, 10) "Selecting view options"
P{xspq frametab vo opt} ++		 "Gen options"
P{xspq frametab vo user} ++		 "User"
P{xspq frametab vo ptr} ++		 "Printer"
P{xspq frametab vo title} ++		 "Title"
P{xspq frametab vo cc} ++		 "Class code"
P{xspq framelab vof opt} ++		 "General Options"
P{xspq opt no conf} ++			 "Do not confirm abort/delete"
P{xspq opt unp conf} ++			 "Confirm abort/delete unprinted jobs"
P{xspq opt all conf} ++			 "Confirm abort/delete all jobs"
P{xspq framelab vof user} ++		 "Limit display to user(s)"
P{xspq opt user lab} ++			 "Users (multiple with ,s)"
P{xspq opt all hosts} ++		 "Show all hosts"
P{xspq opt local only} ++		 "Show local host only"

P{xspq framelab vof ptr} ++		 "Limit display to printer(s)"
P{xspq opt ptr lab} ++			 "Printers (multiple with ,s)" 
P{xspq opt pincl match} ++		 "Show jobs matching printer"
P{xspq opt pincl matchnull} ++		 "Show jobs matching printer plus null"
P{xspq opt pincl all} ++		 "Show all jobs"

P{xspq framelab vof tit} ++		 "Limit display to job title(s)"
P{xspq opt tit lab} ++			 "Titles (multiple with ,s)"
P{xspq opt tit allpr} ++		 "Show all jobs"
P{xspq opt tit unpr} ++			 "Show unprinted jobs only"
P{xspq opt tit pr} ++			 "Show printed jobs only"

P{xspq framlab vof cc} ++		 "Limit view by class code"

E,H{xspq saveopts no fork} ++		 "No fork",
					 "Unable to fork to run saveopts program"
E,H{xspq saveopts failed} ++		 "Saveopts failed",
					 "Saveopts program failed - exit %d0 sig %d1"

P,H{xspq changes not saved ok} ++	 "Changes unsaved",
"You have made changes to options saved
or display columns, are you sure"

# System-ish errors

[
System errors
]

E{Check file setup} = 8000
"Cannot open file - check ownership of files in spool directory, and
this program, %P. All should be owned by `spooler' and %P
should be suid to `spooler'."

E{Fork waiting} ++
"Could not fork - waiting for a bit (you might want to check the
number of processes being run on another terminal!)."

E{Cannot open user file} ++
"Trouble creating user file - please check GNUSpool is installed
correctly."

E{Funny times passwd file} ++
"The current time on your machine comes before the time on your
/etc/passwd file. Corrective action has been taken, but I suggest that
you investigate the situation rapidly!"

E{Cannot lock user ctrl file} ++
"Error - %E
Attempting to lock user control file."

E{Cannot unlock user ctrl file} ++
"Error - %E
Attempting to unlock user control file."

E{Cannot create temp user file} ++
"Error - %E
Attempting to create temporary file %s."

E{Wrong version of product} ++
"User control file GNUSpool version number is %d0.
GNUSpool Version is %d1.
Your user control file is either corrupted, or you are attempting
to a mixture of binaries from different versions of GNUSpool.
I am continuing, but please shut down GNUSpool as soon as possible
and investigate. Thanks."

E{Panic couldnt lock job file} ++
"Unable to lock the job file for saving.
Error was %E"

E{Panic couldnt lock ptr file} ++
"Unable to lock the printer file for saving.
Error was %E"

E{Cannot chdir} = ROUND(@, 100)		"Cannot change directory to spool directory - %E"
E{Spooler not running} ++		"%P - GNUSpool is not running."

E{IPC msg q fill wait} ++		"Message queue full - please wait"

E,H{IPC msg q error} ++
"%P: Error ""%E"" transmitting to message queue.
Please investigate your system (run ""ipcs"").",
"There seems to be some problem sending data to the message
queue for GNUSpool. This may be because someone has deleted
the message queue and/or killed the scheduler process or it
may be because your system has a problem."

E,H{IPC msg q full} ++
"%P - message queue is full
This could be because:
  (a) Your scheduler has crashed - please check for ""spshed""
      in a ""ps"" listing
  (b) Some other process unconnected with GNUSpool is sending messages
      exhausting the maximum limit of messages. Please check
      the output of ""ipcs -o"".",
"%P - message queue is full
This could be because:
  (a) Your scheduler has crashed - please check for ""spshed""
      in a ""ps"" listing
  (b) Some other process unconnected with GNUSpool is sending messages
      exhausting the maximum system-wide limit of messages. Please check
      the output of ""ipcs -o""."

E{setting zero class} ++
"Class code argument %s would give class code of 0 with your existing
class code of %t."

E{specifying zero class} ++		"Cannot specify zero class code"

E{Cannot open jshm} ++
"Panic! Cannot read job queue.
This may be due to incorrect installation.
Please check that the owner of the shared memory segments
are the same as the set-user id of %P which is '%U'."

E{Cannot open pshm} ++
"Panic! Cannot read printer list.
This may be due to incorrect installation.
Please check that the owner of the shared memory segments
are the same as the set-user id of %P which is '%U'."

E{Cannot open buffer shm} ++
"Panic! Cannot open shared memory for passing job and printer
buffers.
Please check that the owner of the shared memory segments
are the same as the set-user id of %P which is '%U'."

E{Cannot attach buffer shm} ++
"Panic! I can find, but the kernel won't let me attach
the shared memory for passing job and printer buffers."

E,H{Transfer buffer full up} ++
"The transfer shared memory buffer is full up.",
"The transfer shared memory buffer is full up.
This should normally be the same as the number of messages
on a message queue."

E{Network IPC queue filling} ++		"Network enqueue is filling up - waiting %d0 seconds"
E{Network IPC job queue filling} ++	"Network job enqueue is filling up - waiting %d0 seconds"
E{Network IPC ptr queue filling} ++	"Network ptr enqueue is filling up - waiting %d0 seconds"
E{Network IPC full up} ++		"Network queue full up - giving up"

E{Cannot create xfer shm} ++		"Failed to create transfer buffer shm segment"
E{Cannot attach xfer shm} ++		"Created, buf failed to attach transfer buffer shm segment"

E{No messages on queue} ++		"Failed to find messages in transfer buffer"
E{Buffer sequence error job} ++		"Apparent job sequence error in transfer buffer"
E{Buffer sequence error printer} ++	"Apparent ptr sequence error in transfer buffer"

E{Cannot open semaphore} ++
"Help! Cannot allocate semaphore - %E"

E{Wait to come up} ++
"Waiting for scheduler process to come up fully.
Please be patient!"

E,H{Not registered yet} ++
"Sorry, but your user id %R has not yet been
registered for GNUSpool. You need to run
(or have run for you) ""gspl-user -i"" ""gspl-xmuser"" or ""gspl-xuser"".",
"Sorry, but your user id %R has not yet been
registered for GNUSpool. You need to run
(or have run for you) ""gspl-user -i"" ""gspl-xmuser"" or ""gspl-xuser""."

E{Warn errors in host file} ++
"%P: Warning: There are errors in the gnuspool.hosts file"

E{Reconnection whilst still connected} ++
"Warning: host %s - reconnection of unclosed connection"

E{Connection attempt from unknown host} ++
"Rejected connection attempt from unknown host %s"

E{Hash function error free_poss} ++
"Panic: hash failure error free_poss hid %s"

E{Hash function error free_probe} ++
"Panic: hash failure error free_probe hid %s"

# Key definition errors

[
Key setup errors
]

E{Global key error} = 9000
"Global key setup error - clash on character %c0 with previously-given
value %d2 and new value %d1.
This means that you have defined one key to do two or more different things,
possibly because you have specified one as a terminal key (\KKILL etc).
Please look in the file %F for lines
of the form ""K%d2:...."" and ""K%d1:.....""
You may be using a different terminal or account than before?"

E{State key error} ++
"State %d3 key setup error - clash on character %c0 with previously-given
value %d2 and new value %d1.
This means that you have defined one key to do two or more different things,
possibly because you have specified one as a terminal key (\KKILL etc).
Please look in the file %F for lines
of the form ""%d3K%d2:...."" or ""K%d2:..."" and ""%d3K%d1:.....""
You may be using a different terminal or account than before?"

E{Key sequence error} ++
"Key setup error on character number %d3 - %c0 with previously-given
value %d2 and new value %d1."

E{Key state not defined} ++
"Keys not defined for state %d3... Aborting."

E{Key string too long} ++
"Key string too long... Aborting."

E{Key string bad syntax} ++
"Bad syntax global key definitions... Aborting."

E{Unknown key name} ++
"Unknown key name `%s' - ignored.
This is harmless, but is because you have called for something not defined in
your ""termcap"" or ""terminfo"" file - probably a ""HOME"" key.
Please look in the file %F for lines that might refer to %s
You may be using a different terminal or account than before?"

E{State key string bad syntax} ++
"Bad syntax state key definitions... Aborting."

E{Undef key seq} ++
"Undefined key sequence - ignored"

E{Key setup errors aborting} ++
"Aborted due to %d0 key setup errors."

E{getkey term input error} ++
"Error on terminal input - %E"

E{program arg error} = ROUND(@, 500)
"Bad argument %s to program %P"
E{program arg bad string} ++
"Bad string argument %s to program %P"

E{program opt expects arg} ++
"Option %s to program %P expects an argument"

# Missing bits

E{Missing help code} = 10000		"Missing help message code %d0"
E{Missing prompt code} ++		"Missing prompt message code %d0"
E{Missing error code} ++		"Missing error message code %d0"
E{argument option error} ++
"Clash on character %c2 with state %d0 and state %d1
(i.e. you have 2 meanings for -%c2)"
E{argument keyword error} ++
"Clash on option %s with state %d0 and state %d1
(i.e. you have 2 meanings for +%s)"
E{arg conflict abort} ++
"Aborted due to %d0 conflict(s) in argument descriptions"
E{NO MEMORY} ++				"Ran out of memory"

[
Things to put in report file when stopped and started
]
E{Sched started} = 12000		"%P started."
E{Sched normal stop} ++			"%P stopped normally."
E{Program fault halt} ++		"%P stopped due to program fault (coredump)"
E{Netmon program fault} ++		"%P stopped due to program fault (coredump) in net monitor"
E{Sched killed} ++			"%P stopped - killed"

[
Macro prompts - add to this??
]

P,H{Job or User macro} = 27000
"Run what:",
"Please give the command you want to run.
Details from the current line will be given as a parameter."

P,H{Printer macro} = 27100
"Run what:",
"Please give the command you want to run.
The printer name will be given as a parameter."

######################################################################
##  Internal-ish messages

S{No title offset} = 10
{Job completed msg} = 50
{Job deleted msg} = 51
{Job aborted msg} = 52
{Job awoper msg} = 53
{Job errors msg} = 54
{Job autodel msg} = 55

[
Job completion messages with and without title
]

E{Job completed msg} - {No title offset}
"Subject: GNUSpool job number %d0 completed

Your print job, job number %d0, completed
on printer %s."

E{Job deleted msg} - {No title offset}
"Subject: GNUSpool job number %d0 deleted

Your print job, job number %d0, was deleted
before it could be printed."

E{Job aborted msg} - {No title offset}
"Subject: GNUSpool job number %d0 aborted

Your print job, job number %d0, was aborted
during printing on printer %s."

E{Job awoper msg} - {No title offset}
"Subject: GNUSpool job number %d0 awaiting attention

Your print job, job number %d0, is awaiting
attention. Operator intervention may be required
to change or align the form.
Printer is: %s"

E{Job errors msg} - {No title offset}
"Subject: GNUSpool job number %d0 errors

Your print job, job number %d0, generated error messages
from ""postprocessing filters"" on printer %s.

Error messages follow:
"

E{Job autodel msg} - {No title offset}
"Subject: GNUSpool job number %d0 deleted automatically

Your print job, job number %d0, was automatically deleted.
This is because it was held on the queue beyond one of the
""automatic deletion"" times set with spr options -t n or -T n."

E{Job completed msg}
"Subject: GNUSpool job `%t', number %d0 completed

Your print job `%t', job number %d0, completed
on printer %s."

E{Job deleted msg}
"Subject: GNUSpool job `%t', number %d0 deleted

Your print job `%t', job number %d0, was deleted
before it could be printed."

E{Job aborted msg}
"Subject: GNUSpool job `%t', number %d0 aborted

Your print job `%t', job number %d0, was aborted
during printing on printer %s."

E{Job awoper msg}
"Subject: GNUSpool job `%t`, number %d0 awaiting attention

Your print job `%t', job number %d0, is awaiting
attention. Operator intervention may be required
to change or align the form.
Printer is: %s"

E{Job errors msg}
"Subject: GNUSpool job `%t', number %d0 errors

Your print job `%t', job number %d0, generated error messages
from ""postprocessing filters"" on printer %s.

Error messages follow:
"

E{Job autodel msg}
"Subject: GNUSpool job `%t', number %d0 deleted automatically

Your print job `%t', job number %d0, was automatically deleted.
This is because it was held on the queue beyond one of the
""automatic deletion"" times set with spr options -t n or -T n."

[
String to insert if no printer allocated
]

P{No printer allocated} = 59		"(No printer allocated yet)"

[
spwrite messages
]

E{spwrite write message} ++		"Message from %U....."
E{spwrite mail message} ++		"Subject: Message diverted to mail"

[
Abbreviated weekdays as 3 chars
]

P{Weekdays 3 chars} = ROUND(@, 100)	"SunMonTueWedThuFriSat"

[
Messages to put in standard banner
]

P{Job submitted message} ++		"Submitted"
P{Job commenced message} ++		"Commenced"
P{spd unnamed} ++			"<unnamed>"

E{Job submitted by} = ROUND(@, 20)	"This job submitted by %s"
E{Job header message} ++		"Job number %d0 size %d1 chars priority %d2"
E{Remote job header message} ++		"Job number %s:%d0 size %d1 chars priority %d2"
E{Filter process gave error} ++		"Filter program %s was error halted
Status %x0 Exit code %x1"
E{restarted message} ++			"***RESTARTED***"
E{aborted message} ++			"***ABORTED***"

[
Internal error messages to go in spshed_reps file
]

E{Internal cannot create pipe} = ROUND(@, 1000)	"Cannot create pipe - %E"
E{Cannot create job error file} ++		"Cannot create job error file - %E"
E{Internal cannot fork}	++			"Cannot fork - %E"
E{Cannot fdopen pipe} ++			"Cannot allocate pipe file descriptor"
E{spd bad swait} ++				"Bad packet swait/pwait"
E{spd invalid swait code} ++			"Invalid `swait' code %x0"
E{spd invalid pwait code} ++			"Invalid `pwait' code %x0"
E{spd bad packet} ++				"Bad packet read"
E{spd bad request} ++				"Bad request code %x0"
E{No spdinit} ++				"Cannot execute initialisation process"
E{Spdinit bad packet} ++			"Bad packet returned from initialisation process"
E{Spdinit error} ++				"Error in initialisation process"
E{Invalid device name} ++			"Invalid device name %s"
E{Network device no network command} ++		"Network printer %s (addr/dev %t)
does not have network command specified or non-network printer does"
E{Cannot open device file} ++			"Open of %s gave error - %E"
E{Device output error} ++			"Error on output - %E"

P,E{Lost track network filter} ++
"Net filt pid lost",
"Seem to have lost track of network filter process (%s) id %d0 - %E"

P,E{Network filter crashed} ++
"Term serv crashed",
"Network filter process (%s) terminated with signal %d0"

P,E{Network filter core dumped} ++
"Core dump term serv",
"Network filter process (%s) terminated with signal %d0 and core dump"

E{Network filter error halted} ++		"Network filter process (%s) terminated with exit code %d0"
E{spd no msg q} ++				"Cannot open MSG chan - %E"
E{spd no pshm} ++				"Cannot find shm segment - %E"
E{spd no jshm} ++				"Cannot find job segment - %E"
E{spd invalid pslot} ++				"Printer slot `%s' not valid!"
E{Banner program crashed} ++			"Banner program `%s' terminated with signal %d0"
E{Banner program core dumped} ++		"Banner program `%s' terminated with signal %d0 and core dump"
E{Banner program error halted} ++		"Banner program `%s' terminated with exit %d0"
E{Cannot run banner program} ++			"Cannot run banner program `%s'"
E{Log error header} ++				"Error report from printer %t on device %s"
E{Log error report} ++				"Report was: %s"
E{spd invalid job slot} ++			"Attempt to print invalid job slot %d1 0x%x1 ptr %s dev %t jobnum %d0" 

P,E{spd network filter hung} ++
"Term serv hung",
"Network filter process (%s) pid %d0 apparently hung"

E{spdinit bad baud} = ROUND(@, 20)		"Error near line %d0 - Invalid baud rate %d9"
E{spdinit align file access error} ++		"Error near line %d0 - No access to alignment file %s"
E{spdinit record file error} ++			"Error near line %d0 - No access to record file %s"
E{spdinit log file error} ++			"Error near line %d0 - No access to log file %s"
E{spdinit usage} ++				"Usage: %P printer formtype"
E{spdinit line too long} ++			"Error near line %d0 - Identifier too long"
E{spdinit Unterminated RE} ++			"Error near line %d0 - Unterminated regular expression"
E{spdinit digit expected} ++			"Error near line %d0 - Number expected"
E{spdinit char expected} ++			"Error near line %d0 - Expected a `%c9'"
E{spdinit exec unexpected} ++			"Error near line %d0 - Did not expect `exec' keyword here"
E{spdinit expected seterror} ++			"Error near line %d0 - Expected ""setoffline"" or ""seterror"" after ""signal"""
E{spdinit expected setoffline} ++		"Error near line %d0 - Expected ""setoffline"" or ""seterror"" after ""exit"""
E{spdinit number range} ++			"Error near line %d0 - Number (%d1) should be between %d2 and %d3"
E{spdinit comma-sep expected} ++		"Error near line %d0 - comma-separated number list expected"
E{spdinit undef name} ++			"Error near line %d0 - probable undefined name `%s'."
E{spdinit no setup dir} ++
"There does not appear to be a directory for the
printer name %s in the printers directory %t.
This must be created and accessible to %U."
E{spdinit no setup file} ++
"There does not appear to be a setup file (even a
possible default one of `%t') in the printer directory
for printer name %s. Please note that this is a recent
change, June 1999, to insist on a setup file being present."

E{Cannot create message queue} = ROUND(@, 100)	"%P fatal error - could not create IPC message queue - %E"
E{Cannot reset message queue} ++		"%P fatal error - Could not reset owner of message queue - %E"
E{Cannot create jfile} ++			"Could not create job save file - %E"
E{Cannot create pfile} ++			"Could not create printer save file - %E"
E{Sched invalid message} ++			"Invalid message type code %x9 (%d9)"
E{Network IPC send fail} ++			"%P fatal error sending/reading IPC message - %E"
E{Sched op message error} ++			"Invalid operator message code %x9 (%d9)"
E{Sched charge message error} ++		"Invalid charge message code %x9 (%d9)"
E{Sched ptr message error} ++			"Invalid printer message code %x9 (%d9)"
E{Sched job message error} ++			"Invalid job message code %x9"
E{Sched ptr message length error} ++		"Invalid printer message length %d9"
E{Sched job message length error} ++		"Invalid job message length %d9"
E{Sched op message length error} ++		"Invalid operator message length %d9"
E{Sched charge message length error} ++		"Invalid charge message length %d9"
E{Sched network message length error} ++	"Invalid network internal message length %d9"
E{Sched network message error} ++		"Invalid network internal message code %x9 (%d9)"
E{Semaphore create error} ++			"%P fatal error - could not create semaphore - %E"
E{Semaphore reset error} ++			"%P fatal error - could not reset ownership of semaphore - %E"
E{Shm create error} ++
"Trouble creating shared memory id %x9 - %E
Possibly some other software is using it all up, or the maximum shared
memory size allocated in your kernel is too small.
If the message given above is ""File Exists"" try removing files with names
starting ""spmm_"" from the spool directory."
E{Shm reset error} ++				"Trouble resetting ownership of shared memory - %E"
E{Create jshm error} ++				"Trouble attaching job data shared memory - %E"
E{Create pshm error} ++				"Trouble attaching printer shared memory - %E"
E{Network process jshm error} ++		"Network monitor trouble attaching shared memory for jobs - %E"
E{Network process pshm error} ++		"Trouble attaching shared memory for printers - %E"
E{Network connect socket error} ++		"Trouble creating connection socket port number %d0 - %E"
E{Network feeder socket error} ++		"Trouble creating feeder socket port number %d0 - %E"
E{Poll or select error} ++			"Poll/select error in netmonitor process - %E"
E{Network probe socket error} ++		"Trouble creating probe uda port number %d0 - %E"
E{Cannot create probe socket} ++		"Trouble creating probe socket port number %d0 - %E"
E{Cannot bind probe socket} ++			"Trouble binding probe socket port number %d0 - %E"
E{Cannot send probe socket} ++			"Trouble sending on probe socket port number %d0 - %E"
E{Truncated TCP message} ++			"Truncated incoming TCP message from %s - closed"
E{Message synch lost} ++
"Incoming message sequence from %s lost.
Expected %d0, read %d1
Command packet was %d2"
E{Create charge file error} ++			"%P cannot create charge file - %E"

E{Unknown network packet} ++			"Unknown incoming message packet %d0 0x%x0"
E{Lost track of job} ++				"Lost track of job on %s slot number %d1 0x%x1"
E{Lost track of printer} ++			"Lost track of printer on %s slot number %d1 0x%x1"

E{Recompile port} ++
"Sorry but the scheduler cannot send a signal to a user process such as gspl-pq
using the ""kill(2)"" system call. It may be that it will have to be recompiled
to run as the superuser.
However probably the permissions of programs such as ""spshed"" and ""gspl-pq""
are incorrect - is it possible that these were restored from backup and the
permissions not reset correctly?"

[
Xtnetserv messages
]

E{xtnet aborted} ++				"Aborted %P due to failure to establish network connections"
E{xtnet no TCP protocol} ++			"%P Unable to obtain TCP protocol number."
E{xtnet no UDP protocol} ++			"%P Unable to obtain UDP protocol number."
E{xtnet unknown qserv} ++			"%P Unable to obtain %t service %s for queue port"
E{xtnet unknown uaserv} ++			"%P Unable to obtain %t service %s for user access port"
E{xtnet cannot open qsock} ++			"%P Unable to open TCP port number %d0 for queue port"
E{xtnet cannot open uasock} ++			"%P Unable to open UDP port number %d0 for user access port"
E{xtnet unknown apiserv} ++			"%P Unable to obtain %t service %s for API request port
(not fatal - but API won't run)"
E{xtnet unknown apipserv} ++
"%P Unable to obtain %t service %s for API prompt port
(not fatal - but API won't run)"

E{exception on TCP connection} = ROUND(@, 20)	"TCP connection died with exception"

E{Cannot create jinfo shm} ++		"Failed to create job info shm segment - %E"
E{Cannot attach jinfo shm} ++		"Created, buf failed to attach job info shm segment - %E"
E{Job information segment corrupted} ++	"Job information segment corrupted njobs=%d0 maxj=%d1"

E{Semaphore error probably undo} ++
"%P - Problems with locking semaphore. This is almost certainly
due to being unable to allocate sufficient ""undo"" structures
(structures which hold adjustments to semaphore values to apply
on exit) within the kernel. If you have the program ""/etc/sysdef""
available, please run it now and look at the parameter SEMMNU.
This should be at least as great as the parameter SEMMNS and
your kernel parameters should be updated to avoid this error.
The error reported was ""%E"".
However this can vary between platforms."

E{Lock error} ++		"Error setting lock - %E"
E{Unlock error} ++		"Error unsetting lock - %E"

[
Spool directory reporting type messages
]

E{cannot stat spool directory} = ROUND(@, 100)	"Cannot stat spool directory %s - aborting"
E{cannot stat spool sub directory} ++		"Subdirectory %t of spool directory %s nonexistent  - aborting"
E{spool directory not owned by spooler} ++	"Spool directory %s is owned by %u8 not %u7 - aborting"
E{subdirectory not owned by spooler} ++		"Subdirectory %t of %s is owned by %u8 not %u7 - aborting"
E{spool directory invalid permissions} ++	"Spool directory %s does not have rwx permissions - mode is %o8 - aborting"
E{subdirectory invalid permissions} ++		"Subdirectory %t of %s does not have rwx permissions - mode is %o8 - aborting"

[
API CGI Routines messages
]

S{Base for API errors} = 26100

E{Base for API errors} -1 COPY
"Remote CGI program error - invalid API fd
(Probably a bug in %P)"

E{Base for API errors} -2 COPY
"Remote CGI error - run out of memory in xbnetserv executing %P
(Probably a bug in %P or xbnetserv)."

E{Base for API errors} -3 COPY
"Remote CGI program error - invalid hostname"

E{Base for API errors} -4 COPY
"Remote CGI program error - invalid service"

E{Base for API errors} -5 COPY
"Remote CGI program error - no default service.
(Expecting xbapi service to be specified)."

E{Base for API errors} -6 COPY
"Remote CGI program error in %P - cannot create socket for API"

E{Base for API errors} -7 COPY
"Remote CGI program error in %P - cannot bind socket for API"

E{Base for API errors} -8 COPY
"Remote CGI program error in %P - cannot connect API socket"

E{Base for API errors} -9 COPY
"Remote CGI program error in %P - Read fail"

E{Base for API errors} -10 COPY
"Remote CGI program error in %P - Write fail"

E{Base for API errors} -11 COPY
"Remote CGI program error - run out of processes"

E{Base for API errors} -23 COPY
"Remote CGI program error in %P - invalid user"

E{Base for API errors} -24 COPY
"Remote CGI program error in %P - invalid (zero) class code"

E{Base for API errors} -25 COPY
"Remote CGI program error in %P - invalid priority"

E{Base for API errors} -26 COPY
"Remote CGI program error in %P - invalid number of copies"

E{Base for API errors} -27 COPY
"Remote CGI program error in %P - invalid form type"

E{Base for API errors} -28 COPY
"Remote CGI program error in %P - no memory accessing
queue file."

E{Base for API errors} -29 COPY
"Remote CGI program error in %P - invalid page file."

E{Base for API errors} -30 COPY
"Remote CGI program error in %P - no memory accessing
page file."

E{Base for API errors} -31 COPY
"Remote CGI program error in %P - cannot create
page file."

E{Base for API errors} -32 COPY
"Remote CGI program error in %P - file system is full
for spool directory."

E{Base for API errors} -33 COPY
"Remote CGI program error in %P - message queue is full.
This may indicate some third party program has failed."

E{Base for API errors} -34 COPY
"Remote CGI program error in %P - empty job file."

E{Base for API errors} -35 COPY
"Remote CGI program error in %P - invalid printer."

E{Base for API errors} -36 COPY
"Remote CGI program error in %P - warning - past job size
limit - job truncated."

E{Base for API errors} -37 COPY
"Remote CGI program error in %P - past job size
limit - job rejected."

E{Base for API errors} -38 COPY
"Remote CGI program error in %P - password required."

E{Base for API errors} -39 COPY
"Remote CGI program error in %P - password invalid."

E{Base for API errors} -40 COPY
"Remote CGI program error in %P - unknown command.
(Probably a bug)."

E{Base for API errors} -41 COPY
"Remote CGI program error in %P - sequence error.
The job or printer has changed. Try again."

E{Base for API errors} -42 COPY
"Remote CGI program error in %P - unknown job"

E{Base for API errors} -43 COPY
"Remote CGI program error in %P - unknown printer"

E{Base for API errors} -44 COPY
"Remote CGI program error in %P - no permission for
requested operation."

E{Base for API errors} -45 COPY
"Remote CGI program error in %P - job not printed."

E{Base for API errors} -46 COPY
"Remote CGI program error in %P - printer not running."

E{Base for API errors} -47 COPY
"Remote CGI program error in %P - printer running."

E{Base for API errors} -48 COPY
"Remote CGI program error in %P - printer null.
(Probably a bug)."

E{Base for API errors} -49 COPY
"Remote CGI program error in %P - no permission to
create or delete printers."

E{Base for API errors} -50 COPY
"Remote CGI program error in %P - invalid slot number."

{Bitmaps} = 11000

[
Bitmaps for big chars for banner pages
]

P{Bitmaps}+' '			"00000000000000000000000000000000"
P{Bitmaps}+'!'			"10101010101010101000101000000000 !"
P{Bitmaps}+'"'			"00242424000000000000000000000000 \""
P{Bitmaps}+'#'			"00001824202078202020227c00000000 #"
P{Bitmaps}+'$'			"00103c5250503c121212523c10000000 $"
P{Bitmaps}+'%'			"000040a1420408102042850200000000 %"
P{Bitmaps}+'&'			"00003844442810284542453800000000 &"
P{Bitmaps}+'''			"00000810100000000000000000000000 '"
P{Bitmaps}+'('			"00000810202020202020100800000000 ("
P{Bitmaps}+')'			"00002010080808080808102000000000 )"
P{Bitmaps}+'*'			"00000000109254381038549210000000 *"
P{Bitmaps}+'+'			"000000001010107c1010100000000000 +"
P{Bitmaps}+','			"00000000000000000000181808100000 ,"
P{Bitmaps}+'-'			"000000000000007e0000000000000000 -"
P{Bitmaps}+'.'			"00000000000000000000181800000000 ."
P{Bitmaps}+'/'			"00000001020408102040800000000000 /"
P{Bitmaps}+'0'			"0000384482868a92a2c2443800000000 0"
P{Bitmaps}+'1'			"00001030101010101010103800000000 1"
P{Bitmaps}+'2'			"00003e42020408102040407e00000000 2"
P{Bitmaps}+'3'			"0000384402023c020202443800000000 3"
P{Bitmaps}+'4'			"0000040c1424447e0404040400000000 4"
P{Bitmaps}+'5'			"00007e4040403c020202423c00000000 5"
P{Bitmaps}+'6'			"00003c424040407c4242423c00000000 6"
P{Bitmaps}+'7'			"00007e42040810202020202000000000 7"
P{Bitmaps}+'8'			"00003c4242423c424242423c00000000 8"
P{Bitmaps}+'9'			"00003c4242423e020202423c00000000 9"
P{Bitmaps}+':'			"00000000000018180000181800000000 :"
P{Bitmaps}+';'			"00000000000018180000181808100000 ;"
P{Bitmaps}+'<'			"00000408102040402010080400000000 <"
P{Bitmaps}+'='			"0000000000007e00007e000000000000 ="
P{Bitmaps}+'>'			"00002010080402020408102000000000 >"
P{Bitmaps}+'?'			"00003c424202020c1010001000000000 ?"
P{Bitmaps}+'@'			"0000384492aaaaaa9488403800000000 @"
P{Bitmaps}+'A'			"00001028444444447c44444400000000 A"
P{Bitmaps}+'B'			"0000fc4242427c42424242fc00000000 B"
P{Bitmaps}+'C'			"00001c22404040404040221c00000000 C"
P{Bitmaps}+'D'			"0000fc4242424242424242fc00000000 D"
P{Bitmaps}+'E'			"00007e4040407c404040407e00000000 E"
P{Bitmaps}+'F'			"00007e4040407c404040404000000000 F"
P{Bitmaps}+'G'			"00001c22404040405e42221c00000000 G"
P{Bitmaps}+'H'			"0000424242427e424242424200000000 H"
P{Bitmaps}+'I'			"00003810101010101010103800000000 I"
P{Bitmaps}+'J'			"00007e08080808080848483800000000 J"
P{Bitmaps}+'K'			"00004242444850605048444200000000 K"
P{Bitmaps}+'L'			"00004040404040404040407e00000000 L"
P{Bitmaps}+'M'			"000082c6aa9292828282828200000000 M"
P{Bitmaps}+'N'			"000082c2a292928a8a86868200000000 N"
P{Bitmaps}+'O'			"00003844828282828282443800000000 O"
P{Bitmaps}+'P'			"00007c4242427c404040404000000000 P"
P{Bitmaps}+'Q'			"0000384482828282a2924c3b00000000 Q"
P{Bitmaps}+'R'			"00007c4242427c605048444200000000 R"
P{Bitmaps}+'S'			"00003c4240403c020202423c00000000 S"
P{Bitmaps}+'T'			"0000fe10101010101010101000000000 T"
P{Bitmaps}+'U'			"00008282828282828282443800000000 U"
P{Bitmaps}+'V'			"00008282828282444444281000000000 V"
P{Bitmaps}+'W'			"000082828282828292aac68200000000 W"
P{Bitmaps}+'X'			"00004444442810102844444400000000 X"
P{Bitmaps}+'Y'			"00004444444428101010101000000000 Y"
P{Bitmaps}+'Z'			"0000ff0102040810204080ff00000000 Z"
P{Bitmaps}+'['			"00003820202020202020203800000000 ["
P{Bitmaps}+'\'			"00000080402010080402010000000000 \\"
P{Bitmaps}+']'			"00003808080808080808083800000000 ]"
P{Bitmaps}+'^'			"00101038549210101010101000000000 ^"
P{Bitmaps}+'_'			"00000000000000000000007e00000000 _"
P{Bitmaps}+'`'			"00001010080000000000000000000000 `"
P{Bitmaps}+'a'			"0000000000003c023e42423e00000000 a"
P{Bitmaps}+'b'			"0000404040407c424242427c00000000 b"
P{Bitmaps}+'c'			"0000000000003c424040423c00000000 c"
P{Bitmaps}+'d'			"0002020202023e424242423e00000000 d"
P{Bitmaps}+'e'			"0000000000003c427c40423c00000000 e"
P{Bitmaps}+'f'			"00000c1010107e101010101000000000 f"
P{Bitmaps}+'g'			"0000000000003e424242423e0202423e g"
P{Bitmaps}+'h'			"0000404040407c424242424200000000 h"
P{Bitmaps}+'i'			"00000000100030101010103800000000 i"
P{Bitmaps}+'j'			"00000000100030101010101010105020 j"
P{Bitmaps}+'k'			"00004040404046485070484600000000 k"
P{Bitmaps}+'l'			"00003010101010101010103800000000 l"
P{Bitmaps}+'m'			"000000000000ec929292929200000000 m"
P{Bitmaps}+'n'			"0000000000407c424242424200000000 n"
P{Bitmaps}+'o'			"0000000000003c424242423c00000000 o"
P{Bitmaps}+'p'			"0000000000007c424242427c40404040 p"
P{Bitmaps}+'q'			"0000000000003c424242423e02020202 q"
P{Bitmaps}+'r'			"0000000000005c624040404000000000 r"
P{Bitmaps}+'s'			"0000000000003e403c02423c00000000 s"
P{Bitmaps}+'t'			"0000101010107e101010100c00000000 t"
P{Bitmaps}+'u'			"00000000000042424242423d00000000 u"
P{Bitmaps}+'v'			"00000000000082828244281000000000 v"
P{Bitmaps}+'w'			"00000000000092929292926c00000000 w"
P{Bitmaps}+'x'			"00000000000042241818244200000000 x"
P{Bitmaps}+'y'			"00000000000042424242423e0202221c y"
P{Bitmaps}+'z'			"0000000000007e040810207e00000000 z"
P{Bitmaps}+'{'			"00081020202040202020100800000000 {"
P{Bitmaps}+'|'			"00101010101010101010101000000000 |"
P{Bitmaps}+'}'			"00100804040402040404081000000000 }"
P{Bitmaps}+'~'			"0000000000205a040000000000000000 ~"
