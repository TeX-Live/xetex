/* otftotfm.cc -- driver for translating OpenType fonts to TeX metrics
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
#include <efont/t1rw.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <efont/t1bounds.hh>
#include <efont/otfcmap.hh>
#include <efont/otfgsub.hh>
#include "glyphfilter.hh"
#include "metrics.hh"
#include "dvipsencoding.hh"
#include "automatic.hh"
#include "secondary.hh"
#include "kpseinterface.h"
#include "util.hh"
#include "otftotfm.hh"
#include "md5.h"
#include <lcdf/clp.h>
#include <lcdf/error.hh>
#include <lcdf/hashmap.hh>
#include <lcdf/straccum.hh>
#include <efont/cff.hh>
#include <efont/otf.hh>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <algorithm>
#include <math.h>
#ifdef HAVE_CTIME
# include <time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

using namespace Efont;

#define VERSION_OPT		301
#define HELP_OPT		302
#define QUERY_SCRIPTS_OPT	303
#define QUERY_FEATURES_OPT	304
#define KPATHSEA_DEBUG_OPT	305

#define SCRIPT_OPT		311
#define FEATURE_OPT		312
#define ENCODING_OPT		313
#define LITERAL_ENCODING_OPT	314
#define EXTEND_OPT		315
#define SLANT_OPT		316
#define LETTERSPACE_OPT		317
#define LIGKERN_OPT		318
#define CODINGSCHEME_OPT	319
#define UNICODING_OPT		320
#define BOUNDARY_CHAR_OPT	321
#define DESIGN_SIZE_OPT		322
#define MINIMUM_KERN_OPT	323
#define ALTSELECTOR_CHAR_OPT	324
#define INCLUDE_ALTERNATES_OPT	325
#define EXCLUDE_ALTERNATES_OPT	326
#define ALTSELECTOR_FEATURE_OPT	327
#define DEFAULT_LIGKERN_OPT	328
#define NO_ECOMMAND_OPT		329
#define LETTER_FEATURE_OPT	330

#define AUTOMATIC_OPT		341
#define FONT_NAME_OPT		342
#define QUIET_OPT		343
#define GLYPHLIST_OPT		344
#define VENDOR_OPT		345
#define TYPEFACE_OPT		346
#define NOCREATE_OPT		347
#define VERBOSE_OPT		348
#define FORCE_OPT		349

#define VIRTUAL_OPT		351
#define PL_OPT			352
#define TFM_OPT			353
#define MAP_FILE_OPT		354

#define DIR_OPTS		360
#define ENCODING_DIR_OPT	(DIR_OPTS + O_ENCODING)
#define TFM_DIR_OPT		(DIR_OPTS + O_TFM)
#define PL_DIR_OPT		(DIR_OPTS + O_PL)
#define VF_DIR_OPT		(DIR_OPTS + O_VF)
#define VPL_DIR_OPT		(DIR_OPTS + O_VPL)
#define TYPE1_DIR_OPT		(DIR_OPTS + O_TYPE1)

#define NO_OUTPUT_OPTS		380
#define NO_ENCODING_OPT		(NO_OUTPUT_OPTS + G_ENCODING)
#define NO_TYPE1_OPT		(NO_OUTPUT_OPTS + G_TYPE1)
#define NO_DOTLESSJ_OPT		(NO_OUTPUT_OPTS + G_DOTLESSJ)

#define CHAR_OPTTYPE		(Clp_FirstUserType)

static Clp_Option options[] = {
    
    { "script", 's', SCRIPT_OPT, Clp_ArgString, 0 },
    { "feature", 'f', FEATURE_OPT, Clp_ArgString, 0 },
    { "letter-feature", 0, LETTER_FEATURE_OPT, Clp_ArgString, 0 },
    { "lf", 0, LETTER_FEATURE_OPT, Clp_ArgString, 0 },
    { "encoding", 'e', ENCODING_OPT, Clp_ArgString, 0 },
    { "literal-encoding", 0, LITERAL_ENCODING_OPT, Clp_ArgString, 0 },
    { "extend", 'E', EXTEND_OPT, Clp_ArgDouble, 0 },
    { "slant", 'S', SLANT_OPT, Clp_ArgDouble, 0 },
    { "letterspacing", 'L', LETTERSPACE_OPT, Clp_ArgInt, 0 },
    { "letterspace", 'L', LETTERSPACE_OPT, Clp_ArgInt, 0 },
    { "min-kern", 'k', MINIMUM_KERN_OPT, Clp_ArgDouble, 0 },
    { "minimum-kern", 'k', MINIMUM_KERN_OPT, Clp_ArgDouble, 0 },
    { "kern-precision", 'k', MINIMUM_KERN_OPT, Clp_ArgDouble, 0 },
    { "ligkern", 0, LIGKERN_OPT, Clp_ArgString, 0 },
    { "no-encoding-commands", 0, NO_ECOMMAND_OPT, 0, Clp_OnlyNegated },
    { "default-ligkern", 0, DEFAULT_LIGKERN_OPT, 0, 0 },
    { "unicoding", 0, UNICODING_OPT, Clp_ArgString, 0 },
    { "coding-scheme", 0, CODINGSCHEME_OPT, Clp_ArgString, 0 },
    { "boundary-char", 0, BOUNDARY_CHAR_OPT, CHAR_OPTTYPE, 0 },
    { "altselector", 0, ALTSELECTOR_CHAR_OPT, CHAR_OPTTYPE, Clp_PreferredMatch },
    { "altselector-char", 0, ALTSELECTOR_CHAR_OPT, CHAR_OPTTYPE, 0 },
    { "altselector-feature", 0, ALTSELECTOR_FEATURE_OPT, Clp_ArgString, 0 },
    { "design-size", 0, DESIGN_SIZE_OPT, Clp_ArgDouble, 0 },
    { "include-alternates", 0, INCLUDE_ALTERNATES_OPT, Clp_ArgString, 0 },
    { "exclude-alternates", 0, EXCLUDE_ALTERNATES_OPT, Clp_ArgString, 0 },
    
    { "pl", 'p', PL_OPT, 0, 0 },
    { "virtual", 0, VIRTUAL_OPT, 0, Clp_Negate },
    { "no-encoding", 0, NO_ENCODING_OPT, 0, Clp_OnlyNegated },
    { "no-type1", 0, NO_TYPE1_OPT, 0, Clp_OnlyNegated },
    { "no-dotlessj", 0, NO_DOTLESSJ_OPT, 0, Clp_OnlyNegated },
    { "map-file", 0, MAP_FILE_OPT, Clp_ArgString, Clp_Negate },
        
    { "automatic", 'a', AUTOMATIC_OPT, 0, Clp_Negate },
    { "name", 'n', FONT_NAME_OPT, Clp_ArgString, 0 },
    { "vendor", 'v', VENDOR_OPT, Clp_ArgString, 0 },
    { "typeface", 0, TYPEFACE_OPT, Clp_ArgString, 0 },
    
    { "encoding-directory", 0, ENCODING_DIR_OPT, Clp_ArgString, 0 },
    { "pl-directory", 0, PL_DIR_OPT, Clp_ArgString, 0 },
    { "tfm-directory", 0, TFM_DIR_OPT, Clp_ArgString, 0 },
    { "vpl-directory", 0, VPL_DIR_OPT, Clp_ArgString, 0 },
    { "vf-directory", 0, VF_DIR_OPT, Clp_ArgString, 0 },
    { "type1-directory", 0, TYPE1_DIR_OPT, Clp_ArgString, 0 },

    { "quiet", 'q', QUIET_OPT, 0, Clp_Negate },
    { "glyphlist", 0, GLYPHLIST_OPT, Clp_ArgString, 0 },
    { "no-create", 0, NOCREATE_OPT, 0, Clp_OnlyNegated },
    { "force", 0, FORCE_OPT, 0, Clp_Negate },
    { "verbose", 'V', VERBOSE_OPT, 0, Clp_Negate },
    { "kpathsea-debug", 0, KPATHSEA_DEBUG_OPT, Clp_ArgInt, 0 },

    { "help", 'h', HELP_OPT, 0, 0 },
    { "version", 0, VERSION_OPT, 0, 0 },

    { "tfm", 't', TFM_OPT, 0, 0 }, // deprecated
    { "query-features", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "qf", 0, QUERY_FEATURES_OPT, 0, 0 },
    { "query-scripts", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    { "qs", 0, QUERY_SCRIPTS_OPT, 0, 0 },
    
};

static const char * const default_ligkerns = "\
space l =: lslash ; space L =: Lslash ; \
question quoteleft =: questiondown ; \
exclam quoteleft =: exclamdown ; \
hyphen hyphen =: endash ; endash hyphen =: emdash ; \
quoteleft quoteleft =: quotedblleft ; \
quoteright quoteright =: quotedblright ;";

static const char *program_name;
static String::Initializer initializer;
static String current_time;
static StringAccum invocation;

static PermString::Initializer perm_initializer;
static PermString dot_notdef(".notdef");

static Vector<Efont::OpenType::Tag> interesting_scripts;
static Vector<Efont::OpenType::Tag> interesting_features;
static Vector<Efont::OpenType::Tag> altselector_features;
static GlyphFilter null_filter;
static HashMap<Efont::OpenType::Tag, GlyphFilter*> feature_filters(&null_filter);

static GlyphFilter alternate_filter;

static String font_name;
static String encoding_file;
static double extend;
static double slant;
static int letterspace;
static double design_size;
static double minimum_kern = 2.0;

static String out_encoding_file;
static String out_encoding_name;

int output_flags = G_ENCODING | G_METRICS | G_VMETRICS | G_PSFONTSMAP | G_TYPE1 | G_DOTLESSJ | G_BINARY;

bool automatic = false;
bool verbose = false;
bool no_create = false;
bool quiet = false;
bool force = false;


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
'Otftotfm' generates TeX font metrics files from an OpenType font (PostScript\n\
flavor only), including ligatures, kerns, and some positionings. Supply\n\
'-s SCRIPT[.LANG]' options to specify a language system, '-f FEAT' options to\n\
turn on optional OpenType features, and a '-e ENC' option to specify a base\n\
encoding. Output files are written to the current directory (but see\n\
'--automatic' and the 'directory' options).\n\
\n\
Usage: %s [-a] [OPTIONS] OTFFILE FONTNAME\n\n",
	   program_name);
    printf("\
Font feature and transformation options:\n\
  -s, --script=SCRIPT[.LANG]   Use features for script SCRIPT[.LANG] [latn].\n\
  -f, --feature=FEAT           Activate feature FEAT.\n\
  --lf, --letter-feature=FEAT  Activate feature FEAT for letters.\n\
  -E, --extend=F               Widen characters by a factor of F.\n\
  -S, --slant=AMT              Oblique characters by AMT, generally <<1.\n\
  -L, --letterspacing=AMT      Letterspace each character by AMT units.\n\
  -k, --min-kern=N             Omit kerns with absolute value < N [2.0].\n\
      --design-size=SIZE       Set font design size to SIZE.\n\
\n\
Encoding options:\n\
  -e, --encoding=FILE          Use DVIPS encoding FILE as a base encoding.\n\
      --literal-encoding=FILE  Use DVIPS encoding FILE verbatim.\n\
      --ligkern=COMMAND        Add a LIGKERN command.\n\
      --unicoding=COMMAND      Add a UNICODING command.\n\
      --no-encoding-commands   Ignore encoding file's LIGKERN/UNICODINGs.\n\
      --default-ligkern        Ignore encoding file's LIGKERNs, use defaults.\n\
      --coding-scheme=SCHEME   Set the output coding scheme to SCHEME.\n\
      --boundary-char=CHAR     Set the boundary character to CHAR.\n\
      --altselector-char=CHAR  Set the alternate selector character to CHAR.\n\
      --altselector-feature=F  Activate feature F for --altselector-char.\n\
      --exclude-alternates=PAT Ignore alternate characters matching PAT.\n\
      --include-alternates=PAT Include only alternate characters matching PAT.\n\
\n");
    printf("\
Automatic mode options:\n\
  -a, --automatic              Install in a TeX Directory Structure.\n\
  -v, --vendor=NAME            Set font vendor for TDS [lcdftools].\n\
      --typeface=NAME          Set typeface name for TDS [<font family>].\n\
      --no-type1               Do not generate Type 1 fonts.\n\
      --no-dotlessj            Do not generate dotless-j fonts.\n\
\n\
Output options:\n\
  -n, --name=NAME              Generated font name is NAME.\n\
  -p, --pl                     Output human-readable PL/VPLs, not TFM/VFs.\n\
      --no-virtual             Do not generate VFs or VPLs.\n\
      --no-encoding            Do not generate an encoding file.\n\
      --no-map                 Do not generate a psfonts.map line.\n\
\n");
    printf("\
File location options:\n\
      --tfm-directory=DIR      Put TFM files in DIR [.|automatic].\n\
      --pl-directory=DIR       Put PL files in DIR [.|automatic].\n\
      --vf-directory=DIR       Put VF files in DIR [.|automatic].\n\
      --vpl-directory=DIR      Put VPL files in DIR [.|automatic].\n\
      --encoding-directory=DIR Put encoding files in DIR [.|automatic].\n\
      --type1-directory=DIR    Put Type 1 fonts in DIR [automatic].\n\
      --map-file=FILE          Update FILE with psfonts.map information [-].\n\
\n\
Other options:\n\
      --glyphlist=FILE         Use FILE to map Adobe glyph names to Unicode.\n\
  -V, --verbose                Print progress information to standard error.\n\
      --no-create              Print messages, don't modify any files.\n\
      --force                  Generate files even if versions already exist.\n"
#if HAVE_KPATHSEA
"      --kpathsea-debug=MASK    Set path searching debug flags to MASK.\n"
#endif
"  -h, --help                   Print this message and exit.\n\
  -q, --quiet                  Do not generate any error messages.\n\
      --version                Print version number and exit.\n\
\n\
Report bugs to <kohler@icir.org>.\n");
}

extern "C" {
static void
sigchld_handler(int s)
{
#if !HAVE_SIGACTION
    signal(s, sigchld_handler);
#else
    (void) s;
#endif
}
}

static void
handle_sigchld()
{
#ifdef SIGCHLD
    int sigchld = SIGCHLD;
#else
    int sigchld = SIGCLD;
#endif
#if HAVE_SIGACTION
    struct sigaction sa;
    sigaction(sigchld, 0, &sa);
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = 0;
    sigaction(sigchld, &sa, 0);
#else
    signal(sigchld, sigchld_handler);
#endif
}


// MAIN

String
suffix_font_name(const String &font_name, const String &suffix)
{
    int pos = font_name.length();
    while (pos > 0 && (isdigit(font_name[pos-1]) || font_name[pos-1] == '-' || font_name[pos-1] == '+'))
	pos--;
    if (pos == 0)
	pos = font_name.length();
    return font_name.substring(0, pos) + suffix + font_name.substring(pos);
}

static inline String
make_base_font_name(const String &font_name)
{
    return suffix_font_name(font_name, "--base");
}

static double
get_design_size(const OpenType::Font &otf)
{
    try {
	String gpos_table = otf.table("GPOS");
	if (!gpos_table)
	    throw OpenType::Error();

	ErrorHandler *errh = ErrorHandler::silent_handler();
	OpenType::Gpos gpos(gpos_table, errh);

	// extract 'size' feature(s)
	int required_fid;
	Vector<int> fids;
	for (const OpenType::Tag *t = interesting_scripts.begin(); t < interesting_scripts.end(); t += 2)
	    gpos.script_list().features(t[0], t[1], required_fid, fids, 0, false);

	int size_fid = gpos.feature_list().find(OpenType::Tag("size"), fids);
	if (size_fid < 0)
	    throw OpenType::Error();

	// it looks like Adobe fonts implement an old, incorrect idea
	// of what the FeatureParams offset means.
	OpenType::Data size_data = gpos.feature_list().params(size_fid, 10, errh, true);
	if (!size_data.length())
	    throw OpenType::Error();

	double result = size_data.u16(0) / 10.;
	// check for insane design sizes
	if (result < 1 || result > 1000)
	    throw OpenType::Error();

	// return a number in 'pt', not 'bp'
	return result * 72.27 / 72.;
	
    } catch (OpenType::Error) {
	return 10.0;
    }
}

static const char * const digit_names[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
};

static inline const char *
lig_context_str(int ctx)
{
    return (ctx == 0 ? "LIG" : (ctx < 0 ? "/LIG" : "LIG/"));
}

static void
fprint_real(FILE *f, const char *prefix, double value, double du, const char *suffix = ")\n")
{
    if (du == 1.)
	fprintf(f, "%s R %g%s", prefix, value, suffix);
    else
        fprintf(f, "%s R %.4f%s", prefix, value * du, suffix);
}

static String
real_string(double value, double du)
{
    if (du == 1.)
	return String(value);
    else {
	char buf[128];
        sprintf(buf, "%.4f", value * du);
        return String(buf);
    }
}

static void
output_pl(const Metrics &metrics, const String &ps_name, int boundary_char,
	  const OpenType::Font &family_otf, const Cff::Font *family_cff,
	  bool vpl, FILE *f)
{
    // XXX check DESIGNSIZE and DESIGNUNITS for correctness

    fprintf(f, "(COMMENT Created by '%s'%s)\n", invocation.c_str(), current_time.c_str());

    // calculate a TeX FAMILY name using afm2tfm's algorithm
    String family_name = String("TeX-") + ps_name;
    if (family_name.length() > 19)
	family_name = family_name.substring(0, 9) + family_name.substring(-10);
    fprintf(f, "(FAMILY %s)\n", family_name.c_str());

    if (metrics.coding_scheme())
	fprintf(f, "(CODINGSCHEME %.39s)\n", String(metrics.coding_scheme()).c_str());
    int design_units = metrics.design_units();

    if (design_size <= 0)
	design_size = get_design_size(family_otf);
    fprintf(f, "(DESIGNSIZE R %.1f)\n"
	    "(DESIGNUNITS R %d.0)\n"
	    "(COMMENT DESIGNSIZE (1 em) IS IN POINTS)\n"
	    "(COMMENT OTHER DIMENSIONS ARE MULTIPLES OF DESIGNSIZE/%d)\n"
	    "(FONTDIMEN\n", design_size, design_units, design_units);

    // figure out font dimensions
    Transform font_xform;
    if (extend)
	font_xform.scale(extend, 1);
    if (slant)
	font_xform.shear(slant);
    int bounds[4], width;
    double du = (design_units == 1000 ? 1. : design_units / 1000.);
    
    double val;
    (void) family_cff->dict_value(Efont::Cff::oItalicAngle, 0, &val);
    double actual_slant = -tan(val * M_PI / 180.0) + slant;
    if (actual_slant)
	fprintf(f, "   (SLANT R %g)\n", actual_slant);

    OpenType::Cmap cmap(family_otf.table("cmap"));
    
    if (char_bounds(bounds, width, family_cff, cmap, ' ', font_xform)) {
	// advance space width by letterspacing
	width += letterspace;
	fprint_real(f, "   (SPACE", width, du);
	if (family_cff->dict_value(Efont::Cff::oIsFixedPitch, 0, &val) && val) {
	    // fixed-pitch: no space stretch or shrink
	    fprint_real(f, "   (STRETCH", 0, du);
	    fprint_real(f, "   (SHRINK", 0, du);
	    fprint_real(f, "   (EXTRASPACE", width, du);
	} else {
	    fprint_real(f, "   (STRETCH", width / 2., du);
	    fprint_real(f, "   (SHRINK", width / 3., du);
	    fprint_real(f, "   (EXTRASPACE", width / 6., du);
	}
    }

    // XXX what if 'x', 'm', 'z' were subject to substitution?
    int xheight = 1000;
    static const int xheight_unis[] = { 'x', 'm', 'z', 0 };
    for (const int *x = xheight_unis; *x; x++)
	if (char_bounds(bounds, width, family_cff, cmap, 'x', font_xform) && bounds[3] < xheight)
	    xheight = bounds[3];
    if (xheight < 1000)
	fprint_real(f, "   (XHEIGHT", xheight, du);
    
    fprint_real(f, "   (QUAD", 1000, du);
    fprintf(f, "   )\n");

    if (boundary_char >= 0)
	fprintf(f, "(BOUNDARYCHAR D %d)\n", boundary_char);

    // write MAPFONT
    if (vpl)
	for (int i = 0; i < metrics.n_mapped_fonts(); i++) {
	    String name = metrics.mapped_font_name(i);
	    if (!name)
		name = make_base_font_name(font_name);
	    fprintf(f, "(MAPFONT D %d\n   (FONTNAME %s)\n   (FONTDSIZE R %.1f)\n   )\n", i, name.c_str(), design_size);
	}
    
    // figure out the proper names and numbers for glyphs
    Vector<String> glyph_ids;
    Vector<String> glyph_comments(257, String());
    Vector<String> glyph_base_comments(257, String());
    for (int i = 0; i < 256; i++) {
	if (metrics.glyph(i)) {
	    PermString name = metrics.code_name(i), expected_name;
	    if (i >= '0' && i <= '9')
		expected_name = digit_names[i - '0'];
	    else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z'))
		expected_name = PermString((char)i);
	    
	    if (expected_name
		&& name.length() >= expected_name.length()
		&& memcmp(name.c_str(), expected_name.c_str(), expected_name.length()) == 0)
		glyph_ids.push_back("C " + String((char)i));
	    else
		glyph_ids.push_back("D " + String(i));
	    
	    if (name != expected_name)
		glyph_comments[i] = " (COMMENT " + String(name) + ")";

	    int base = metrics.base_code(i);
	    if (base != i)
		glyph_base_comments[i] = " (COMMENT " + String(metrics.code_name(base)) + ")";
	    else
		glyph_base_comments[i] = glyph_comments[i];
	    
	} else
	    glyph_ids.push_back("X");
    }
    // finally, BOUNDARYCHAR
    glyph_ids.push_back("BOUNDARYCHAR");

    // LIGTABLE
    fprintf(f, "(LIGTABLE\n");
    Vector<int> lig_code2, lig_outcode, lig_context, kern_code2, kern_amt;
    // don't print KRN x after printing LIG x
    uint32_t used[8];
    for (int i = 0; i <= 256; i++)
	if (metrics.glyph(i)) {
	    int any_lig = metrics.ligatures(i, lig_code2, lig_outcode, lig_context);
	    int any_kern = metrics.kerns(i, kern_code2, kern_amt);
	    if (any_lig || any_kern) {
		StringAccum kern_sa;
		memset(&used[0], 0, 32);
		for (int j = 0; j < lig_code2.size(); j++) {
		    kern_sa << "   (" << lig_context_str(lig_context[j])
			    << ' ' << glyph_ids[lig_code2[j]]
			    << ' ' << glyph_ids[lig_outcode[j]]
			    << ')' << glyph_comments[lig_code2[j]]
			    << glyph_comments[lig_outcode[j]] << '\n';
		    used[lig_code2[j] >> 5] |= (1 << (lig_code2[j] & 0x1F));
		}
		for (Vector<int>::const_iterator k2 = kern_code2.begin(); k2 < kern_code2.end(); k2++)
		    if (!(used[*k2 >> 5] & (1 << (*k2 & 0x1F)))) {
			double this_kern = kern_amt[k2 - kern_code2.begin()];
			if (fabs(this_kern) >= minimum_kern)
			    kern_sa << "   (KRN " << glyph_ids[*k2]
				    << " R " << real_string(this_kern, du)
				    << ')' << glyph_comments[*k2] << '\n';
		    }
		if (kern_sa)
		    fprintf(f, "   (LABEL %s)%s\n%s   (STOP)\n", glyph_ids[i].c_str(), glyph_comments[i].c_str(), kern_sa.c_str());
	    }
	}
    fprintf(f, "   )\n");
    
    // CHARACTERs
    Vector<Setting> settings;
    StringAccum sa;
    
    for (int i = 0; i < 256; i++)
	if (metrics.setting(i, settings)) {
	    fprintf(f, "(CHARACTER %s%s\n", glyph_ids[i].c_str(), glyph_comments[i].c_str());

	    // unparse settings into DVI commands
	    sa.clear();
	    CharstringBounds boundser(font_xform);
	    const CharstringProgram *program = family_cff;
	    for (int j = 0; j < settings.size(); j++) {
		Setting &s = settings[j];
		if (s.op == Setting::SHOW) {
		    boundser.char_bounds(program->glyph_context(s.y));
		    // 3.Aug.2004 -- reported by Marco Kuhlmann: Don't use
		    // glyph_ids[] array when looking at a different font.
		    if (program == family_cff)
			sa << "      (SETCHAR " << glyph_ids[s.x] << ')' << glyph_base_comments[s.x] << "\n";
		    else
			sa << "      (SETCHAR D " << s.x << ")\n";
		} else if (s.op == Setting::MOVE && vpl) {
		    boundser.translate(s.x, s.y);
		    if (s.x)
			sa << "      (MOVERIGHT R " << real_string(s.x, du) << ")\n";
		    if (s.y)
			sa << "      (MOVEUP R " << real_string(s.y, du) << ")\n";
		} else if (s.op == Setting::RULE && vpl) {
		    boundser.mark(Point(0, 0));
		    boundser.mark(Point(s.x, s.y));
		    boundser.translate(s.x, 0);
		    sa << "      (SETRULE R " << real_string(s.y, du) << " R " << real_string(s.x, du) << ")\n";
		} else if (s.op == Setting::FONT && vpl) {
		    sa << "      (SELECTFONT D " << (int) s.x << ")\n";
		    program = metrics.mapped_font((int) s.x);
		}
	    }

	    // output information
	    boundser.output(bounds, width);
	    fprint_real(f, "   (CHARWD", width, du);
	    if (bounds[3] > 0)
		fprint_real(f, "   (CHARHT", bounds[3], du);
	    if (bounds[1] < 0)
		fprint_real(f, "   (CHARDP", -bounds[1], du);
	    if (bounds[2] > width)
		fprint_real(f, "   (CHARIC", bounds[2] - width, du);
	    if (vpl && (settings.size() > 1 || settings[0].op != Setting::SHOW))
		fprintf(f, "   (MAP\n%s      )\n", sa.c_str());
	    fprintf(f, "   )\n");
	}
}

static void
output_pl(const Metrics &metrics, const String &ps_name, int boundary_char,
	  const OpenType::Font &family_otf, const Cff::Font *family_cff,
	  bool vpl, String filename, ErrorHandler *errh)
{
    if (no_create)
	errh->message("would create %s", filename.c_str());
    else {
	if (verbose)
	    errh->message("creating %s", filename.c_str());
	if (FILE *f = fopen(filename.c_str(), "w")) {
	    output_pl(metrics, ps_name, boundary_char, family_otf, family_cff, vpl, f);
	    fclose(f);
	} else
	    errh->error("%s: %s", filename.c_str(), strerror(errno));
    }
}

struct Lookup {
    bool used;
    bool required;
    Vector<OpenType::Tag> features;
    GlyphFilter* filter;
    Lookup()			: used(false), required(false), filter(0) { }
};

static void
find_lookups(const OpenType::ScriptList& scripts, const OpenType::FeatureList& features, Vector<Lookup>& lookups, ErrorHandler* errh)
{
    Vector<int> fids, lookupids;
    int required;

    // go over all scripts
    for (int i = 0; i < interesting_scripts.size(); i += 2) {
	OpenType::Tag script = interesting_scripts[i];
	OpenType::Tag langsys = interesting_scripts[i+1];
	
	// collect features applying to this script
	scripts.features(script, langsys, required, fids, errh);

	// only use the selected features
	features.filter(fids, interesting_features);

	// mark features as having been used
	for (int j = (required < 0 ? 0 : -1); j < fids.size(); j++) {
	    int fid = (j < 0 ? required : fids[j]);
	    OpenType::Tag ftag = features.tag(fid);
	    if (features.lookups(fid, lookupids, errh) < 0)
		lookupids.clear();
	    for (int k = 0; k < lookupids.size(); k++) {
		int l = lookupids[k];
		if (l < 0 || l >= lookups.size())
		    errh->error("lookup for '%s' feature out of range", OpenType::Tag::langsys_text(script, langsys).c_str());
		else {
		    lookups[l].used = true;
		    lookups[l].features.push_back(ftag);
		    if (j < 0)
			lookups[l].required = true;
		}
	    }
	}
    }

    // now check for compatible glyph filters
    for (Lookup* l = lookups.begin(); l < lookups.end(); l++)
	if (l->used && !l->required) {
	    l->filter = feature_filters[l->features[0]];
	    for (OpenType::Tag* ft = l->features.begin() + 1; ft < l->features.end(); ft++)
		if (!l->filter->check_eq(*feature_filters[*ft])) {
		    errh->error("'%s' and '%s' features share a lookup, but have different filters", l->features[0].text().c_str(), ft->text().c_str());
		    break;
		}
	}
}

static int
write_encoding_file(String &filename, const String &encoding_name,
		    StringAccum &contents, ErrorHandler *errh)
{
    FILE *f;
    int ok_retval = (access(filename.c_str(), R_OK) >= 0 ? 0 : 1);

    if (no_create) {
	errh->message((ok_retval ? "would create encoding file %s" : "would update encoding file %s"), filename.c_str());
	return ok_retval;
    } else if (verbose)
	errh->message((ok_retval ? "creating encoding file %s" : "updating encoding file %s"), filename.c_str());
    
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0)
	return errh->error("%s: %s", filename.c_str(), strerror(errno));
    f = fdopen(fd, "r+");
    // NB: also change update_autofont_map if you change this code

#if defined(F_SETLKW) && defined(HAVE_FTRUNCATE)
    {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	int result;
	while ((result = fcntl(fd, F_SETLKW, &lock)) < 0 || errno == EINTR)
	    /* try again */;
	if (result < 0) {
	    result = errno;
	    fclose(f);
	    return errh->error("locking %s: %s", filename.c_str(), strerror(result));
	}
    }
#endif

    // read old data from encoding file
    StringAccum sa;
    while (!feof(f))
	if (char *x = sa.reserve(8192)) {
	    int amt = fread(x, 1, 8192, f);
	    sa.forward(amt);
	} else {
	    fclose(f);
	    return errh->error("Out of memory!");
	}
    String old_encodings = sa.take_string();
    bool created = (!old_encodings);

    // append old encodings
    int pos1 = old_encodings.find_left("\n%%");
    while (pos1 < old_encodings.length()) {
	int pos2 = old_encodings.find_left("\n%%", pos1 + 1);
	if (pos2 < 0)
	    pos2 = old_encodings.length();
	if (old_encodings.substring(pos1 + 3, encoding_name.length()) == encoding_name) {
	    // encoding already exists, don't change it
	    fclose(f);
	    if (verbose)
		errh->message("%s unchanged", filename.c_str());
	    return 0;
	} else
	    contents << old_encodings.substring(pos1, pos2 - pos1);
	pos1 = pos2;
    }
    
    // rewind file
#ifdef HAVE_FTRUNCATE
    rewind(f);
    ftruncate(fd, 0);
#else
    fclose(f);
    f = fopen(filename.c_str(), "w");
#endif

    fwrite(contents.data(), 1, contents.length(), f);

    fclose(f);

    // inform about the new file if necessary
    if (created)
	update_odir(O_ENCODING, filename, errh);
    return 0;
}
	
static void
output_encoding(const Metrics &metrics,
		const Vector<PermString> &glyph_names,
		ErrorHandler *errh)
{
    static const char * const hex_digits = "0123456789ABCDEF";

    // collect encoding data
    Vector<Metrics::Glyph> glyphs;
    metrics.base_glyphs(glyphs);
    StringAccum sa;
    for (int i = 0; i < 256; i++) {
	if ((i & 0xF) == 0)
	    sa << (i ? "\n%" : "%") << hex_digits[(i >> 4) & 0xF] << '0' << '\n' << ' ';
	else if ((i & 0x7) == 0)
	    sa << '\n' << ' ';
	if (int g = glyphs[i])
	    sa << ' ' << '/' << glyph_names[g];
	else
	    sa << " /.notdef";
    }
    sa << '\n';

    // digest encoding data
    MD5_CONTEXT md5;
    md5_init(&md5);
    md5_update(&md5, (const unsigned char *)sa.data(), sa.length());
    char text_digest[MD5_TEXT_DIGEST_SIZE + 1];
    md5_final_text(text_digest, &md5);

    // name encoding using digest
    out_encoding_name = "AutoEnc_" + String(text_digest);

    // create encoding filename
    out_encoding_file = getodir(O_ENCODING, errh) + String("/a_") + String(text_digest).substring(0, 6) + ".enc";

    // exit if we're not responsible for generating an encoding
    if (!(output_flags & G_ENCODING))
	return;

    // put encoding block in a StringAccum
    // 3.Jun.2003: stick command line definition at the end of the encoding,
    // where it won't confuse idiotic ps2pk
    StringAccum contents;
    contents << "% THIS FILE WAS AUTOMATICALLY GENERATED -- DO NOT EDIT\n\n\
%%" << out_encoding_name << "\n\
% Encoding created by otftotfm" << current_time << "\n\
% Command line follows encoding\n";
    
    // the encoding itself
    contents << '/' << out_encoding_name << " [\n" << sa << "] def\n";
    
    // write banner -- unfortunately this takes some doing
    String banner = String("Command line: '") + String(invocation.data(), invocation.length()) + String("'");
    char *buf = banner.mutable_data();
    // get rid of crap characters
    for (int i = 0; i < banner.length(); i++)
	if (buf[i] < ' ' || buf[i] > 0176) {
	    if (buf[i] == '\n' || buf[i] == '\r')
		buf[i] = ' ';
	    else
		buf[i] = '.';
	}
    // break lines at 80 characters -- it would be nice if this were in a
    // library
    while (banner.length() > 0) {
	int pos = banner.find_left(' '), last_pos = pos;
	while (pos < 75 && pos >= 0) {
	    last_pos = pos;
	    pos = banner.find_left(' ', pos + 1);
	}
	if (last_pos < 0 || (pos < 0 && banner.length() < 75))
	    last_pos = banner.length();
	contents << "% " << banner.substring(0, last_pos) << '\n';
	banner = banner.substring(last_pos + 1);
    }
    
    // open encoding file
    if (write_encoding_file(out_encoding_file, out_encoding_name, contents, errh) == 1)
	update_odir(O_ENCODING, out_encoding_file, errh);
}

static int
temporary_file(String &filename, ErrorHandler *errh)
{
    if (no_create)
	return 0;		// random number suffices
    
#ifdef HAVE_MKSTEMP
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir)
	filename = String(tmpdir) + "/otftotfm.XXXXXX";
    else {
# ifdef P_tmpdir
	filename = P_tmpdir "/otftotfm.XXXXXX";
# else
	filename = "/tmp/otftotfm.XXXXXX";
# endif
    }
    int fd = mkstemp(filename.mutable_c_str());
    if (fd < 0)
	errh->error("temporary file '%s': %s", filename.c_str(), strerror(errno));
    return fd;
#else  // !HAVE_MKSTEMP
    for (int tries = 0; tries < 5; tries++) {
	if (!(filename = tmpnam(0)))
	    return errh->error("cannot create temporary file");
# ifdef O_EXCL
	int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 0600);
# else
	int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
# endif
	if (fd >= 0)
	    return fd;
    }
    return errh->error("temporary file '%s': %s", filename.c_str(), strerror(errno));
#endif
}

static void
output_tfm(const Metrics &metrics, const String &ps_name, int boundary_char,
	   const OpenType::Font &family_otf, const Cff::Font *family_cff,
	   String tfm_filename, String vf_filename, ErrorHandler *errh)
{
    String pl_filename;
    int pl_fd = temporary_file(pl_filename, errh);
    if (pl_fd < 0)
	return;

    bool vpl = vf_filename;

    if (no_create) {
	errh->message("would write %s to temporary file", (vpl ? "VPL" : "PL"));
	pl_filename = "<temporary>";
    } else {
	if (verbose)
	    errh->message("writing %s to temporary file", (vpl ? "VPL" : "PL"));
	FILE *f = fdopen(pl_fd, "w");
	output_pl(metrics, ps_name, boundary_char, family_otf, family_cff, vpl, f);
	fclose(f);
    }

    StringAccum command;
    if (vpl)
	command << "vptovf " << pl_filename << ' ' << vf_filename << ' ' << tfm_filename;
    else
	command << "pltotf " << pl_filename << ' ' << tfm_filename;
    
    int status = mysystem(command.c_str(), errh);

    if (!no_create)
	unlink(pl_filename.c_str());
    
    if (status != 0)
	errh->fatal("%s execution failed", (vpl ? "vptovf" : "pltotf"));
    else {
	update_odir(O_TFM, tfm_filename, errh);
	if (vpl)
	    update_odir(O_VF, vf_filename, errh);
    }
}

void
output_metrics(Metrics &metrics, const String &ps_name, int boundary_char,
	       const OpenType::Font &family_otf, const Cff::Font *family_cff,
	       const String &encoding_name, const String &encoding_file,
	       const String &font_name,
	       void (*dvips_include)(const String &ps_name, StringAccum &, ErrorHandler *),
	       ErrorHandler *errh)
{
    String base_font_name = font_name;
    bool need_virtual = metrics.need_virtual(257);
    if (need_virtual) {
	if (output_flags & G_VMETRICS)
	    base_font_name = make_base_font_name(font_name);
	else
	    errh->warning("features require virtual fonts");
    }
    
    // output virtual metrics
    if (!(output_flags & G_VMETRICS))
	/* do nothing */;
    else if (!need_virtual) {
	if (automatic) {
	    // erase old virtual font
	    String vf = getodir(O_VF, errh) + "/" + font_name + ".vf";
	    if (no_create)
		errh->message("would remove potential VF file '%s'", vf.c_str());
	    else if (verbose)
		errh->message("removing potential VF file '%s'", vf.c_str());
	    if (!no_create && unlink(vf.c_str()) < 0 && errno != ENOENT)
		errh->error("removing %s: %s", vf.c_str(), strerror(errno));
	}
    } else {
	if (output_flags & G_BINARY) {
	    String tfm = getodir(O_TFM, errh) + "/" + font_name + ".tfm";
	    String vf = getodir(O_VF, errh) + "/" + font_name + ".vf";
	    output_tfm(metrics, ps_name, boundary_char, family_otf, family_cff, tfm, vf, errh);
	} else {
	    String outfile = getodir(O_VPL, errh) + "/" + font_name + ".vpl";
	    output_pl(metrics, ps_name, boundary_char, family_otf, family_cff, true, outfile, errh);
	    update_odir(O_VPL, outfile, errh);
	}
	metrics.make_base(257);
    }

    // output metrics
    if (!(output_flags & G_METRICS))
	/* do nothing */;
    else if (output_flags & G_BINARY) {
	String tfm = getodir(O_TFM, errh) + "/" + base_font_name + ".tfm";
	output_tfm(metrics, ps_name, boundary_char, family_otf, family_cff, tfm, String(), errh);
    } else {
	String outfile = getodir(O_PL, errh) + "/" + base_font_name + ".pl";
	output_pl(metrics, ps_name, boundary_char, family_otf, family_cff, false, outfile, errh);
	update_odir(O_PL, outfile, errh);
    }

    // print DVIPS map line
    if (errh->nerrors() == 0 && (output_flags & G_PSFONTSMAP)) {
	StringAccum sa;
	sa << base_font_name << ' ' << ps_name << " \"";
	if (extend)
	    sa << extend << " ExtendFont ";
	if (slant)
	    sa << slant << " SlantFont ";
	if (encoding_name)
	    sa << encoding_name << " ReEncodeFont\" <[" << pathname_filename(encoding_file);
	else
	    sa << "\"";
	sa << ' ';
	dvips_include(ps_name, sa, errh);
	sa << '\n';
	update_autofont_map(base_font_name, sa.take_string(), errh);
	// if virtual font, remove any map line for base font name
	if (base_font_name != font_name)
	    update_autofont_map(font_name, "", errh);
    }
}


enum { F_GSUB_TRY = 1, F_GSUB_PART = 2, F_GSUB_ALL = 4,
       F_GPOS_TRY = 8, F_GPOS_PART = 16, F_GPOS_ALL = 32,
       X_UNUSED = 0, X_BOTH_NONE = 1, X_GSUB_NONE = 2, X_GSUB_PART = 3,
       X_GPOS_NONE = 4, X_GPOS_PART = 5, X_COUNT };

static const char * const x_messages[] = {
    "% ignored, not supported by font",
    "% ignored, too complex for me",
    "complex substitutions from % ignored",
    "some complex substitutions from % ignored",
    "complex positionings from % ignored",
    "some complex positionings from % ignored",
};

static void
report_underused_features(const HashMap<uint32_t, int> &feature_usage, ErrorHandler *errh)
{
    Vector<String> x[X_COUNT];
    for (int i = 0; i < interesting_features.size(); i++) {
	OpenType::Tag f = interesting_features[i];
	int fu = feature_usage[f.value()];
	String ftext = "'" + f.text() + "'";
	if (fu == 0)
	    x[X_UNUSED].push_back(ftext);
	else if ((fu & (F_GSUB_TRY | F_GPOS_TRY)) == fu)
	    x[X_BOTH_NONE].push_back(ftext);
	else {
	    if (fu & F_GSUB_TRY) {
		if ((fu & (F_GSUB_PART | F_GSUB_ALL)) == 0)
		    x[X_GSUB_NONE].push_back(ftext);
		else if (fu & F_GSUB_PART)
		    x[X_GSUB_PART].push_back(ftext);
	    }
	    if (fu & F_GPOS_TRY) {
		if ((fu & (F_GPOS_PART | F_GPOS_ALL)) == 0)
		    x[X_GPOS_NONE].push_back(ftext);
		else if (fu & F_GPOS_PART)
		    x[X_GPOS_PART].push_back(ftext);
	    }
	}
    }

    for (int i = 0; i < X_COUNT; i++)
	if (x[i].size())
	    goto found;
    return;

  found:
    for (int i = 0; i < X_COUNT; i++)
	if (x[i].size()) {
	    StringAccum sa;
	    const char* msg_pct = strchr(x_messages[i], '%');
	    sa.append(x_messages[i], msg_pct - x_messages[i]);
	    const char* sep = (x[i].size() > 2 ? ", " : " ");
	    for (const String* a = x[i].begin(); a < x[i].end() - 1; a++)
		sa << *a << sep;
	    sa << (x[i].size() > 1 ? "and " : "") << x[i].back()
	       << (x[i].size() > 1 ? " features" : " feature") << (msg_pct+1);
	    sa.append_break_lines(sa.take_string(), 61);
	    errh->warning("%s", sa.c_str());
	}
}

static String otf_filename;

static void
main_dvips_map(const String &ps_name, StringAccum &sa, ErrorHandler *errh)
{
    if (String fn = installed_type1(otf_filename, ps_name, (output_flags & G_TYPE1), errh))
	sa << "<" << pathname_filename(fn);
    else
	sa << "<<" << pathname_filename(otf_filename);
}

static void
do_gsub(Metrics& metrics, const OpenType::Font& otf, DvipsEncoding& dvipsenc, bool dvipsenc_literal, HashMap<uint32_t, int>& feature_usage, const Vector<PermString>& glyph_names, ErrorHandler* errh)
{
    // apply activated GSUB features
    OpenType::Gsub gsub(otf.table("GSUB"), errh);
    Vector<Lookup> lookups(gsub.nlookups(), Lookup());
    find_lookups(gsub.script_list(), gsub.feature_list(), lookups, errh);
    Vector<OpenType::Substitution> subs;
    for (int i = 0; i < lookups.size(); i++)
	if (lookups[i].used) {
	    OpenType::GsubLookup l = gsub.lookup(i);
	    subs.clear();
	    bool understood = l.unparse_automatics(gsub, subs);

	    // check for -ffina, which should apply only at the ends of words,
	    // and -finit, which should apply only at the beginnings.
	    OpenType::Tag feature = (lookups[i].features.size() == 1 ? lookups[i].features[0] : OpenType::Tag());
	    if (feature == OpenType::Tag("fina") || feature == OpenType::Tag("fin2") || feature == OpenType::Tag("fin3")) {
		if (dvipsenc.boundary_char() < 0)
		    errh->warning("'-ffina' requires a boundary character\n(The input encoding didn't specify a boundary character, but\nI need one to implement '-ffina' features correctly.  Try\nthe '--boundary-char' option.)");
		else {
		    int bg = metrics.boundary_glyph();
		    for (int j = 0; j < subs.size(); j++)
			subs[j].add_outer_right(bg);
		}
	    } else if (feature == OpenType::Tag("init")) {
		int bg = metrics.boundary_glyph();
		for (int j = 0; j < subs.size(); j++)
		    subs[j].add_outer_left(bg);
	    }

	    //for (int subno = 0; subno < subs.size(); subno++) fprintf(stderr, "%5d\t%s\n", i, subs[subno].unparse().c_str());

	    // figure out which glyph filter to use
	    int nunderstood = metrics.apply(subs, !dvipsenc_literal, i, *lookups[i].filter + alternate_filter, glyph_names);

	    // mark as used
	    int d = (understood && nunderstood == subs.size() ? F_GSUB_ALL : (nunderstood ? F_GSUB_PART : 0)) + F_GSUB_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}

    // apply 'aalt' feature if we have variant selectors
    if (metrics.altselectors() && !dvipsenc_literal) {
	// apply default features
	if (!altselector_features.size()) {
	    altselector_features.push_back(OpenType::Tag("dlig"));
	    altselector_features.push_back(OpenType::Tag("salt"));
	}
	// do lookups
	altselector_features.swap(interesting_features);
	Vector<Lookup> alt_lookups(gsub.nlookups(), Lookup());
	find_lookups(gsub.script_list(), gsub.feature_list(), alt_lookups, ErrorHandler::silent_handler());
	Vector<OpenType::Substitution> alt_subs;
	for (int i = 0; i < alt_lookups.size(); i++)
	    if (alt_lookups[i].used) {
		OpenType::GsubLookup l = gsub.lookup(i);
		alt_subs.clear();
		(void) l.unparse_automatics(gsub, alt_subs);
		metrics.apply_alternates(alt_subs, i, alternate_filter, glyph_names);
	    }
	altselector_features.swap(interesting_features);
    }
}

static void
do_gpos(Metrics& metrics, const OpenType::Font& otf, HashMap<uint32_t, int>& feature_usage, ErrorHandler* errh)
{
    OpenType::Gpos gpos(otf.table("GPOS"), errh);
    Vector<Lookup> lookups(gpos.nlookups(), Lookup());
    find_lookups(gpos.script_list(), gpos.feature_list(), lookups, errh);
    Vector<OpenType::Positioning> poss;
    for (int i = 0; i < lookups.size(); i++)
	if (lookups[i].used) {
	    OpenType::GposLookup l = gpos.lookup(i);
	    poss.clear();
	    bool understood = l.unparse_automatics(poss);
	    int nunderstood = metrics.apply(poss);

	    // mark as used
	    int d = (understood && nunderstood == poss.size() ? F_GPOS_ALL : (nunderstood ? F_GPOS_PART : 0)) + F_GPOS_TRY;
	    for (int j = 0; j < lookups[i].features.size(); j++)
		feature_usage.find_force(lookups[i].features[j].value()) |= d;
	}
}

static void
do_file(const String &otf_filename, const OpenType::Font &otf,
	const DvipsEncoding &dvipsenc_in, bool dvipsenc_literal,
	ErrorHandler *errh)
{
    // get font
    Cff cff(otf.table("CFF"), errh);
    if (!cff.ok())
	return;

    Cff::FontParent *fp = cff.font(PermString(), errh);
    if (!fp || !fp->ok())
	return;
    Cff::Font *font = dynamic_cast<Cff::Font *>(fp);
    if (!font) {
	errh->error("CID-keyed fonts not supported");
	return;
    }

    // save glyph names
    Vector<PermString> glyph_names;
    font->glyph_names(glyph_names);
    OpenType::debug_glyph_names = glyph_names;

    // set typeface name from font family name
    {
	String typeface = font->dict_string(Cff::oFamilyName);

	// make it reasonable for the shell
	StringAccum sa;
	for (int i = 0; i < typeface.length(); i++)
	    if (isalnum(typeface[i]) || typeface[i] == '_' || typeface[i] == '-' || typeface[i] == '.' || typeface[i] == ',' || typeface[i] == '+')
		sa << typeface[i];

	set_typeface(sa.length() ? sa.take_string() : font_name, false);
    }

    // initialize encoding
    DvipsEncoding dvipsenc(dvipsenc_in); // make copy
    Metrics metrics(font, font->nglyphs());
    OpenType::Cmap cmap(otf.table("cmap"), errh);
    assert(cmap.ok());
    if (dvipsenc_literal)
	dvipsenc.make_metrics(metrics, cmap, font, 0, true, errh);
    else {
	T1Secondary secondary(font, cmap);
	secondary.set_font_information(font_name, otf, otf_filename);
	dvipsenc.make_metrics(metrics, cmap, font, &secondary, false, errh);
    }
    // encode boundary glyph at 256; pretend its Unicode value is '\n'
    metrics.encode(256, '\n', metrics.boundary_glyph());

    // maintain statistics about features
    HashMap<uint32_t, int> feature_usage(0);

    // apply activated GSUB features
    try {
	do_gsub(metrics, otf, dvipsenc, dvipsenc_literal, feature_usage, glyph_names, errh);
    } catch (OpenType::BlankTable) {
	// nada
    } catch (OpenType::Error e) {
	errh->warning("GSUB '%s' error, continuing", e.description.c_str());
    }
    
    // apply LIGKERN ligature commands to the result
    dvipsenc.apply_ligkern_lig(metrics, errh);

    // test fake ligature mechanism
    //metrics.add_threeligature('T', 'h', 'e', '0');
    
    // reencode characters to fit within 8 bytes (+ 1 for the boundary)
    if (!dvipsenc_literal)
	metrics.shrink_encoding(257, dvipsenc_in, errh);
    
    // apply activated GPOS features
    try {
	do_gpos(metrics, otf, feature_usage, errh);
    } catch (OpenType::BlankTable) {
	// nada
    } catch (OpenType::Error e) {
	errh->warning("GPOS '%s' error, continuing", e.description.c_str());
    }

    // apply LIGKERN kerning commands to the result
    dvipsenc.apply_ligkern_kern(metrics, errh);

    // remove extra characters
    metrics.cut_encoding(257);
    //metrics.unparse();

    // apply letterspacing, if any
    if (letterspace) {
	for (int code = 0; code < 256; code++) {
	    int g = metrics.glyph(code);
	    if (g != 0 && g != Metrics::VIRTUAL_GLYPH && code != dvipsenc.boundary_char()) {
		metrics.add_single_positioning(code, letterspace / 2, 0, letterspace);
		metrics.add_kern(code, 256, -letterspace / 2);
		metrics.add_kern(256, code, -letterspace / 2);
	    }
	}
    }

    // reencode right components of boundary_glyph as boundary_char
    int boundary_char = dvipsenc.boundary_char();
    if (metrics.reencode_right_ligkern(256, boundary_char) > 0
	&& boundary_char < 0) {
	errh->warning("no boundary character, ignoring some ligatures and/or kerns\n");
	errh->message("(You may want to try the --boundary-char option.)");
    }

    // report unused and underused features if any
    report_underused_features(feature_usage, errh);

    // figure out our FONTNAME
    if (!font_name) {
	// derive font name from OpenType font name
	font_name = font->font_name();
	if (encoding_file) {
	    int slash = encoding_file.find_right('/') + 1;
	    int dot = encoding_file.find_right('.');
	    if (dot < slash)	// includes dot < 0 case
		dot = encoding_file.length();
	    font_name += String("--") + encoding_file.substring(slash, dot - slash);
	}
	if (interesting_scripts.size() != 2 || interesting_scripts[0] != OpenType::Tag("latn") || interesting_scripts[1].valid())
	    for (int i = 0; i < interesting_scripts.size(); i += 2) {
		font_name += String("--S") + interesting_scripts[i].text();
		if (interesting_scripts[i+1].valid())
		    font_name += String(".") + interesting_scripts[i+1].text();
	    }
	for (int i = 0; i < interesting_features.size(); i++)
	    if (feature_usage[interesting_features[i].value()])
		font_name += String("--F") + interesting_features[i].text();
    }
    
    // output encoding
    if (dvipsenc_literal) {
	out_encoding_name = dvipsenc_in.name();
	out_encoding_file = dvipsenc_in.filename();
    } else
	output_encoding(metrics, glyph_names, errh);

    // set up coding scheme
    if (metrics.coding_scheme())
	metrics.set_design_units(1);
    else
	metrics.set_coding_scheme(out_encoding_name);

    // output
    ::otf_filename = otf_filename;
    output_metrics(metrics, font->font_name(), dvipsenc.boundary_char(),
		   otf, font,
		   out_encoding_name, out_encoding_file,
		   font_name, main_dvips_map, errh);
    
    // done
    delete font;
}


extern "C" {
static int
clp_parse_char(Clp_Parser *clp, const char *arg, int complain, void *)
{
    if (arg[0] && !arg[1] && !isdigit(arg[0])) {
	clp->val.i = (unsigned char)arg[0];
	return 1;
    } else if (arg[0] == '-' || isdigit(arg[0])) {
	char *end;
	clp->val.i = strtol(arg, &end, 10);
	if (clp->val.i <= 255 && !*end)
	    return 1;
    }
    if (complain)
	Clp_OptionError(clp, "'%O' expects a character, not '%s'", arg);
    return 0;
}
}

int
main(int argc, char *argv[])
{
    handle_sigchld();
    String::static_initialize();
    Clp_Parser *clp =
	Clp_NewParser(argc, (const char * const *)argv, sizeof(options) / sizeof(options[0]), options);
    Clp_AddType(clp, CHAR_OPTTYPE, 0, clp_parse_char, 0);
    program_name = Clp_ProgramName(clp);
#if HAVE_KPATHSEA
    kpsei_init(argv[0]);
#endif
#ifdef HAVE_CTIME
    {
	time_t t = time(0);
	char *c = ctime(&t);
	current_time = " on " + String(c).substring(0, -1); // get rid of \n
    }
#endif
    for (int i = 0; i < argc; i++)
	invocation << (i ? " " : "") << argv[i];
    
    ErrorHandler *errh = ErrorHandler::static_initialize(new FileErrorHandler(stderr, String(program_name) + ": "));
    const char *input_file = 0;
    const char *glyphlist_file = SHAREDIR "/glyphlist.txt";
    bool literal_encoding = false;
    Vector<String> ligkern;
    Vector<String> unicoding;
    bool no_ecommand = false, default_ligkern = false;
    String codingscheme;
    
    while (1) {
	int opt = Clp_Next(clp);
	switch (opt) {

	  case SCRIPT_OPT: {
	      String arg = clp->arg;
	      int period = arg.find_left('.');
	      OpenType::Tag scr(period <= 0 ? arg : arg.substring(0, period));
	      if (scr.valid() && period > 0) {
		  OpenType::Tag lang(arg.substring(period + 1));
		  if (lang.valid()) {
		      interesting_scripts.push_back(scr);
		      interesting_scripts.push_back(lang);
		  } else
		      usage_error(errh, "bad language tag");
	      } else if (scr.valid()) {
		  interesting_scripts.push_back(scr);
		  interesting_scripts.push_back(OpenType::Tag());
	      } else
		  usage_error(errh, "bad script tag");
	      break;
	  }

	  case FEATURE_OPT: {
	      OpenType::Tag t(clp->arg);
	      if (t.valid())
		  interesting_features.push_back(t);
	      else
		  usage_error(errh, "bad feature tag");
	      break;
	  }
      
	  case LETTER_FEATURE_OPT: {
	      OpenType::Tag t(clp->arg);
	      if (t.valid()) {
		  interesting_features.push_back(t);
		  GlyphFilter* gf = new GlyphFilter;
		  gf->add_substitution_filter("<Letter>", false, errh);
		  feature_filters.insert(t, gf); 
	      } else
		  usage_error(errh, "bad feature tag");
	      break;
	  }

	  case ENCODING_OPT:
	    if (encoding_file)
		usage_error(errh, "encoding specified twice");
	    encoding_file = clp->arg;
	    break;

	  case LITERAL_ENCODING_OPT:
	    if (encoding_file)
		usage_error(errh, "encoding specified twice");
	    encoding_file = clp->arg;
	    literal_encoding = true;
	    break;

	  case EXTEND_OPT:
	    if (extend)
		usage_error(errh, "extend value specified twice");
	    extend = clp->val.d;
	    break;

	  case SLANT_OPT:
	    if (slant)
		usage_error(errh, "slant value specified twice");
	    slant = clp->val.d;
	    break;

	  case LETTERSPACE_OPT:
	    if (letterspace)
		usage_error(errh, "letterspacing value specified twice");
	    letterspace = clp->val.i;
	    break;

	  case DESIGN_SIZE_OPT:
	    if (design_size > 0)
		usage_error(errh, "design size value specified twice");
	    else if (clp->val.d <= 0)
		usage_error(errh, "design size must be > 0");
	    design_size = clp->val.d;
	    break;

	  case LIGKERN_OPT:
	    ligkern.push_back(clp->arg);
	    break;

	  case NO_ECOMMAND_OPT:
	    no_ecommand = true;
	    break;

	  case DEFAULT_LIGKERN_OPT:
	    default_ligkern = true;
	    break;

	  case BOUNDARY_CHAR_OPT:
	    ligkern.push_back(String("|| = ") + String(clp->val.i));
	    break;

	  case ALTSELECTOR_CHAR_OPT:
	    ligkern.push_back(String("^^ = ") + String(clp->val.i));
	    break;

	  case ALTSELECTOR_FEATURE_OPT: {
	      OpenType::Tag t(clp->arg);
	      if (t.valid())
		  altselector_features.push_back(t);
	      else
		  usage_error(errh, "bad feature tag");
	      break;
	  }

	  case EXCLUDE_ALTERNATES_OPT:
	  case INCLUDE_ALTERNATES_OPT: {
	      const char *s = clp->arg;
	      while (*s) {
		  const char *start = s;
		  while (*s && !isspace(*s))
		      s++;
		  if (s > start) {
		      String str(start, s - start);
		      alternate_filter.add_alternate_filter(str, opt == EXCLUDE_ALTERNATES_OPT, errh);
		  }
		  while (isspace(*s))
		      s++;
	      }
	      break;
	  }
	    
	  case UNICODING_OPT:
	    unicoding.push_back(clp->arg);
	    break;
	    
	  case CODINGSCHEME_OPT:
	    if (codingscheme)
		usage_error(errh, "coding scheme specified twice");
	    codingscheme = clp->arg;
	    if (codingscheme.length() > 39)
		errh->warning("only first 39 characters of coding scheme are significant");
	    if (codingscheme.find_left('(') >= 0 || codingscheme.find_left(')') >= 0)
		usage_error(errh, "coding scheme cannot contain parentheses");
	    break;

	  case AUTOMATIC_OPT:
	    automatic = !clp->negated;
	    break;

	  case VENDOR_OPT:
	    if (!set_vendor(clp->arg))
		usage_error(errh, "vendor name specified twice");
	    break;

	  case TYPEFACE_OPT:
	    if (!set_typeface(clp->arg, true))
		usage_error(errh, "typeface name specified twice");
	    break;

	  case VIRTUAL_OPT:
	    if (clp->negated)
		output_flags &= ~G_VMETRICS;
	    else
		output_flags |= G_VMETRICS;
	    break;

	  case NO_ENCODING_OPT:
	  case NO_TYPE1_OPT:
	  case NO_DOTLESSJ_OPT:
	    output_flags &= ~(opt - NO_OUTPUT_OPTS);
	    break;

	  case MINIMUM_KERN_OPT:
	    minimum_kern = clp->val.d;
	    break;
	    
	  case MAP_FILE_OPT:
	    if (clp->negated)
		output_flags &= ~G_PSFONTSMAP;
	    else {
		output_flags |= G_PSFONTSMAP;
		if (!set_map_file(clp->arg))
		    usage_error(errh, "map file specified twice");
	    }
	    break;
	    
	  case PL_OPT:
	    output_flags = (output_flags & ~G_BINARY) | G_ASCII;
	    break;

	  case TFM_OPT:
	    output_flags = (output_flags & ~G_ASCII) | G_BINARY;
	    break;
	    
	  case ENCODING_DIR_OPT:
	  case TFM_DIR_OPT:
	  case PL_DIR_OPT:
	  case VF_DIR_OPT:
	  case VPL_DIR_OPT:
	  case TYPE1_DIR_OPT:
	    if (!setodir(opt - DIR_OPTS, clp->arg))
		usage_error(errh, "%s directory specified twice", odirname(opt - DIR_OPTS));
	    break;
	    
	  case FONT_NAME_OPT:
	  font_name:
	    if (font_name)
		usage_error(errh, "font name specified twice");
	    font_name = clp->arg;
	    break;

	  case GLYPHLIST_OPT:
	    glyphlist_file = clp->arg;
	    break;
	    
	  case QUERY_FEATURES_OPT:
	    usage_error(errh, "run 'otfinfo --query-features' instead");
	    break;

	  case QUERY_SCRIPTS_OPT:
	    usage_error(errh, "run 'otfinfo --query-scripts' instead");
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

	  case NOCREATE_OPT:
	    no_create = clp->negated;
	    break;

	  case FORCE_OPT:
	    force = !clp->negated;
	    break;

	  case KPATHSEA_DEBUG_OPT:
#if HAVE_KPATHSEA
	    kpsei_set_debug_flags(clp->val.u);
#else
	    errh->warning("Not compiled with kpathsea!");
#endif
	    break;

	  case VERSION_OPT:
	    printf("otftotfm (LCDF typetools) %s\n", VERSION);
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

	  case Clp_NotOption:
	    if (input_file && font_name)
		usage_error(errh, "too many arguments");
	    else if (input_file)
		goto font_name;
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
    try {
	if (!input_file)
	    usage_error(errh, "no font filename provided");
	if (!encoding_file) {
	    errh->warning("no encoding provided");
	    errh->message("(Use '-e ENCODING' to choose an encoding. '-e texnansx' often works,\nor say '-e -' to turn off this warning.)");
	} else if (encoding_file == "-")
	    encoding_file = "";
    
	// read font
	String font_data = read_file(input_file, errh);
	if (errh->nerrors())
	    exit(1);

	LandmarkErrorHandler cerrh(errh, printable_filename(input_file));
	BailErrorHandler bail_errh(&cerrh);

	OpenType::Font otf(font_data, &bail_errh);
	assert(otf.ok());

	// figure out scripts we care about
	if (!interesting_scripts.size()) {
	    interesting_scripts.push_back(Efont::OpenType::Tag("latn"));
	    interesting_scripts.push_back(Efont::OpenType::Tag());
	}
	std::sort(interesting_features.begin(), interesting_features.end());

	// read glyphlist
	if (String s = read_file(glyphlist_file, errh, true))
	    DvipsEncoding::parse_glyphlist(s);

	// read encoding
	DvipsEncoding dvipsenc;
	if (encoding_file) {
	    if (String path = locate_encoding(encoding_file, errh))
		dvipsenc.parse(path, no_ecommand || default_ligkern, no_ecommand, errh);
	    else
		errh->fatal("encoding '%s' not found", encoding_file.c_str());
	} else {
	    // use encoding from font
	    Cff cff(otf.table("CFF"), &bail_errh);
	    Cff::FontParent *font = cff.font(PermString(), &bail_errh);
	    assert(cff.ok() && font->ok());
	    if (Type1Encoding *t1e = font->type1_encoding()) {
		for (int i = 0; i < 256; i++)
		    dvipsenc.encode(i, (*t1e)[i]);
	    } else
		errh->fatal("font has no encoding, specify one explicitly");
	    delete font;
	}

	// apply default ligkern commands
	if (default_ligkern || (!dvipsenc.file_had_ligkern() && !ligkern.size() && !no_ecommand))
	    dvipsenc.parse_ligkern(default_ligkerns, ErrorHandler::silent_handler());
    
	// apply command-line ligkern commands and coding scheme
	cerrh.set_landmark("--ligkern command");
	for (int i = 0; i < ligkern.size(); i++)
	    dvipsenc.parse_ligkern(ligkern[i], &cerrh);
	cerrh.set_landmark("--unicoding command");
	for (int i = 0; i < unicoding.size(); i++)
	    dvipsenc.parse_unicoding(unicoding[i], &cerrh);
	if (codingscheme)
	    dvipsenc.set_coding_scheme(codingscheme);

	do_file(input_file, otf, dvipsenc, literal_encoding, errh);
	
    } catch (OpenType::Error e) {
	errh->error("unhandled exception '%s'", e.description.c_str());
    }
    
    return (errh->nerrors() == 0 ? 0 : 1);
}
