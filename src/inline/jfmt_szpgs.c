/* jfmt_szpgs.c -- display size of job in pages

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

static  fmt_t   fmt_szpages(const struct spq *jp, const int fwidth)
{
        if  (jp->spq_npages > 1)  {
                char    *res = prin_size(jp->spq_npages);
                int     resl = strlen(res);
                if  (resl + 1 >= fwidth)  {
                        sprintf(bigbuff, "=%s", res);
                        return  (fmt_t) (resl + 1);
                }
                sprintf(bigbuff, "=%*s", fwidth-1, res);
        }
        else  {
                char    *res = prin_size(jp->spq_size);
                int     resl = strlen(res);
                if  (resl >= fwidth)  {
                        strcpy(bigbuff, res);
                        return  (fmt_t) resl;
                }
                sprintf(bigbuff, "%*s", fwidth, res);
        }
        return  (fmt_t) fwidth;
}
