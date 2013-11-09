/* sdi_gram.c -- parser for spdinit

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
#include <ctype.h>
#include <setjmp.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>
#ifdef  HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "initp.h"
#include "kw.h"
#include "incl_unix.h"

#define HASHMOD 509             /*  Being prime  */
#define MAXID   60
#define INITSTR 150
#define INCSTR  20

void  error(const int);
void  report(const int);

static  int     line_count = 1;
struct  kw      *lastsym;
extern  FILE    *infile;
extern  char    *ptdir,
                *curr_file,
                *default_form,
                *formname,
                *Suffix;
extern  struct  string  **out_str_last;
extern  struct  string  *su_str, *hlt_str, *ds_str, *de_str, *ss_str, *se_str;
extern  struct  string  *ps_str, *pe_str, *rc_str, *bds_str, *bde_str;
extern  struct  string  *ab_str, *res_str;
extern  struct  string  *out_bannprog, *out_align, *out_portsu, *out_filter, *out_netfilt, *out_sttystring;

extern  struct  initpkt out_params;
extern  int     execf;

extern  char    *mbeg, *mend;
extern  int     Nott, Isbann, Hadbann;

struct  kw      *hashtab[HASHMOD];
jmp_buf syntax_error;

void  error(const int messno)
{
        disp_arg[0] = line_count;
        report(messno);
        longjmp(syntax_error, 1);
}

static  char *rdexpr()
{
        int     ch;
        char    buf[80];
        char    *bp = buf, *res;
        int     bcnt = 0;

        for  (;;)       switch  (ch = getc(infile))  {
        default:
                if  (!isalnum(ch))  {
                        ungetc(ch, infile);
                        *bp = '\0';
                        if  ((res = (char *) malloc((unsigned)(bcnt+1))) == (char *) 0)
                                nomem();
                        strcpy(res, buf);
                        return  res;
                }

        case '_':case '-':
        case '[':case ']':case '^':case '!':case '*':case '?':
        case '{':case '}':case '@':
                if  (++bcnt >= 80)
                        error($E{spdinit Unterminated RE});
                *bp++ = (char) ch;
                continue;
        }
}

static  int  mel(char *a, char *b)
{
        int     had, not, r;

        switch  (*b)  {
        default:
                if  (!*a)
                        return  -1;
                if  (*a != *b)
                        return  0;
                return  mel(a+1, b+1);

        case  '\0':
                if  (*a)
                        return  0;
                if  (!mend)
                        mend = a;
                return  1;

        case  '{':
                mbeg = a;
                return  mel(a, b+1);

        case  '}':
                mend = a;
                return  mel(a, b+1);

        case  '*':
                b++;
                for  (;;)  {
                        if  ((r = mel(a, b)) == 1)
                                return  1;
                        if  (r < 0)
                                return  -1;
                        a++;
                }
        case  '?':
                if  (!*a)
                        return  -1;
                return  mel(a+1, b+1);
        case  '[':
                if  (!*a)
                        return  -1;
                b++;
                not = 0;
                had = 0;
                if  (*b == '^' || *b == '!')  {
                        b++;
                        not = 1;
                }
                while  (*b != ']' && *b != '\0')  {
                        int     first, last;

                        first = last = *b++;
                        if  (*b == '-')  {
                                b++;
                                last = *b++;
                        }
                        if  (*a >= first && *a <= last)
                                had = 1;
                }
                return  had == not? 0: mel(a+1, b+1);
        }
}

static  int  match(char *re)
{
        char    *a = Suffix;
        char    *b = re;

        /* '@' matches the null suffix */

        if  (*b == '@')
                return  *a == '\0';

        mbeg = (char *) 0;
        mend = (char *) 0;

        return  mel(a, b) > 0;
}

static  struct kw *findhash(char *nam)
{
        unsigned  hashval = 0;
        char  *cp = nam;
        struct  kw  *kp, **kpp;
        unsigned        nams = 1;

        while  (*cp)  {
                nams++;
                hashval = (hashval << 1) + *cp++;
        }

        kpp = &hashtab[hashval % HASHMOD];

        for  (kp = *kpp;  kp;  kp = kp->k_next)
                if  (strcmp(nam, kp->k_name) == 0)
                        return  kp;

        if  ((kp = (struct kw *)malloc(sizeof(struct kw))) == (struct kw *) 0)
                nomem();

        kp->k_name = (char *) 0;
        kp->k_type = TK_UNDEF;
        kp->k_un.k_str = (struct string *) 0;
        kp->k_nams = (USHORT) nams;
        kp->k_next = *kpp;
        *kpp = kp;
        return  kp;
}

/* Put mnemonic into hash table for proc */

void    sethash(char *mnem, void (*proc)(const int, const unsigned), const unsigned arg)
{
        struct  kw  *kp = findhash(mnem);

        kp->k_name = mnem;
        kp->k_type = TK_PROC;
        kp->k_un.k_pa.k_proc = proc;
        kp->k_un.k_pa.k_arg = arg;
        kp->k_nams = 0;
}

/* Put mnemonic into hash table for keyword.  */

void  setsymb(char *mnem, const int tok)
{
        struct  kw  *kp = findhash(mnem);

        kp->k_name = mnem;
        kp->k_type = (SHORT) tok;
        kp->k_un.k_str = (struct string *) 0;
        kp->k_nams = 0;
}

/* Look up user's name in hash table.  */

static  struct  kw *lookuphash(char *nam)
{
        struct  kw  *kp = findhash(nam);

        if  (kp->k_name == (char *) 0)  {
                if  ((kp->k_name = (char *) malloc((unsigned)(kp->k_nams))) == (char *) 0)
                        nomem();
                strcpy(kp->k_name, nam);
        }
        return  kp;
}

/* Free string if finished with */

void  freestring(struct string *item)
{
        free(item->s_str);
        free((char *) item);
}

/* Copy string */

struct  string  *copystring(struct string *src)
{
        struct string *result = (struct string *) malloc(sizeof(struct string));
        if  (!result)
                nomem();
        result->s_next = (struct string *) 0;
        result->s_length = src->s_length;
        result->s_str = stracpy(src->s_str);
        return  result;
}

/* Apply keyword */

static  void  defkw(struct kw *k, struct string *item)
{
        if  (k->k_type == TK_STR)
                freestring(k->k_un.k_str);
        k->k_type = TK_STR;
        k->k_un.k_str = item;
}

static int  peek()
{
        int     ch;

        for  (;;)  {
                ch = getc(infile);
                if  (ch == '#')  {
                        do  ch = getc(infile);
                        while  (ch != '\n' && ch != EOF);
                        line_count++;
                        continue;
                }
                if  (ch == '\n')  {
                        line_count++;
                        continue;
                }
                if  (!isspace(ch))
                        break;
        }
        ungetc(ch, infile);
        return  ch;
}

static  int  gettok()
{
        char    instring[MAXID];
        char    *bp;
        int     ch, bcnt;

        for  (;;)  {
                ch = getc(infile);
        nxt:
                switch  (ch)  {
                case '#':
                        do  ch = getc(infile);
                        while  (ch != '\n'  &&  ch != EOF);
                        goto  nxt;

                case  '-':
                        Nott = 1;
                        continue;

                default:
                        if  (!isalpha(ch))
                                return  ch;

                case '_':
                        bcnt = 1;
                        bp = instring;
                        do  {
                                *bp++ = (char) ch;
                                if  (++bcnt >= MAXID)
                                        error($E{spdinit line too long});
                                ch = getc(infile);
                        }  while  (isalnum(ch) || ch == '_');
                        ungetc(ch, infile);
                        *bp++ = '\0';
                        lastsym = lookuphash(instring);
                        return  lastsym->k_type;

                case  '\n':
                        line_count++;

                case  ' ':
                case  '\t':
                        continue;

                case  EOF:
                        if  (!formname)
                                return  EOF;
                        fclose(infile);
                        if  (!(infile = fopen(curr_file = formname, "r"))  &&
                             !(default_form[0] &&
                               (infile = fopen(curr_file = default_form, "r"))))  {
                                disp_str = ptdir;
                                disp_str2 = default_form;
                                report($E{spdinit no setup file});
                                return  EOF;
                        }
                        line_count = 1;
                        formname = (char *) 0; /* Stop it happening again */
                        continue;
                }
        }
}

/* Read a string.  */

struct  string *rdstr(const int flags)
{
        int     termch, rc, i;
        char    *result;
        struct  string  *rr;
        unsigned  rlng, reslng;

        rlng = INITSTR;
        reslng = 0;
        if  ((result = (char *) malloc(INITSTR+1)) == (char *) 0)
                nomem();

        for  (;;)  {
                termch = getc(infile);

                switch  (termch)  {
                case  '\n':
                case  EOF:
                        line_count++;
                case  '\t':
                        goto  fin;

                case  ' ':      if  (flags & ST_SPTERM) goto  fin;
                                break;
                case  '\'':     if  (flags & ST_SQTERM) goto  fin;
                                break;
                case  '\"':     if  (flags & ST_DQTERM) goto  fin;
                                break;
                case  '>':      if  (flags & ST_GTTERM) goto  fin;
                                break;

                case  '^':
                        if  (flags & ST_NOESC)
                                break;
                        if  ((termch = getc(infile)) == '^')
                                break;

                        if  (islower(termch))  {
                                termch -= 'a' + 1;
                                break;
                        }

                        if  (termch >= '@' && termch <= '_')  {
                                termch -= '@';
                                break;
                        }
                        break;

                case  '\\':
                        if  (flags & ST_NOESC)  {
                                /* Still allow escape of quote.  */
                                if  (flags & (ST_SQTERM|ST_DQTERM|ST_GTTERM))  {
                                        termch = getc(infile);
                                        switch  (termch)  {
                                        default:
                                                ungetc(termch, infile);
                                                termch = '\\';
                                        case  '\'':case '\"':case '>':
                                                break;
                                        }
                                }
                                break;
                        }

                        termch = getc(infile);

                        switch  (termch)  {
                        case  EOF:
                                goto  fin;

                        default:
                        case  '\\':
                        case  '^':
                        case  '\'':case '\"':
                                break;

                        case  '\n':
                                line_count++;
                                continue;

                        case  'e':
                        case  'E':
                                termch = '\033';
                                break;

                        case  'b':
                        case  'B':
                                termch = '\010';
                                break;

                        case  'r':
                        case  'R':
                                termch = '\r';
                                break;

                        case  'n':
                        case  'N':
                                termch = '\n';
                                break;

                        case  'f':
                        case  'F':
                                termch = 0x0c;
                                break;

                        case  's':
                        case  'S':
                                termch = ' ';
                                break;

                        case  't':
                        case  'T':
                                termch = '\t';
                                break;

                        case  'v':
                        case  'V':
                                termch = '\v';
                                break;

                        case  '0':
                                rc = 0;
                                termch = getc(infile);
                                for  (i = 0;  i < 3;  i++)  {
                                        if  (termch < '0' || termch > '7')
                                                break;
                                        rc = (rc << 3) + termch - '0';
                                        termch = getc(infile);
                                }
                                ungetc(termch, infile);
                                result[reslng++] = (char) rc;
                                goto  chk;

                        case  'x':
                        case  'X':
                                rc = 0;
                                termch = getc(infile);
                                for  (i = 0;  i < 2;  i++)  {
                                        if  (isdigit(termch))
                                                rc = (rc << 4) + termch - '0';
                                        else if (termch>='a' && termch<='f')
                                                rc = (rc << 4) + termch-'a'+10;
                                        else if (termch>='A' && termch<='F')
                                                rc = (rc << 4) + termch-'A'+10;
                                        else
                                                break;
                                        termch = getc(infile);
                                }
                                ungetc(termch, infile);
                                result[reslng++] = (char) rc;
                                goto  chk;
                        }
                }
                result[reslng++] = (char) termch;
chk:            if  (reslng >= rlng)  {
                        rlng += INCSTR;
                        if  ((result = (char *) realloc(result, rlng+1)) == (char *) 0)
                                nomem();
                }
        }

fin:
        /* Result is one char bigger to aid null terminated string stuff.  */

        result[reslng] = '\0';
        if  ((rr = (struct string *)malloc(sizeof(struct string))) == (struct string *) 0)
                nomem();
        rr->s_length = (USHORT) reslng;
        rr->s_str = result;
        rr->s_next = (struct string *) 0;
        return  rr;
}

/* Where we reset a string, deallocate all the bits */

static void  zapstring(struct string **str)
{
        struct  string  *curr = *str;

        while  (curr)  {
                struct string *nxt = curr->s_next;
                freestring(curr);
                curr = nxt;
        }
        *str = (struct string *) 0;
}

static  void  findeol()
{
        int     ch;

        do  ch = getc(infile);
        while  (ch != '\n' && ch != EOF);
}

int  rdnum()
{
        int  result, ch;

        do  {
                if  ((ch = getc(infile)) == '\n')
                        line_count++;
                else  if  (ch == '#')  {
                        do  ch = getc(infile);
                        while  (ch != '\n'  &&  ch != EOF);
                        line_count++;
                }
        }  while  (isspace(ch));

        if  (!isdigit(ch))
                error($E{spdinit digit expected});

        result = 0;
        do  {
                result = result * 10 + ch - '0';
                ch = getc(infile);
        }  while  (isdigit(ch));
        ungetc(ch, infile);
        return  result;
}

void  checkfor(const int ch, char *name)
{
        if  (getc(infile) != ch)   {
                disp_arg[9] = ch;
                if  ((disp_str = name))
                        error($E{spdinit undef name});
                else
                        error($E{spdinit char expected});
        }
}

static void  readfile(const int);

static void  docase(int obey)
{
        char    *re;

        while  (peek() == '(')  {
                checkfor('(', (char *) 0);
                re = rdexpr();
                checkfor(')', (char *) 0);
                if  (obey && match(re))  {
                        readfile(1);
                        obey = 0;
                }
                else
                        readfile(0);
                free(re);
        }
        checkfor('}', (char *) 0);
}

static  void  readcodelist(const int obey, ULONG *result, const unsigned lo, const unsigned hi)
{
        int     ch;
        unsigned  nn, nn2;

        if  (obey)  {
                nn = lo >> 5;
                nn2 = hi >> 5;
                while  (nn <= nn2)
                        result[nn++] = 0;
        }
        for  (;;)  {
                nn = (unsigned) rdnum();
                if  (nn < lo || nn > hi)  {
                        disp_arg[1] = nn;
                        disp_arg[2] = lo;
                        disp_arg[3] = hi;
                        error($E{spdinit number range});
                }
                do ch = getc(infile);
                while  (ch == ' ' || ch == '\t');
                if  (ch == '-')  {
                        nn2 = (unsigned) rdnum();
                        do ch = getc(infile);
                        while  (ch == ' ' || ch == '\t');
                }
                else
                        nn2 = nn;
                if  (obey)  {
                        if  (nn2 < nn)  {
                                unsigned  tmp = nn;
                                nn = nn2;
                                nn2 = tmp;
                        }
                        while  (nn <= nn2)  {
                                result[nn >> 5] |= 1 << (nn & 31);
                                nn++;
                        }
                }
                if  (ch == '\n')  {
                        line_count++;
                        return;
                }
                if  (ch != ',')
                        error($E{spdinit comma-sep expected});
        }
}

static  void  readfile(const int obey)
{
        int     tok, execflag = 0, n;
        struct string   *rdstr(const int);
        struct  string  *r, **oldlast = out_str_last, **wotp;
        unsigned  oldflags = out_params.pi_flags;

        for  (;;)  {
                tok = gettok();
                switch  (tok)  {
                default:                        /*  Assume single char!  */
                        ungetc(tok, infile);
                case  EOF:
                        if  (!obey)  {
                                out_params.pi_flags = oldflags;
                                out_str_last = oldlast;
                        }
                        return;

                case  TK_UNDEF:
                defn:   checkfor('=', lastsym->k_name);
                        r = rdstr(0);
                        if  (obey)
                                defkw(lastsym, r);
                        else
                                freestring(r);
                        continue;

                case  TK_STR:
                        if  (peek() == '=')
                                goto  defn;
                        if  (obey)  {
                                struct  string  *rr = copystring(lastsym->k_un.k_str);
                                *out_str_last = rr;
                                out_str_last = &rr->s_next;
                        }
                        continue;

                case  TK_PROC:
                        (*lastsym->k_un.k_pa.k_proc)(obey, lastsym->k_un.k_pa.k_arg);
                        Nott = 0;
                        continue;

                case  TK_DELIM:
                        if  (isdigit(peek()))  {
                                int     nn = rdnum();
                                if  (nn <= 0)
                                        nn = 1;
                                if  (obey)
                                        out_params.pi_rcount = nn;
                        }
                        else  if  (obey)
                                out_params.pi_rcount = DEF_RCOUNT;
                        out_str_last = &rc_str;
                        execf = 0;
                        goto  end_string;

                case  TK_EXEC:
                        if  (execf == 0)  {
                                error($E{spdinit exec unexpected});
                                continue;
                        }
                        if  (Nott)  {
                                execflag = execf >= PI_EXEXALIGN? ST_NOESC: 0;
                                out_params.pi_flags &= ~execf;
                                Nott = 0;
                        }
                        else  {
                                execflag = ST_NOESC;
                                out_params.pi_flags |= execf;
                        }
                        continue;

                case  TK_SETUP:
                        out_str_last = &su_str;
                        execf = PI_EX_SETUP;
                        goto  end_string;
                case  TK_HALT:
                        out_str_last = &hlt_str;
                        execf = PI_EX_HALT;
                        goto  end_string;
                case  TK_BANNER:
                        if  (Nott)  {
                                Isbann = 0;
                                Nott = 0;
                        }
                        else  if  (obey)  {
                                Hadbann++;
                                Isbann = 1;
                        }
                        continue;

                case  TK_DOCST:
                        if  (Isbann)  {
                                out_str_last = &bds_str;
                                execf = PI_EX_BDOCST;
                        }
                        else  {
                                out_str_last = &ds_str;
                                execf = PI_EX_DOCST;
                        }
                        goto  end_string;
                case  TK_DOCEND:
                        if  (Isbann)  {
                                out_str_last = &bde_str;
                                execf = PI_EX_BDOCEND;
                        }
                        else  {
                                out_str_last = &de_str;
                                execf = PI_EX_DOCEND;
                        }
                        goto  end_string;
                case  TK_SUFST:
                        out_str_last = &ss_str;
                        execf = PI_EX_SUFST;
                        goto  end_string;
                case  TK_SUFEND:
                        out_str_last = &se_str;
                        execf = PI_EX_SUFEND;
                        goto  end_string;
                case  TK_PAGESTART:
                        out_str_last = &ps_str;
                        execf = PI_EX_PAGESTART;
                        goto  end_string;
                case  TK_PAGEEND:
                        out_str_last = &pe_str;
                        execf = PI_EX_PAGEEND;
                        goto  end_string;
                case  TK_ABORT:
                        out_str_last = &ab_str;
                        execf = PI_EX_ABORT;
                        goto  end_string;
                case  TK_RESTART:
                        out_str_last = &res_str;
                        execf = PI_EX_RESTART;
                end_string:
                        /* If we have an = then we reset the string to be zero length. */
                        if (peek() == '=')  {
                                getc(infile);
                                if  (obey)
                                        zapstring(out_str_last);
                        }
                        while  (*out_str_last)
                                out_str_last = &(*out_str_last)->s_next;
                        execflag = (out_params.pi_flags & execf) != 0? ST_NOESC: 0;
                        break;

                /* Fun special cases for new string or function keywords */

                case  TK_BANNPROG:
                        wotp = &out_bannprog;
                        execf = PI_EXBANNPROG;
                        goto  endexecstr;
                case  TK_ALIGN:
                        wotp = &out_align;
                        if  (obey)
                                out_params.pi_flags &= ~PI_EX_ALIGN;
                        execf = PI_EXEXALIGN;
                        goto  endexecstr;
                case  TK_EXECALIGN:
                        wotp = &out_align;
                        if  (obey)
                                out_params.pi_flags |= PI_EX_ALIGN;
                        execf = PI_EXEXALIGN;
                        goto  endexecstr;
                case  TK_PORTSU:
                        wotp = &out_portsu;
                        execf = PI_EXPORTSU;
                        goto  endexecstr;
                case  TK_FILTER:
                        wotp = &out_filter;
                        execf = PI_EXFILTPROG;
                        goto  endexecstr;
                case  TK_NETFILT:
                        wotp = &out_netfilt;
                        execf = PI_EXNETFILT;
                        goto  endexecstr;
                case  TK_STTY:
                        wotp = &out_sttystring;
                endexecstr:
                        execflag = ST_NOESC;            /* Never include escapes */
                        if  (peek() == '=')  {
                                struct  string  *r;
                                getc(infile);
                                r = rdstr(ST_NOESC);
                                if  (obey)  {
                                        zapstring(wotp);
                                        *wotp = r;
                                }
                                else
                                        freestring(r);
                        }
                        else  {
                                out_str_last = wotp;
                                while  (*out_str_last)
                                        out_str_last = &(*out_str_last)->s_next;
                        }
                        break;

                case  '{':
                        docase(obey);
                        continue;

                case  '\'':     n = ST_SQTERM;  goto  quote;
                case  '\"':     n = ST_DQTERM;  goto  quote;
                case  '<':      n = ST_GTTERM;
                quote:
                        r = rdstr(n|execflag);
                        if  (obey)  {
                                *out_str_last = r;
                                out_str_last = &r->s_next;
                        }
                        else
                                freestring(r);
                        continue;

                case  TK_SIGNAL:
                        tok = gettok();
                        switch  (tok)  {
                        default:
                                error($E{spdinit expected seterror});
                                continue;
                        case  TK_OFFLINE:
                                readcodelist(obey, &out_params.pi_offlsig, 1, 31);
                                continue;
                        case  TK_ERROR:
                                readcodelist(obey, &out_params.pi_errsig, 1, 31);
                                continue;
                        }

                case  TK_EXIT:
                        tok = gettok();
                        switch  (tok)  {
                        default:
                                error($E{spdinit expected setoffline});
                                continue;
                        case  TK_OFFLINE:
                                readcodelist(obey, out_params.pi_offlexit, 0, 255);
                                continue;
                        case  TK_ERROR:
                                readcodelist(obey, out_params.pi_errexit, 0, 255);
                                continue;
                        }
                }
        }
}

void  readdescr()
{
        if  (setjmp(syntax_error))
                findeol();
        readfile(1);
}
