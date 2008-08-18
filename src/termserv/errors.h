/* errors.h -- error handling for xilp

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

#define EXIT_OFFLINE		(1)
#define EXIT_DEVERROR		(2)
#define EXIT_USAGE		(3)
#define EXIT_SYSERROR		(4)

#define INTERRUPTED		(-1)
#define TIMED_OUT		EXIT_OFFLINE
#define INVALID_ARGUMENT	EXIT_USAGE
#define NO_HOST_NAME		EXIT_USAGE
#define TCP_DATABASE_ERROR	EXIT_SYSERROR
#define SOCKET_CREATE_ERROR	EXIT_SYSERROR
#define CANT_CONNECT_TO_SERVER	EXIT_DEVERROR
#define CANT_SETEUID		EXIT_USAGE
#define TCP_SEND_ERROR		EXIT_DEVERROR
#define TCP_RECV_ERROR		EXIT_DEVERROR
#define CANT_OPEN_FILE		EXIT_DEVERROR
#define CANT_READ_FILE		EXIT_DEVERROR
#define CANT_WRITE_FILE		EXIT_DEVERROR
#define SERVER_ABORTING_REQUEST EXIT_OFFLINE
#define SOCKET_BIND_ERROR	EXIT_SYSERROR
#define SOCKET_LISTEN_ERROR	EXIT_SYSERROR
#define SOCKET_ACCEPT_ERROR	EXIT_DEVERROR
#define INVALID_REQUEST		EXIT_OFFLINE
#define FORK_ERROR		EXIT_SYSERROR
#define WAIT_ERROR		EXIT_SYSERROR
#define SPLIST_ERROR		EXIT_SYSERROR

/* other stuff */

#define QUEUE_A_JOB			(2)
#define PRINT_SHORT_QUEUE_LISTING	(3)
#define PRINT_LONG_QUEUE_LISTING	(4)
#define REMOVE_A_JOB			(5)

#define READING_USER_NAME		(0)
#define READING_JOB_NUMBER		(1)
#define READING_FILE_NAME		(2)
#define READING_SIZE			(3)
