/* o_oddeven.c -- set no odd/even and swap flags

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

static int	o_oddeven(const char * arg)
{
	if  (!arg)
		return  -1;

#ifdef	INLINE_SQCHANGE
	doing_something++;
	oe_changes++;
#endif

	switch  (*arg)  {
	default:
		disp_str = arg;
		print_error($E{Invalid odd even arg});
		exit(E_USAGE);
	case  '-':
		SPQ.spq_jflags &= ~(SPQ_EVENP|SPQ_ODDP|SPQ_REVOE);
		break;
	case  'o':case  'O':
		SPQ.spq_jflags &= ~(SPQ_EVENP|SPQ_REVOE);
		SPQ.spq_jflags |= SPQ_ODDP;
		break;
	case  'e':case  'E':
		SPQ.spq_jflags &= ~(SPQ_ODDP|SPQ_REVOE);
		SPQ.spq_jflags |= SPQ_EVENP;
		break;
	case  'a':case  'A':
		SPQ.spq_jflags &= ~SPQ_ODDP;
		SPQ.spq_jflags |= SPQ_EVENP|SPQ_REVOE;
		break;
	case  'b':case  'B':
		SPQ.spq_jflags &= ~SPQ_EVENP;
		SPQ.spq_jflags |= SPQ_ODDP|SPQ_REVOE;
		break;
	}

	return  1;
}
