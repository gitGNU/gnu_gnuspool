/* asn.c -- asn handling

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
#include "asn.h"

struct asn_node *asn_nodealloc()
{
        struct  asn_node  *result = (struct asn_node *) malloc(sizeof(struct asn_node));

        if  (!result)
                abort();

        result->asn_length = 0;
        result->asn_parent = result->asn_next = result->asn_prev = 0;
        result->asn_firstchild = result->asn_lastchild = 0;
        result->asn_un.asn_string = 0;
        return  result;
}

void  asn_nodefree(struct asn_node *nd)
{
        if  (nd->asn_type == ASN_OCTET_STR  ||  (nd->asn_type == ASN_OBJECT_ID && nd->asn_length != 0))
                free(nd->asn_un.asn_string);
        free((char *) nd);
}

struct  asn_node *asn_alloc_bool(const int val)
{
        struct  asn_node  *result = asn_nodealloc();

        result->asn_type = ASN_BOOLEAN;
        result->asn_length = 1;
        result->asn_int_type = ASN_INT_BOOL;
        result->asn_un.asn_bool = val? 255: 0;
        return  result;
}

struct asn_node *asn_alloc_int(const long val)
{
        struct  asn_node  *result = asn_nodealloc();

        result->asn_type = ASN_INTEGER;
        result->asn_length = 1;
        result->asn_int_type = ASN_INT_SIGNED;
        result->asn_un.asn_signed = val;
        return  result;
}

struct asn_node *asn_alloc_unsigned(const unsigned long val)
{
        struct  asn_node  *result = asn_nodealloc();

        result->asn_type = ASN_INTEGER;
        result->asn_length = 1;
        result->asn_int_type = ASN_INT_UNSIGNED;
        result->asn_un.asn_unsigned = val;
        return  result;
}

struct asn_node *asn_alloc_string(const char *val)
{
        struct  asn_node  *result = asn_nodealloc();

        result->asn_type = ASN_OCTET_STR;
        result->asn_length = strlen(val);
        result->asn_int_type = ASN_INT_STRING;
        result->asn_un.asn_string = malloc(result->asn_length+1);
        if  (!result->asn_un.asn_string)
                abort();
        strcpy(result->asn_un.asn_string, val);
        return  result;
}

static unsigned  countbits(unsigned long n)
{
        unsigned  result = 0;
        while  (n)  {
                n &= n-1;
                result++;
        }
        return  result;
}

static unsigned  objid_segsreq(unsigned long n)
{
        return  (countbits(n) + 6) / 7;
}

static asn_octet *objid_packval(asn_octet *buf, unsigned long n)
{
        unsigned  had = 0;
        asn_octet  *endbuf = buf + objid_segsreq(n);
        asn_octet  *rp = endbuf;
        do  {
                *--rp = (n & 0x7f) | had;
                had = 0x80;
                n >>= 7;
        }  while  (n);
        return  endbuf;
}

struct asn_node *asn_alloc_objid(const char *val)
{
        struct  asn_node  *result = asn_nodealloc();
        const  char  *dp;
        asn_octet  *rp;
        unsigned  segs;
        unsigned  long  sv1, sv2;

        result->asn_type = ASN_OBJECT_ID;
        result->asn_length = 0;
        result->asn_int_type = ASN_INT_OBJID;

        /* Count the number of bytes required
           Have to treat the first two components specially */

        dp = val;
        sv1 = sv2 = 0;
        while  (isdigit(*dp))
                sv1 = sv1 * 10 + *dp++ - '0';
        if  (*dp != '.')
                return  result;
        dp++;
        while  (isdigit(*dp))
                sv2 = sv2 * 10 + *dp++ - '0';

        segs = objid_segsreq(sv1 * 40 + sv2);

        while  (*dp == '.')  {
                dp++;
                sv1 = 0;
                while  (isdigit(*dp))
                        sv1 = sv1 * 10 + *dp++ - '0';
                segs += objid_segsreq(sv1);
        }

        result->asn_length = segs;
        rp = result->asn_un.asn_objid = (asn_octet *) malloc(segs);
        if  (!rp)
                abort();

        /* Now do it again to pack the stuff in,
           again contortions with the first two */

        dp = val;
        sv1 = sv2 = 0;
        while  (isdigit(*dp))
                sv1 = sv1 * 10 + *dp++ - '0';
        if  (*dp != '.')
                return  result;
        dp++;
        while  (isdigit(*dp))
                sv2 = sv2 * 10 + *dp++ - '0';

        rp = objid_packval(rp, sv1 * 40 + sv2);

        while  (*dp == '.')  {
                dp++;
                sv1 = 0;
                while  (isdigit(*dp))
                        sv1 = sv1 * 10 + *dp++ - '0';
                rp = objid_packval(rp, sv1);
        }
        return  result;
}

struct asn_node *asn_alloc_null()
{
        struct  asn_node  *result = asn_nodealloc();
        result->asn_type = ASN_NULL;
        result->asn_length = 0;
        result->asn_int_type = ASN_INT_NULL;
        return  result;
}

struct asn_node *asn_alloc_sequence(const unsigned type)
{
        struct  asn_node  *result = asn_nodealloc();

        result->asn_type = type;
        result->asn_int_type = ASN_INT_SEQUENCE;
        return  result;
}

void  asn_free_sequence(struct asn_node *nd)
{
        struct  asn_node  *nxt;

        do  {
                if  (nd->asn_firstchild)
                        asn_free_sequence(nd->asn_firstchild);
                nxt = nd->asn_next;
                asn_nodefree(nd);
                nd = nxt;
        }  while  (nd);
}

struct asn_node *asn_addnext(struct asn_node *prev, struct asn_node *next)
{
        next->asn_parent = prev->asn_parent;
        next->asn_prev = prev;
        if  ((next->asn_next = prev->asn_next))
                next->asn_next->asn_prev = next;
        else  if  (next->asn_parent)
                next->asn_parent->asn_lastchild = next;
        prev->asn_next = next;
        return  next;
}

struct asn_node *asn_addchild(struct asn_node *parent, struct asn_node *child)
{
        child->asn_parent = parent;
        if  (parent->asn_lastchild)
                return  asn_addnext(parent->asn_lastchild, child);
        child->asn_next = child->asn_prev = 0;
        parent->asn_firstchild = parent->asn_lastchild = child;
        return  parent;
}

struct asn_node *asn_adopt(struct asn_node *parent, struct asn_node *firstchild)
{
        struct  asn_node  *nxt, *last;

        if  (parent->asn_lastchild)  {
                parent->asn_lastchild->asn_next = firstchild;
                firstchild->asn_prev = parent->asn_lastchild;
        }
        else
                parent->asn_firstchild = firstchild;
        last = 0;
        for  (nxt = firstchild;  nxt;  nxt = nxt->asn_next)  {
                nxt->asn_parent = parent;
                last = nxt;
        }
        parent->asn_lastchild = last;
        return  parent;
}

unsigned long  asn_conv_unsigned(struct asn_node *nd)
{
        unsigned  long  result = 0;
        unsigned  bytes = nd->asn_length;
        asn_octet  *op = nd->asn_un.asn_objid;

        while  (bytes != 0)  {
                result = (result << 8) | *op++;
                bytes--;
        }
        return  result;
}

void  ber_enc_init(struct ber_encoding *bec)
{
        asn_octet  *newbuff = (asn_octet *) malloc(BER_ENC_BUFFSIZE);
        if  (!newbuff)
                abort();
        bec->buffer = newbuff;
        bec->endbuffer = newbuff + BER_ENC_BUFFSIZE;
        bec->buffsize = BER_ENC_BUFFSIZE;
        bec->indx = 0;
        bec->nxtptr = bec->endbuffer;
}

void  ber_enc_free(struct ber_encoding *bec)
{
        free((char *) bec->buffer);
}

void  ber_enc_checksize(struct ber_encoding *bec, const unsigned reqsize)
{
        unsigned  nincs, incsize;
        asn_octet  *newbuff;
        unsigned  existing_used = (unsigned) - bec->indx;

        if  (reqsize + existing_used < bec->buffsize)
                return;

        /* Just in case it's longer... */

        nincs = (reqsize + BER_ENC_BUFFINC - 1) / BER_ENC_BUFFINC;

        incsize = bec->buffsize + nincs * BER_ENC_BUFFINC;
        if  (!(newbuff = (asn_octet *) malloc(incsize)))
                abort();

        memcpy(newbuff + incsize - existing_used, bec->endbuffer - existing_used, existing_used);

        free(bec->buffer);
        bec->buffer = newbuff;
        bec->endbuffer = newbuff + incsize;
        bec->buffsize = incsize;
        bec->nxtptr = bec->endbuffer + bec->indx;
}

void  asn_add_octet(struct ber_encoding *bec, const unsigned val)
{
        *--(bec->nxtptr) = val;
        bec->indx--;
}

/* Cheating version for now - just one byte as we don't hold class or p/c separately. */

unsigned  ber_enc_id(struct ber_encoding *bec, const unsigned type)
{
        ber_enc_checksize(bec, 1);
        asn_add_octet(bec, type);
        return  1;
}

unsigned  ber_enc_len(struct ber_encoding *bec, const unsigned len)
{
        unsigned  bcount, lngv;

        ber_enc_checksize(bec, 5); /* Max possible */
        if  (len <= 127)  {
                asn_add_octet(bec, len);
                return  1;
        }
        bcount = 0;
        for  (lngv = len;  lngv;  lngv >>= 8)  {
                asn_add_octet(bec, lngv);
                bcount++;
        }
        asn_add_octet(bec, bcount);
        return  bcount + 1;
}

unsigned  ber_enc_bool(struct ber_encoding *bec, struct asn_node *str)
{
        ber_enc_checksize(bec, 1);
        asn_add_octet(bec, str->asn_un.asn_bool);
        return  1;
}

unsigned  ber_enc_unsigned(struct ber_encoding *bec, struct asn_node *str)
{
        unsigned  bcount = 0, valv, lastbyte = 0xffff;

        ber_enc_checksize(bec, 5); /* Max possible */
        for  (valv = str->asn_un.asn_unsigned;  valv  ||  (lastbyte & 0x80);  valv >>= 8)  {
                asn_add_octet(bec, valv);
                bcount++;
                lastbyte = valv;
        }
        return  bcount;
}

unsigned  ber_enc_int(struct ber_encoding *bec, struct asn_node *str)
{
        unsigned  bcount = 0;

        ber_enc_checksize(bec, 5); /* Max possible */

        if  (str->asn_un.asn_signed < 0)  {
                unsigned  lastbyte = 0;
                long    valv;
                for  (valv = str->asn_un.asn_signed;  valv != -1L  || (lastbyte & 0x80) == 0;  valv >>= 8)  {
                        asn_add_octet(bec, valv);
                        bcount++;
                        lastbyte = valv;
                }
        }
        else  {
                unsigned  lastbyte = 0xffffffff;
                long    valv;
                for  (valv = str->asn_un.asn_signed;  valv  || (lastbyte & 0x80);  valv >>= 8)  {
                        asn_add_octet(bec, valv);
                        bcount++;
                        lastbyte = valv;
                }
        }
        return  bcount;
}

unsigned  ber_enc_string(struct ber_encoding *bec, struct asn_node *str)
{
        unsigned  bcount = str->asn_length;
        const  char  *cp = str->asn_un.asn_string + bcount;
        ber_enc_checksize(bec, bcount);
        while  (bcount != 0)  {
                asn_add_octet(bec, *--cp);
                bcount--;
        }
        return  str->asn_length;
}

unsigned  ber_enc_objid(struct ber_encoding *bec, struct asn_node *str)
{
        unsigned  bcount = str->asn_length;
        const  asn_octet  *cp = str->asn_un.asn_objid + bcount;
        ber_enc_checksize(bec, bcount);
        while  (bcount != 0)  {
                asn_add_octet(bec, *--cp);
                bcount--;
        }
        return  str->asn_length;
}

unsigned  ber_enc_item(struct ber_encoding *bec, struct asn_node *str)
{
        unsigned  totalbytes = 0;

        do  {
                unsigned  databytes;

                switch  (str->asn_int_type)  {
                default:
                        databytes = 0;
                        break;

                case  ASN_INT_NULL:
                        databytes = 0;
                        break;

                case  ASN_INT_BOOL:
                        databytes = ber_enc_bool(bec, str);
                        break;

                case  ASN_INT_SIGNED:
                        databytes = ber_enc_int(bec, str);
                        break;

                case  ASN_INT_UNSIGNED:
                        databytes = ber_enc_unsigned(bec, str);
                        break;

                case  ASN_INT_STRING:
                        databytes = ber_enc_string(bec, str);
                        break;

                case  ASN_INT_OBJID:
                        databytes = ber_enc_objid(bec, str);
                        break;

                case  ASN_INT_SEQUENCE:
                        databytes = str->asn_lastchild? ber_enc_item(bec, str->asn_lastchild): 0;
                        break;
                }

                databytes += ber_enc_len(bec, databytes);
                databytes += ber_enc_id(bec, str->asn_type);
                totalbytes += databytes;
                str = str->asn_prev;
        }  while  (str);

        return  totalbytes;
}

unsigned  ber_encode(asn_octet **res, struct asn_node *seq)
{
        asn_octet  *result;
        unsigned  outlen;
        struct  ber_encoding  bec;

        ber_enc_init(&bec);
        outlen = ber_enc_item(&bec, seq);
        if  (outlen != 0)  {
                result = (asn_octet *) malloc(outlen);
                if  (!result)
                        abort();
                memcpy(result, bec.nxtptr, outlen);
                *res = result;
        }
        ber_enc_free(&bec);
        return  outlen;
}

void  parse_stat_init(struct ber_parse_status *ps, asn_octet *chars, const unsigned len)
{
        ps->seq_start = ps->seq_next = chars;
        ps->seq_len = ps->seq_left = len;
}

unsigned  parse_next_byte(struct ber_parse_status *ps)
{
        if  (ps->seq_left == 0)
                return  0;
        ps->seq_left--;
        return  *ps->seq_next++;
}

long  asn_parse_int(struct ber_parse_status *ps, unsigned len)
{
        long    result;
        unsigned  b = parse_next_byte(ps);

        result = b & 0x80? -1L: 0;

        result <<= 8;
        result |= b;

        while  (--len > 0)
                result = (result << 8) | parse_next_byte(ps);

        return  result;
}

unsigned  get_oid_val(asn_octet **buf, unsigned *len)
{
        unsigned  res = 0;
        unsigned  nxt;

        do  {
                nxt = **buf;
                ++*buf;
                --*len;
                res = (res << 7) | (nxt & 0x7f);
        }  while  (*len != 0  &&  (nxt & 0x80));

        return  res;
}

unsigned  oid_nchars(unsigned val)
{
        unsigned  res = 0;
        do  {
                res++;
                val /= 10;
        }  while  (val != 0);
        return  res;
}

unsigned  parse_oid_len(asn_octet *buf, unsigned len)
{
        unsigned  result;
        unsigned  val = get_oid_val(&buf, &len);

        result = oid_nchars(val / 40) + oid_nchars(val % 40) + 1;

        /* Allow only for 1 '.' */

        while  (len != 0)  {
                val = get_oid_val(&buf, &len);
                result += oid_nchars(val) + 1;
        }
        return  result;
}

char *parse_oid_string(asn_octet *buf, unsigned len)
{
        char    *result = malloc(parse_oid_len(buf, len)+1);
        char    *rp = result;
        unsigned  val;

        if  (!result)
                abort();

        val = get_oid_val(&buf, &len);
#ifdef  CHARSPRINTF
        rp += strlen(sprintf(rp, "%u.%u", val / 40, val % 40));
#else
        rp += sprintf(rp, "%u.%u", val / 40, val % 40);
#endif
        while  (len != 0)  {
                val = get_oid_val(&buf, &len);
#ifdef  CHARSPRINTF
                rp += strlen(sprintf(rp, ".%u", val));
#else
                rp += sprintf(rp, ".%u", val);
#endif
        }
        *rp = '\0';
        return  result;
}

struct asn_node  *parse_asn_item(struct ber_parse_status *ps)
{
        struct  asn_node  *result = 0, *nextres = 0;

        do  {
                unsigned  type, len;
                struct  asn_node  *nxt, *kids;

                type = parse_next_byte(ps);
                len = parse_next_byte(ps);
                if  (type == 0  &&  len == 0)
                        return  result;

                nxt = asn_nodealloc();
                nxt->asn_type = type;

                if  (len & ASN_BIT8)  {
                        unsigned  blen = len & ~ASN_BIT8;
                        if  (blen != 0)  {
                                len = 0;
                                do  {
                                        len = (len << 8) | parse_next_byte(ps);
                                }  while  (--blen > 0);
                        }
                }
                if  (type & ASN_CONSTRUCTOR)  {
                        nxt->asn_int_type = ASN_INT_SEQUENCE;
                        kids = parse_asn_item(ps);
                        if  (!kids)
                                return  0;
                        asn_adopt(nxt, kids);
                }
                else  switch  (type)  {
                default:
                        {
                                asn_octet  *op;
                                nxt->asn_int_type = ASN_OTHER;
                                nxt->asn_length = len;
                                nxt->asn_un.asn_objid = op = (asn_octet *) malloc(len);
                                if  (!op)
                                        abort();
                                while  (len > 0)  {
                                        *op++ = parse_next_byte(ps);
                                        len--;
                                }
                        }
                        break;

                case ASN_OBJECT_ID:
                        nxt->asn_int_type = ASN_INT_OBJID;
                        if  (len != 0)  {
                                asn_octet  *op, *opc;
                                nxt->asn_length = len;
                                opc = op = (asn_octet *) malloc(len);
                                if  (!op)
                                        abort();
                                do  {
                                        *op++ = parse_next_byte(ps);
                                }  while  (--len > 0);
                                nxt->asn_un.asn_string = parse_oid_string(opc, nxt->asn_length);
                                free((char *) opc);
                        }
                        break;

                case  ASN_BOOLEAN:
                        if  (len != 1)
                                return  0;
                        nxt->asn_int_type = ASN_INT_NULL;
                        nxt->asn_length = 1;
                        nxt->asn_un.asn_bool = parse_next_byte(ps);
                        break;

                case  ASN_INTEGER:
                        nxt->asn_int_type = ASN_INT_SIGNED;
                        nxt->asn_un.asn_signed = asn_parse_int(ps, len);
                        break;

                case  ASN_OCTET_STR:
                        nxt->asn_int_type = ASN_INT_STRING;
                        {
                                char  *op;
                                nxt->asn_length = len;
                                op = nxt->asn_un.asn_string = malloc(len+1);
                                if  (!op)
                                        abort();
                                while  (len != 0)  {
                                        *op++ = parse_next_byte(ps);
                                        len--;
                                }
                                *op = '\0';
                        }
                        break;

                case  ASN_NULL:
                        if  (len != 0)
                                return  0;
                        nxt->asn_int_type = ASN_INT_NULL;
                        break;
                }

                if  (result)
                        nextres = asn_addnext(nextres, nxt);
                else
                        nextres = result = nxt;

        }  while  (ps->seq_left != 0);

        return  result;
}
