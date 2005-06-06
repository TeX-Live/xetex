/*  $Header: /home/cvsroot/dvipdfmx/src/dpxutil.h,v 1.2 2003/11/29 16:21:16 hirata Exp $

    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>

    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _MX_UTIL_H_
#define _MX_UTIL_H_

#undef  MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#undef  MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#undef  ABS
#define ABS(a)    (((a) < 0) ? -(a) : (a))

#ifndef is_space
#define is_space(c) ((c) == ' '  || (c) == '\t' || (c) == '\f' || \
		     (c) == '\r' || (c) == '\n' || (c) == '\0')
#endif
#ifndef is_delim
#define is_delim(c) ((c) == '(' || (c) == '/' || \
                     (c) == '<' || (c) == '>' || \
		     (c) == '[' || (c) == ']' || \
                     (c) == '{' || (c) == '}' || \
                     (c) == '%')
#endif

extern void skip_white_spaces (unsigned char **s, unsigned char *endptr);
extern int  xtoi     (char c);
extern int  getxpair (unsigned char **str);
extern int  putxpair (unsigned char c, unsigned char **str);

extern unsigned char ostrtouc (unsigned char **inbuf, unsigned char *inbufend, unsigned char *valid);
extern unsigned char esctouc  (unsigned char **inbuf, unsigned char *inbufend, unsigned char *valid);

extern void mangle_name (char *name);

#endif /* _MX_UTIL_H_ */
