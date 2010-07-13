/* db.h: lookups in an externally built db file.

   Copyright 1994, 1995, 2008, 2010 Karl Berry.
   Copyright 1999, 2003, 2005 Olaf Weber.

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

#ifndef KPATHSEA_DB_H
#define KPATHSEA_DB_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>
#include <kpathsea/str-list.h>

#ifdef MAKE_KPSE_DLL /* libkpathsea internal only */

/* Initialize the database.  Until this is called, no ls-R matches will
   be found.  */
extern void kpathsea_init_db (kpathsea kpse);

/* Return list of matches for NAME in the ls-R file matching PATH_ELT.  If
   ALL is set, return (null-terminated list) of all matches, else just
   the first.  If no matches, return a pointer to an empty list.  If no
   databases can be read, or PATH_ELT is not in any of the databases,
   return NULL.  */

extern str_list_type *kpathsea_db_search (kpathsea kpse, const_string name, 
                                          const_string path_elt, boolean all);

extern str_list_type *kpathsea_db_search_list (kpathsea kpse,
                                               const_string* names,
                                               const_string  path_elt,
                                               boolean all);

#endif /* MAKE_KPSE_DLL */

/* Insert the filename FNAME into the database.
   Called by mktexpk et al.  */
extern KPSEDLL void kpathsea_db_insert (kpathsea kpse, const_string fname);

#if defined(KPSE_COMPAT_API)

#ifdef MAKE_KPSE_DLL /* libkpathsea internal only */

extern void kpse_init_db (void);

extern str_list_type *kpse_db_search (const_string name, 
                                      const_string path_elt, boolean all);

extern str_list_type *kpse_db_search_list (const_string* names,
                                           const_string  path_elt,
                                           boolean all);

#endif /* MAKE_KPSE_DLL */

extern KPSEDLL void kpse_db_insert (const_string fname);

#endif

#endif /* not KPATHSEA_DB_H */
