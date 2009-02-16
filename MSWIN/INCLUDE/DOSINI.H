/* dosini.h -- Home-grown DOS versions of the private profile stuff

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

int	DosGetPrivateProfileString(const char *,const char *,const char *,char *,UINT,const char *);
UINT DosGetPrivateProfileInt(const char *,const char *,const int,const char *);
BOOL DosWritePrivateProfileString(const char *,const char *,const char *,const char *);
BOOL DosWritePrivateProfileInt(const char *,const char *,unsigned long,const char *);
BOOL DosWritePrivateProfileHex(const char *,const char *,unsigned long,const char *);
