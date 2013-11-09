/* client_if.h -- structures and declarations for MS client programs

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

struct  client_if       {
        unsigned  char  flag;                   /* 0 ok otherwise error */
        unsigned  char  resvd[3];               /* Padding */
        jobno_t         jobnum;                 /* Job number or error code */
};

/* OK (or not) */

#define XTNQ_OK         0

/* Error codes */

#define XTNR_UNKNOWN_CLIENT     1
#define XTNR_NOT_CLIENT         2
#define XTNR_NOT_USERNAME       3
#define XTNR_ZERO_CLASS         4
#define XTNR_BAD_PRIORITY       5
#define XTNR_BAD_COPIES         6
#define XTNR_BAD_FORM           7
#define XTNR_NOMEM_QF           8
#define XTNR_BAD_PF             9
#define XTNR_NOMEM_PF           10
#define XTNR_CC_PAGEFILE        11
#define XTNR_FILE_FULL          12
#define XTNR_QFULL              13
#define XTNR_EMPTYFILE          14
#define XTNR_BAD_PTR            15
#define XTNR_WARN_LIMIT         16
#define XTNR_PAST_LIMIT         17
#define XTNR_NO_PASSWD          18
#define XTNR_PASSWD_INVALID     19
#define XTNR_LOST_SYNC          20
#define XTNR_LOST_SEQ           21

/* UDP interface for RECEIVING data from client.
   The interface which we listen on to accept jobs and enquiries.  */

#define CL_SV_UENQUIRY          0       /* Request for permissions (single byte) */
#define CL_SV_STARTJOB          1       /* Start job */
#define CL_SV_CONTDELIM         2       /* More delimiter */
#define CL_SV_JOBDATA           3       /* Job data */
#define CL_SV_ENDJOB            4       /* End of last job */
#define CL_SV_HANGON            5       /* Hang on for next block of data */

#define CL_SV_ULIST             10      /* Send list of valid users */

#define SV_CL_TOENQ             20      /* Are you still there? (single byte) */
#define SV_CL_PEND_FULL         21      /* Queue of pending jobs full */
#define SV_CL_UNKNOWNC          22      /* Unknown command */
#define SV_CL_BADPROTO          23      /* Something wrong protocol */
#define SV_CL_UNKNOWNJ          24      /* Out of sequence job */

/* Reply to request for permissions */

struct  ua_reply        {
        char    ua_uname[UIDSIZE+1];    /* UNIX user name */
        struct  spdet   ua_perm;
};

#define UA_PASSWDSZ     31

/* ua_login structure has now been enhanced to get the machine name in
   as well.  We try to support the old procedure (apart from
   password check) as much as possible.  */

struct  ua_login        {
        unsigned  char  ual_op;                         /* Operation/result as below */
        unsigned  char  ual_fill;                       /* Filler (non-zero for UNIX hosts)*/
        USHORT          ual_fill1;                      /* Filler */
        char            ual_name[WUIDSIZE+1];           /* User or default user */
        char            ual_passwd[UA_PASSWDSZ+1];      /* Password */
        union   {
            /* We don't actually use the machine name any more but we keep the field
               to make the structure the same length as old versions of things were expecting. */
            char        ual_machname[HOSTNSIZE+2];      /* Client's machine name + 1 byte filler */
            int_ugid_t  ual_uid;                        /* User ID to distinguish UNIX clients */
        }  ua_un;
};

#define UAL_LOGIN       30      /* Log in with user name & password */
#define UAL_LOGOUT      31      /* Log out */
#define UAL_ENQUIRE     32      /* Enquire about user id */
#define UAL_OK          33      /* Logged in ok */
#define UAL_NOK         34      /* Not logged in yet */
#define UAL_INVU        35      /* Not logged in, invalid user */
#define UAL_INVP        36      /* Not logged in, invalid passwd */
#define UAL_ULOGIN      37      /* Log in as UNIX user */
#define UAL_UENQUIRE    38      /* Enquire about user from UNIX */

#define CL_SV_BUFFSIZE  512     /* Buffer for data client/server */

struct  ua_pal  {               /* Talk to friends */
        unsigned  char  uap_op;         /* Msg - see below */
        unsigned  char  uap_fill;       /* Filler */
        USHORT          uap_fill1;      /* Filler */
        netid_t         uap_netid;      /* IP we'return talking about */
        char            uap_name[UIDSIZE+1];    /* Unix end user */
        char            uap_wname[WUIDSIZE+1];  /* Windross server */
};

#define UAU_MAXU        20      /* Limit on number times one user logged in */

struct  ua_asku_rep  {
        USHORT          uau_n;          /* Number of people */
        USHORT          uau_fill;       /* Filler */
        netid_t         uau_ips[UAU_MAXU];
};

#define SV_SV_LOGGEDU   50      /* Confirm OK to other servers */
#define SV_SV_ASKU      51      /* Ask other servers about specific user */
#define SV_SV_ASKALL    52      /* Ask other servers about all users */

#define CL_SV_KEEPALIVE 70      /* Keep connection alive */

/* Structure for passing jobs etc across on TCP links.
   We resort to this to avoid problems with missed packets.  */

struct  tcp_data  {
        unsigned  char  tcp_code;       /* What we're doing */

#define TCP_STARTJOB    0               /* Start of job */
#define TCP_PAGESPEC    1               /* Page spec */
#define TCP_ENDJOB      2               /* Job end */
#define TCP_CLOSESOCK   3               /* End of conversation */
#define TCP_DATA        4               /* Data */
#define TCP_STARTJOB_U  5               /* Start of job from UNIX */

        unsigned  char  tcp_seq;        /* Sequence number in data */

        unsigned  short tcp_size;       /* Size for sending data */

        char    tcp_buff[CL_SV_BUFFSIZE];       /* Yer actual data */
};
