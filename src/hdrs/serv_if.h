/* serv_if.h -- server interface for MS clients mostly

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

#define SV_CL_DATA              0       /* Data from server */
#define CL_SV_OK                0       /* Data ok */
#define CL_SV_OFFLINE           1       /* Device is offline */
#define CL_SV_ERROR             2       /* Device is error-bound */

#define SV_CL_BUFFSIZE          256
#define SV_CL_MSGBUFF           1024            /* Buffer for spwrite-type messages */
