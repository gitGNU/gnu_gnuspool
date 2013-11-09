/* o_qhost.c -- option to select which host job is queued on

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

OPTION(o_queuehost)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (strcmp(arg, "-") == 0)  {
                Out_host = 0L;
                return  OPTRESULT_ARG_OK;
        }
        if  ((Out_host = look_int_hostname(arg)) == -1)  {
                disp_str = arg;
                print_error($E{Unknown host name});
                exit(E_USAGE);
        }
#ifdef  INLINE_RSPR
        if  (Out_host == 0)  {
                disp_str = arg;
                print_error($E{Send to self});
                exit(E_USAGE);
        }
#endif
        return  OPTRESULT_ARG_OK;
}
