/* spuser.h -- user permission structures

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

#define        SMAXUID 30000           /*  Above that we do special things  */

#define PV_ADMIN_BIT    0               /*  Administrator  */
#define PV_SSTOP_BIT    1               /*  Can run sstop  */
#define PV_FORMS_BIT    2               /*  Can use other forms  */
#define PV_CPRIO_BIT    3               /*  Can change priority on spq  */
#define PV_OTHERJ_BIT   4               /*  Can change other users' jobs */
#define PV_PRINQ_BIT    5               /*  Can move to printer queue  */
#define PV_HALTGO_BIT   6               /*  Can halt, restart printer  */
#define PV_ANYPRIO_BIT  7               /*  Can set any priority on spq  */
#define PV_CDEFLT_BIT   8               /*  Can change own default prio  */
#define PV_ADDDEL_BIT   9               /*  Can add/delete printers */
#define PV_COVER_BIT    10              /*  Can override class  */
#define PV_UNQUEUE_BIT  11              /*  Can undump queue */
#define PV_VOTHERJ_BIT  12              /*  Can view other jobs not necc edit */
#define PV_REMOTEJ_BIT  13              /*  Can access remote jobs */
#define PV_REMOTEP_BIT  14              /*  Can access remote printers */
#define PV_FREEZEOK_BIT 15              /*  Can freeze parameters */
#define PV_ACCESSOK_BIT 16              /*  Can access other options */
#define PV_OTHERP_BIT   17              /*  Can use other printers */
#define PV_MASQ_BIT     18              /*  Can masquerade as other users */

#define PV_ADMIN        (1 << PV_ADMIN_BIT)             /*  Administrator  */
#define PV_SSTOP        (1 << PV_SSTOP_BIT)             /*  Can run sstop  */
#define PV_FORMS        (1 << PV_FORMS_BIT)             /*  Can use other forms  */
#define PV_CPRIO        (1 << PV_CPRIO_BIT)             /*  Can change priority on spq  */
#define PV_OTHERJ       (1 << PV_OTHERJ_BIT)            /*  Can change other users' jobs */
#define PV_PRINQ        (1 << PV_PRINQ_BIT)             /*  Can move to printer queue  */
#define PV_HALTGO       (1 << PV_HALTGO_BIT)            /*  Can halt, restart printer  */
#define PV_ANYPRIO      (1 << PV_ANYPRIO_BIT)           /*  Can set any priority on spq  */
#define PV_CDEFLT       (1 << PV_CDEFLT_BIT)            /*  Can change own default prio  */
#define PV_ADDDEL       (1 << PV_ADDDEL_BIT)            /*  Can add/delete printers */
#define PV_COVER        (1 << PV_COVER_BIT)             /*  Can override class  */
#define PV_UNQUEUE      (1 << PV_UNQUEUE_BIT)           /*  Can undump queue */
#define PV_VOTHERJ      (1 << PV_VOTHERJ_BIT)           /*  Can view other jobs not necc edit */
#define PV_REMOTEJ      (1 << PV_REMOTEJ_BIT)           /*  Can access remote jobs */
#define PV_REMOTEP      (1 << PV_REMOTEP_BIT)           /*  Can access remote printers */
#define PV_FREEZEOK     (1 << PV_FREEZEOK_BIT)          /*  Can freeze parameters */
#define PV_ACCESSOK     (1 << PV_ACCESSOK_BIT)          /*  Can access other options */
#define PV_OTHERP       (1 << PV_OTHERP_BIT)            /*  Can use other printers */
#define PV_MASQ         (1 << PV_MASQ_BIT)              /*  Can masquerade as other users */
#define ALLPRIVS        0x7ffff         /*  All of the above  */
#define NUM_PRIVS       19              /*  Number of bits */

/* Default defaults.  */

#define U_DF_MINP       100
#define U_DF_MAXP       200
#define U_DF_DEFP       150
#define U_DF_PRIV       (PV_FREEZEOK|PV_ACCESSOK|PV_OTHERP|PV_FORMS|PV_PRINQ|PV_HALTGO|PV_REMOTEJ|PV_REMOTEP)
#define U_DF_CPS        10
#define U_DF_CLASS      0xffffffffL
#define U_MAX_CLASS     0xffffffffL

/* Try quite hard to make everything on a boundary which is a multiple
   of its size.  This will hopefully avoid arguments about gaps
   between machines.  Note that MAXFORM % 4 must == 2 */

/*APISTART - beginning of section copied for API*/

#ifdef  unix

/* DOS C++ Version we aren't interested in header file stuff. */

struct  sphdr   {
        unsigned  char  sph_version;    /* Major GNUspool version number  */

        char    sph_form[MAXFORM+1];    /* Form type (35 bytes) */

        time_t          sph_lastp;      /* Last read password file */

        unsigned  char  sph_minp,       /* Minimum pri */
                        sph_maxp,       /* Maximum pri */
                        sph_defp,       /* Default pri */
                        sph_cps;        /* Copies */

        ULONG   sph_flgs;               /* Privileges */
        classcode_t     sph_class;      /* Class code */
        char            sph_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
        char            sph_ptr[PTRNAMESIZE+1]; /* Default printer */
        char            sph_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
};

#endif

struct  spdet   {
        unsigned  char  spu_isvalid;    /* Valid user id = 1 */
#define SPU_INVALID     0
#define SPU_VALID       1

        char    spu_form[MAXFORM+1];    /* Default form (35 bytes) */

        int_ugid_t      spu_user;       /* User id */

        unsigned  char  spu_minp,       /* Minimum priority  */
                        spu_maxp,       /* Maximum priority  */
                        spu_defp,       /* Default priority  */
                        spu_cps;        /* Copies */

        ULONG   spu_flgs;               /* Privileges  */
        classcode_t     spu_class;      /* Class of printers */
        char            spu_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
        char            spu_ptr[PTRNAMESIZE+1]; /* Default printer */
        char            spu_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
#ifdef  __cplusplus
        classcode_t     resultclass(const classcode_t rclass) const
        {
                return  (spu_flgs & PV_COVER) ? rclass: rclass & spu_class;
        }
        int  ispriv(const ULONG flag) const
        {
                return  flag & spu_flgs? 1: 0;
        }
#endif
};

/* Charge record generated by scheduler */

struct  spcharge        {
        time_t          spch_when;      /* When it happened */
        netid_t         spch_host;      /* Host responsible */
        int_ugid_t      spch_user;      /* Uid charged for */
        USHORT  spch_pri;               /* Priority */
        USHORT  spch_what;              /* Type of charge */
#define SPCH_RECORD     0               /* Record left by spshed */
#define SPCH_FEE        1               /* Impose fee */
#define SPCH_FEEALL     2               /* Impose fee everywhere */
#define SPCH_CONSOL     3               /* Consolidation of previous charges */
#define SPCH_ZERO       4               /* Zero record for given user */
#define SPCH_ZEROALL    5               /* Zero record for all users */
        LONG            spch_chars;     /* Chars printed */
        LONG            spch_cpc;       /* Charge per character or charge (FEE/CONSOL) */
};
/*APIEND - end of section copied for API */

extern  void    putspuhdr();
extern  void    putspulist(struct spdet *);
extern  void    putspuentry(struct spdet *);

extern  classcode_t     hextoi(const char *);
extern  char            *hex_disp(const classcode_t, const int);
extern  struct  spdet   *getspuser(const uid_t);
extern  struct spdet    *getspuentry(const uid_t);
extern  struct spdet    *getspulist();

extern  int     spu_new_format;
