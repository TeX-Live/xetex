/*****************************************************************************
 *                                  _   _ ____  _     
 *  Project                     ___| | | |  _ \| |    
 *                             / __| | | | |_) | |    
 *                            | (__| |_| |  _ <| |___ 
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2000, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * In order to be useful for every potential user, curl and libcurl are
 * dual-licensed under the MPL and the MIT/X-derivate licenses.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the MPL or the MIT/X-derivate
 * licenses. You may pick one of these licenses.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: escape.c,v 1.18 2001/10/11 09:32:19 bumblebury Exp $
 *****************************************************************************/

/* Escape and unescape URL encoding in strings. The functions return a new
 * allocated string or NULL if an error occurred.  */

#include "setup.h"
#include <ctype.h>
#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The last #include file should be: */
#ifdef MALLOCDEBUG
#include "memdebug.h"
#endif

char *curl_escape(const char *string, int length)
{
  int alloc = (length?length:(int)strlen(string))+1;  
  char *ns = malloc(alloc);
  unsigned char in;
  int newlen = alloc;
  int index=0;

  length = alloc-1;
  while(length--) {
    in = *string;
    if(' ' == in)
      ns[index++] = '+';
    else if(!(in >= 'a' && in <= 'z') &&
            !(in >= 'A' && in <= 'Z') &&
            !(in >= '0' && in <= '9')) {
      /* encode it */
      newlen += 2; /* the size grows with two, since this'll become a %XX */
      if(newlen > alloc) {
        alloc *= 2;
        ns = realloc(ns, alloc);
        if(!ns)
          return NULL;
      }
      sprintf(&ns[index], "%%%02X", in);

      index+=3;
    }
    else {
      /* just copy this */
      ns[index++]=in;
    }
    string++;
  }
  ns[index]=0; /* terminate it */
  return ns;
}

char *curl_unescape(const char *string, int length)
{
  int alloc = (length?length:(int)strlen(string))+1;
  char *ns = malloc(alloc);
  unsigned char in;
  int index=0;
  unsigned int hex;
  char querypart=FALSE; /* everything to the right of a '?' letter is
                           the "query part" where '+' should become ' '.
                           RFC 2316, section 3.10 */
  
  while(--alloc > 0) {
    in = *string;
    if(querypart && ('+' == in))
      in = ' ';
    else if(!querypart && ('?' == in)) {
      /* we have "walked in" to the query part */
      querypart=TRUE;
    }
    else if('%' == in) {
      /* encoded part */
      if(sscanf(string+1, "%02X", &hex)) {
        in = hex;
        string+=2;
        alloc-=2;
      }
    }
    
    ns[index++] = in;
    string++;
  }
  ns[index]=0; /* terminate it */
  return ns;
  
}

/*
 * local variables:
 * eval: (load-file "../curl-mode.el")
 * end:
 * vim600: fdm=marker
 * vim: et sw=2 ts=2 sts=2 tw=78
 */
