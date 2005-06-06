/*
 *  ChkTeX v1.5, operating system specific code for ChkTeX.
 *  Copyright (C) 1995-96 Jens T. Berger Thielemann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact the author at:
 *		Jens Berger
 *		Spektrumvn. 4
 *		N-0666 Oslo
 *		Norway
 *		E-mail: <jensthi@ifi.uio.no>
 *
 *
 */

#ifndef OPSYS_H
#define OPSYS_H
#ifndef CHKTEX_H
# include "ChkTeX.h"
#endif /* CHKTEX_H */
#ifndef UTILITY_H
# include "Utility.h"
#endif /* UTILITY_H */

/********************************************************************/
/**************** START OF USER SETTABLE PREFERENCES ****************/

/*
 * Note: This file contains most defines you'll wish to change if you
 * wish to adopt ChkTeX to a new system. It is, as you might notice,
 * heavily documented. If you wish to get into the internals of ChkTeX,
 * the interesting stuff is at the bottom of this file, and in the .c
 * files. However, you should also take a look at the "config.h.in" file
 * in this directory if you haven't got a shell able to run the "configure"
 * script.
 *
 * This program relies heavily on that the system which
 * automagically free()'s all malloc()'ed memory, works. The program
 * itself does not call free() very much. This is because we're doing
 * lots of tiny allocations, and a properly designed pooling system will
 * hopefully do a quicker job than we'll be able to do. So there.
 *
 * To keep things simple, we trust that the fclose()'ing of fopen()'ed
 * also happens automagically.
 *
 * Please use the getopt included, as we will modify optarg during
 * command processing.
 *
 * You may wish to modify the SetupVars() (OpSys.c) to better suit your
 * preferences. In any case, it should put the filename (and full path)
 * of the `.chktexrc' file into the ConfigFile array. The array is sized
 * BUFSIZ bytes.
 *
 * The program does also assume that AMIGA is defined if the source
 * compiled on an Amiga machine, and that __unix__ is defined if the
 * source is compiled on a UNIX machine.
 *
 */


/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

/*
 * Here you should define what codes which should be returned to the
 * shell upon success/failure.
 *
 */

#ifndef EXIT_FAILURE
#  ifdef  AMIGA
#    define  EXIT_FAILURE    20
#  else
#    define  EXIT_FAILURE    1
#  endif
#endif

#ifndef EXIT_SUCCESS
#  define  EXIT_SUCCESS    0
#endif

/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

/*
 * SLASH should be defined to the character your computer uses to
 * separate files/directories. Most systems use '/', messydos uses
 * '\'.
 *
 * DIRCHARS should be defined to the characters a directory entry
 * may end on. On Amigas, this is ":/" (either "FOO:BAR/" or "FOO:"),
 * Unix uses only "/", while messydos uses ":\\".
 *
 * This data will be used to automatically concatenate a directory
 * path and a filename.
 *
 * Adjust both to suit your needs.
 */


#if defined(__unix__) || defined(AMIGA) || defined(WIN32)
#  define  SLASH  '/'
#elif defined(__MSDOS__)
#  define SLASH   '\\'
#endif

#if defined(AMIGA)
#  define DIRCHARS ":/"
#elif defined(__unix__)
#  define DIRCHARS "/"
#elif defined(__MSDOS__)
#  define DIRCHARS ":\\"
#elif defined(WIN32)
#define DIRCHARS ":\\/"
#endif

/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

/*
 * Here, define what key-combination which is used to abort stdin
 * keyboard input. It should be a string, as we we'll type it out as
 * information to the user.
 */

#ifdef AMIGA
#  define STDIN_BREAK "Ctrl-\\"
#elif defined(__unix__)
#  define STDIN_BREAK "Ctrl-D"
#elif defined(__MSDOS__)
#  define STDIN_BREAK "Ctrl-Z + Enter"
#else
#  define STDIN_BREAK "stdin break combination"
#endif

/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

/*
 * For fancy printing of commands, we'll use these strings to turn
 * on/off the error indication. The codes listed here are ANSI
 * compatible; if you don't have that type of terminal, you may wish
 * to adjust this. Use "chktex -v2 Test.tex" to check the effects of
 * these macros. Note: These strings will be printf()'ed, so watch your
 * %'s.
 *
 * Under UNIX, we'll ignore these values and use termcap instead, where
 * that is installed.
 *
 * If these strings can't be specified statically, you'll have to add
 * code in the SetupTerm() function.
 *
 * PRE_ERROR_STR is of course printed in front of each location we
 * wish to show as an error, and POST_ERROR_STR after each location.
 *
 * The codes #defined here, will switch back- and foreground colours.
 * We're using '\033[' as escape character, some terminals may like
 * '\233' better.
 *
 */

#ifdef AMIGA
#  define PRE_ERROR_STR   "\033[43m\033[32m"
#  define POST_ERROR_STR  "\033[0m"
#else
#  define PRE_ERROR_STR   "\033[7m"
#  define POST_ERROR_STR  "\033[0m"
#endif


/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

/*
 * This macro should contain the appendix for backup files, which
 * will be appended onto the original filename. It should contain
 * a leading dot.
 */

#ifdef __MSDOS__
#  define BAKAPPENDIX ".$cl"
#else
#  define BAKAPPENDIX ".bak"
#endif

/***************** END OF USER SETTABLE PREFERENCES *****************/
/********************************************************************/

#ifndef WORDLIST_DEFINED
struct WordList;
#endif
         /* Sorry; there are now cyclic dependencies in the
		  * source tree. :-/ 
		  */

extern STRPTR ReverseOn, ReverseOff;
extern TEXT ConfigFile[BUFSIZ];

STRPTR  MatchFileName   __PROTO((STRPTR String));
BOOL    SetupVars       __PROTO((void));
void    SetupTerm       __PROTO((void));
void    AddAppendix     __PROTO((STRPTR Name, const STRPTR App));
void    tackon          __PROTO((STRPTR, const STRPTR));
BOOL    LocateFile      __PROTO((const STRPTR Filename, STRPTR Dest,
                                 const STRPTR App, struct WordList *wl));


#endif /* OPSYS_H */

