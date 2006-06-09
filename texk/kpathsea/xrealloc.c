/* xrealloc.c: realloc with error checking.

    Copyright 2005 Olaf Weber
    Copyright 1992, 93 Karl Berry.

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

#include <kpathsea/config.h>

extern void *xmalloc P1H(unsigned);

void *
xrealloc P2C(void *, old_ptr, unsigned, size)
{
    void *new_mem;
    
    if (old_ptr == NULL) {
        new_mem = xmalloc(size);
    } else {
        new_mem = (void *)realloc(old_ptr, size);
        if (new_mem == NULL) {
            /* We used to print OLD_PTR here using %x, and casting its
               value to unsigned, but that lost on the Alpha, where
               pointers and unsigned had different sizes.  Since the info
               is of little or no value anyway, just don't print it.  */
            fprintf(stderr,
                    "fatal: memory exhausted (realloc of %u bytes).\n",
                    size);
            exit(EXIT_FAILURE);
        }
    }
    
    return new_mem;
}
