/* spr_dumpo.c -- dump out spr options

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

static void	dumpstr(FILE *dest, const char *str)
{
	if  (strpbrk(str, " \t"))
		fprintf(dest, " \'%s\'", str);
	else
		fputs(str, dest);
}

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);

	cancont = spitoption(verbose? $A{spr set verbose}:
			     $A{spr no verbose},
			     $A{spr explain}, dest, '=', cancont);
	cancont = spitoption(interpolate? $A{spr interpolate}:
			     $A{spr no interpolate},
			     $A{spr explain}, dest, ' ', cancont);
	cancont = spitoption(SPQ.spq_jflags & SPQ_NOH? $A{spr no banner}:
			     $A{spr banner},
			     $A{spr explain}, dest, ' ', cancont);
	if  ((SPQ.spq_jflags & (SPQ_MAIL|SPQ_WRT)) != (SPQ_MAIL|SPQ_WRT))
		cancont = spitoption($A{spr no messages},
				     $A{spr explain}, dest, ' ', cancont);
	if  (SPQ.spq_jflags & SPQ_MAIL)
		cancont = spitoption($A{spr mail message},
				     $A{spr explain}, dest, ' ', cancont);
	if  (SPQ.spq_jflags & SPQ_WRT)
		cancont = spitoption($A{spr write message},
				     $A{spr explain}, dest, ' ', cancont);
	if  ((SPQ.spq_jflags & (SPQ_MATTN|SPQ_WATTN)) != (SPQ_MATTN|SPQ_WATTN))
		cancont = spitoption($A{spr no attention},
				     $A{spr explain}, dest, ' ', cancont);
	if  (SPQ.spq_jflags & SPQ_MATTN)
		cancont = spitoption($A{spr mail attention},
				     $A{spr explain}, dest, ' ', cancont);
	if  (SPQ.spq_jflags & SPQ_WATTN)
		cancont = spitoption($A{spr write attention},
				     $A{spr explain}, dest, ' ', cancont);
	cancont = spitoption(SPQ.spq_jflags & SPQ_RETN? $A{spr retain}:
			     $A{spr no retain},
			     $A{spr explain}, dest, ' ', cancont);
	cancont = spitoption(SPQ.spq_jflags & SPQ_LOCALONLY? $A{spr local only}:
			     $A{spr network wide},
			     $A{spr explain}, dest, ' ', cancont);
	spitoption($A{spr copies},
			  $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %d", SPQ.spq_cps);
	if  (SPQ.spq_file[0])  {
		spitoption($A{spr header},
				  $A{spr explain}, dest, ' ', 0);
		dumpstr(dest, SPQ.spq_file);
	}
	spitoption($A{spr formtype},
			  $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %s", wotform);
	if  (SPQ.spq_ptr[0])  {
		spitoption($A{spr printer},
				  $A{spr explain}, dest, ' ', 0);
		dumpstr(dest, SPQ.spq_ptr);
	}
	if  (strcmp(SPQ.spq_puname, SPQ.spq_uname) != 0)  {
		spitoption($A{spr post user},
				  $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %s", SPQ.spq_puname);
	}
	spitoption($A{spr priority},
			  $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %d", SPQ.spq_pri);
	spitoption($A{spr classcode},
			  $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %s", hex_disp(SPQ.spq_class, 0));
	if  (SPQ.spq_flags[0])  {
		spitoption($A{spr post proc flags},
				  $A{spr explain}, dest, ' ', 0);
		dumpstr(dest, SPQ.spq_flags);
	}
	if  (SPQ.spq_start != 0  ||  SPQ.spq_end <= LOTSANDLOTS)  {
		spitoption($A{spr page range},
				  $A{spr explain}, dest, ' ', 0);
		if  (SPQ.spq_start != 0)
			fprintf(dest, " %ld", SPQ.spq_start+1L);
		putc('-', dest);
		if  (SPQ.spq_end <= LOTSANDLOTS)
			fprintf(dest, "%ld", SPQ.spq_end+1L);
	}
	if  (SPQ.spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
		spitoption($A{spr odd even},
				  $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %c", SPQ.spq_jflags & SPQ_ODDP?
			       (SPQ.spq_jflags & SPQ_REVOE? 'A': 'O'): (SPQ.spq_jflags & SPQ_REVOE? 'B': 'E'));
	}
	spitoption($A{spr printed timeout}, $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %d", SPQ.spq_ptimeout);
	spitoption($A{spr not printed timeout}, $A{spr explain}, dest, ' ', 0);
	fprintf(dest, " %d", SPQ.spq_nptimeout);
	if  (SPQ.spq_hold != 0)  {
		time_t	ht = SPQ.spq_hold;
		struct	tm	*tp = localtime(&ht);
		spitoption($A{spr delay until}, $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %.2d/%.2d/%.2d,%.2d:%.2d:%.2d",
			       tp->tm_year % 100,
			       tp->tm_mon + 1,
			       tp->tm_mday,
			       tp->tm_hour,
			       tp->tm_min,
			       tp->tm_sec);
	}
	if  (pfe.delimnum != 1  || pfe.deliml != 1  || delimiter[0] != '\f')  {
		int	ii;
		spitoption($A{spr delimiter number}, $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %ld", (long) pfe.delimnum);
		spitoption($A{spr delimiter}, $A{spr explain}, dest, ' ', 0);
		putc(' ', dest);
		for  (ii = 0;  ii < pfe.deliml;  ii++)  {
			int	ch = delimiter[ii] & 255;
			if  (!isascii(ch))
				fprintf(dest, "\\x%.2x", ch);
			else  if  (iscntrl(ch))  {
				switch  (ch)  {
				case  033:
					fputs("\\e", dest);
					break;
				case  ('h' & 0x1f):
					fputs("\\b", dest);
					break;
				case  '\r':
					fputs("\\r", dest);
					break;
				case  '\n':
					fputs("\\n", dest);
					break;
				case  '\f':
					fputs("\\f", dest);
					break;
				case  '\t':
					fputs("\\t", dest);
					break;
				case  '\v':
					fputs("\\v", dest);
					break;
				default:
					fprintf(dest, "^%c", ch | 0x40);
					break;
				}
			}
			else  {
				switch  (ch)  {
				case  '\\':
				case  '^':
					putc(ch, dest);
				default:
					putc(ch, dest);
					break;
				case  '\'':
				case  '\"':
					putc('\\', dest);
					putc(ch, dest);
					break;
				}
			}
		}
	}
	if  (jobtimeout != 0)  {
		spitoption($A{spr wait time}, $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %d", jobtimeout);
	}

	spitoption($A{spr page limit}, $A{spr explain}, dest, ' ', 0);
	if  (SPQ.spq_pglim != 0)  {
		fprintf(dest, " %c%u", SPQ.spq_dflags & SPQ_ERRLIMIT? 'E': 'N', SPQ.spq_pglim);
		if  (SPQ.spq_dflags & SPQ_PGLIMIT)
			putc('P', dest);
	}
	else
		fputs(" -", dest);

	if  (Out_host != 0L)  {
		spitoption($A{spr host name}, $A{spr explain}, dest, ' ', 0);
		fprintf(dest, " %s", look_host(Out_host));
	}
	putc('\n', dest);
}
