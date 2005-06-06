/* otp2ocp.c: Main program for transforming OTP files into OCP files.

This file is part of Omega,
which is based on the web2c distribution of TeX,

Copyright (c) 1994--2001 John Plaice and Yannis Haralambous
Copyright (c) 2002 Behdad Esfahbod
Copyright (c) 2002 Roozbeh Pournader

Omega is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Omega is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Omega; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

#include <kpathsea/config.h>
#include <kpathsea/types.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/c-std.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/tex-file.h>
#include "routines.h"

extern FILE *yyin;
int yywrap
P1H (void)
{
  return 1;
}

void output
P2C (FILE *, output_file, int, i)
{
/* make sure output is in big-endian form */
  char j;
  int k;
  k = i;
  j = k >> 24;
  fwrite (&j, 1, 1, output_file);
  k = k & 0xffffff;
  j = k >> 16;
  fwrite (&j, 1, 1, output_file);
  k = k & 0xffff;
  j = k >> 8;
  fwrite (&j, 1, 1, output_file);
  j = k & 0xff;
  fwrite (&j, 1, 1, output_file);
}

void otp_read
P2C (string, input_name, string, output_name)
{
  int i, j, len, no_words;
  int *table, *instrs;
  FILE *input_file, *output_file;

  input_file = xfopen (input_name, FOPEN_R_MODE);
  store_state ("INITIAL");
  yyin = input_file;
  if (yyparse ())
    exit (EXIT_FAILURE);
  output_file = xfopen (output_name, FOPEN_WBIN_MODE);
  room_for_tables = 0;
  for (i = 0; i < no_tables; i++)
    {
      room_for_tables = room_for_tables + tables[i].length;
    }
  room_for_states = 0;
  for (i = 0; i < no_states; i++)
    {
      room_for_states = room_for_states + states[i].length;
    }
  no_words = no_tables + room_for_tables + no_states + room_for_states + 7;
  output (output_file, no_words);
  output (output_file, input_bytes);
  output (output_file, output_bytes);
  output (output_file, no_tables);
  output (output_file, room_for_tables);
  output (output_file, no_states);
  output (output_file, room_for_states);
  for (i = 0; i < no_tables; i++)
    {
      len = tables[i].length;
      output (output_file, len);
    }
  for (i = 0; i < no_tables; i++)
    {
      len = tables[i].length;
      table = tables[i].table;
      for (j = 0; j < len; j++)
	{
	  output (output_file, table[j]);
	}
    }
  for (i = 0; i < no_states; i++)
    {
      len = states[i].length;
      output (output_file, len);
    }
  for (i = 0; i < no_states; i++)
    {
      len = states[i].length;
      instrs = states[i].instrs;
      for (j = 0; j < len; j++)
	{
	  output (output_file, instrs[j]);
	}
    }
}

int main
P2C (int, argc, string *, argv)
{
  string input_name, full_input_name;
  string output_name;

  kpse_set_progname (argv[0]);
  switch (argc)
    {
    case 1:
      fprintf (stderr, "otp2ocp: No file given\n");
      return EXIT_FAILURE;
    case 2:
      {
	int len;
	input_name = argv[1];
	len = strlen (input_name);
	output_name = malloc (len + 5);
	if (!output_name)
	  {
	    fprintf (stderr, "otp2ocp: Memory exhausted\n");
	    return EXIT_FAILURE;
	  }
	strcpy (output_name, xbasename (input_name));
	len = strlen (output_name);
	if (len > 4 && 0 == strcmp (output_name + len - 4, ".otp"))
	  {
	    strcpy (output_name + len - 4, ".ocp");
	  }
	else
	  {
	    strcat (output_name, ".ocp");
	  }
	break;
      }
    case 3:
      input_name = argv[1];
      output_name = argv[2];
      break;
    default:
      fprintf (stderr, "otp2ocp: Too many arguments\n");
      return EXIT_FAILURE;
    }
  full_input_name = kpse_find_file (input_name, kpse_otp_format, true);
  if (!full_input_name)
    {
      fprintf (stderr, "otp2ocp: %s not found\n", input_name);
      return EXIT_FAILURE;
    }
  otp_read (full_input_name, output_name);

  return EXIT_SUCCESS;
}
