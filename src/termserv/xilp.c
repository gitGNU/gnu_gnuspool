/* xilp.c -- LPD interface program semi-deprecated

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
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "incl_net.h"
#include "incl_sig.h"
#include "errors.h"
#include "incl_unix.h"
#include "xilp.h"
#include "defaults.h"
#include "files.h"

static char TEMPORARY_FILE_NAME[32];
static char CONTROL_FILE [32];

RETSIGTYPE  on_signal_quit(int i)
{
	/* Signal handler to remove the temporary files.  */

	unlink(CONTROL_FILE);
	unlink(TEMPORARY_FILE_NAME);
	exit(INTERRUPTED);
}

RETSIGTYPE  on_signal_alarm(int i)
{
	/* Signal handler to remove the temporary files */

	unlink(CONTROL_FILE);
	unlink(TEMPORARY_FILE_NAME);
	fprintf(stderr, "Timed out\n");
	exit(TIMED_OUT);
}

static int  process_arguments(int argc, char **argv, struct information *info)
{
	/* Parse the command line looking for arguments.  */

	int c;
	extern char *optarg;
	char *progname;
	int invalid_arg = 0;

	init_info(info);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];
#define USAGE_STR "hr:pport:Hhost:Uuser:Cclass:Jtitle:Ftitle:#copies:"

	/* check for args that are not supported */

	while  ((c = getopt(argc, argv, "p:hr#:H:U:P:C:J:F:?")) != EOF)
		 switch(c)	{
		 case 'p':
			 info->port_number = get_port_number(optarg);
			 if  (info->port_number	< 0)	{
				 fprintf(stderr, "%s: Invalid port %s\n", progname, optarg);
				 invalid_arg = 1;
			 }
			 continue;
		 case 'h':
			 info->banner_page_on = 1;
			 continue;
		 case 'r':
			 info->remove_on_completion = 1;
			 continue;
		 case '#':
			 info->number_of_copies = get_number_of_copies(optarg);
			 if  (info->number_of_copies < 1)	{
				 fprintf(stderr, "%s: Invalid number of copies %s\n", progname, optarg);
				 invalid_arg = 1;
			 }
			 continue;
		 case 'H':
			 info->host_name = get_host_name(optarg);
			 if  (!info->host_name)	{
				 fprintf(stderr, "%s: Invalid host name %s\n", progname, optarg);
				 invalid_arg = 1;
			 }
			 continue;
		 case 'U':
			 info->user_name = optarg;
			 continue;
		 case 'P':
			 info->printer_name = optarg;
			 continue;
		 case 'C':
			 info->class_name = optarg;
			 continue;
		 case 'J':
		 case 'F':
			 info->file_name = optarg;
			 continue;
		 case '?':
			 show_usage(progname, USAGE_STR);
			 exit(0);
		 default:
			 fprintf(stderr, "%s: Invalid argument %c %s\n", progname, c, optarg);
			 invalid_arg = 1;
			 continue;
		 }

	if  (invalid_arg)
		return -INVALID_ARGUMENT;
	if  (!info->printer_name)
		info->printer_name = "lp";
	if  (!info->host_name)
		return -NO_HOST_NAME;
	if  (!info->port_number)
		info->port_number = get_port_number("printer");

	return  get_number_of_files(argc, argv, "p:hr#:H:U:P:C:JF:");
}

void  print_error_and_quit(int code, char *error_string)
{
	/* Display an error message on stderr and quit.  */

	alarm(0);

	/* Get rid of the temporary files.  */

	unlink(TEMPORARY_FILE_NAME);
	unlink(CONTROL_FILE);
	fprintf(stderr, "%d: %s\n", code, error_string);
	exit(code);
}

int  transmit_header(int sockfd, char *printer_name)
{
	/* Transmit the initial command to tell the remote server that
	   we wish to send some files for printing on the remote machine.  */

	char buffer[64];

	sprintf(buffer, "%c%s\n", 2, printer_name);
	alarm(TIME_OUT_LENGTH);
	if  (send(sockfd, buffer, (unsigned int)strlen(buffer), 0) < 0)
		return -TCP_SEND_ERROR;

	/* Wait for an acknowledgement from the server.  */

	alarm(TIME_OUT_LENGTH);
	if  (recv(sockfd, buffer, sizeof(buffer), 0) < 0)
		return -TCP_RECV_ERROR;
	alarm(0);

	/* Check the acknowledgement.  */

	if  (buffer[0])
		return buffer[0];
	return 0;
}

int  save_in_temporary_file(char *file_name, int input_fd)
{
	/* Store the file "input_fd" in a temporary file returning its
	   size in bytes.  */

	int output_fd;
	char buf[256];
	int size = 0;
	int bytes;

	output_fd = creat(file_name, 0666);
	if  (output_fd < 0)
		return -CANT_OPEN_FILE;
	while  ((bytes = read(input_fd, buf, sizeof(buf))) > 0)	{
		bytes = write(output_fd, buf, bytes);
		if  (bytes < 0)
			return -CANT_WRITE_FILE;
		size += bytes;
	}
	close(output_fd);
	if  (input_fd)
		close(input_fd);
	return size;
}

int  transmit_control_info(int sockfd, struct information *info, int size, int fd)
{
	/* Send the control file to the remote server.  */

	int	bytes;
	char	buffer[256];

	/* Create the name of the control file.  */

	sprintf(buffer, "%c%d cfA%03d%s\n", 2, size, info->first_seq_no, info->local_host_name);

	/* Tell the server its name and size.  */

	alarm(TIME_OUT_LENGTH);
	if  (send(sockfd, buffer, (unsigned int)strlen(buffer), 0) < 0)
		return -TCP_SEND_ERROR;
	alarm(TIME_OUT_LENGTH);

	/* Wait for an acknowledgement.  */

	if  (recv(sockfd, buffer, sizeof(buffer), 0) <= 0)
		return -TCP_RECV_ERROR;
	alarm(0);

	/* Check the acknowledgement.  */

	if  (buffer[0])	{
		return -SERVER_ABORTING_REQUEST;
	}

	/* Rewind the control file and read it in and send it to the server */

	lseek(fd, 0L, 0);

	for  (;;)	{
		if  ((bytes = read(fd, buffer, sizeof(buffer))) < 0)
			return -CANT_READ_FILE;
		if  (!bytes)
			break;
		alarm(TIME_OUT_LENGTH);
		if  (send(sockfd, buffer, (unsigned int)bytes, 0) < 0)
			return -TCP_SEND_ERROR;
		alarm(0);
	}

	/* Send a termintaing zero.  */

	buffer[0] = '\0';
	alarm(TIME_OUT_LENGTH);
	if  (send(sockfd, buffer, (unsigned int)1, 0) < 0)
		return -TCP_SEND_ERROR;

	/* Wait for an acknowledgement.  */

	alarm(TIME_OUT_LENGTH);
	if  (recv(sockfd, buffer, sizeof(buffer), 0) <= 0)
		return -TCP_RECV_ERROR;
	alarm(0);

	/* remove the local control file.  */

	unlink(CONTROL_FILE);

	/* Check the acknowledgement */

	if  (buffer[0])
		return -SERVER_ABORTING_REQUEST;
	return 0;
}

int  get_sequence_number()
{
	/* Attempt to read a sequence file ".seq" if it does not exist create one */

	int fd;
	char buffer[10];
	int bytes;
	int seq_no = 0;
	int new_seq_no;
	int i;

	if  ((fd = open("/tmp/.seq", O_CREAT | O_RDWR, 0666)) < 0)	{
		fd = creat("/tmp/.seq", 0666);
		if  (fd < 0)
			return -EXIT_SYSERROR;
	} else	{

	/*	if  (flock(fd, LOCK_EX | LOCK_NB) < 0)
		return -2;*/

	/* read the sequence number */

		if  ((bytes = read(fd, buffer, sizeof(buffer))) < 0)
			return -EXIT_SYSERROR;
		if  (!bytes)
			new_seq_no = seq_no = 0;
		else
			for  (i = 0 ; i < bytes - 1; i++)
				seq_no = seq_no * 10 + (buffer[i] - '0');
	}

	/* increment the number, modding by 1000 */

	new_seq_no = (seq_no + 1) % 1000;
	sprintf(buffer, "%03d\n", new_seq_no);

	/* save it.  */

	lseek(fd, 0L, 0);
	if  (write(fd, buffer, (unsigned int)strlen(buffer)) < 0)
		return -EXIT_SYSERROR;
	close(fd);
	return seq_no;
}

int  transmit_data(int sockfd, struct information *info, int input_fd, off_t length)
{
	/* send the data to the remote machine */

	off_t	size;
	int	ack_length, temp_file_fd = -1, bytes;
	char	buffer[64];

	/* If we don't know the length of the file ie. it's coming
	   from stdin, save it in a temporary file and make a
	   note of its size */

	if  (length <= 0)
		size = save_in_temporary_file(TEMPORARY_FILE_NAME, input_fd);
	else
		size = length;

	if  (size < 0)
		print_error_and_quit(-size, "error in temporary file");

	/* create the name for the data file and send it to the remote machine */

	sprintf(buffer, "%c%ld df%c%03d%s\n", 3, size, (info->file_no) + 'A', info->seq_no, info->local_host_name );

	alarm(TIME_OUT_LENGTH);
	if  (send(sockfd, buffer, (unsigned int)strlen(buffer), 0) < 0)
		return -TCP_SEND_ERROR;

	/* Wait for an acknowledgement */

	alarm(TIME_OUT_LENGTH);
	ack_length = recv(sockfd, buffer, sizeof(buffer), 0);
	alarm(0);

	/* Check the acknowledgement */

	if  (ack_length < 0)
		return -TCP_RECV_ERROR;
	if  (buffer[0])
		return -SERVER_ABORTING_REQUEST;

	/* if we saved the file in a temporary file, open it.  */

	if  (length == 0L)
		if  ((temp_file_fd = open(TEMPORARY_FILE_NAME, O_RDONLY, 0666)) < 0)
			return -CANT_OPEN_FILE;

	/* read in the data and send it to the remote machine */

	for    (;;)	{
		if  ((bytes = read(length ? input_fd : temp_file_fd, buffer, sizeof(buffer))) < 0)
			return -CANT_READ_FILE;
		size -= bytes;
		if  (!bytes)
			break;
		alarm(TIME_OUT_LENGTH);
		while  (bytes -= send(sockfd, buffer, (unsigned int)bytes, 0))
			alarm(TIME_OUT_LENGTH);

		alarm(0);
		if  (size <= 0)
			break;
	}

	/* Send a terminating 0 */

	buffer[0] = '\0';
	alarm(TIME_OUT_LENGTH);
	if  (send(sockfd, buffer, (unsigned int)1, 0) < 1)
		return -TCP_SEND_ERROR;
	alarm(0);

	/* Cleanup the temporary file */

	if  (!length)
		close(temp_file_fd);
	unlink(TEMPORARY_FILE_NAME);

	/* Wait for an acknowledgement.  */

	alarm(TIME_OUT_LENGTH);
	ack_length = recv(sockfd, buffer, sizeof(buffer), 0);
	alarm(0);

	/* Check the acknowledgement */

	if  (ack_length < 0)
		return -TCP_RECV_ERROR;
	if  (buffer[0])
		return -SERVER_ABORTING_REQUEST;
	return 0;
}

int  create_control_file(struct information *info, int *size)
{
	/* Create the first part of the control file.  The host name
		user name etc.  Save it in a temporary file.  Keep a
		track of the size of this file.  */

	int fd;
	char buffer[64];

	if  ((fd = open(CONTROL_FILE, O_CREAT | O_RDWR, 0666)) < 0)
		return -EXIT_SYSERROR;
	sprintf(buffer, "H%s\n", info->local_host_name);
	*size += write(fd, buffer, (unsigned int)strlen(buffer));
	sprintf(buffer, "P%s\n", info->user_name);
	*size += write(fd, buffer, (unsigned int)strlen(buffer));
	if  (info->banner_page_on)	{
		sprintf(buffer, "J%s\n", info->file_name);
		*size += write(fd, buffer, (unsigned int)strlen(buffer));
		sprintf(buffer, "C%s\n", info->class_name);
		*size += write(fd, buffer, (unsigned int)strlen(buffer));
		sprintf(buffer, "L%s\n", info->user_name);
		*size += write(fd, buffer, (unsigned int)strlen(buffer));
	}
	if  (info->mail_user_on_completion)	{
		sprintf(buffer, "M%s\n", info->user_name);
		*size += write(fd, buffer, (unsigned int)strlen(buffer));
	}
	return fd;
}

void  append_control_file(struct information *info, int *size, int fd)
{
	/* Append to the control file the information which is
	   specific to a particular file.  Keep track of the size.  */

	char buffer[64];

	sprintf(buffer, "fdf%c%03d%s\n", info->file_no + 'A', info->seq_no, info->local_host_name);
	*size += write(fd, buffer, (unsigned int)strlen(buffer));
	sprintf(buffer, "Udf%c%03d%s\n", info->file_no + 'A', info->first_seq_no, info->local_host_name);
	*size += write(fd, buffer, (unsigned int)strlen(buffer));
	sprintf(buffer, "N%s\n", info->file_name);
	*size += write(fd, buffer, (unsigned int)strlen(buffer));
}

void  create_temporary_file_names()
{
	sprintf(CONTROL_FILE, ".rlpcnt%d", (int) getpid());
	sprintf(TEMPORARY_FILE_NAME, ".rlpdata%d", (int) getpid());
}

MAINFN_TYPE  main(int argc, char **argv)
{
	int sockfd;
	int err;
	int number_of_files = 0;
	int size_of_control_file = 0;
	int control_file_fd;
	int i;
	struct information info;

	versionprint(argv, "$Revision: 1.2 $", 1);

	/* Parse the command line for options.  While doing so count
	   the number of file names.  Should this turn out to be
	   zero, we will assume that input is comming from
	   standard input.  Mark: had to change this as err was
	   used before set */

	if  ((number_of_files = process_arguments(argc, argv, &info)) < 0)
		print_error_and_quit(0, "invalid command line");

	info.file_name = "stdin";

	/* Catch any signals.  This is necessary to allow us to remove
	   any temporary files before exiting.  */

	signal(SIGQUIT, on_signal_quit);
	signal(SIGINT, on_signal_quit);
	signal(SIGHUP, on_signal_quit);
	signal(SIGTERM, on_signal_quit);

	/* "alarm" is used throughout, to ensure that we can time out
	   if the remote server dies.  */

	signal(SIGALRM, on_signal_alarm);

	/* We need to be root to gain access to a reserved local port.  */

	create_temporary_file_names();

	if  (setuid(0))
		print_error_and_quit(EXIT_USAGE, "Cannot setuid");

	/* Connect to the remote server */

	if  ((sockfd = make_connection(&info)) < 0)
		print_error_and_quit(-sockfd, "unable to connect to remote host");

	/* Tell the remote server of our intentions.  */

	if  ((err = transmit_header(sockfd, info.printer_name)) != 0)	{
		close(sockfd);
		print_error_and_quit(err, "Unable to send header info");
	}

	/* Start the control file */

	info.file_no = 0;
	if  ((control_file_fd = create_control_file(&info, &size_of_control_file)) < 0)	{
		close(sockfd);
		print_error_and_quit(EXIT_SYSERROR, "Cannot create control file");
	}

	info.first_seq_no = -1;
	info.first_seq_no = info.seq_no = get_sequence_number();
	if  (info.first_seq_no < 0)
		print_error_and_quit(EXIT_SYSERROR, "Cannot create sequence no.");

	if  (number_of_files == 0)	{

	/* reading from standard input.  Send the file */

		if  ((err = transmit_data(sockfd, &info, 0, 0)) < 0)	{
			close(sockfd);
			print_error_and_quit(-err, "Unable to send data");
		}

	/* finish of the control file...  */

		append_control_file(&info, &size_of_control_file, control_file_fd);

	/* ...and send it.  */

		if  ((err = transmit_control_info(sockfd, &info, size_of_control_file, control_file_fd)) < 0)	{
			close(sockfd);
			print_error_and_quit(-err, "Error sending control file");
		}
	} else {
		while  (info.number_of_copies--)	{

			/* sending files which have been supplied as arguments */

			for  (i = 1; i < argc; i++)	{

	/* Read along the command line looking for files to send.  */

				int input_file_fd;
				struct stat file_status;

	/* skip over arguments.  */

				if  (argv[i][0] == '-')	{
					int found = 0;

					static  char args_string[] = "#pHUPCJF";
					char *ptr = args_string;

					while  (*ptr)
						if  (*ptr++ == argv[i][1])
							found = 1;
					if  (found)
						if  (!argv[i][2])
							i++;
					continue;
				}

	/* Open the file and get its size */

				if  ((input_file_fd = open(argv[i], O_RDONLY, 0666)) < 0)	{
					close(sockfd);
					print_error_and_quit(EXIT_SYSERROR, "Error opening file");
				}
				stat(argv[i], &file_status);
				info.file_name = argv[i];


	/* Send the file.  */

				if  ((err = transmit_data(sockfd, &info, input_file_fd, file_status.st_size)) < 0)	{
					close(sockfd);
					print_error_and_quit(-err, "Unable to send data");
				}

				/* if it the last copy and it needs to
				   be removed */

				if  (info.remove_on_completion && !info.number_of_copies)
					unlink(argv[i]);

	/* Store the necessary details in the control file */

				append_control_file(&info, &size_of_control_file, control_file_fd);
				info.file_no++;
				close(input_file_fd);
			}
		}

	/* finally send the control file...  */

		if  ((err = transmit_control_info(sockfd, &info, size_of_control_file, control_file_fd)) < 0)	{
			close(sockfd);
			print_error_and_quit(-err, "Error sending control file");
		}
	}

	/* ...close the connection...  */

	close(sockfd);

	/* ...and quit.  */
	return 0;
}
