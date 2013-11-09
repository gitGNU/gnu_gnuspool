/* pfmt_state.c -- display state and feedback message if appropriate

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

static  fmt_t   fmt_state(const struct spptr *pp, const int fwidth)
{
        int     staten = pp->spp_state >= SPP_NSTATES? SPP_NULL: pp->spp_state;
#ifdef  CHARSPRINTF
        if  (staten < SPP_HALT  &&  pp->spp_feedback[0])  {
                sprintf(bigbuff, "%s:%s", statenames[staten], pp->spp_feedback);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (staten < SPP_HALT  &&  pp->spp_feedback[0])
                return  (fmt_t) sprintf(bigbuff, "%s:%s", statenames[staten], pp->spp_feedback);
#endif
        return  (fmt_t) strlen(strcpy(bigbuff, statenames[staten]));
}
