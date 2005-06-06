/* $XConsortium: util.c,v 1.6 92/03/26 16:42:26 eswu Exp $ */
/* Copyright International Business Machines,Corp. 1991
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the name of IBM not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.
 *
 * IBM PROVIDES THIS SOFTWARE "AS IS", WITHOUT ANY WARRANTIES
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT
 * LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE QUALITY AND
 * PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF
 * THE SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT IBM) ASSUMES
 * THE ENTIRE COST OF ALL SERVICING, REPAIR AND CORRECTION.  IN
 * NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/* Author: Katherine A. Hitchcock    IBM Almaden Research Laboratory */
 
#include "types.h"
#include <stdio.h>
#include "util.h"
#include "fontmisc.h"

extern char *Xalloc();

static char *vm_base = NULL;  /* Start of virtual memory area */
       char *vm_next = NULL;  /* Pointer to first free byte */
       LONG  vm_free = 0;     /* Count of free bytes */
       LONG  vm_size = 0;     /* Total size of memory */

/*
 * Initialize memory.
 */

extern char *tokenMaxP;   /* Pointer to end of VM we may use + 1 */

boolean vm_init()
{
  vm_next = vm_base = (char *) Xalloc(MAX_STRING_LEN);
 
  if (vm_base != NULL) {
    vm_free = MAX_STRING_LEN;
    vm_size = MAX_STRING_LEN;
    tokenMaxP = vm_next_byte() + MAX_STRING_LEN;
    return(TRUE);
  }
  else
    return(FALSE);
}
 
char *vm_alloc(bytes)
  unsigned int bytes;
{
  char *answer;
 
  /* Round to next LONG multiple (32-bit) */
#if defined(sun) || defined(__hpux) || defined(__alpha)
  bytes = (bytes + 7) & ~7;
#else
  bytes = (bytes + 3) & ~3;
#endif
 
  /* take space from what is left otherwise allocate another CHUNK */
  if (bytes > vm_free)
    if (!vm_init()) return NULL;
  if (bytes <= vm_free) {
    answer = vm_next;
    vm_free -= bytes;
    vm_next += bytes;
    return answer;
  }
  else {
    printf("Trying to allocate %d bytes (%d maximum)\n", bytes, MAX_STRING_LEN);
    return NULL; 
  }
}

/*
 * Format an Integer object
 */
void objFormatInteger(objP,value)
  psobj *objP;
  int value;
{
  if (objP != NULL) {
    objP->type         = OBJ_INTEGER;
    objP->len          = 0;
    objP->data.integer = value;
  }
}
 
/*
 * Format a Real object
 */
#if 0
void objFormatReal(objP,value)
  psobj *objP;
  float value;
#else
void objFormatReal(psobj *objP, float value)
#endif
{
  if (objP != NULL) {
    objP->type       = OBJ_REAL;
    objP->len        = 0;
    objP->data.real  = value;
  }
}
 
/*
 * Format a Boolean object
 */
void objFormatBoolean(objP,value)
  psobj *objP;
  boolean value;
{
  if (objP != NULL) {
    objP->type         = OBJ_BOOLEAN;
    objP->len          = 0;
    objP->data.boolean = value;
  }
}
 
/*
 * Format an Encoding object
 */
void objFormatEncoding(objP,length,valueP)
  psobj *objP;
  int length;
  psobj *valueP;
{
  if (objP != NULL) {
    objP->type        = OBJ_ENCODING;
    objP->len         = length;
    objP->data.arrayP = valueP;
  }
}
 
/*
 * Format an Array object
 */
void objFormatArray(objP,length,valueP)
  psobj *objP;
  int length;
  psobj *valueP;
{
  if (objP != NULL) {
    objP->type        = OBJ_ARRAY;
    objP->len         = length;
    objP->data.arrayP = valueP;
  }
}
 
 
/*
 * Format a String object
 */
void objFormatString(objP,length,valueP)
  psobj *objP;
  int length;
  char *valueP;
{
  if (objP != NULL) {
    objP->type         = OBJ_STRING;
    objP->len          = length;
    objP->data.valueP  = valueP;
  }
}
 
/*
 * Format a Name object
 */
void objFormatName(objP,length,valueP)
  psobj *objP;
  int length;
  char *valueP;
{
  if (objP != NULL) {
    objP->type         = OBJ_NAME;
    objP->len          = length;
    objP->data.nameP   = valueP;
  }
}
 
/*
 * Format a File object
 */
void objFormatFile(objP,valueP)
  psobj *objP;
  FILE *valueP;
{
  if (objP != NULL) {
    objP->type         = OBJ_FILE;
    objP->len          = 0;
    objP->data.fileP   = valueP;
  }
}
 
