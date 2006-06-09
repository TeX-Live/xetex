/* str-llist.h: A linked list of strings,

   It's pretty disgusting that both this and str-list exist; the reason is
   that C cannot express iterators very well, and I don't want to change
   all the for loops right now.

   Copyright 1993, 94 Karl Berry.

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

#ifndef STR_LLIST_H
#define STR_LLIST_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* It's a little bizarre to be using the same type for the list and the
   elements of the list, but no reason not to in this case, I think --
   we never need a NULL string in the middle of the list, and an extra
   NULL/NULL element always at the end is inconsequential.  */

struct str_llist_elt
{
  string str;
  boolean moved;
  struct str_llist_elt *next;
};
typedef struct str_llist_elt str_llist_elt_type;
typedef struct str_llist_elt *str_llist_type;

#define STR_LLIST(sl) ((sl).str)
#define STR_LLIST_MOVED(sl) ((sl).moved)
#define STR_LLIST_NEXT(sl) ((sl).next)


/* Add the new string E to the end of the list L.  */
extern void str_llist_add P2H(str_llist_type *l, string e);

/* Reorganize L so that E is below only other elements that have already
   been moved.  Set `moved' member for E.  */
extern void str_llist_float P2H(str_llist_type *l, str_llist_elt_type *e);

#endif /* not STR_LLIST_H */
