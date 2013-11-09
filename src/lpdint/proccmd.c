/* proccmd.c -- xtlpd process command routines

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
#include <sys/types.h>
#include <errno.h>
#include "incl_dir.h"
#include "incl_unix.h"
#include "lpdtypes.h"

#define INLINE_MAX      160

static  char    current_string[INLINE_MAX];
static  int     current_number;

int     indentation;

extern  int     debug_level;

extern  char  *expandvars(char *);

#ifndef HAVE_LONG_FILE_NAMES
#include "inline/owndirops.c"
#endif

void  tf_unlink(char *fil, const int dlev)
{
        if  (unlink(fil) >= 0)  {
                if  (debug_level > dlev)
                        fprintf(stderr, "unlink of %s OK\n", fil);
        }
        else  if  (debug_level > dlev)
                fprintf(stderr, "unlink of %s failed: %s\n", fil, errno == ENOENT? "Did not exist": "Other error");
}

static int  my_getline(FILE *inf, char *buf)
{
        int     cnt = 0, ch;

        while  ((ch = getc(inf)) != '\n'  &&  ch != EOF)
                if  (cnt < INLINE_MAX-1)
                        buf[cnt++] = (char) ch;
        if  (ch == EOF)  {
                if  (debug_level > 2)
                        fprintf(stderr, "Read cfile: EOF\n");
                return  0;
        }
        buf[cnt] = '\0';
        if  (debug_level > 2)
                fprintf(stderr, "Read cfile: \'%s\'\n", buf);
        return  cnt;
}

static char *expand_bits(char *str)
{
        unsigned  length = 1;
        char    *result, *rp, *sp, *varname;
        char    nbuf[20];

        if  (!(sp = str))  {
                if  (!(varname = malloc(1)))
                        nomem();
                varname[0] = '\0';
                return  varname;
        }

        varname = malloc((unsigned) (1+strlen(str)));
        if  (!varname)
                nomem();

        /* Calculate length.  */

        while  (*sp)  {
                if  (*sp == '<')  {
                        switch  (*++sp)  {
                        default:
                                while  (*sp && *sp != '>')
                                        ;
                                break;
                        case  's':case  'S':
                        case  'f':case  'F':
                                length += strlen(current_string);
                                do  sp++;
                                while  (*sp && *sp != '>');
                                break;
                        case  'n':case  'N':
                                sprintf(nbuf, "%d", current_number);
                                length += strlen(nbuf);
                                do  sp++;
                                while  (*sp && *sp != '>');
                                break;
                        }
                        if  (*sp == '>')
                                sp++;
                }
                else  {
                        length++;
                        sp++;
                }
        }

        if  (!(result = malloc(length)))
                nomem();

        rp = result;
        sp = str;

        /* Copy over text.  */

        while  (*sp)  {
                if  (*sp == '<')  {
                        switch  (*++sp)  {
                        default:
                                while  (*sp && *sp != '>')
                                        ;
                                break;
                        case  's':case  'S':
                        case  'f':case  'F':
                                strcpy(rp, current_string);
                                rp += strlen(current_string);
                                do  sp++;
                                while  (*sp && *sp != '>');
                                break;
                        case  'n':case  'N':
                                sprintf(nbuf, "%d", current_number);
                                strcpy(rp, nbuf);
                                rp += strlen(nbuf);
                                do  sp++;
                                while  (*sp && *sp != '>');
                                break;
                        }
                        if  (*sp == '>')
                                sp++;
                }
                else
                        *rp++ = *sp++;
        }
        *rp = '\0';
        free(varname);
        return  result;
}

static  void  runcommand(char *filename, char *command)
{
        int     ch;
        FILE    *inf, *outf;

        if  (debug_level > 1)
                fprintf(stderr, "Printing %s to \'%s\'\n", filename, command);

        if  (!(inf = fopen(filename, "r")))  {
                fprintf(stderr, "Lost file %s??\n", filename);
                return;
        }
        if  (!(outf = popen(command, "w")))  {
                fprintf(stderr, "Cannot write to %s\n", command);
                fclose(inf);
                return;
        }
        if  (indentation > 0)  {
                for  (;;)  {
                        ch = getc(inf);
                        if  (ch == EOF)
                                break;
                        if  (ch != '\n')  { /* Avoid indenting empty lines */
                                int     icnt;
                                for  (icnt = 0;  icnt < indentation;  icnt++)
                                        putc(' ', outf);
                                do  {
                                        putc(ch, outf);
                                        if  ((ch = getc(inf)) == EOF)
                                                goto  dun;
                                }  while  (ch != '\n');
                        }
                        putc(ch, outf);
                }
        }
        else
                while  ((ch = getc(inf)) != EOF)
                        putc(ch, outf);
 dun:
        pclose(outf);
        fclose(inf);
}

static  void  runcommandlist(char *str, char *command)
{
        char    *cp, *np;

        if  (!str)
                return;
        for  (cp = str;  *cp == ' ';  cp++)
                ;
        for  (;  (np = strchr(cp, ' '));  cp = np)  {
                *np = '\0';
                runcommand(cp, command);
                *np = ' ';
                do  np++;
                while  (*np  &&  *np == ' ');
        }
        if  (*cp)
                runcommand(cp, command);
}

static void  execute(struct ctrltype *cp)
{
        char    *newstr, *estr, *chp, *np;

        switch  (cp->ctrl_action)  {
        case  CT_NONE:
                return;
        case  CT_INDENT:
                indentation = cp->ctrl_fieldtype == CT_STRING? atoi(cp->ctrl_string): current_number;
                if  (debug_level > 3)
                        fprintf(stderr, "Indentation set to %d\n", indentation);
                return;
        case  CT_UNLINK:
                newstr = expand_bits(cp->ctrl_string);
                estr = expandvars(newstr);
                free(newstr);
                if  (debug_level > 1)
                        fprintf(stderr, "Unlinks: \"%s\"\n", estr);
                for  (chp = estr;  *chp == ' ';  chp++)
                        ;
                for  (;  (np = strchr(chp, ' '));  chp = np)  {
                        *np = '\0';
                        if  (debug_level > 3)
                                fprintf(stderr, "Unlink file: \'%s\'\n", chp);
                        tf_unlink(chp, 3);
                        do  np++;
                        while  (*np  &&  *np == ' ');
                }
                if  (*chp)  {
                        if  (debug_level > 3)
                                fprintf(stderr, "Unlink file: \'%s\'\n", chp);
                        tf_unlink(chp, 3);
                }
                free(estr);
                return;
        case  CT_ASSIGN:
                if  (cp->ctrl_var->vn_value)
                        free(cp->ctrl_var->vn_value);
                estr = expand_bits(cp->ctrl_string);
                cp->ctrl_var->vn_value = expandvars(estr);
                free(estr);
                if  (debug_level > 3)
                        fprintf(stderr, "Assigned %s = %s\n", cp->ctrl_var->vn_name, cp->ctrl_var->vn_value);
                return;
        case  CT_PREASSIGN:
        case  CT_POSTASSIGN:
                newstr = expand_bits(cp->ctrl_string);
                estr = expandvars(newstr);
                free(newstr);
                if  (cp->ctrl_var->vn_value)  {
                        newstr = malloc((unsigned) (strlen(cp->ctrl_var->vn_value) + strlen(estr) + 1));
                        if  (!newstr)
                                nomem();
                        if  (cp->ctrl_action == CT_PREASSIGN)
                                sprintf(newstr, "%s%s", estr, cp->ctrl_var->vn_value);
                        else
                                sprintf(newstr, "%s%s", cp->ctrl_var->vn_value, estr);
                        free(cp->ctrl_var->vn_value);
                        cp->ctrl_var->vn_value = newstr;
                        free(estr);
                }
                else
                        cp->ctrl_var->vn_value = estr;
                if  (debug_level > 3)
                        fprintf(stderr, "%s-assigned %s = %s\n",
                                       cp->ctrl_action == CT_PREASSIGN? "Pre" : "Post",
                                       cp->ctrl_var->vn_name, cp->ctrl_var->vn_value);
                return;
        case  CT_PIPECOMMAND:
                estr = expandvars(cp->ctrl_string);
                if  (cp->ctrl_var)
                        runcommandlist(cp->ctrl_var->vn_value, estr);
                else
                        runcommand(current_string, estr);
                free(estr);
                return;
        }
}

void  process_input(FILE *inf)
{
        int     rcnt, more;
        struct  ctrltype        *cp;
        char    *bp1, *bp2, *tp;
        char    inbuf[INLINE_MAX], inbuf2[INLINE_MAX];

        for  (cp = begin_ctrl;  cp;  cp = cp->ctrl_next)
                execute(cp);

        bp1 = inbuf;
        bp2 = inbuf2;

        more = my_getline(inf, inbuf);
        while  (more)  {
                rcnt = 1;
                for  (;;)  {
                        if  (!(more = my_getline(inf, bp2)))
                                break;
                        if  (strcmp(bp1, bp2) != 0)
                                break;
                        rcnt++;
                }
                if  (*bp1 <= ' ' || *bp1 >= 127)  {
                        fprintf(stderr, "Invalid command line char 0x%x\n", *bp1);
                        goto  nxt;
                }
                cp = ctrl_list[*bp1 - ' '];
                if  (!cp)  {
                        fprintf(stderr, "Unknown control character: %c\n", *bp1);
                        goto  nxt;
                }

                if  (cp->ctrl_repeat)  {
                        if  (repeat_ctrl)  {
                                int     save = current_number;
                                struct  ctrltype  *np;
                                current_number = rcnt;
                                for  (np = repeat_ctrl;  np;  np = np->ctrl_next)
                                        if  (rcnt > 1  ||  !np->ctrl_repeat)
                                                execute(np);
                                current_number = save;
                        }
                        for  (;  cp;  cp = cp->ctrl_next)  {
                                switch  (cp->ctrl_fieldtype)  {
                                case  CT_NUMBER:
                                        current_number = atoi(bp1 + 1);
                                        break;
                                case  CT_STRING:
                                case  CT_FILENAME:
                                        strcpy(current_string, bp1 + 1);
                                        if  (current_string[0] == '\0' || current_string[0] == ' ')
                                                continue;
                                        break;
                                }
                                execute(cp);
                        }
                        if  (norepeat_ctrl)  {
                                int     save = current_number;
                                struct  ctrltype  *np;
                                current_number = rcnt;
                                for  (np = norepeat_ctrl;  np;  np = np->ctrl_next)
                                        if  (rcnt > 1  ||  !np->ctrl_repeat)
                                                execute(np);
                                current_number = save;
                        }
                }
                else  {
                        int     lcnt;
                        for  (lcnt = 0;  lcnt < rcnt;  lcnt++)  {
                                struct  ctrltype        *np;
                                for  (np = cp;  np;  np = np->ctrl_next)  {
                                        switch  (np->ctrl_fieldtype)  {
                                        case  CT_NUMBER:
                                                current_number = atoi(bp1 + 1);
                                                break;
                                        case  CT_STRING:
                                        case  CT_FILENAME:
                                                strcpy(current_string, bp1 + 1);
                                                if  (current_string[0] == '\0' || current_string[0] == ' ')
                                                        continue;
                                                break;
                                        }
                                        execute(np);
                                }
                        }
                }
        nxt:
                tp = bp1;  bp1 = bp2;  bp2 = tp;
        }

        /* Do the end thing.  */

        for  (cp = end_ctrl;  cp;  cp = cp->ctrl_next)
                execute(cp);
}

void  printfiles(char *cfile)
{
        DIR     *dir;
        FILE    *cf;
        struct  dirent  *dent;

        if  (cfile)  {
                if  ((cf = fopen(cfile, "r")))  {
                        process_input(cf);
                        fclose(cf);
                        if  (debug_level > 1)
                                fprintf(stderr, "Unlink cfile \'%s\'\n", cfile);
                        tf_unlink(cfile, 1);
                }
                return;
        }
        if  (!(dir = opendir(".")))
                return;
        while  ((dent = readdir(dir)))  {
                if  (dent->d_name[0] != 'c'  ||  dent->d_name[1] != 'f')
                        continue;
                if  (!(cf = fopen(dent->d_name, "r")))
                        continue;
                process_input(cf);
                fclose(cf);
                if  (debug_level > 1)
                        fprintf(stderr, "Unlink dcfile \'%s\'\n", dent->d_name);
                tf_unlink(dent->d_name, 1);
        }
        closedir(dir);
}
