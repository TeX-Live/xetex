/* otfinfo.cc -- driver for reporting information about OpenType fonts
 *
 * Copyright (c) 2003-2004 Eddie Kohler
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
#include <efont/psres.hh>
#include <efont/otfgsub.hh>
#include <efont/otfgpos.hh>
#include <efont/otfname.hh>
#include <efont/cff.hh>
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <algorithm>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#if defined(_MSDOS) || defined(_WIN32)
# include <fcntl.h>
# include <io.h>
#endif

using namespace Efont;

#define VERSION_OPT		301
#define HELP_OPT		302
#define QUIET_OPT		303
#define VERBOSE_OPT		304
#define SCRIPT_OPT		305

#define QUERY_SCRIPTS_OPT	320
#define QUERY_FEATURES_OPT	321
#define QUERY_OPTICAL_SIZE_OPT	322
#define QUERY_POSTSCRIPT_NAME_OPT 323

Clp_Option options[] = {
    
    { "script", 0, SCRIPT_OPT, Clp_ArgString, 0 },
    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "verbose", 'V', VERBOSE_OPT, 0, Clp_Negate },
    { "query-features", 'f', QUERY_FEATURES_OPT, 0, 0 },
    { "qf", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "query-scripts", 's', QUERY_SCRIPTS_OPT, 0, 0 },
    { "qs", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "query-optical-size", 'z', QUERY_OPTICAL_SIZE_OPT, 0, 0 },
    { "query-postscript-name", 'p', QUERY_POSTSCRIPT_NAME_OPT, 0, 0 },
    { "help", 'h', HELP_OPT, 0, 0 },
    { "version", 0, VERSION_OPT, 0, 0 },
    
};


static const char *program_name;
static String::Initializer initializer;

static Efont::OpenType::Tag script, langsys;

bool verbose = false;
bool quiet = false;


void
usage_error(ErrorHandler *errh, const char *error_message, ...)
{
    va_list val;
    va_start(val, error_message);
    if (!error_message)
	errh->message("Usage: %s [OPTION]... FONT", program_name);
    else
	errh->verror(ErrorHandler::ERR_ERROR, String(), error_message, val);
    errh->message("Type %s --help for more information.", program_name);
    exit(1);
}

void
usage()
{
    printf("\
'Otfinfo' reports information about an OpenType font to standard output.\n\
Supply a '--query' to say what information you want.\n\
\n\
Usage: %s QUERY_OPTION [OTFFILES...]\n\n",
	   program_name);
    printf("\
Query options:\n\
  -s, --query-scripts          Report font's supported scripts.\n\
  -f, --query-features         Report font's GSUB/GPOS features.\n\
  -z, --query-optical-size     Report font's optical size information.\n\
  -p, --query-postscript-name  Report font's PostScript name.\n\
\n\
Other options:\n\
      --script=SCRIPT[.LANG]   Set script used for --query-features [latn].\n\
  -V, --verbose                Print progress information to standard error.\n\
  -h, --help                   Print this message and exit.\n\
  -q, --quiet                  Do not generate any error messages.\n\
      --version                Print version number and exit.\n\
\n\
Report bugs to <kohler@icir.org>.\n");
}

String
read_file(String filename, ErrorHandler *errh, bool warning = false)
{
    FILE *f;
    if (!filename || filename == "-") {
	filename = "<stdin>";
	f = stdin;
#if defined(_MSDOS) || defined(_WIN32)
	// Set the file mode to binary
	_setmode(_fileno(f), _O_BINARY);
#endif
    } else if (!(f = fopen(filename.c_str(), "rb"))) {
	errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, strerror(errno));
	return String();
    }
    
    StringAccum sa;
    while (!feof(f)) {
	if (char *x = sa.reserve(8192)) {
	    int amt = fread(x, 1, 8192, f);
	    sa.forward(amt);
	} else {
	    errh->verror_text((warning ? errh->ERR_WARNING : errh->ERR_ERROR), filename, "Out of memory!");
	    break;
	}
    }
    if (f != stdin)
	fclose(f);
    return sa.take_string();
}

String
printable_filename(const String &s)
{
    if (!s || s == "-")
	return String::stable_string("<stdin>");
    else
	return s;
}

static void
collect_script_descriptions(const OpenType::ScriptList &script_list, Vector<String> &output, ErrorHandler *errh)
{
    Vector<OpenType::Tag> script, langsys;
    script_list.language_systems(script, langsys, errh);
    for (int i = 0; i < script.size(); i++) {
	String what = script[i].text();
	const char *s = script[i].script_description();
	String where = (s ? s : "<unknown script>");
	if (!langsys[i].null()) {
	    what += String(".") + langsys[i].text();
	    s = langsys[i].language_description();
	    where += String("/") + (s ? s : "<unknown language>");
	}
	if (what.length() < 8)
	    output.push_back(what + String("\t\t") + where);
	else
	    output.push_back(what + String("\t") + where);
    }
}

static void
do_query_scripts(const OpenType::Font &otf, ErrorHandler *errh, ErrorHandler *result_errh)
{
    Vector<String> results;
    if (String gsub_table = otf.table("GSUB")) {
	OpenType::Gsub gsub(gsub_table, errh);
	collect_script_descriptions(gsub.script_list(), results, errh);
    }
    if (String gpos_table = otf.table("GPOS")) {
	OpenType::Gpos gpos(gpos_table, errh);
	collect_script_descriptions(gpos.script_list(), results, errh);
    }

    if (results.size()) {
	std::sort(results.begin(), results.end());
	String *unique_result = std::unique(results.begin(), results.end());
	for (String *sp = results.begin(); sp < unique_result; sp++)
	    result_errh->message("%s", sp->c_str());
    }
}

static void
collect_feature_descriptions(const OpenType::ScriptList &script_list, const OpenType::FeatureList &feature_list, Vector<String> &output, ErrorHandler *errh)
{
    int required_fid;
    Vector<int> fids;
    // collect features applying to this script
    script_list.features(script, langsys, required_fid, fids, errh);
    for (int i = -1; i < fids.size(); i++) {
	int fid = (i < 0 ? required_fid : fids[i]);
	if (fid >= 0) {
	    OpenType::Tag tag = feature_list.tag(fid);
	    const char *s = tag.feature_description();
	    output.push_back(tag.text() + String("\t") + (s ? s : "<unknown feature>"));
	}
    }
}

static void
do_query_features(const OpenType::Font &otf, ErrorHandler *errh, ErrorHandler *result_errh)
{
    Vector<String> results;
    if (String gsub_table = otf.table("GSUB")) {
	OpenType::Gsub gsub(gsub_table, errh);
	collect_feature_descriptions(gsub.script_list(), gsub.feature_list(), results, errh);
    }
    if (String gpos_table = otf.table("GPOS")) {
	OpenType::Gpos gpos(gpos_table, errh);
	collect_feature_descriptions(gpos.script_list(), gpos.feature_list(), results, errh);
    }

    if (results.size()) {
	std::sort(results.begin(), results.end());
	String *unique_result = std::unique(results.begin(), results.end());
	for (String *sp = results.begin(); sp < unique_result; sp++)
	    result_errh->message("%s", sp->c_str());
    }
}

static void
do_query_optical_size(const OpenType::Font &otf, ErrorHandler *errh, ErrorHandler *result_errh)
{
    int before_nerrors = errh->nerrors();
    try {
	String gpos_table = otf.table("GPOS");
	if (!gpos_table)
	    throw OpenType::Error();

	OpenType::Gpos gpos(gpos_table, errh);
	OpenType::Name name(otf.table("name"), errh);

	// extract 'size' feature
	int required_fid;
	Vector<int> fids;
	gpos.script_list().features(script, langsys, required_fid, fids, errh);

	int size_fid = gpos.feature_list().find(OpenType::Tag("size"), fids);
	if (size_fid < 0)
	    throw OpenType::Error();

	// it looks like Adobe fonts implement an old, incorrect idea
	// of what the FeatureParams offset means.
	OpenType::Data size_data = gpos.feature_list().params(size_fid, 10, errh, true);
	if (!size_data.length())
	    throw OpenType::Error();

	StringAccum sa;
	sa << "design size " << (size_data.u16(0) / 10.) << " pt";
	if (size_data.u16(2) != 0) {
	    sa << ", size range (" << (size_data.u16(6) / 10.) << " pt, "
	       << (size_data.u16(8) / 10.) << " pt], "
	       << "subfamily ID " << size_data.u16(2);
	    if (String n = name.name(std::find_if(name.begin(), name.end(), OpenType::Name::EnglishPlatformPred(size_data.u16(4)))))
		sa << ", subfamily name " << n;
	}
	
	result_errh->message("%s", sa.c_str());
	
    } catch (OpenType::Error) {
	if (errh->nerrors() == before_nerrors)
	    result_errh->message("no optical size information");
    }
}

static void
do_query_postscript_name(const OpenType::Font &otf, ErrorHandler *errh, ErrorHandler *result_errh)
{
    int before_nerrors = errh->nerrors();
    String postscript_name = "no PostScript name information";

    if (String name_table = otf.table("name")) {
	OpenType::Name name(name_table, errh);
	if (name.ok())
	    postscript_name = name.name(std::find_if(name.begin(), name.end(), OpenType::Name::EnglishPlatformPred(OpenType::Name::N_POSTSCRIPT)));
    }

    if (errh->nerrors() == before_nerrors)
	result_errh->message("%s", postscript_name.c_str());
}

int
main(int argc, char *argv[])
{
    String::static_initialize();
    Clp_Parser *clp =
	Clp_NewParser(argc, (const char * const *)argv, sizeof(options) / sizeof(options[0]), options);
    program_name = Clp_ProgramName(clp);
    
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr, String(program_name) + ": "));
    Vector<const char *> input_files;
    int query = 0;
  
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {

	  case SCRIPT_OPT: {
	      if (!script.null())
		  usage_error(errh, "--script already specified");
	      String arg = clp->arg;
	      int period = arg.find_left('.');
	      OpenType::Tag scr(period <= 0 ? arg : arg.substring(0, period));
	      if (scr.valid() && period > 0) {
		  OpenType::Tag lang(arg.substring(period + 1));
		  if (lang.valid()) {
		      script = scr;
		      langsys = lang;
		  } else
		      usage_error(errh, "bad language tag");
	      } else if (scr.valid())
		  script = scr;
	      else
		  usage_error(errh, "bad script tag");
	      break;
	  }
	    
	  case QUERY_SCRIPTS_OPT:
	  case QUERY_FEATURES_OPT:
	  case QUERY_OPTICAL_SIZE_OPT:
	  case QUERY_POSTSCRIPT_NAME_OPT:
	    if (query)
		usage_error(errh, "supply exactly one --query option");
	    query = opt;
	    break;

	  case QUIET_OPT:
	    if (clp->negated)
		errh = ErrorHandler::default_handler();
	    else
		errh = ErrorHandler::silent_handler();
	    break;

	  case VERBOSE_OPT:
	    verbose = !clp->negated;
	    break;

	  case VERSION_OPT:
	    printf("otfinfo (LCDF typetools) %s\n", VERSION);
	    printf("Copyright (C) 2003-2004 Eddie Kohler\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty, not even for merchantability or fitness for a\n\
particular purpose.\n");
	    exit(0);
	    break;
      
	  case HELP_OPT:
	    usage();
	    exit(0);
	    break;

	  case Clp_NotOption:
	    input_files.push_back(clp->arg);
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
    if (!query)
	usage_error(errh, "supply exactly one --query option");
    if (!input_files.size())
	input_files.push_back("-");
    if (script.null())
	script = Efont::OpenType::Tag("latn");

    FileErrorHandler stdout_errh(stdout);
    for (const char **input_filep = input_files.begin(); input_filep != input_files.end(); input_filep++) {
	int before_nerrors = errh->nerrors();
	String font_data = read_file(*input_filep, errh);
	if (errh->nerrors() != before_nerrors)
	    continue;

	String input_file = printable_filename(*input_filep);
	LandmarkErrorHandler cerrh(errh, input_file);
	OpenType::Font otf(font_data, &cerrh);
	if (!otf.ok())
	    continue;

	PrefixErrorHandler stdout_cerrh(&stdout_errh, input_file + ":");
	ErrorHandler *result_errh = (input_files.size() > 1 ? static_cast<ErrorHandler *>(&stdout_cerrh) : static_cast<ErrorHandler *>(&stdout_errh));
	if (query == QUERY_SCRIPTS_OPT)
	    do_query_scripts(otf, &cerrh, result_errh);
	else if (query == QUERY_FEATURES_OPT)
	    do_query_features(otf, &cerrh, result_errh);
	else if (query == QUERY_OPTICAL_SIZE_OPT)
	    do_query_optical_size(otf, &cerrh, result_errh);
	else if (query == QUERY_POSTSCRIPT_NAME_OPT)
	    do_query_postscript_name(otf, &cerrh, result_errh);
    }
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
