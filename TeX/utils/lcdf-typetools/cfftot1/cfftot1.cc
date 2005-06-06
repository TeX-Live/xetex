/* cfftot1.cc -- driver for translating CFF fonts to Type 1 fonts
 *
 * Copyright (c) 2002-2004 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/t1rw.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include "maket1font.hh"
#include <efont/cff.hh>
#include <efont/otf.hh>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_CTIME
# include <time.h>
#endif
#if defined(_MSDOS) || defined(_WIN32)
# include <fcntl.h>
# include <io.h>
#endif

using namespace Efont;

#define VERSION_OPT	301
#define HELP_OPT	302
#define QUIET_OPT	303
#define PFB_OPT		304
#define PFA_OPT		305
#define OUTPUT_OPT	306
#define NAME_OPT	307

Clp_Option options[] = {
    { "ascii", 'a', PFA_OPT, 0, 0 },
    { "binary", 'b', PFB_OPT, 0, 0 },
    { "help", 'h', HELP_OPT, 0, 0 },
    { "name", 'n', NAME_OPT, Clp_ArgString, 0 },
    { "output", 'o', OUTPUT_OPT, Clp_ArgString, 0 },
    { "pfa", 'a', PFA_OPT, 0, 0 },
    { "pfb", 'b', PFB_OPT, 0, 0 },
    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "version", 'v', VERSION_OPT, 0, 0 },
};


static const char *program_name;
static bool binary = true;


void
usage_error(ErrorHandler *errh, const char *error_message, ...)
{
    va_list val;
    va_start(val, error_message);
    if (!error_message)
	errh->message("Usage: %s [OPTIONS] [FONTFILE [OUTPUTFILE]]", program_name);
    else
	errh->verror(ErrorHandler::ERR_ERROR, String(), error_message, val);
    errh->message("Type %s --help for more information.", program_name);
    exit(1);
}

void
usage()
{
    printf("\
'Cfftot1' translates a PostScript font from the Compact Font Format (CFF) to\n\
the usual Type 1 format. The input file should be either a raw CFF font or a\n\
PostScript-flavored OpenType font. The result, which is usually written to the\n\
standard output, is written in PFB or PFA format.\n\
\n\
Usage: %s [OPTIONS] [FONTFILE [OUTPUTFILE]]\n\
\n\
Options:\n\
  -a, --pfa                    Output PFA font.\n\
  -b, --pfb                    Output PFB font. This is the default.\n\
  -n, --name=NAME              Select font NAME from CFF.\n\
  -o, --output=FILE            Write output to FILE.\n\
  -q, --quiet                  Do not generate any error messages.\n\
  -h, --help                   Print this message and exit.\n\
  -v, --version                Print version number and exit.\n\
\n\
Report bugs to <kohler@icir.org>.\n", program_name);
}


// MAIN

static void
do_file(const char *infn, const char *outfn, PermString name, ErrorHandler *errh)
{
    FILE *f;
    if (!infn || strcmp(infn, "-") == 0) {
	f = stdin;
	infn = "<stdin>";
#if defined(_MSDOS) || defined(_WIN32)
	_setmode(_fileno(f), _O_BINARY);
#endif
    } else if (!(f = fopen(infn, "rb")))
	errh->fatal("%s: %s", infn, strerror(errno));
  
    int c = getc(f);
    ungetc(c, f);

    Cff::Font *font = 0;
    
    if (c == EOF)
	errh->fatal("%s: empty file", infn);
    if (c == 1 || c == 'O') {
	StringAccum sa(150000);
	while (!feof(f)) {
	    int forward = fread(sa.reserve(32768), 1, 32768, f);
	    sa.forward(forward);
	}
	if (f != stdin)
	    fclose(f);

	LandmarkErrorHandler cerrh(errh, infn);
	String data = sa.take_string();
	if (c == 'O')
	    data = Efont::OpenType::Font(data, &cerrh).table("CFF");

	Cff *cff = new Cff(data, &cerrh);
	Cff::FontParent *fp = cff->font(name, &cerrh);
	if (errh->nerrors() == 0
	    && !(font = dynamic_cast<Cff::Font *>(fp)))
	    errh->fatal("%s: CID-keyed fonts not supported", infn);
    } else
	errh->fatal("%s: not a CFF or OpenType/CFF font", infn);

    if (errh->nerrors() > 0)
	return;
    
    Type1Font *font1 = create_type1_font(font, errh);

    if (!outfn || strcmp(outfn, "-") == 0) {
	f = stdout;
	outfn = "<stdout>";
    } else if (!(f = fopen(outfn, "wb")))
	errh->fatal("%s: %s", outfn, strerror(errno));

    if (binary) {
#if defined(_MSDOS) || defined(_WIN32)
	_setmode(_fileno(f), _O_BINARY);
#endif
	Type1PFBWriter t1w(f);
	font1->write(t1w);
    } else {
	Type1PFAWriter t1w(f);
	font1->write(t1w);
    }

    if (f != stdout)
	fclose(f);
}

int
main(int argc, char *argv[])
{
    Clp_Parser *clp =
	Clp_NewParser(argc, (const char * const *)argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
  
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr, String(program_name) + ": "));
    const char *input_file = 0;
    const char *output_file = 0;
    const char *font_name = 0;
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {

	  case PFA_OPT:
	    binary = false;
	    break;
      
	  case PFB_OPT:
	    binary = true;
	    break;

	  case NAME_OPT:
	    if (font_name)
		usage_error(errh, "font name specified twice");
	    font_name = clp->arg;
	    break;
	    
	  case QUIET_OPT:
	    if (clp->negated)
		errh = ErrorHandler::default_handler();
	    else
		errh = ErrorHandler::silent_handler();
	    break;
      
	  case VERSION_OPT:
	    printf("cfftot1 (LCDF typetools) %s\n", VERSION);
	    printf("Copyright (C) 2002-2004 Eddie Kohler\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
	    exit(0);
	    break;
      
	  case HELP_OPT:
	    usage();
	    exit(0);
	    break;

	  case OUTPUT_OPT:
	  output_file:
	    if (output_file)
		usage_error(errh, "output file specified twice");
	    output_file = clp->arg;
	    break;

	  case Clp_NotOption:
	    if (input_file && output_file)
		usage_error(errh, "too many arguments");
	    else if (input_file)
		goto output_file;
	    else
		input_file = clp->arg;
	    break;
      
	  case Clp_Done:
	    goto done;
      
	  case Clp_BadOption:
	    usage_error(errh, 0);
	    break;
      
	  default:
	    break;
      
	}
    }
  
  done:
    do_file(input_file, output_file, font_name, errh);
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
