/* common.c -- xilp common routines

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include <pwd.h>
#include "incl_net.h"
#include "errors.h"
#include "incl_unix.h"
#include "xilp.h"
#include "defaults.h"
#include "files.h"

int	make_connection(struct information *info)
{
	/* Attempt to make a connect to the remote server.  */

	struct hostent *host;
	int sockfd;
	int resvport;
	int port_num;
	extern  char *getlogin();
	struct	sockaddr_in	server_address;

	/* Get the users login name from the environment. Sorry mark
	   if it's run from the spooler there won't be a login
	   name as that got started when the machine was booted
	   before anyone logged in.  */

	port_num = info->port_number;

	if  (!info->user_name)	{
		char	*lname;
		if  (!(lname = getenv("SPOOLPUNAME"))  &&
		     !(lname = getenv("SPOOLJUNAME"))  &&
		     !(lname = getenv("LOGNAME"))  &&
		     !(lname = getlogin()))  {
			struct	passwd	*pw = getpwuid(getuid());
			lname = pw? pw->pw_name: "unknown";
		}
		info->user_name = (char *)malloc((unsigned) strlen(lname)+1);
		strcpy(info->user_name, lname);
	}

	/* Work out the remote hosts internet address.  */

	if  ((host = gethostbyname(info->host_name)) == NULL)
		return -TCP_DATABASE_ERROR;

	/* Get the local hosts name */

	if  (gethostname(info->local_host_name, 64) < 0)
		return -TCP_DATABASE_ERROR;
	if  (!info->class_name)
		info->class_name = info->local_host_name;

	/* Fill in the socket address structure */

	BLOCK_ZERO((char *)&server_address, sizeof(struct sockaddr_in));
	server_address.sin_family = AF_INET;
	server_address.sin_port = (SHORT) port_num;
	BLOCK_COPY((char *)&(server_address.sin_addr), host->h_addr, host->h_length);

	/* Obtain a reserved port for the local machine.  */

	resvport = IPPORT_RESERVED - 1;
	if  ((sockfd = rresvport(&resvport)) < 0)
		return -SOCKET_CREATE_ERROR;

	/* Attempt to connect to the remote machine.  */

	if  (connect(sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) < 0)	{
		close(sockfd);
		return -CANT_CONNECT_TO_SERVER;
	}

	/* Setuid back to the users id.  */

	if  (setuid(getuid()) < 0)
		return -CANT_SETEUID;

	return sockfd;
}

int  accept_connection(struct information *info)
{
	/* Wait for a connection from a remote client.  */

	int sockfd;
	struct	sockaddr_in server_address;

	/* Get the local hosts name */

	if  (gethostname(info->local_host_name, 64) < 0)
		return -TCP_DATABASE_ERROR;

	/* Fill in the socket address structure */

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = info->port_number;
	BLOCK_ZERO((char *)&(server_address.sin_zero), sizeof(server_address.sin_zero));

	if  ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -SOCKET_CREATE_ERROR;

	if  (bind(sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) < 0)
		return SOCKET_BIND_ERROR;

	if  (listen(sockfd, 5) < 0)
		return -SOCKET_LISTEN_ERROR;

	/* Setuid back to the users id.  */

	if  (setuid(getuid()) < 0)
		return -CANT_SETEUID;
	return sockfd;
}

int  get_port_number(char *port_text)
{
	/* returns the network byte ordered port number for either a
	   port name or port number arg a value of -1 is returned
	   upon error */

	int port_number = 0;
	int is_text = 0;
	int i, lpt;
	struct servent *sp;

	if  (port_text == (char *)NULL || !port_text[0])	{

	/* no arg given therefore use the value returned from "printer" */

		if  (!(sp = getservbyname("printer", "tcp")) && !(sp = getservbyname("printer", "TCP")))
			return -1;
		else
			return sp->s_port;
	}

	/* work out whether the arg is a number or a name */

	lpt = strlen(port_text);
	for  (i = 0; i < lpt; i++)
		if  (!isdigit(port_text[i]))	{
			is_text = 1;
			break;
		}

	if  (is_text)	{

	/* if it's a name */

		if  (!(sp = getservbyname(port_text, "tcp")) && !(sp = getservbyname(port_text, "TCP")))
			return -1;
		else
			return sp->s_port;

	}	else	{

	/* otherwise it's a number */

		for  (i = 0; i < lpt; i++)
			port_number = port_number * 10 + (port_text[i] - '0');

		return htons(port_number);
	}
}

int  get_number_of_files(int argc, char **argv, char *arg_chars)
{
	/* whizz along the command line searching for arguments that
	   do not belong to an option */

	int	i, number_of_files = argc - 1;
	char	*ptr;

	for  (i = 1; i < argc; i++)	{

	/* if it's not an option it's a file name */

		if  (argv[i][0] != '-')
			continue;

	/* if it's the string "-" : illegal sorry mark changed this to
	   something quicker which picard cc doesn't winge at */

		if  (!argv[i][1])
			return -1;

	/* otherwise:
		it must be one of three cases:
		CASE 1:		its not a legal option
		CASE 2:		its an option which doesn't use an argument
		CASE 3:		its an option which does use an argument */

		if  ((ptr = strrchr(arg_chars, argv[i][1])) == (char *)0)
			return -1;

		else  if  (*(ptr + 1) != ':')
			number_of_files--;
		else
			number_of_files -= !argv[i][1] ? 2 : 1;
	}
	return number_of_files;
}

int  get_number_of_copies(char *text)
{
	/* convert a string into an integer returning a -1 if any non digit is found.  */

	int number_of_copies = 0;
	int i, lt = strlen(text);

	for  (i = 0; i < lt; i++)
		if  (!isdigit(text[i]))
			return 1;
		else
			number_of_copies = number_of_copies * 10 + (text[i] - '0');

	return number_of_copies;
}

char *get_host_name(char *host_name)
{
	LONG	hostid;

	if  (host_name == (char *)0 || !host_name[0])
		return (char *)0;

	if  (isdigit(host_name[0]))	{
		struct hostent *hp;
#ifdef	DGAVIION
		struct	in_addr  ina_str;
		ina_str = inet_addr(host_name);
		hostid = ina_str.s_addr;
#else
		hostid = inet_addr(host_name);
#endif
		if  (hostid == -1L)
			return (char *)0;

		hp = gethostbyaddr((char *)&hostid, sizeof(hostid), AF_INET);

		return  hp? (char *) hp->h_name: (char *) 0;
	}
	else
		return  host_name;
}

int  show_usage(char *progname, char *option_str)
{
	int i;
	int singles_done = 0;
	int	optlen;

	if  (option_str == (char *)0)
		return -1;

	fprintf(stderr, "usage %s: ", progname);

	if  (option_str[0] != ':')
		fprintf(stderr, "-[");
	else
		singles_done = 1;

	optlen = strlen(option_str);
	for  (i = 0; i < optlen; i++)	{
		if  (!singles_done)	{
			fprintf(stderr, "%c", option_str[i]);
			if  (option_str[i+1] == ':')	{
				singles_done = 1;
				fprintf(stderr, "] ");
				i++;
			}
		}	else	{
			fprintf(stderr, "[-%c<", option_str[i++]);
			while  (option_str[i] && option_str[i] != ':')
				fprintf(stderr, "%c", option_str[i++]);
			fprintf(stderr, ">] ");
		}
	}
	return 0;
}

int  get_user_id(char *name)
{
	struct passwd *pw;

	pw = getpwnam(name);
	if  (pw == (struct passwd *)0)
		return -1;
	else
		return pw->pw_uid;
}

void  init_info(struct information *info)
{
	info->host_name = (char *)0;
	info->printer_name = (char *)0;
	info->local_host_name[0] = '\0';
	info->user_names[0] = '\0';	/* this is needed for xilprm */
	info->user_name = (char *)0;
	info->job_numbers[0] = '\0';	/* this is needed for xilprm */
	info->class_name = (char *)0;
	info->temp_file_name[0] = '\0';
	info->file_name = (char *)0;
	info->port_number = 0;
	info->request_type = 0;
	info->long_listing = 0;
	info->all = 0;
	info->seq_no = 0;
	info->first_seq_no = 0;
	info->file_no = 0;
	info->number_of_copies = 1;
	info->banner_page_on = 0;
	info->mail_user_on_completion = 0;
	info->remove_on_completion = 0;
}

int  connection_accepted(int sockfd)
{
	/* Wait for a client to connect with us.  */

	struct sockaddr_in client_address;
	int new_socket_fd;
	SOCKLEN_T	client_address_size = sizeof(struct sockaddr_in);

	if  ((new_socket_fd = accept(sockfd, (struct sockaddr *)&client_address,
				     &client_address_size)) < 0)
		return -SOCKET_ACCEPT_ERROR;
	return new_socket_fd;
}
