/* stringvec.c -- nice OO vector of strings

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

#include "config.h"
#include <stdio.h>
#include "incl_unix.h"
#include "stringvec.h"

/* This is a much neater version of having a vector of strings.
   Various things need changing to use it */

void	stringvec_init(struct stringvec *sv)
{
	sv->memb_cnt = 0;
	sv->memb_max = STRINGVEC_INIT;
	sv->memb_list = (char **) malloc(STRINGVEC_INIT * sizeof(char *));
	if  (!sv->memb_list)
		nomem();
}

void	stringvec_insert_unique(struct stringvec *sv, const char *newitem)
{
	int  first = 0, last = sv->memb_cnt;

	/* This is binary search and insert */

	while  (first < last)  {
		int	mid = (first + last) / 2;
		int	cmp = strcmp(sv->memb_list[mid], newitem);
		if  (cmp == 0)
			return;
		if  (cmp < 0)
			first = mid + 1;
		else
			last = mid;
	}

	/* Ready to insert at "first", move rest up */

	if  (sv->memb_cnt >= sv->memb_max)  {
		sv->memb_max += STRINGVEC_INC;
		sv->memb_list = realloc(sv->memb_list, (unsigned) (sv->memb_max * sizeof(char *)));
		if  (!sv->memb_list)
			nomem();
	}
	for  (last = sv->memb_cnt;  last > first;  last--)
		sv->memb_list[last] = sv->memb_list[last-1];

	sv->memb_list[first] = stracpy(newitem);
	sv->memb_cnt++;
}

void	stringvec_append(struct stringvec *sv, const char *newitem)
{
	if  (sv->memb_cnt >= sv->memb_max)  {
		sv->memb_max += STRINGVEC_INC;
		sv->memb_list = realloc(sv->memb_list, (unsigned) (sv->memb_max * sizeof(char *)));
		if  (!sv->memb_list)
			nomem();
	}
	sv->memb_list[sv->memb_cnt] = stracpy(newitem);
	sv->memb_cnt++;
}

void	stringvec_free(struct stringvec *sv)
{
	int	cnt;
	for  (cnt = 0;  cnt < sv->memb_cnt;  cnt++)
		free(sv->memb_list[cnt]);
	free((char *) sv->memb_list);
}
