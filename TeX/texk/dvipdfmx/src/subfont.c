/*  $Header: /home/cvsroot/dvipdfmx/src/subfont.c,v 1.12 2004/01/30 18:34:23 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
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

#include "system.h"
#include "mem.h"
#include "error.h"
#include "mfileio.h"
#include "pdfparse.h"
#include "pdflimits.h"

struct a_sfd_record {
  char *sfd_name, *sub_name;
  unsigned int vector[256];
} *sfd_record = NULL;

static int num_sfd_records = 0, max_sfd_records = 0;
static char *start, *end, *p;
static FILE *sfd_file;

static void read_a_sfd_record(unsigned int *vector)
{
  int i;
  long lbegin = 0, lend = 0, offset = 0;

  for (i = 0; i < 256; i++) vector[i] = 0;

  offset = 0;
  skip_white(&start, end);
  do {
    lbegin = strtol(start, &p, 0);
    if (start == p || lbegin < 0 || lbegin > 0xFFFF)
      ERROR ("Invalid subfont range (begin).");
    if (*p == ':') {
      if ((offset = lbegin) > 0xFF) ERROR ("Invalid subfont offset.");
      start = ++p;
      skip_white(&start, end);
      continue;
    } else if (*p == '_') {
      start = ++p;
      if (!isdigit(*start)) ERROR ("Invalid subfont range entry.");
      lend = strtol(start, &p, 0);
      if (start == p || lend < 0 || lend > 0xFFFF)
        ERROR ("Invalid subfont range (end).");
      if (*p && !isspace(*p))
        ERROR ("Invalid subfont range entry.");
      if (lend < lbegin)
        ERROR ("End of subfont range too small.");
      if (offset + (lend - lbegin) > 255)
        ERROR ("Subfont range too large for current offset.");
    } else if (isspace(*p) || !*p)
      lend = lbegin;

    for (; lbegin <= lend; lbegin++) {
      if (vector[offset] != 0)
        ERROR ("Overlapping subfont ranges.");
      vector[offset++] = lbegin;
    }

    start = p;
    skip_white(&start, end);

    if (*start == '\\') {
      if ((start = mfgets(work_buffer, WORK_BUFFER_SIZE, sfd_file)) == NULL)
        break;
      end = work_buffer + strlen(work_buffer);
      skip_white(&start, end);
    }
  } while (start < end);
}

/* Make sure that sfd_name does not have the extension '.sfd' */
int load_sfd_record(char *sfd_name, char *sub_name)
{
  int result;
  char *filename, *full_filename;

  for (result = 0; result < num_sfd_records; result++) {
    if (!strcmp(sfd_record[result].sfd_name, sfd_name) &&
        !strcmp(sfd_record[result].sub_name, sub_name))
      return result;
  }

  filename = NEW (strlen(sfd_name)+5, char);
  strcpy(filename, sfd_name);
  strcat(filename, ".sfd");

  /* Change the program name temporarily to find SFD files from
   * the texmf/ttf2pk directory (contributed by Akira Kakuto). */
#ifdef MIKTEX
  if (!miktex_find_app_input_file("ttf2pk", filename, full_filename = work_buffer))
    full_filename = NULL;
#else
  kpse_reset_program_name("ttf2pk");
  full_filename = kpse_find_file(filename, kpse_program_text_format, 1);
  kpse_reset_program_name("dvipdfm");
#endif
  RELEASE (filename);

  if (full_filename == NULL || 
      (sfd_file = MFOPEN (full_filename, FOPEN_R_MODE)) == NULL) {
    fprintf(stderr, "Error: Could not open the SubFont Definition file '%s.sfd'\n", sfd_name);
    ERROR ("\n");
  }

  result = -1;  

  while ((start = mfgets(work_buffer, WORK_BUFFER_SIZE, sfd_file)) != NULL) {
    end = work_buffer + strlen(work_buffer);
    skip_white(&start, end);
    if (start >= end) continue;
    if (*start == '#') continue;

    p = start;
    while (*start && !isspace(*start)) start++; /* read sub_name */
    *(start++) = '\0';
    if (!strcmp(p, sub_name)) { /* found an entry matched with sub_name */
      if (num_sfd_records >= max_sfd_records) {
        max_sfd_records += MAX_FONTS;
        sfd_record = RENEW (sfd_record, max_sfd_records, struct a_sfd_record);
      }
      sfd_record[num_sfd_records].sfd_name = NEW (strlen(sfd_name)+1, char);
      strcpy(sfd_record[num_sfd_records].sfd_name, sfd_name);
      sfd_record[num_sfd_records].sub_name = NEW (strlen(sub_name)+1, char);
      strcpy(sfd_record[num_sfd_records].sub_name, sub_name);
      read_a_sfd_record(sfd_record[num_sfd_records].vector);
      result = num_sfd_records++;
      break;
    }
  }

  MFCLOSE (sfd_file);
  return result;
}

unsigned int lookup_sfd_record(int subfont_id, unsigned char s)
{
  if (sfd_record && subfont_id >= 0 && subfont_id < num_sfd_records)
    return (sfd_record[subfont_id].vector)[s];
  else {
    ERROR ("lookup_sfd_record: not defined.");
    return 0;
  }
}

void release_sfd_record(void)
{
  int i;

  if (sfd_record) {
    for (i = 0; i < num_sfd_records; i++) {
      RELEASE (sfd_record[i].sfd_name);
      RELEASE (sfd_record[i].sub_name);
    }
    RELEASE (sfd_record);
  }
}

char *sfd_sfd_name(int subfont_id)
{
  if (subfont_id >= 0 && subfont_id < num_sfd_records)
    return sfd_record[subfont_id].sfd_name;
  else
    return NULL;
}

char *sfd_sub_name(int subfont_id)
{
  if (subfont_id >= 0 && subfont_id < num_sfd_records)
    return sfd_record[subfont_id].sub_name;
  else
    return NULL;
}
