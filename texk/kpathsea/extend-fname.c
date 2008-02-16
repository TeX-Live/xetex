/* extend-fname.c: give a filename a suffix, if necessary.

   Copyright 1992, 1993, 2008 Karl Berry.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#include <kpathsea/config.h>


/* We may or may not return NAME.  It's up to the caller not to assume
   the return value is modifiable.  */

string 
extend_filename P2C(const_string, name, const_string, default_suffix)
{
  string new_s;
  const_string suffix = find_suffix (name);

  new_s = suffix == NULL ? concat3 (name, ".", default_suffix)
                         : (string) name;
  return new_s;
}
