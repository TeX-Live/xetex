#ifndef __FORMDATA_H
#define __FORMDATA_H

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
 * $Id: formdata.h,v 1.9 2001/12/14 12:59:16 bagder Exp $
 *****************************************************************************/
/* plain and simple linked list with lines to send */
struct FormData {
  struct FormData *next;
  char *line;
  long length;
};

struct Form {
  struct FormData *data; /* current form line to send */
  int sent; /* number of bytes of the current line that has already
	       been sent in a previous invoke */
};

/* used by FormAdd for temporary storage */
typedef struct FormInfo {
  char *name;
  long namelength;
  char *value;
  long contentslength;
  char *contenttype;
  long flags;
  struct curl_slist* contentheader;
  struct FormInfo *more;
} FormInfo;

int Curl_FormInit(struct Form *form, struct FormData *formdata );

struct FormData *Curl_getFormData(struct HttpPost *post,
                                  int *size);

/* fread() emulation */
int Curl_FormReader(char *buffer,
                    size_t size,
                    size_t nitems,
                    FILE *mydata);

/* possible (old) fread() emulation that copies at most one line */
int Curl_FormReadOneLine(char *buffer,
                         size_t size,
                         size_t nitems,
                         FILE *mydata);

char *Curl_FormBoundary(void);

void Curl_formclean(struct FormData *);

#endif

