/*  $Header: /home/cvsroot/dvipdfmx/src/pdfparse.c,v 1.24 2004/01/31 01:22:28 hirata Exp $

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "numbers.h"
#include "dvi.h"
#include "pdfparse.h"
#include "pdfspecial.h"
#include "pdfobj.h"
#include "pdfdoc.h"
#include "pdfdev.h"

#ifndef WITHOUT_TOUNICODE
#if 0
/* ??? */
#include <stdarg.h>
#endif
#include "cmap.h"

static int xtoi (char ch);

static int tounicode_cmap = -1;

void set_tounicode_cmap (char *cmap_name)
{
  if (cmap_name) {
    tounicode_cmap = CMap_cache_find(cmap_name);
  } else
    tounicode_cmap = -1;
}
#endif

static char *save;
#define DUMP_LIMIT 50
void dump (char *start, char *end)
{
  char *p = start;
  MESG("\nCurrent input buffer is -->");
  while (p < end && p < start+DUMP_LIMIT)
    MESG("%c", *(p++));
  if (p == start+DUMP_LIMIT)
    MESG("...");
  MESG("<--\n");
}

void skip_line (char **start, char *end)
{
  /* Note: PDF spec says that all platforms must end line with '\n'
   * after a "stream" keyword.
   */
  while (*start < end && **start != '\n' && **start != '\r')
    (*start)++;
  /* The carriage return (CR; \r; 0x0D) and line feed (LF; \n; 0x0A)
   * characters, also called newline characters, are treated as
   * end-of-line (EOL) markers. The combination of a carriage return
   * followed immediately by a line feed is treated as one EOL marker.
   */
  if (*start < end && **start == '\r')
    (*start)++;
  if (*start < end && **start == '\n')
    (*start)++;
}

void skip_white (char **start, char *end)
{
  /* The null (NUL; 0x00) character is a white-space character in PDF spec
   * but isspace(0x00) returns FALSE; on the other hand, the vertical tab
   * (VT; 0x0B) character is not a white-space character in PDF spec but
   * isspace(0x0B) returns TRUE. */
  while (*start < end && (isspace(**start) || **start == '%')) {
    if (**start == '%') 
      skip_line (start, end);
    else /* Skip the white char */
      (*start)++;
  }
}

int is_an_int (const char *s)
{
  if (!s || !*s) return 0;
  if (*s == '+' || *s == '-') s++;
  while (*s)
    if (!isdigit(*s++)) return 0;
  return 1;
}

int is_a_number (const char *s)
{
  if (!s || !*s) return 0;
  if (*s == '+' || *s == '-') s++;
  while (*s)
    if (!isdigit(*s++)) break;
  if (!*s) return 1;
  if (*(s-1) != '.') return 0;
  while (*s)
    if (!isdigit(*s++)) return 0;
  return 1;
}

static char *parsed_string (char *end)
{
  register char *result = NULL;
  register int len = end - save;
  if (len > 0) {
    result = NEW (len+1, char);
    memcpy(result, save, len);
    result[len] = 0;
  }
  return result;
}

char *parse_number (char **start, char *end)
{
  skip_white(start, end); save = *start;

  if (*start < end && (**start == '+' || **start == '-'))
    (*start)++;
  while (*start < end && isdigit(**start))
    (*start)++;
  if (*start < end && **start == '.') {
    (*start)++;
    while (*start < end && isdigit(**start))
      (*start)++;
  }
  return parsed_string(*start);
}

char *parse_unsigned (char **start, char *end)
{
  skip_white(start, end); save = *start;

  while (*start < end && isdigit(**start))
    (*start)++;
  return parsed_string(*start);
}

static char *parse_gen_ident (char **start, char *end, const char *valid_chars)
{
  save = *start;
  while (*start < end && strchr(valid_chars, **start))
    (*start)++;
  return parsed_string(*start);
}

char *parse_ident (char **start, char *end)
{
  static const char *valid_chars = "!\"#$&'*+,-.0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz|~";
  return parse_gen_ident(start, end, valid_chars);
}

char *parse_val_ident (char **start, char *end)
{
  static const char *valid_chars = "!\"#$&'*+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz|~";
  return parse_gen_ident(start, end, valid_chars);
}

char *parse_c_ident (char **start, char *end)
{
  static const char *valid_chars = "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
  return parse_gen_ident(start, end, valid_chars);
}

char *parse_opt_ident (char **start, char *end)
{
  if (*start < end && **start == '@') {
    (*start)++;
    return parse_ident(start, end);
  }
  return NULL;
}

/*
 * PDF Object
 */
static pdf_obj *parse_pdf_number (char **start, char *end)
{
  register pdf_obj *result;
  register char *number;

  skip_white(start, end); save = *start;

  if ((number = parse_number(start, end))) {
    result = pdf_new_number(atof(number));
    RELEASE (number);
    return result;
  }
  WARN("Could not find a numeric object.");
  dump(*start = save, end);
  return NULL;
}

/*
 * PDF Name
 *
 *  PDF-1.2+: Two hexadecimal digits preceded by a number sign.
 */
static unsigned char *decode_pdf_name (unsigned char *name)
{
  unsigned char *p, *cur;
  
  cur = p = name;
  while (*p != '\0') {
    unsigned char c = *p++;
    if (c == '#') {
      if (*p == '\0' || *(p+1) == '\0') {
        WARN("Premature end of input name string.");
	break;
      }
      c = (unsigned char) (xtoi(*p++) << 4);
      c += xtoi(*p++);
    }
    if (c != 0) /* Ignore null */
      *(cur++) = c;
  }
  *cur = '\0';

  return name;
}

pdf_obj *parse_pdf_name (char **start, char *end)
{
  pdf_obj *result;
  unsigned char *name;

  skip_white(start, end); save = *start;

  if (*start < end && **start == '/') {
    (*start)++;
    if ((name = (unsigned char *) parse_ident(start, end)) &&
	(name = decode_pdf_name(name))) {
      /*
       * PDF name does not contain null character.
       */
      result = pdf_new_name(name);
      RELEASE (name);
      return result;
    }
  }
  WARN("Could not find a name object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_boolean (char **start, char *end)
{
  register int len;

  skip_white (start, end); save = *start;

  if ((len = end - *start) >= 4 && !strncmp(*start, "true", 4)) {
    *start += 4;
    return pdf_new_boolean(1);
  }
  if (len >= 5 && !strncmp(*start, "false", 5)) {
    *start += 5;
    return pdf_new_boolean(0);
  }
  WARN("Could not find a boolean object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_null (char **start, char *end)
{
  register int len;

  skip_white (start, end); save = *start;

  if ((len = end - *start) >= 4 && !strncmp(*start, "null", 4)) {
    *start += 4;
    return pdf_new_null();
  }
  WARN("Could not find a null object.");
  dump(*start = save, end);
  return NULL;
}

/*
 * PDF Literal String
 */
#ifndef isodigit
#define isodigit(c) ((c) >= '0' && (c) <= '7')
#endif
static int parse_escape_char (char **start, char *end,
                              const char *escape_str, unsigned char *ch)
{
  int skip = 0;

  /* Caller should check this. */
  if (**start != '\\') return 0;

  switch (*(++(*start))) {
  case 'n': *ch = '\n'; (*start)++; break;
  case 'r': *ch = '\r'; (*start)++; break;
  case 't': *ch = '\t'; (*start)++; break;
  case 'b': *ch = '\b'; (*start)++; break;
  case 'f': *ch = '\f'; (*start)++; break;
    /*
     * An end-of-line marker preceded by a backslash must be ignored.
     */
  case '\n':
    skip = 1; (*start)++;
    break;
  case '\r':
    skip = 1; (*start)++;
    if (*start < end && **start == '\n') (*start)++;
    break;
  default:
    if (strchr(escape_str, **start)) {
      *ch = **start; (*start)++;
    } else if (isodigit(**start)) {
      int i = 3, val = 0;
      while (i-- > 0 && *start < end && isodigit(**start))
        val = (val << 3) + (*((*start)++) - '0');
#if 0
      /* Not sure how to handle this. */
      if (val > 255) skip = 1;
#endif
      *ch = (unsigned char) (val & 0xff);
    } else /* Ignore only backslash. */
      skip = 1;
  }

  return skip ? 0 : 1;
}

static pdf_obj *parse_pdf_literal_string (char **start, char *end)
{
  pdf_obj *result;
  unsigned char *string;
  int balance = 0, len = 0;

  skip_white(start, end); save = *start;
  ASSERT(*start < end-1 && **start == '(');

  string = NEW(end - *start, unsigned char);
  /*
   * Accroding to the PDF spec., an end-of-line marker, not preceded
   * by a backslash, must be converted to single \n.
   */
  (*start)++;
  while (*start < end && ((**start != ')' || balance > 0))) {
    if (**start == '\\') {
      if (parse_escape_char(start, end, "\\()", &(string[len])))
	len++;
    } else if (**start == '\r') {
      (*start)++;
      if (*start < end && **start == '\n')
	(*start)++;
      string[len++] = '\n';
    } else {
      if (**start == '(') balance++;
      if (**start == ')') balance--;
      string[len++] = *((*start)++);
    }
  }
  if (balance > 0 || **start != ')' || *start >= end) {
    WARN("Unbalanced parens/truncated PDF literal string.");
    RELEASE(string);
    dump(*start = save, end);
    return NULL;
  }

  result = pdf_new_string(string, len);
  RELEASE(string);

  (*start)++;
  return result;
}

/*
 * PDF Hex String
 */
static int xtoi (char ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'A' && ch <= 'F')
    return (ch - 'A') + 10;
  if (ch >= 'a' && ch <= 'f')
    return (ch - 'a') + 10;
  return -1;
}

static pdf_obj *parse_pdf_hex_string (char **start, char *end)
{
  pdf_obj *result;
  long     len = 0;
  unsigned char *string;

  skip_white(start, end); save = *start;
  ASSERT(*start < end-1 && **start == '<');

  string = NEW((end-(*start))/2 + 1, unsigned char);
  /*
   * PDF Reference does not describe how to treat invalid char.
   * Zero is appended if final hex digit is missing.
   */
  (*start)++;
  while (*start < end && **start != '>') {
    int hi, lo;
    skip_white(start, end);
    if (*start >= end || **start == '>')
      break;
    hi = xtoi(*((*start)++));
    skip_white(start, end);
    if (*start >= end || **start == '>') {
      lo = 0;
    } else
      lo = xtoi(*((*start)++));
    string[len++] = (unsigned char) ((hi << 4)|lo);
  }
  if (**start != '>') {
    WARN("Premature end of input hex string.");
    RELEASE(string);
    dump(*start = save, end);
    return NULL;
  }

  result = pdf_new_string(string, len);
  RELEASE(string);

  (*start)++;
  return result;
}

pdf_obj *parse_pdf_string (char **start, char *end)
{
  pdf_obj *result;

  skip_white(start, end); save = *start;
  if (*start >= end-1) {
    WARN("Could not find a literal string object.");
    dump(*start = save, end);
    return NULL;
  }

  if (**start == '(')
    result = parse_pdf_literal_string(start, end);
  else if (**start == '<' &&
	   (*(*start+1) == '>' || isxdigit(*(*start+1))))
    result = parse_pdf_hex_string(start, end);
  else {
    WARN("Could not find a PDF string object.");
    dump(*start = save, end);
    return NULL;
  }

  return result;
}

#ifndef WITHOUT_TOUNICODE
static unsigned char *parse_hexadecimal (char **start, char *end, int *len)
{
  unsigned char *string;

  save = (*start)++; skip_white(start, end); *len = 0;
  string = NEW ((end - *start) / 2 + 1, unsigned char);
  while (*start < end && **start != '>') {
    string[*len] = (unsigned char) (xtoi(*((*start)++)) << 4);
    if (*start < end && **start != '>')
      string[*len] += (unsigned char) xtoi(*((*start)++));
    (*len)++;
    skip_white(start, end);
  }
  if (*start < end) {
    (*start)++;
    return string;
  }
  WARN("Hexadecimal string object ended prematurely.");
  dump(*start = save, end);
  return NULL;
}

#define WBUF_SIZE 4096
static pdf_obj *parse_pdf_string_tounicode (char **start, char *end)
{
  CMap *cmap;
  char  unbalance = 1;
  unsigned char ibuf[WBUF_SIZE], obuf[WBUF_SIZE], *obufcur;
  long  len = 0, obufleft = WBUF_SIZE;

  skip_white(start, end);
  save = *start;
  if (*start >= end-1 || **start != '(') {
    WARN("Could not find any PDF literal string.");
    dump(*start = save, end);
    return NULL;
  } else if (*(*start+1) == ')') {
    *start += 2;
    return pdf_new_string(NULL, 0);
  }

  *start += 1;
  /*
   * Call parse_pdf_string() if the string is already Unicode.
   */
  if (*start <= end - 8 && !strncmp(*start, "\\376\\377", 8))
    return parse_pdf_string (start, end);

  if (tounicode_cmap < 0) {
    WARN("ToUnicode CMap not found.");
    return NULL;
  }
  cmap = CMap_cache_get(tounicode_cmap);
  /*
   * BOM
   */
  obuf[0] = 0xfe; obuf[1] = 0xff; obufcur = obuf + 2; obufleft -= 2;
  while (*start < end && unbalance) {
#ifndef PDF_PARSE_STRICT
    unsigned char c = (unsigned char) **start;
    if (strstr(CMap_get_name(cmap), "RKSJ") &&
	((c >= 0x81 && c <= 0x9F) ||
	 (c >= 0xE0 && c <= 0xFC)) &&
	*start < end - 1) {
      /*
       * Quick workaround for Shift_JIS.
       * Note that it does not always work correctly.
       */
      ibuf[len++] = *(*start)++;
      ibuf[len++] = *(*start)++;
      continue;
    }
#endif /* !PDF_PARSE_STRICT */
    switch (**start) {
    case '\\':
      /*
       * Treatment of newline is wrong in this code.
       */
#ifndef PDF_PARSE_STRICT
      /*
       *  Those extensions vaiolates convention that input string is a PDF string.
       */
      if (*start+6 < end && strncmp(*start+1, "0x", 2) == 0) {
	/*
	 * Problematic.
	 * It is not recommended to mix different encodings in a single string.
	 * \0xXXXX should be, at least, changed to \uXXXX to indicate Unicode.
	 */
	char buf[5], *p;
	unsigned short ucv;
	if (len > 0) {
	  unsigned char *cur = ibuf;
	  CMap_decode(cmap, (const unsigned char **) &cur, &len, &obufcur, &obufleft);
	  if (len != 0)
	    ERROR("Invalid input string.");
	}
	memcpy(buf, *start+3, 4); buf[4] = 0;
	ucv = (unsigned short) strtol(buf, &p, 16);
	if (*p == 0) {
	  *obufcur++ = ucv >> 8;
	  *obufcur++ = ucv & 0xff;
	  obufleft  -= 2;
	  *start    += 7;
	} else
	  ERROR("Invalid input text string.");
      } else
#endif /* !PDF_PARSE_STRICT */
	if (parse_escape_char(start, end, "\\()", &(ibuf[len])))
	  len++;
      break;
    default:
      if (**start == '(')
	unbalance++;
      else if (**start == ')')
	unbalance--;
      if (unbalance)
	ibuf[len++] = *(*start)++;
      break;
    }
  }
  if (unbalance)
    ERROR("Truncated input text string?");
    
  if (len > 0) {
    unsigned char *cur = ibuf;
    CMap_decode(cmap, (const unsigned char **)&cur, &len, &obufcur, &obufleft);
    if (len != 0)
      ERROR("Invalid input string.");
  }

  (*start)++;
  return pdf_new_string(obuf, WBUF_SIZE-obufleft);
}

static pdf_obj *parse_pdf_hex_string_tounicode (char **start, char *end)
{
  pdf_obj *result;
  CMap *cmap;
  long len, obufleft;
  unsigned char *xstr;
  unsigned char *ibufcur, *obufcur;
  unsigned char wbuf[WBUF_SIZE];

  skip_white(start, end);
  save = *start;
  if (*start >= end || **start != '<' ||
      !(xstr = parse_hexadecimal(start, end, (int *)&len))) {
    WARN("Could not find any PDF hexadecimal string.");
    dump(*start = save, end);
    return NULL;
  }

  /*
   * Do nothing if the string is already Unicode.
   */
  if (len > 1 && *xstr == 0xfe && *(xstr+1) == 0xff)
    result = pdf_new_string(xstr, len);
  else {
    cmap = CMap_cache_get(tounicode_cmap);
    if (!cmap) {
      WARN("ToUnicode CMap not found.");
      result = NULL;
    } else {
      ibufcur  = xstr;
      wbuf[0]  = 0xfe; wbuf[1] = 0xff;
      obufcur  = wbuf + 2;
      obufleft = WBUF_SIZE - 2;
      CMap_decode(cmap, (const unsigned char **)&ibufcur, &len, &obufcur, &obufleft);
      if (len > 0)
	ERROR("Invalid input text string.");
      result = pdf_new_string(wbuf, WBUF_SIZE-obufleft);
    }
  }
  RELEASE(xstr);

  return result;
}

pdf_obj *parse_pdf_text_string (char **start, char *end)
{
  skip_white(start, end); save = *start;

  if (*start < end)
    switch (**start) {
    case '<': 
      return (tounicode_cmap < 0 ? parse_pdf_hex_string(start, end) :
                         parse_pdf_hex_string_tounicode(start, end));
    case '(':
      return (tounicode_cmap < 0 ? parse_pdf_string(start, end) :
                         parse_pdf_string_tounicode(start, end));
    }
  WARN("Could not find a text string object.");
  dump(*start = save, end);
  return NULL;
}
#endif /* !WITHOUT_TOUNICODE */

pdf_obj *parse_pdf_ann_dict (char **start, char *end)
{
  register pdf_obj *result, *tmp1, *tmp2;
  register char *name;

  skip_white(start, end); save = *start;

  if (*((*start)++) == '<' && *((*start)++) == '<') {
    result = pdf_new_dict();
    skip_white(start, end);
    while (*start < end && **start != '>') {
      if ((tmp1 = parse_pdf_name(start, end)) == NULL) {
        pdf_release_obj(result); 
        WARN("[parse] pdf_ann_dict: Could not find a name object.");
        dump(*start = save, end);
        return NULL;
      }
      /*
       * Check the type of the parsed name is the text string object.
       * See PDF Reference 1.4, p.490-492, Table 8.10.
       */
      name = pdf_string_value(tmp1);
#ifndef WITHOUT_TOUNICODE
      if (strcmp(name, "Contents") == 0)
      /* TODO: some problem occurred with ConTeXt in the case of "T" */
        tmp2 = parse_pdf_text_string(start, end);
      /* TODO
      else if (!strcmp(name, "A"))
        tmp2 = parse_pdf_action_dict(start, end);
      */
      else
#endif
        tmp2 = parse_pdf_object(start, end);
      if (tmp2 == NULL) {
        pdf_release_obj(result);
        pdf_release_obj(tmp1); 
        WARN("[parse] pdf_ann_dict: Could not find a valid object for /%s.", name);
        dump(*start = save, end);
        return NULL;
      }
      pdf_add_dict(result, tmp1, tmp2);
      skip_white(start, end);
    }
    if (*((*start)++) == '>' && *((*start)++) == '>')
      return result;
    else {
      pdf_release_obj(result);
      WARN("[parse] pdf_ann_dict: Dictionary object ended prematurely.");
      dump(*start = save, end);
      return NULL;
    }
  }
  WARN("[parse] pdf_ann_dict: Could not find a dictionary object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_out_dict (char **start, char *end)
{
  register pdf_obj *result, *tmp1, *tmp2;
  register char *name;

  skip_white(start, end); save = *start;

  if (*((*start)++) == '<' && *((*start)++) == '<') {
    result = pdf_new_dict();
    skip_white(start, end);
    while (*start < end && **start != '>') {
      if ((tmp1 = parse_pdf_name(start, end)) == NULL) {
        pdf_release_obj(result); 
        WARN("[parse] pdf_out_dict: Could not find a name object.");
        dump(*start = save, end);
        return NULL;
      }
      /*
       * Check the type of the parsed name is the text string object.
       * See PDF Reference 1.4, p.478, Table 8.4.
       */
      name = pdf_string_value(tmp1);
#ifndef WITHOUT_TOUNICODE
      if (strcmp(name, "Title") == 0)
        tmp2 = parse_pdf_text_string(start, end);
      /* TODO
      else if (!strcmp(name, "A"))
        tmp2 = parse_pdf_action_dict(start, end);
      */
      else
#endif
        tmp2 = parse_pdf_object(start, end);
      if (tmp2 == NULL) {
        pdf_release_obj(result);
        pdf_release_obj(tmp1); 
        WARN("[parse] pdf_out_dict: Could not find a valid object for /%s.", name);
        dump(*start = save, end);
        return NULL;
      }
      pdf_add_dict(result, tmp1, tmp2);
      skip_white(start, end);
    }
    if (*((*start)++) == '>' && *((*start)++) == '>')
      return result;
    else {
      pdf_release_obj(result);
      WARN("[parse] pdf_out_dict: Dictionary object ended prematurely.");
      dump(*start = save, end);
      return NULL;
    }
  }
  WARN("[parse] pdf_out_dict: Could not find a dictionary object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_info_dict (char **start, char *end)
{
  register pdf_obj *result, *tmp1, *tmp2;
  register char *name;

  skip_white(start, end); save = *start;

  if (*((*start)++) == '<' && *((*start)++) == '<') {
    result = pdf_new_dict();
    skip_white(start, end);
    while (*start < end && **start != '>') {
      if ((tmp1 = parse_pdf_name(start, end)) == NULL) {
        pdf_release_obj(result); 
        WARN("[parse] pdf_info_dict: Could not find a name object.");
        dump(*start = save, end);
        return NULL;
      }
      /*
       * Check the type of the parsed name is the text string object.
       * See PDF Reference 1.4, p.576, Table 9.2.
       */
      name = pdf_string_value(tmp1);
#ifndef WITHOUT_TOUNICODE
      if (strcmp(name, "Title") == 0 || strcmp(name, "Author") == 0 ||
          strcmp(name, "Subject") == 0 || strcmp(name, "Keywords") == 0 ||
          strcmp(name, "Creator") == 0 || strcmp(name, "Producer") == 0)
        tmp2 = parse_pdf_text_string(start, end);
      /* TODO
      else if (!strcmp(name, "A"))
        tmp2 = parse_pdf_action_dict(start, end);
      */
      else
#endif
        tmp2 = parse_pdf_object(start, end);
      if (tmp2 == NULL) {
        pdf_release_obj(result);
        pdf_release_obj(tmp1); 
        WARN("[parse] pdf_info_dict: Could not find a valid object for /%s.", name);
        dump(*start = save, end);
        return NULL;
      }
      pdf_add_dict(result, tmp1, tmp2);
      skip_white(start, end);
    }
    if (*((*start)++) == '>' && *((*start)++) == '>')
      return result;
    else {
      pdf_release_obj(result);
      WARN("[parse] pdf_info_dict: Dictionary object ended prematurely.");
      dump(*start = save, end);
      return NULL;
    }
  }
  WARN("[parse] pdf_info_dict: Could not find a dictionary object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_dict (char **start, char *end)
{
  register pdf_obj *result, *tmp1, *tmp2;

  skip_white(start, end); save = *start;

  if (*((*start)++) == '<' && *((*start)++) == '<') {
    result = pdf_new_dict();
    skip_white(start, end);
    while (*start < end && **start != '>') {
      if ((tmp1 = parse_pdf_name(start, end)) == NULL) {
        pdf_release_obj(result); 
        WARN("Could not find a name object in this dictionary object.");
        dump(*start = save, end);
        return NULL;
      }
      if ((tmp2 = parse_pdf_object(start, end)) == NULL) {
        pdf_release_obj(result);
        pdf_release_obj(tmp1); 
        WARN("Could not find a valid object in this dictionary object.");
        dump(*start = save, end);
        return NULL;
      }
      pdf_add_dict(result, tmp1, tmp2);
      skip_white(start, end);
    }
    if (*((*start)++) == '>' && *((*start)++) == '>')
      return result;
    else {
      pdf_release_obj(result);
      WARN("Dictionary object ended prematurely.");
      dump(*start = save, end);
      return NULL;
    }
  }
  WARN("Could not find a dictionary object.");
  dump(*start = save, end);
  return NULL;
}

pdf_obj *parse_pdf_array (char **start, char *end)
{
  register pdf_obj *result, *tmp1;

  skip_white(start, end); save = *start;

  if (*((*start)++) == '[') {
    result = pdf_new_array();
    skip_white(start, end);
    while (*start < end && **start != ']') {
      if ((tmp1 = parse_pdf_object(start, end)) == NULL) {
        pdf_release_obj(result); 
        WARN("Could not find a valid object in this array object.");
        dump(*start = save, end);
        return NULL;
      }
      pdf_add_array(result, tmp1);
      skip_white(start, end);
    }
    if (*start < end) {
      (*start)++;
      return result;
    } else {
      pdf_release_obj(result);
      WARN("Array object ended prematurely.");
      dump(*start = save, end);
      return NULL;
    }
  }
  WARN("Could not find an array object.");
  dump(*start = save, end);
  return NULL;
}

static pdf_obj *parse_pdf_stream (char **start, char *end, pdf_obj *dict)
{
  pdf_obj *result, *sdict;
  pdf_obj *tmp;
  long     len;

#if 0
  /*
   * I don't know why this was commented out.
   */
  if (pdf_lookup_dict(dict, "F")) {
    WARN("File streams not implemented (yet)");
    return NULL;
  }
#endif

  skip_white(start, end);
  save = *start;
  if (end - *start < 6 || strncmp(*start, "stream", 6)) {
    WARN("Could not find a stream object.");
    dump(*start = save, end);
    return NULL;
  }

  *start += 6;
  if (*start < end && **start == '\r') (*start)++;
  if (*start < end && **start == '\n') (*start)++;

  if ((tmp = pdf_lookup_dict(dict, "Length")) != NULL) {
    pdf_obj *tmp2 = pdf_deref_obj(tmp);
    len = (long) pdf_number_value(tmp2);
    pdf_release_obj(tmp2);
#ifndef PDF_PARSE_STRICT
  } else if ((*start)+9 < end) {
    /*
     * This was added to allow TeX users to write PDF stream object directly
     * in their TeX source. This violates PDF spec.
     */
    char *cur;
    for (cur = end-9; cur >= *start && strncmp(cur, "endstream", 9); cur--);
    if (cur == *start) {
      WARN("Cound not find \"endstream\".");
      dump(*start = save, end);
      return NULL;
    }
    len = cur - 1 - (*start);
#endif /* !PDF_PARSE_STRICT */
  } else {
    WARN("Not PDF stream object?");
    dump(*start = save, end);
    return NULL;
  }

  /* skip_line(start, end);  */
#if 0
  /*
   * Epdf doesn't work with this... Bad implementation of dvipdfm.
   * pdf_new_stream(STREAM_COMPRESS) add Filter when the stream object is created.
   */
  /*
   * If Filter is not applied, set STREAM_COMPRESS flag.
   * Should we use filter for ASCIIHexEncode/ASCII85Encode-ed streams?
   */
  if ((tmp = pdf_lookup_dict(dict, "Filter")) == NULL) {
    result = pdf_new_stream(STREAM_COMPRESS);
  } else
#endif
    result = pdf_new_stream(0);
  sdict = pdf_stream_dict(result);
  pdf_merge_dict(sdict, dict);
  pdf_release_obj(dict);
  pdf_add_stream(result, *start, len);
  *start += len;

  skip_white(start, end);
  if (end - *start < 9 || strncmp(*start, "endstream", 9)) {
    WARN("Stream object ended prematurely.");
    dump(*start = save, end);
    return NULL;
  }

  *start += 9;
  return result;
}

#ifndef WITHOUT_TOUNICODE
static pdf_obj *parse_pdf_reference (char **start, char *end)
{
  register pdf_obj *result;
  register char *name;

  skip_white(start, end); save = *start;

  if ((name = parse_opt_ident(start, end))) {
    if ((result = lookup_reference(name)) == NULL) {
      WARN("Could not find the named reference (@%s).", name);
      dump(*start = save, end);
    }
    RELEASE (name);
    return result;
  }
  WARN("Could not find a reference name.");
  dump(*start = save, end);
  return NULL;
}
#endif

pdf_obj *parse_pdf_object (char **start, char *end)
{
  register pdf_obj *result, *num_obj1, *num_obj2;
  register char *tmp_pos;

  skip_white(start, end); save = *start;

  if (*start < end)
    switch (**start) {
    case '<': 
      if (*(*start+1) != '<')
        /* Hexadecimal string object */
        return parse_pdf_hex_string(start, end);
      result = parse_pdf_dict(start, end);
      skip_white(start, end);
      if (end - *start >= 6 && !strncmp(*start, "stream", 6))
        /* Stream object */
        return parse_pdf_stream(start, end, result);
      else
        /* Dictionary object */
        return result;
    case '(':
      /* Literal string object */
      return parse_pdf_string(start, end);
    case '[':
      /* Array object */
      return parse_pdf_array(start, end);
    case '/':
      /* Name object */
      return parse_pdf_name(start, end);
#ifndef WITHOUT_TOUNICODE
    case '@':
      /* Reference for internal use only */
      return parse_pdf_reference(start, end);
#endif
    case 'n':
      /* Null object */
      return parse_pdf_null(start, end);
    case 't':
    case 'f':
      /* Boolean object */
      return parse_pdf_boolean(start, end);
    case '+':
    case '-':
    case '.':
      /* Numeric object */
      return parse_pdf_number(start, end);
    default:
      if (isdigit(**start)) {
        num_obj1 = parse_pdf_number(start, end);
        skip_white(start, end);
        tmp_pos = *start;
        if (*start < end && isdigit(**start))
	  num_obj2 = parse_pdf_number(start, end);
	else
          /* Numeric object */
          return num_obj1;
        skip_white(start, end);
        if (*start < end && *((*start)++) == 'R') {
	  result = pdf_new_ref((unsigned long)pdf_number_value(num_obj1), 
			       (int)pdf_number_value(num_obj2));
	  pdf_release_obj(num_obj1);
	  pdf_release_obj(num_obj2);
          /* Indirect object */
	  return result;
        }
	pdf_release_obj(num_obj2);
	*start = tmp_pos;
        /* Numeric object */
        return num_obj1;
      }
    }
  WARN("Could not find any valid object.");
  dump(*start = save, end);
  return NULL;
}

char *parse_c_string (char **start, char *end)
{
  register unsigned char *result;
  register int len = 0;

  skip_white(start, end); save = *start;

  if (*start < end && **start == '"') {
    (*start)++;
    result = NEW (end - *start + 1, unsigned char);
    while (*start < end && **start != '"')
      if (**start == '\\' &&
          parse_escape_char(start, end, "\\\"", &(result[len]))) len++;
      else
        result[len++] = *((*start)++);
    if (*start < end) {
      (*start)++;
      result[len] = 0;
      return (char *)result;
    }
  }
  WARN("Could not find a C string.");
  dump(*start = save, end);
  return NULL;
}

void parse_key_val (char **start, char *end, char **key, char **val) 
{
  skip_white(start, end);
  if ((*key = parse_c_ident(start, end))) {
    skip_white(start, end);
    if (*start < end && **start == '=') {
      (*start)++;
      skip_white(start, end);
      if (*start < end) {
        if (**start == '"')
	  *val = parse_c_string(start, end);
        else
	  *val = parse_val_ident(start, end);
        return;
      }
    }
  }
  *val = NULL;
}
