/* lpcover.c -- pretend to be unix "LP" command

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
#include "defaults.h"
#include "incl_sig.h"
#include <sys/types.h>
#include "incl_unix.h"
#include "files.h"

#define MAXVEC  30

/* The following are the priorities we map lp -q priorities 0 and 40
   to respectively. Intermediate ones are scaled appropriately.

   These can be arbitrarily reassigned as required.  */

#define HIGHPRI 230
#define LOWPRI  70

char    Ptrname[MAXVEC],
        Formname[MAXVEC],
        Mflag[8],
        Wflag[8],
        Cflag[20],
        Sflag[8],
        prioflag[10],
        Header[MAXVEC],
        Vflag[] = " -v",
        *Rbuf,
        *tempf;

int     needs_copy;

static void  P_option(const char *arg)
{
        if  (strchr(arg, ','))
                needs_copy = 1;

        if  ((Rbuf = (char *) malloc((unsigned) strlen(arg) + 1)) == (char *) 0)  {
                fprintf(stderr, "No memory...\n");
                exit(255);
        }
        strcpy(Rbuf, arg);
}

/* Delete temporary file if we are aborted */

RETSIGTYPE  catchit(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(n, SIG_IGN);
#endif
        unlink(tempf);
        exit(10);
}

void  copy_to_tmp()
{
        int     ch;
        FILE    *outf;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zc;
        zc.sighandler_el = catchit;
        sigmask_clear(zc);
        zc.sigflags_el = SIGVEC_INTFLAG;
#endif
        tempf = tmpnam((char *) 0);

#ifdef  STRUCT_SIG
        sigact_routine(SIGINT, &zc, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &zc, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &zc, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &zc, (struct sigstruct_name *) 0);
#else
        signal(SIGINT, catchit);
        signal(SIGQUIT, catchit);
        signal(SIGHUP, catchit);
        signal(SIGTERM, catchit);
#endif

        if  ((outf = fopen(tempf, "w")) == (FILE *) 0)  {
                fprintf(stderr, "Cannot open temporary file\n");
                exit(2);
        }

        while  ((ch = getchar()) != EOF)
                putc(ch, outf);
        fclose(outf);
}

void  argcat(char *buf, char **arglist)
{
        char    *ep = &buf[strlen(buf)], **ap;
        int     l;

        for  (ap = arglist;  *ap;  ap++)  {
                l = strlen(*ap);
                *ep++ = ' ';
                strcpy(ep, *ap);
                ep += l;
        }
}

int  submit(char **arglist)
{
        int     length = 1 + 6, ret;
        char    *cbuf, *ebuf, **argp;

        for  (argp = arglist;  *argp;  argp++)
                length += strlen(*argp) + 1;

        length += strlen(Ptrname) +
                  strlen(Formname) +
                  strlen(Mflag) +
                  strlen(Wflag) +
                  strlen(Cflag) +
                  strlen(Sflag) +
                  strlen(prioflag) +
                  strlen(Header) +
                  strlen(Vflag);
        if  (Rbuf)
                length += strlen(Rbuf) + 4;

        if  ((cbuf = (char *) malloc((unsigned) length)) == (char *) 0)  {
                fprintf(stderr, "No memory for command\n");
                if  (tempf)
                        unlink(tempf);
                exit(255);
        }

        sprintf(cbuf, "gspl-pr%s%s%s%s%s%s%s%s%s",
                       Ptrname, Formname,
                       Mflag, Wflag,
                       Cflag, Sflag,
                       prioflag, Header,
                       Vflag);
        ebuf = &cbuf[strlen(cbuf)];
        if  (Rbuf)  {
                char    *ap = Rbuf, *cp;

                for  (;;)  {
                        /* If it's several ranges, split each up */
                        if  ((cp = strchr(ap, ',')))
                                *cp = '\0';

                        /* Already as a range - copy literally
                           otherwise generate same start/end page */

                        if  (strchr(ap, '-'))
                                sprintf(ebuf, " -R %s", ap);
                        else
                                sprintf(ebuf, " -R %s-%s", ap, ap);

                        /* Do the business, repeating if we had a , */

                        argcat(ebuf, arglist);
                        if  ((ret = system(cbuf)) != 0)
                                return  ret;
                        if  (!cp)
                                return  0;
                        *cp = ',';
                        ap = cp + 1;
                }
        }

        /* Otherwise do the business and return the exit code.  */

        argcat(ebuf, arglist);
        return  system(cbuf);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        extern  char    *optarg;
        extern  int     optind;
        int     c, ret;
        char    *progname, *lpd;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        if  ((lpd = getenv("LPDEST")))
                sprintf(Ptrname, " -P %s", lpd);

        while  ((c = getopt(argc, argv, "cd:f:H:mn:o:P:q:sS:t:T:wy:")) != EOF)  {
                switch  (c)  {
                default:
                        fprintf(stderr, "Invalid option to %s\n", progname);
                        return  1;

                case  'c':
                        fprintf(stderr, "Warning: files are always copied -c ignored\n");
                        continue;

                case  'd':
                        if  (strcmp(optarg, "any") != 0)  {
                                if  (lpd) /* Make it cancel existing one */
                                        Ptrname[0] = '\0';
                                else
                                        sprintf(Ptrname, " -P %s", optarg);
                        }
                        continue;

                case  'f':
                        sprintf(Formname, " -f %s", optarg);
                        continue;

                case  'H':
                        fprintf(stderr, "There is no direct mapping for -H %s\n", optarg);
                        fprintf(stderr, "Try using -n time to hold print for given time\n");
                        continue;

                case  'm':
                        strcpy(Mflag, " -m");
                        continue;

                case  'n':
                        {
                                int     n = atoi(optarg);
                                if  (n > 1)
                                        sprintf(Cflag, " -c %d", n);
                        }
                        continue;

                case  'o':
                        if  (strcmp(optarg, "nobanner") == 0)  {
                                strcpy(Sflag, " -s");
                                continue;
                        }

                case  'S':
                case  'T':
                case  'y':
                        break;

                case  'P':
                        P_option(optarg);
                        continue;

                case  'q':
                        {
                                int     inpri = atoi(optarg), outpri;

                                /* Map priorities 0 to HIGHPRI, 40 to
                                   LOWPRI with appropriate linear
                                   scale.  */

                                outpri = HIGHPRI -
                                        (inpri * (HIGHPRI - LOWPRI) + 20) / 40;
                                sprintf(prioflag, " -p %d", outpri);
                        }
                        continue;

                case  's':
                        Vflag[0] = '\0';
                        continue;

                case  't':
                        sprintf(Header, " -h \'%s\'", optarg);
                        continue;

                case  'w':
                        strcpy(Wflag, " -w");
                        continue;
                }
                fprintf(stderr, "Sorry no direct mapping for -%c %s option.\nPlease use setup files (and suffixes)\n", c, optarg);

        }

        if  (!argv[optind]  &&  needs_copy)  {
                char    *al[2];

                /* If we have > 1 range, we need to copy to a
                   temporary file */

                copy_to_tmp();

                /* If there is no header present, make one with just a
                   space in so we don't get the name of the
                   temporary file.  */

                if  (Header[0] == '\0')
                        strcpy(Header, " -h \' \'");
                al[0] = tempf;
                al[1] = (char *) 0;
                ret = submit(al);
                unlink(tempf);
        }
        else
                ret = submit(&argv[optind]);

        return  ret;
}
