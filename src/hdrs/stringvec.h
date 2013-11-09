/* stringvec.h -- vector of strings

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

#define STRINGVEC_INIT  10
#define STRINGVEC_INC   5

struct  stringvec  {
        int     memb_cnt, memb_max;
        char    **memb_list;
};

extern void  stringvec_init(struct stringvec *);
extern void  stringvec_insert_unique(struct stringvec *, const char *);
extern void  stringvec_append(struct stringvec *, const char *);
extern void  stringvec_free(struct stringvec *);
extern void  stringvec_split(struct stringvec *, const char *, const char);

#define is_init(sv)     ((sv).memb_list != (char **) 0)
#define stringvec_count(sv)  (sv).memb_cnt
#define stringvec_nth(sv, n) (sv).memb_list[n]
