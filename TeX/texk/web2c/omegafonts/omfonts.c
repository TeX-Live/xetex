/* omfonts.c: Main routine for ofm2opl, opl2ofm, ovf2ovp, ovp2ovf.

This file is part of Omega,
which is based on the web2c distribution of TeX,

Copyright (c) 1994--2001 John Plaice and Yannis Haralambous

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

#define OFM2OPL
#define OPL2OFM
#define OVP2OVF
#define OVF2OVP
#include "cpascal.h"
#include "parser.h"
#include "manifests.h"
#include "omfonts.h"
#include "list_routines.h"
#include "error_routines.h"
#include "header_routines.h"
#include "font_routines.h"
#include "param_routines.h"
#include "char_routines.h"
#include "dimen_routines.h"
#include "parse_ofm.h"

#define PROG_MIN		0
#define PROG_OFM2OPL	0
#define PROG_OVF2OVP	1
#define PROG_OPL2OFM	2
#define PROG_OVP2OVF	3
#define PROG_MAX		3

extern FILE *yyin;

#define cmdline(i) (argv[i])

int verbose_option;
int char_format = CHAR_CODE_NUM;
int num_format  = NUM_CODE_HEX;
int text_format = TEXT_CODE_MIXED;

int program;

string name_ofm = NULL;
string name_opl = NULL;
string name_ovp = NULL;
string name_ovf = NULL;

FILE *file_ofm = NULL;
FILE *file_opl = NULL;
FILE *file_ovp = NULL;
FILE *file_ovf = NULL;
FILE *file_output = NULL;

unsigned char *ofm = NULL;
unsigned char *ovf = NULL;

unsigned length_ofm = 0;
unsigned length_ovf = 0;

const_string name_program;
const_string *name_help;
string name_msg;

string name_empty = NULL;

int no_files=0;
string *files[3] = {NULL, NULL, NULL};
string suffixes[3] = {NULL, NULL, NULL};
string full_suffixes[3] = {NULL, NULL, NULL};

static struct option long_options[] = {
    {"verbose", 0, &verbose_option, 1},
    {"char-format", 1, 0, 0},
    {"num-format", 1, 0, 0},
    {"text-format", 1, 0, 0},
    {"help", 0, 0, 0},
    {"version", 0, 0, 0},
    {0, 0, 0, 0}
};

int
main (int argc, string *argv)
{
    int getopt_return_val;
    int option_index = 0;

    name_program = xbasename(argv[0]);
    if (!strcmp(name_program, "ofm2opl") ||
        !strcmp(name_program, "OFM2OPL.EXE")) {
        program = PROG_OFM2OPL;
        name_help = OFM2OPLHELP;
	name_program = "ofm2opl";
        name_msg = "This is ofm2opl, Version 2.0";
        no_files = 2;
        files[0] = &name_ofm;
        files[1] = &name_opl;
        suffixes[0] = "ofm";
        suffixes[1] = "opl";
        full_suffixes[0] = ".ofm";
        full_suffixes[1] = ".opl";
    } else if (!strcmp(name_program, "opl2ofm") ||
               !strcmp(name_program, "OPL2OFM.EXE")) {
        program = PROG_OPL2OFM;
        name_help = OPL2OFMHELP;
	name_program = "opl2ofm";
        name_msg = "This is opl2ofm, Version 2.0";
        no_files = 2;
        files[0] = &name_opl;
        files[1] = &name_ofm;
        suffixes[0] = "opl";
        suffixes[1] = "ofm";
        full_suffixes[0] = ".opl";
        full_suffixes[1] = ".ofm";
    } else if (!strcmp(name_program, "ovp2ovf") ||
               !strcmp(name_program, "OVP2OVF.EXE")) {
        program = PROG_OVP2OVF;
        name_help = OVP2OVFHELP;
	name_program = "ovp2ovf";
        name_msg = "This is ovp2ovf, Version 2.0";
        no_files = 3;
        files[0] = &name_ovp;
        files[1] = &name_ovf;
        files[2] = &name_ofm;
        suffixes[0] = "ovp";
        suffixes[1] = "ovf";
        suffixes[2] = "ofm";
        full_suffixes[0] = ".ovp";
        full_suffixes[1] = ".ovf";
        full_suffixes[2] = ".ofm";
    } else if (!strcmp(name_program, "ovf2ovp") ||
               !strcmp(name_program, "OVF2OVP.EXE")) {
        program = PROG_OVF2OVP;
        name_help = OVF2OVPHELP;
	name_program = "ovf2ovp";
        name_msg = "This is ovf2ovp, Version 2.0";
        no_files = 3;
        files[0] = &name_ovf;
        files[1] = &name_ofm;
        files[2] = &name_ovp;
        suffixes[0] = "ovf";
        suffixes[1] = "ofm";
        suffixes[2] = "ovp";
        full_suffixes[0] = ".ovf";
        full_suffixes[1] = ".ofm";
        full_suffixes[2] = ".ovp";
    } else {
        fprintf(stderr , "Unrecognized program: %s\n", name_program);
        fprintf(stderr ,
        "This binary supports ofm2opl, opl2ofm, ovf2ovp, and ovp2ovf\n");
        exit(1);
    }
    kpse_set_program_name(name_program, NULL);
    kpse_init_prog(uppercasify(name_program), 0, nil, nil);

    do {
        getopt_return_val =
        getopt_long_only(argc, argv, "", long_options, &option_index) ;
        if (getopt_return_val == -1) { ; }
        else if ( getopt_return_val == 63 ) {
            usage (name_program);
        } else if (!strcmp(long_options[option_index].name, "help")) {
            usagehelp (name_help, NULL);
        } else if (!strcmp(long_options[option_index ].name, "version")) {
            printversionandexit(name_msg, nil,
                "J. Plaice, Y. Haralambous, D.E. Knuth");
        } else if (!strcmp(long_options[option_index ].name, "char-format")) {
            if (!strcmp(optarg, "ascii")) char_format = CHAR_CODE_ASCII;
            else if (!strcmp(optarg, "num")) char_format = CHAR_CODE_NUM;
            else warning_s("Bad character code format (%s)", optarg);
        } else if (!strcmp(long_options[option_index ].name, "num-format")) {
            if (!strcmp(optarg, "hex")) num_format = NUM_CODE_HEX;
            else if (!strcmp(optarg, "octal")) num_format = NUM_CODE_OCTAL;
            else warning_s("Bad number code format (%s)", optarg);
        } else if (!strcmp(long_options[option_index ].name, "text-format")) {
            if (!strcmp(optarg, "upper")) text_format = TEXT_CODE_UPPER;
            else if (!strcmp(optarg, "mixed")) text_format = TEXT_CODE_MIXED;
            else warning_s("Bad text code format (%s)", optarg);
        }
    } while (getopt_return_val != -1);
    if (((argc-optind) > no_files) || ((argc-optind) < 1)) {
        fprintf(stderr , "%s: %s\n", name_program,
                no_files == 2 ? "Need one or two file arguments."
                              : "Need one to three file arguments.");
        usage (name_program);
    }
    *(files[0]) = extend_filename(cmdline(optind) , suffixes[0]);
    if (optind+2 <= argc) {
        *(files[1]) = extend_filename(cmdline(optind+1) , suffixes[1]);
        if (no_files == 3) {
            if (optind+3 <= argc) {
                *(files[2]) = extend_filename(cmdline(optind+2) , suffixes[2]);
            } else if (program == PROG_OVP2OVF) {
                *(files[2]) = extend_filename(cmdline(optind+1), suffixes[2]);
            }
        }
    } else if (program != PROG_OFM2OPL) {
        *(files[1]) = basenamechangesuffix(*(files[0]),
                      full_suffixes[0], full_suffixes[1]);
        if ((no_files == 3) && (program == PROG_OVP2OVF)) {
            *(files[2]) = basenamechangesuffix(*(files[0]),
                      full_suffixes[0], full_suffixes[2]);
        }
    }

    switch(program) {
        case PROG_OFM2OPL: {
            file_ofm = kpse_open_file(name_ofm, kpse_ofm_format);
            read_in_whole(&ofm, &length_ofm, file_ofm, name_ofm);
            (void)fclose(file_ofm);
            if (name_opl==NULL) file_opl = stdout;
            else rewrite(file_opl, name_opl);
            file_output = file_opl;
            parse_ofm(FALSE);
            break;
        }
        case PROG_OVF2OVP: {
            file_ovf = kpse_open_file(name_ovf, kpse_ovf_format);
            read_in_whole(&ovf, &length_ovf, file_ovf, name_ovf);
            (void)fclose(file_ovf);
            file_ofm = kpse_open_file(name_ofm, kpse_ofm_format);
            read_in_whole(&ofm, &length_ofm, file_ofm, name_ofm);
            (void)fclose(file_ofm);
            if (name_ovp==NULL) file_ovp = stdout;
            else rewrite(file_ovp, name_ovp);
            file_output = file_ovp;
            parse_ofm(TRUE);
            break;
        }
        case PROG_OPL2OFM: {
            file_opl = kpse_open_file(name_opl, kpse_opl_format);
            rewritebin(file_ofm, name_ofm);
            init_tables();
            yyin = file_opl;
            (void)yyparse();
            output_ofm_file();
            (void)fclose(file_ofm);
            break;
        }
        case PROG_OVP2OVF: {
            file_ovp = kpse_open_file(name_ovp, kpse_ovp_format);
            rewritebin(file_ovf, name_ovf);
            rewritebin(file_ofm, name_ofm);
            init_tables();
            yyin = file_ovp;
            (void)yyparse();
            output_ofm_file();
            /*(void)fclose(file_ofm);*/
            output_ovf_file();
            (void)fclose(file_ovf);
            break;
        }
        default: {exit(1);}
    }
    exit(0);
}

#define BIG_BLOCK 0x20000
#define LITTLE_BLOCK 0x1000

void
read_in_whole(unsigned char **contents_loc,
              unsigned *length_loc,
              FILE *file,
              string name)
{
    unsigned no_read;
    unsigned no_total_read = 0;
    unsigned size = BIG_BLOCK;
    string where;
    string current_block;

    current_block = (char *) xmalloc(size);
    where = current_block;

    while (1) {
        no_read = fread(where, 1, LITTLE_BLOCK, file);
        no_total_read += no_read;
        if ((no_total_read+LITTLE_BLOCK) > size) {
            size *= 2;
            current_block = (char *) xrealloc(current_block, size);
        }
        where = current_block + no_total_read;
        if (ferror(file)) {
            fatal_error_s("Error while reading file %s", name);
        } else if (feof(file)) {
            break;
        }
    }

    *contents_loc = (unsigned char *) current_block;
    *length_loc = no_total_read;
}

void
init_tables(void)
{
    font_table_init(); /* subsidiary fonts in virtual fonts */
    init_header();
    init_planes();
    init_measures();
}

void
output_text_file(FILE *dest)
{
}

