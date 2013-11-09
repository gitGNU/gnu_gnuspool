/* sstop.c -- stop spooler tidily

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <ctype.h>
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xfershm.h"
#include "q_shm.h"

#define DEFAULT_SUSTIME 300     /* 5 minutes */
#define MAX_SUSTIME     7200    /* 2 hours */

void  nomem()
{
        fprintf(stderr,"Ran out of memory\n");
        exit(E_NOMEM);
}

void  confirm_op(const int promptnum, const int errnum)
{
        char    *cp, *prompt, string[80];

        prompt = gprompt(promptnum);
        fputs(prompt, stdout);
        fflush(stdout);
        cp = fgets(string, sizeof(string), stdin);
        cp = string;
        while  (*cp  &&  !isalpha(*cp))
                cp++;
        if  (toupper(*cp) != 'Y')  {
                print_error(errnum);
                exit(0);
        }
}

/* Ye olde main routine. No arguments are expected.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     countdown = MSGQ_BLOCKS;
        struct  spr_req oreq;
        struct  spdet   *mypriv;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];
        init_mcfile();

        Realuid = getuid();
        Effuid = geteuid();
        INIT_DAEMUID;
        Cfile = open_cfile(MISC_UCONFIG, "rest.help");
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);
        mypriv = getspuser(Realuid);

        if  (Realuid != ROOTID  &&  Realuid != Daemuid  &&  !(mypriv->spu_flgs & PV_SSTOP))  {
                print_error($E{No sstop priv});
                exit(E_NOPRIV);
        }

        /* Open control MSGID */

        if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
                print_error($E{Spooler not running});
                exit(E_NOTRUN);
        }

        BLOCK_ZERO(&oreq, sizeof(oreq));
        oreq.spr_mtype = MT_SCHED;
        oreq.spr_un.o.spr_pid = getpid();

        if  (strcmp(progname, "ssuspend") == 0)  {
                int     hadconf = 0, sustime = 0;
                if  (argc > 1)  {
                        if  (argc > 2)  {
                                if  (argc > 3  ||  argv[1][0] != '-'  ||  toupper(argv[1][1]) != 'Y')  {
                                ssu:
                                        print_error($E{Sstop ssuspend usage});
                                        exit(E_USAGE);
                                }
                                hadconf++;
                                if  (!isdigit(argv[2][0]))
                                        goto  ssu;
                                sustime = atoi(argv[2]);
                        }
                        if  (argv[1][0] == '-')  {
                                if  (toupper(argv[1][1]) != 'Y')
                                        goto  ssu;
                                hadconf++;
                        }
                        else  {
                                if  (!isdigit(argv[1][0]))
                                        goto  ssu;
                                sustime = atoi(argv[1]);
                        }
                }
                if  (!hadconf)
                        confirm_op($P{Confirm suspend scheduling}, $E{Sstop ssuspend not suspended});
                if  (sustime <= 0)  {
                        char    *prompt = gprompt($P{Scheduling suspension time}), *cp;
                        char    string[80];
                        fputs(prompt, stdout);
                        fflush(stdout);
                        cp = fgets(string, sizeof(string), stdin);
                        cp = string;
                        while  (*cp  &&  !isdigit(*cp))
                                cp++;
                        if  (*cp)
                                sustime = atoi(cp);
                        if  (sustime <= 0  ||  sustime > MAX_SUSTIME)
                                sustime = DEFAULT_SUSTIME;
                }
                oreq.spr_un.o.spr_act = SO_SUSPSCHED;
                oreq.spr_un.o.spr_arg1 = sustime;
        }
        else  if  (strcmp(progname, "srelease") == 0)
                oreq.spr_un.o.spr_act = SO_UNSUSPEND;
        else  {
                if  (argc > 1)  {
                        if  (argc != 2 || argv[1][0] != '-'  ||  (argv[1][1] != 'y'  && argv[1][1] != 'Y') || argv[1][2] != '\0')  {
                                print_error($E{sstop usage});
                                exit(E_USAGE);
                        }
                }
                else
                        confirm_op($P{Confirm stop spooler}, $E{sstop not stopped});
                oreq.spr_un.o.spr_act = SO_SSTOP;
                oreq.spr_un.o.spr_arg1 = $E{Sched normal stop};
        }

        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)  {
                        print_error($E{IPC msg q error});
                        exit(E_SETUP);
                }
                print_error($E{IPC msg q full});
                if  (--countdown < 0)  {
                        print_error($E{Sstop given up});
                        exit(E_SETUP);
                }
                print_error($E{Sstop waiting});
                sleep(MSGQ_BLOCKWAIT);
        }
        return  0;
}
