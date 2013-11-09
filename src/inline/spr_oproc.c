/* spr_oproc.c -- table of options for spr

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

optparam  optprocs[] = {
o_explain,      o_togverbose,   o_verbose,      o_noverbose,
o_nohdrs,       o_hdrs,         o_nomailwrt,    o_wrt,
o_mail,         o_noattn,       o_mattn,        o_wattn,
o_noretn,       o_retn,         o_localonly,    o_nolocalonly,
o_interp,       o_nointerp,     o_copies,       o_header,
o_formtype,     o_priority,     o_printer,      o_user,
o_setclass,     o_range,        o_flags,        o_ptimeout,
o_nptimeout,    o_tdelay,       o_dtime,        o_delimnum,
o_delim,        o_oddeven,      o_jobwait,      o_pagelimit,
o_queuehost,    o_orighost,     o_queueuser,    o_external,
o_freezecd,     o_freezehd
};
