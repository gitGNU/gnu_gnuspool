/* owndirops.c -- stuff to simulte directory ops on machines which don't have them

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

#include <errno.h>

typedef struct  {
        int     dd_fd;
}       DIR;
struct dirent  {
        LONG    d_ino;
        char    d_name[1];
};

/* Big enough to ensure that nulls on end */

static  union  {
        struct  dirent  result_d;
        char    result_b[sizeof(struct direct) + 2];
}  Result;

static  DIR     Res;

DIR     *opendir(filename)
char    *filename;
{
        int     fd;
        struct  stat    sbuf;

        if  ((fd = open(filename, 0)) < 0)
                return  (DIR *) 0;
        if  (fstat(fd, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                errno = ENOTDIR;
                close(fd);
                return  (DIR *) 0;
        }

        Res.dd_fd = fd;
        return  &Res;
}

struct  dirent  *readdir(dirp)
DIR     *dirp;
{
        struct  dirent  indir;

        while  (read(dirp->dd_fd, (char *)&indir, sizeof(indir)) > 0)  {
                if  (indir.d_ino == 0)
                        continue;
                Result.result_d.d_ino = indir.d_ino;
                strncpy(Result.result_d.d_name, indir.d_name, DIRSIZ);
                return  &Result.result_d;
        }
        return  (struct dirent *) 0;
}

void    seekdir(dirp, loc)
DIR     *dirp;
LONG    loc;
{
        lseek(dirp->dd_fd, (long) loc, 0);
}

#define rewinddir(dirp) seekdir(dirp,0)

int     closedir(dirp)
DIR     *dirp;
{
        return  close(dirp->dd_fd);
}
