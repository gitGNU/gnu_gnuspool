/* jfmt_delim.c -- for display routines - delimiters

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

static fmt_t    fmt_delimnum(const struct spq *jp, const int fwidth)
{
        int             ret;
        char            *delim = (char *) 0;
        unsigned        pagenums = 0;
        LONG            *pageoffsets = (LONG *) 0;
        struct  pages   pfe;

        if  ((ret = rdpgfile(jp, &pfe, &delim, &pagenums, &pageoffsets)) < 0)
                return  0;

        if  (ret == 0)  {
#ifdef  CHARSPRINTF
                sprintf(bigbuff, "%*u", fwidth, 1);
                return  (fmt_t) strlen(bigbuff);
#else
                return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, 1);
#endif
        }
        if  (pageoffsets)
                free((char *) pageoffsets);
        if  (delim)
                free((char *) delim);
#ifdef  CHARSPRINTF
        sprintf(bigbuff, "%*lu", fwidth, (unsigned long) pfe.delimnum);
        return  (fmt_t) strlen(bigbuff);
#else
        return  (fmt_t) sprintf(bigbuff, "%*lu", fwidth, (unsigned long) pfe.delimnum);
#endif
}

static fmt_t  fmt_delim(const struct spq *jp, const int fwidth)
{
        int             ret;
        unsigned        ii;
        char            *delim = (char *) 0, *outp = bigbuff;
        unsigned        pagenums = 0;
        LONG            *pageoffsets = (LONG *) 0;
        struct  pages   pfe;

        if  ((ret = rdpgfile(jp, &pfe, &delim, &pagenums, &pageoffsets)) < 0)
                return  0;

        if  (pageoffsets)
                free((char *) pageoffsets);
        if  (ret == 0  ||  !delim)
                return  strlen(strcpy(bigbuff, "\\f"));

        for  (ii = 0;  ii < pfe.deliml;  ii++)  {
                int     ch = delim[ii] & 255;
                if  (!isascii(ch))
                        sprintf(outp, "\\x%.2x", ch);
                else  if  (iscntrl(ch))  {
                        switch  (ch)  {
                        case  033:
                                strcpy(outp, "\\e");
                                break;
                        case  ('h' & 0x1f):
                                strcpy(outp, "\\b");
                                break;
                        case  '\r':
                                strcpy(outp, "\\r");
                                break;
                        case  '\n':
                                strcpy(outp, "\\n");
                                break;
                        case  '\f':
                                strcpy(outp, "\\f");
                                break;
                        case  '\t':
                                strcpy(outp, "\\t");
                                break;
                        case  '\v':
                                strcpy(outp, "\\v");
                                break;
                        default:
                                sprintf(outp, "^%c", ch | 0x40);
                                break;
                        }
                }
                else  {
                        if  (ch == '\\'  ||  ch == '^')
                                *outp++ = ch;
                        *outp++ = ch;
                        continue;
                }
                outp += strlen(outp);
        }
        free((char *) delim);
        *outp = '\0';
        return  (fmt_t) strlen(bigbuff);
}
