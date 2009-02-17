#! /bin/sh
#
#   Copyright 2009 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Generate man pages and HTML versions from POD files
#

rm -f man/* html/*
for i in pod/*.[1-8]
do
	eval `echo $i|sed -e 's/\(.*\/\)\(.*\)\.\(.*\)/dir=\1 name=\2 sect=\3/'`
	ln $i $name
	pod2man --section=$sect --release="GNUspool Release 1" --center="GNUspool Print Manager" $name >man/$name.$sect
	pod2html --noindex --outfile=html/${name}_$sect.html --infile=$name
	rm -f $name
done
