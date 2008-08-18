/* asn.h -- header file for asn handling

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

typedef	unsigned char	asn_octet;

struct	asn_node  {
	unsigned  		asn_type;
	unsigned		asn_length;
	unsigned		asn_int_type;
#define	ASN_INT_NULL		0
#define	ASN_INT_BOOL		1
#define	ASN_INT_SIGNED		2
#define	ASN_INT_UNSIGNED	3
#define	ASN_INT_STRING		4
#define	ASN_INT_OBJID		5
#define	ASN_INT_SEQUENCE	6
#define	ASN_OTHER		10
	struct  asn_node	*asn_parent;
	struct  asn_node	*asn_next;
	struct  asn_node	*asn_prev;
	struct	asn_node	*asn_firstchild;
	struct	asn_node	*asn_lastchild;
	union  {
		int			asn_bool;
		long			asn_signed;
		unsigned  long		asn_unsigned;
		char			*asn_string;
		asn_octet		*asn_objid;
	}  asn_un;
};

#define ASN_BOOLEAN	    0x01
#define ASN_INTEGER	    0x02
#define ASN_BIT_STR	    0x03
#define ASN_OCTET_STR	    0x04
#define ASN_NULL	    0x05
#define ASN_OBJECT_ID	    0x06
#define ASN_SEQUENCE	    0x10
#define ASN_SET		    0x11

#define ASN_UNIVERSAL	    0x00
#define ASN_APPLICATION     0x40
#define ASN_CONTEXT	    0x80
#define ASN_PRIVATE	    0xC0

#define ASN_PRIMITIVE	    0x00
#define ASN_CONSTRUCTOR	    0x20

#define ASN_LONG_LEN	    0x80
#define ASN_EXTENSION_ID    0x1F
#define ASN_BIT8	    0x80

#define IS_CONSTRUCTOR(tbyte)	((tbyte) & ASN_CONSTRUCTOR)
#define IS_EXTENSION_ID(tbyte)	(((tbyte) & ASN_EXTENSION_ID) == ASN_EXTENSION_ID)

#define	BER_ENC_BUFFSIZE    1000
#define	BER_ENC_BUFFINC	     200

struct	ber_encoding  {
	asn_octet	*buffer; 		/* Buffer we are building in */
	asn_octet	*endbuffer;		/* Just past end of that buffer */
	unsigned	buffsize; 		/* Actual size (same as endbuffer-buffer) */
	int		indx;			/* index back from end of buffer */
	asn_octet	*nxtptr; 		/* Next pointer */
};

struct	ber_parse_status  {
	asn_octet	*seq_start; 		/* Start of whole sequence we are decoding */
	asn_octet	*seq_next; 		/* Next byte to be decoded */
	unsigned	seq_len; 		/* Actual length */
	unsigned	seq_left; 		/* Length left */
};

extern void	asn_nodefree(struct asn_node *);
extern void	asn_free_sequence(struct asn_node *);
extern void	ber_enc_init(struct ber_encoding *);
extern void	ber_enc_free(struct ber_encoding *);
extern void	parse_stat_init(struct ber_parse_status *, asn_octet *, const unsigned);

extern struct asn_node *asn_nodealloc(void);
extern struct asn_node *asn_alloc_bool(const int);
extern struct asn_node *asn_alloc_int(const long);
extern struct asn_node *asn_alloc_unsigned(const unsigned long);
extern struct asn_node *asn_alloc_string(const char *);
extern struct asn_node *asn_alloc_objid(const char *);
extern struct asn_node *asn_alloc_null(void);
extern struct asn_node *asn_alloc_sequence(const unsigned);
extern struct asn_node *asn_addnext(struct asn_node *, struct asn_node *);
extern struct asn_node *asn_addchild(struct asn_node *, struct asn_node *);
extern struct asn_node *parse_asn_item(struct ber_parse_status *);

extern unsigned	ber_encode(asn_octet **, struct asn_node *);
extern unsigned long	asn_conv_unsigned(struct asn_node *);

extern void	prinbuf(asn_octet *, const unsigned);
extern void	prin_asn(struct asn_node *, const int);
