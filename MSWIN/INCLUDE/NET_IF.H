/* net_if.h -- network interface

   Copyright 2009 Free Software Foundation, Inc.

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

#define	NET_IF_SENT_OK		0
#define	NET_IF_RECVD_OK		0
#define	NET_IF_SOCK_ERR		1
#define	NET_IF_BIND_ERR		2
#define	NET_IF_SEND_ERR		3
#define	NET_IF_RECV_ERR		4
#define	NET_IF_SERV_NAME	5

//  The following routines are changed as appropriate to an interface module
//  for the network protocol - written in C as C++ is pushing it a bit.

#ifdef	__cplusplus
extern	"C"	{
#endif
short	get_tcp_proto(void);
short	get_udp_proto(void);
short	get_tcp_port(const char *);
short	get_udp_port(const char *);
short	net_port_plus1(short);
short	make_list_sock(short, short);
short	make_probe_sock(short, short);
short	net_tohs(const short);
short	host_tons(const short);
long	net_tohl(const long);
long	host_tonl(const long);
void	net_dbcleanup(void);
short	net_accept(short, netid_t *);
char	*net_hostbyaddr(netid_t, char **);
int	probe_send(const netid_t, const short, const short, char *, const unsigned);
int	probe_recv(const short, char *, const unsigned);
short	net_if_connect(const netid_t, const short, const short);
int	net_if_peek(const short, char *, const unsigned, const unsigned);
int	net_if_read(const short, char *, unsigned, const unsigned);
int	net_if_get(const short, char *, unsigned);
int	net_if_write(const short, const char *, const unsigned);
void	net_if_close(const short);
void	net_fd_zero(void);
void	net_fd_set(const short);
void	net_if_select(const short, const short, const unsigned short);
void	net_if_terminate(void);

/* In udp_ua.c */
int	ua_enq(const netid_t, const short, char *, const unsigned);
#ifdef	__cplusplus
}
#endif
