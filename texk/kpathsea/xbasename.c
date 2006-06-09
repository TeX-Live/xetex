/* basename.c: return the last element in a path.

    Copyright 2005             Olaf Weber.
    Copyright 1992, 94, 95, 96 Karl Berry.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/* Have to include this first to get c-auto.h.  */
#include <kpathsea/config.h>

#include <kpathsea/c-pathch.h>

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  For example, `basename ("/foo/bar.baz")'
   returns "bar.baz".  */

const_string
xbasename P1C(const_string, name)
{
    const_string base = NULL;
    unsigned len;
  
    for (len = strlen(name); len > 0; len--) {
        if (IS_DIR_SEP(name[len - 1]) || IS_DEVICE_SEP(name[len - 1])) {
            base = name + len;
            break;
        }
    }
    
    if (!base)
        base = name;
    
    return base;
}
