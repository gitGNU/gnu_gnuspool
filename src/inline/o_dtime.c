/* o_dtime.c -- option to set "hold until" time

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

static int  o_dtime(const char *arg)
{
        int     year, month, day, hour, min, sec, num, num2, i;
        time_t  result, testit, now;
        struct  tm      *tp;
        static  char    month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

        if  (!arg)
                return  -1;

#ifdef  INLINE_SQCHANGE
        doing_something++;
        tdel_changes++;
#endif

        disp_str = arg;         /* In case we get an error */

        if  (*arg == '-')  {
                SPQ.spq_hold = 0l;
                return  1;
        }

        now = time((time_t *) 0);
        tp = localtime(&now);
        year = tp->tm_year;
        month = tp->tm_mon + 1;
        day = tp->tm_mday;
        hour = tp->tm_hour;
        min = tp->tm_min;
        sec = tp->tm_sec;

        if  (!isdigit(*arg))  {
        badtime:
                print_error($E{Time arg inval});
                exit(E_USAGE);
        }

        num = 0;
        do      num = num * 10 + *arg++ - '0';
        while  (isdigit(*arg));

        if  (*arg == '/')  {    /* It's a date I think */
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                num2 = 0;
                do      num2 = num2 * 10 + *arg++ - '0';
                while  (isdigit(*arg));

                if  (*arg == '/')  { /* First digits were year */
                        if  (num > 1900)
                                year = num - 1900;
                        else  if  (num > 110)
                                goto  badtime;
                        else  if  (num < 90)
                                year = num + 100;
                        else
                                year = num;
                        arg++;
                        if  (!isdigit(*arg))
                                goto  badtime;
                        month = num2;
                        day = 0;
                        do      day = day * 10 + *arg++ - '0';
                        while  (isdigit(*arg));
                }
                else  {         /* Day/month or Month/day
                                   Decide by which side of the Atlantic */

#ifdef  HAVE_TM_ZONE
                        if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
                        if  (timezone >= 4 * 60 * 60)  {
#endif
                                month = num;
                                day = num2;
                        }
                        else  {
                                month = num2;
                                day = num;
                        }
                        if  (month < tp->tm_mon + 1  ||
                             (month == tp->tm_mon + 1 && day < tp->tm_mday))
                                year++;
                }
                if  (*arg == '\0')
                        goto  finish;
                if  (*arg != ',')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                hour = 0;
                do      hour = hour * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (*arg != ':')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                min = 0;
                do      min = min * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                sec = 0;
                if  (*arg != ':')  {
                        if  (*arg != '\0')
                                goto  badtime;
                }
                else  {
                        arg++;
                        do      sec = sec * 10 + *arg++ - '0';
                        while  (isdigit(*arg));
                }
        }
        else  {

                /* If tomorrow advance date */

                hour = num;
                if  (*arg != ':')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                min = 0;
                do      min = min * 10 + *arg++ - '0';
                while  (isdigit(*arg));

                sec = 0;
                if  (*arg != ':')  {
                        if  (*arg != '\0')
                                goto  badtime;
                }
                else  {
                        arg++;
                        do      sec = sec * 10 + *arg++ - '0';
                        while  (isdigit(*arg));
                }

                if  (hour < tp->tm_hour  ||
                     (hour == tp->tm_hour && min <= tp->tm_min))  {
                        day++;
                        month_days[1] = year % 4 == 0? 29: 28;
                        if  (day > month_days[month-1])  {
                                day = 1;
                                if  (++month > 12)  {
                                        month = 1;
                                        year++;
                                }
                        }
                }
        }

        if  (*arg != '\0')
                goto  badtime;

 finish:
        if  (month > 12 || hour > 23 || min > 59 || sec > 59)
                goto  badtime;

        month_days[1] = year % 4 == 0? 29: 28;
        month--;
        year -= 70;
        if  (day > month_days[month])
                goto  badtime;

        result = year * 365;
        if  (year > 2)
                result += (year + 1) / 4;

        for  (i = 0;  i < month;  i++)
                result += month_days[i];
        result = (result + day - 1) * 24;

        /* Build it up once as at 12 noon and work out timezone shift from that */

        testit = (result + 12) * 60 * 60;
        tp = localtime(&testit);
        result = ((result + hour + 12 - tp->tm_hour) * 60 + min) * 60 + sec;

        if  (result <= now)  {
                print_error($E{Time arg passed});
                exit(E_USAGE);
        }
        SPQ.spq_hold = (LONG) result;
        return  1;
}
