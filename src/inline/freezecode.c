/* freezecode.c -- code to include to implement "freeze" options

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

if  (freeze_wanted)  {
	int	ret1 = 0, ret2 = 0;

	if  (!(mypriv->spu_flgs & PV_FREEZEOK))  {
		print_error($E{No freeze permission});
		exit(E_NOFREEZE);
	}
	if  (freeze_cd)  {
		char	*varname = make_varname();
		if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
			Curr_pwd = runpwd();
		if  ((ret1 = proc_save_opts(Curr_pwd, varname, spit_options)) != 0)
			print_error(ret1);
		free(varname);
	}
	if  (freeze_hd)  {
		char  *hd = envprocess("${HOME-/}");
		char	*varname = make_varname();
		if  ((ret2 = proc_save_opts(hd, varname, spit_options)) != 0)
			print_error(ret2);
		free(hd);
		free(varname);
	}
#ifdef	FREEZE_EXIT
	if  (argv[0] == (char *) 0)
		exit(ret1 != 0  ||  ret2 != 0? E_SETUP: 0);
#endif
}
