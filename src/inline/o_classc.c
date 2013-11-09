/* o_classc.c -- option for selecting job by classcode

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

OPTION(o_classcode)
{
        classcode_t     ca;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        ca = Displayopts.opt_classcode = hextoi(arg);
        if  (!(mypriv->spu_flgs & PV_COVER))
                Displayopts.opt_classcode &= mypriv->spu_class;
        if  (Displayopts.opt_classcode == 0)  {
                disp_str = arg;
                disp_str2 = hex_disp(mypriv->spu_class, 0);
                print_error(ca? $E{setting zero class}: $E{specifying zero class});
                exit(E_BADCLASS);
        }

        return  OPTRESULT_ARG_OK;
}
