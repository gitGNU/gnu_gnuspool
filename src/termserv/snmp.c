/* snmp.c -- SNMP operations

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
#include "incl_unix.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "asn.h"
#include "snmp.h"

extern  int     debug;

asn_octet *gen_snmp_get(const char *commun, const char *objid, unsigned *outlen, const int next)
{
        struct  asn_node  *mainnd, *opnd, *varopnd, *reqnd;
        asn_octet  *coded;

        mainnd = asn_alloc_sequence(ASN_CONSTRUCTOR | ASN_SEQUENCE);
        asn_addchild(mainnd, asn_alloc_int(0)); /* Version */
        asn_addchild(mainnd, asn_alloc_string(commun));
        opnd = asn_alloc_sequence(next? 0xA1: 0xA0);
        srand(time(0));
        asn_addchild(opnd, asn_alloc_unsigned(((rand() % 126 + 1) << 24) | ((rand() % 250 + 1) << 16) | ((rand() % 250 + 1) << 8) | (rand() % 250 + 1)));
        asn_addchild(opnd, asn_alloc_int(0));
        asn_addchild(opnd, asn_alloc_int(0));

        varopnd = asn_alloc_sequence(ASN_CONSTRUCTOR | ASN_SEQUENCE);
        reqnd = asn_alloc_sequence(ASN_CONSTRUCTOR | ASN_SEQUENCE);
        asn_addchild(reqnd, asn_alloc_objid(objid));
        asn_addchild(reqnd, asn_alloc_null());
        asn_addchild(varopnd, reqnd);
        asn_addchild(opnd, varopnd);
        asn_addchild(mainnd, opnd);
        *outlen = ber_encode(&coded, mainnd);
        asn_free_sequence(mainnd);
        return  coded;
}

static void  snmp_gen_ip(struct asn_node *asn, struct snmp_result *rbuf)
{
        asn_octet  *r = asn->asn_un.asn_objid;
        char    obuf[4*4];
        sprintf(obuf, "%u.%u.%u.%u", r[0], r[1], r[2], r[3]);
        rbuf->res_type = RES_TYPE_STRING;
        if  (!(rbuf->res_un.res_string = stracpy(obuf)))
                abort();
}

static void  snmp_gen_counter(struct asn_node *asn, struct snmp_result *rbuf)
{
        rbuf->res_type = RES_TYPE_UNSIGNED;
        rbuf->res_un.res_unsigned = asn_conv_unsigned(asn);
}

static void  snmp_gen_ticks(struct asn_node *asn, struct snmp_result *rbuf)
{
        unsigned  long  t = asn_conv_unsigned(asn);
        unsigned  hours, mins, secs, centisecs;
        char    obuf[30];
        hours = t / 360000L;
        t %= 360000L;
        mins = t / 6000L;
        t %= 6000L;
        secs = t / 100L;
        centisecs = t % 100L;
        sprintf(obuf, "%u:%u:%u.%u", hours, mins, secs, centisecs);
        rbuf->res_type = RES_TYPE_STRING;
        if  (!(rbuf->res_un.res_string = stracpy(obuf)))
                abort();
}

static void  snmp_gen_string(struct asn_node *asn, struct snmp_result *rbuf)
{
        rbuf->res_type = RES_TYPE_STRING;
        if  (!(rbuf->res_un.res_string = malloc(asn->asn_length+1)))
                abort();
        /* Use this as it may have null chars */
        memcpy(rbuf->res_un.res_string, asn->asn_un.asn_string, asn->asn_length);
        rbuf->res_un.res_string[asn->asn_length] = '\0';
}

int  snmp_parse_result(asn_octet *buffer, const unsigned bfsize, struct snmp_result *rbuf)
{
        struct  asn_node  *parsed, *rseq, *ecs;
        struct  ber_parse_status  ps;

        parse_stat_init(&ps, buffer, bfsize);
        parsed = parse_asn_item(&ps);
        if  (debug > 2)  {
                fprintf(stderr, "Parsed ASN sequence...\n");
                prin_asn(parsed, 0);
        }
        if  (!parsed)
                return  RES_ERROR;
        rseq = parsed->asn_lastchild;
        if  (!rseq)
                return  RES_ERROR;
        ecs = rseq->asn_firstchild->asn_next;
        if  (!ecs)
                return  RES_ERROR;
        if  (ecs->asn_int_type != ASN_INT_SIGNED)  {
                asn_free_sequence(parsed);
                return  RES_ERROR;
        }
        if  (ecs->asn_un.asn_signed != 0)  {
                asn_free_sequence(parsed);
                return  ecs->asn_un.asn_signed == 2? RES_UNDEF: RES_ERROR;
        }
        ecs = rseq->asn_lastchild->asn_firstchild->asn_lastchild;
        if  (!ecs)
                return  RES_ERROR;

        rbuf->res_id_string = (char *) 0;
        if  (ecs->asn_prev->asn_int_type == ASN_INT_OBJID)
                rbuf->res_id_string = stracpy(ecs->asn_prev->asn_un.asn_string);

        rbuf->res_length = ecs->asn_length;
        switch  (ecs->asn_type)  {
        default:
                asn_free_sequence(parsed);
                return  RES_ERROR;
        case  0x40:             /* IP Address */
                snmp_gen_ip(ecs, rbuf);
                break;
        case  0x41:             /* Counter */
                snmp_gen_counter(ecs, rbuf);
                break;
        case  0x43:             /* Timeticks */
                snmp_gen_ticks(ecs, rbuf);
                break;
        case  0x46:             /* Counter 64 */
                snmp_gen_counter(ecs, rbuf);
                break;
        case  ASN_INTEGER:              /* Integer */
                rbuf->res_type = RES_TYPE_SIGNED;
                rbuf->res_un.res_signed = ecs->asn_un.asn_signed;
                break;
        case  ASN_OCTET_STR:            /* Octet str */
                snmp_gen_string(ecs, rbuf);
                break;
        case  ASN_NULL:         /* Null */
                rbuf->res_type = RES_NULL;
                break;
        case  ASN_OBJECT_ID:
                /* Parser converts this */
                if  (!(rbuf->res_un.res_string = stracpy(ecs->asn_un.asn_string)))
                        nomem();
                rbuf->res_length = strlen(rbuf->res_un.res_string);
                break;
        }
        asn_free_sequence(parsed);
        return  RES_OK;
}
