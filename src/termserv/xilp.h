/* xilp.h -- declarations for xilp

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
#include <string.h>

struct information	{
	char *host_name;
	char *printer_name;
	char local_host_name[64];
	char user_names[64];	/* this is needed for xilprm */
	char *user_name;
	char job_numbers[64];	/* this is needed for xilprm */
	char *class_name;
	char temp_file_name[20];
	char *file_name;
	int port_number;
	int request_type;
	int long_listing;
	int all;
	int seq_no;
	int first_seq_no;
	int file_no;
	int number_of_copies;
	int banner_page_on;
	int mail_user_on_completion;
	int remove_on_completion;
};

void	init_info(struct information *);
int	show_usage(char *, char *);
int	make_connection(struct information *);
int	accept_connection(struct information *);
int	get_port_number(char *);
int	get_number_of_copies(char *);
char *	get_host_name(char *);
int	get_number_of_files(int, char **, char *);

#define TIME_OUT_LENGTH 5

#define debugd(name) fprintf(stderr, "%s = %d\n", #name, name);
#define debugs(name) fprintf(stderr, "%s = %s\n", #name, name);
