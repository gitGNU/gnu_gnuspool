/* o_setclass.c -- option to set class code

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

OPTION(o_setclass)
{
        classcode_t     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        num = hextoi(arg);
#ifndef INLINE_RSPR
        if  (!(mypriv->spu_flgs & PV_COVER))
                num &= mypriv->spu_class;
#endif
        if  (num == 0)  {
                disp_str = arg;
                disp_str2 = hex_disp(mypriv->spu_class, 0);
#ifdef  INLINE_SQCHANGE
                print_error($E{setting zero class});
#endif
#ifdef  INLINE_RSPR
                print_error($E{setting zero class});
#endif
#ifdef  INLINE_SPR
                print_error($E{setting zero class});
#endif
                exit(E_BADCLASS);
        }

#ifdef  INLINE_SQCHANGE
        doing_something++;
        cc_changes++;
#endif
#ifdef  INLINE_SPSTART
        set_classcode = num;
        setc++;
#else
        SPQ.spq_class = num;
#endif
        return  OPTRESULT_ARG_OK;
}
