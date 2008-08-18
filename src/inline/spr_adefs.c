/* spr_adefs.c -- options for spr

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

static	const	Argdefault	Adefs[] = {
  {  '?', $A{spr explain} },
  {  'v', $A{spr toggle verbose} },
  {  'V', $A{spr toggle verbose} },
  {  's', $A{spr no banner} },
  {  'r', $A{spr banner} },
  {  'x', $A{spr no messages} },
  {  'w', $A{spr write message} },
  {  'm', $A{spr mail message} },
  {  'b', $A{spr no attention} },
  {  'a', $A{spr mail attention} },
  {  'A', $A{spr write attention} },
  {  'z', $A{spr no retain} },
  {  'q', $A{spr retain} },
  {  'l', $A{spr local only} },
  {  'L', $A{spr network wide} },
  {  'i', $A{spr interpolate} },
  {  'I', $A{spr no interpolate} },
  {  'c', $A{spr copies} },
  {  'h', $A{spr header} },
  {  'f', $A{spr formtype} },
  {  'p', $A{spr priority} },
  {  'P', $A{spr printer} },
  {  'u', $A{spr post user} },
  {  'C', $A{spr classcode} },
  {  'R', $A{spr page range} },
  {  'F', $A{spr post proc flags} },
  {  't', $A{spr printed timeout} },
  {  'T', $A{spr not printed timeout} },
  {  'n', $A{spr delay for} },
  {  'N', $A{spr delay until} },
  {  'd', $A{spr delimiter number} },
  {  'D', $A{spr delimiter} },
  {  'O', $A{spr odd even} },
  {  'j', $A{spr wait time} },
  {  'Z', $A{spr page limit} },
  {  'Q', $A{spr host name} },
  {  'o', $A{spr originating host} },
  {  'U', $A{spr originating user} },
  {  'E', $A{spr external system} },
  {  0, 0 }
};
