/*  $Header: /home/cvsroot/dvipdfmx/src/dpxutil.c,v 1.2 2003/11/29 16:21:16 hirata Exp $

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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dpxutil.h"

int
xtoi (char c)
{
  if (c >= '0' && c <= '9')
    return (c - '0');
  else if (c >= 'a' && c <= 'f')
    return (c - 'W');
  else if (c >= 'A' && c <= 'F')
    return (c - '7');
  else
    return -1;
}

int
getxpair (unsigned char **s)
{
  int hi, lo;
  hi = xtoi(**s);
  if (hi < 0)
    return hi;
  (*s)++;
  lo = xtoi(**s);
  if (lo < 0)
    return lo;
  (*s)++;
  return ((hi << 4)| lo);
}

int
putxpair (unsigned char c, unsigned char **s)
{
  char hi = (c >> 4), lo = c & 0x0f;
  **s = (hi < 10) ? hi + '0' : hi + '7';
  *(*s+1) = (lo < 10) ? lo + '0' : lo + '7';
  *s += 2;
  return 2;
}

/* Overflowed value is set to invalid char.  */
unsigned char
ostrtouc (unsigned char **inbuf, unsigned char *inbufend, unsigned char *valid)
{
  unsigned char *cur = *inbuf;
  unsigned int   val = 0;

  while (cur < inbufend && cur < *inbuf + 3 &&
	 (*cur >= '0' && *cur <= '7')) {
    val = (val << 3) | (*cur - '0');
    cur++;
  }
  if (val > 255 || cur == *inbuf)
    *valid = 0;
  else
    *valid = 1;

  *inbuf = cur;
  return (unsigned char) val;
}

unsigned char
esctouc (unsigned char **inbuf, unsigned char *inbufend, unsigned char *valid)
{
  unsigned char unescaped, escaped;

  escaped = **inbuf;
  *valid    = 1;
  switch (escaped) {
    /* Backslash, unbalanced paranthes */
  case '\\': case ')': case '(':
    unescaped = escaped;
    (*inbuf)++;
    break;
    /* Other escaped char */ 
  case 'n': unescaped = '\n'; (*inbuf)++; break;
  case 'r': unescaped = '\r'; (*inbuf)++; break;
  case 't': unescaped = '\t'; (*inbuf)++; break;
  case 'b': unescaped = '\b'; (*inbuf)++; break;
  case 'f': unescaped = '\f'; (*inbuf)++; break;
    /*
     * An end-of-line marker preceeded by backslash is not part of a
     * literal string
     */
  case '\r':
    unescaped = 0;
    *valid    = 0;
    *inbuf   += (*inbuf < inbufend - 1 && *(*inbuf+1) == '\n') ? 2 : 1;
    break;
  case '\n':
    unescaped = 0;
    *valid    = 0;
    (*inbuf)++;
    break;
    /* Possibly octal notion */ 
  default:
    unescaped = ostrtouc(inbuf, inbufend, valid);
  }

  return unescaped;
}

void
skip_white_spaces (unsigned char **s, unsigned char *endptr)
{
  while (*s < endptr)
    if (!is_space(**s))
      break;
    else
      (*s)++;
}

void
mangle_name(char *name)
{
  int i;
  char ch;
  static char first = 1;
  memmove (name+7, name, strlen(name)+1);
  /* The following procedure isn't very random, but it
     doesn't need to be for this application. */
  if (first) {
    srand (time(NULL));
    first = 0;
  }
  for (i=0; i<6; i++) {
    ch = rand() % 26;
    name[i] = ch+'A';
  }
  name[6] = '+';
}
