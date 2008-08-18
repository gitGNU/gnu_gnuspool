/* incl_unix.h -- cope with all sorts of funny places that declarations get put

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

/*------------------------------------------------------------------
 * This merges together everything which involves standard libraries,
 * malloc, and string operations.
 *
 * This is closely linked with posix-compatibility and so forth.
 * We replace the previous includes:
 *
 *	incl_str.h
 *	blockcopy.h
 *	incl_alloc.h
 *	unixdefs.h
 */

#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef	HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef	HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef	HAVE_WAIT_H
#include <wait.h>
#endif

#define	QSORTP1	(void *)
#define	QSORTP4	(int (*)(const void *,const void *))

#ifdef	STDC_HEADERS
#include <string.h>
#else
#ifndef	HAVE_STRCHR
#define	strchr	index
#define	strrchr	rindex
#endif
char	*strchr(),
	*strrchr(),
	*strcpy(),
	*strncpy(),
	*strpbrk(),
	*strcat(),
	*strncat();
#endif /* !STDC_HEADERS */

#ifdef	HAVE_MEMCPY
#define	BLOCK_COPY(to, from, count)	memcpy((char *) (to), (char *) (from), (unsigned) (count))
#define	BLOCK_ZERO(to, count)		memset((char *) (to), 0, (unsigned) (count))
#define	BLOCK_CMP(a, b, count)		memcmp((char *) (a), (char *) (b), (unsigned) (count)) == 0
#elif	defined(HAVE_BCOPY)
#define	BLOCK_COPY(to, from, count)	bcopy((char *) (from), (char *) (to), (unsigned) (count))
#define	BLOCK_ZERO(to, count)		bzero((char *) (to), (unsigned) (count))
#define	BLOCK_CMP(a, b, count)		bcmp((char *) (a), (char *) (b), (unsigned) (count)) == 0
#else
#define	BLOCK_COPY(to, from, count)	undefined_block_copy_routine(to, from, count);
#define	BLOCK_ZERO(to, count)		undefined_block_zero_routine(to, count);
#define	BLOCK_CMP(a, b, count)		undefined_block_cmp_routine(a, b, count);
#endif

/* These are our own string-ish things */

extern int	ncstrcmp(const char *, const char *);
extern int	ncstrncmp(const char *, const char *, int);

extern int	qmatch(char *, const char *);
extern int	issubset(char *, char *);
extern int	repattok(const char *);

extern char *	stracpy(const char *);
extern char *	strread(FILE *, const char *);
extern char *	runpwd(void);

extern	void	nomem(void);
