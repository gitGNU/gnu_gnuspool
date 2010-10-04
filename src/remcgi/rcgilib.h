/* rcgilib.h -- header for remote CGI library

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

struct	jobswanted	{
	slotno_t	slot;		/* Slot number */
	jobno_t		jno;		/* Job number */
	netid_t		host;		/* Host id */
};
struct	ptrswanted	{
	char		*ptrname;	/* Job number */
	netid_t		host;		/* Host id */
	slotno_t	slot;
};
struct	ptr_with_slot  {
	slotno_t		slot;
	struct	apispptr	ptr;
};

extern int  sort_p(struct ptr_with_slot *, struct ptr_with_slot *);
extern int  numeric(const char *);
extern int  decode_jnum(char *, struct jobswanted *);
extern int  decode_pname(char *, struct ptrswanted *);
extern struct apispptr *find_ptr(const slotno_t);
extern int  find_ptr_by_name(const char **, struct ptr_with_slot *);
extern void  api_open(char *);
extern void  api_readptrs(const unsigned);
extern void  read_jobqueue(const unsigned);

extern	int		gspool_fd;
extern	struct	apispdet  mypriv;

extern	int		Njobs, Nptrs;
extern	struct	apispq	*job_list;
extern	slotno_t	*jslot_list;
extern	struct	ptr_with_slot	*ptr_sl_list;
