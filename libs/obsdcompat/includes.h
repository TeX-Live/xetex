/*	$OpenBSD: includes.h,v 1.22 2006/01/01 08:59:27 stevesk Exp $	*/

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * This file includes most of the needed system headers.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * ---------------------------------------------------------------------------
 *
 * Modified (butchered) 2006 by Martin Schröder for pdfTeX
 *
 */

#ifndef INCLUDES_H
#define INCLUDES_H

#define RCSID(msg) \
static /**/const char *const rcsid[] = { (const char *)rcsid, "\100(#)" msg }

#include "obsdcompat/config.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#ifdef __MINGW32__
/* In mingw32, the eof() function is part of the !_NO_OLDNAMES section
   of <io.h>, that is read in automatically via <fcntl.h>. We cannot
   allow that because web2c/lib/eofeoln.c defines a private,
   incompatible function named eof().
   But many of the other things defined via !_NO_OLDNAMES are needed,
   so #define _NO_OLDNAMES cannot be used. So, temporarily define eof
   as a macro.
*/
#define eof saved_eof
#include <fcntl.h> /* For O_NONBLOCK */
#undef eof
#else
#include <fcntl.h> /* For O_NONBLOCK */
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef HAVE_LIMITS_H
# include <limits.h> /* For PATH_MAX */
#endif
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif

/*
 *-*-nto-qnx needs these headers for strcasecmp and LASTLOG_FILE respectively
 */
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <sys/types.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_SYS_BITYPES_H
# include <sys/bitypes.h> /* For u_intXX_t */
#endif
#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h> /* For __P() */
#endif
#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h> /* For MIN, MAX, etc */
#endif

#include "defines.h"

#endif /* INCLUDES_H */
