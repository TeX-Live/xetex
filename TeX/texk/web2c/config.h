/* config.h: All .c files include this first.

Copyright (C) 1995, 96 Karl Berry.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef WEB2C_CONFIG_H
#define WEB2C_CONFIG_H

#if defined (TEX_DLL) && (defined (WIN32) || defined (__CYGWIN__))
#ifdef MAKE_TEX_DLL
#define TEXDLL __declspec (dllexport)
#else /* ! MAKE_TEX_DLL */
#define TEXDLL __declspec (dllimport)
#endif
#else /* ! (TEX_DLL && (WIN32 || __CYGWIN__)) */
#define TEXDLL
#endif

/* The stuff from the path searching library.  */
#include <kpathsea/config.h>
#include <web2c/c-auto.h>

#include <kpathsea/c-vararg.h>

/* How to open a binary file.  */
#include <kpathsea/c-fopen.h>

/* The smallest signed type: use `signed char' if ANSI C, `short' if
   char is unsigned, otherwise `char'.  */
#ifndef SCHAR_TYPE
#if __STDC__
#define SCHAR_TYPE signed char
#else /* not __STDC */
#ifdef __CHAR_UNSIGNED__
#define SCHAR_TYPE short
#else
#define SCHAR_TYPE char
#endif
#endif /* not __STDC__ */
#endif /* not SCHAR_TYPE */
typedef SCHAR_TYPE schar;

/* The type `integer' must be a signed integer capable of holding at
   least the range of numbers (-2^31)..(2^31-1).  If your compiler goes
   to great lengths to make programs fail, you might have to change this
   definition.  If this changes, you may have to modify
   web2c/fixwrites.c, since it generates code to do integer output using
   "%ld", and casts all integral values to be printed to `long'.
   
   If you define your own INTEGER_TYPE, you have to define your own
   INTEGER_MAX and INTEGER_MIN, too. */
#ifndef INTEGER_TYPE

#if SIZEOF_LONG > 4 && !defined (NO_DUMP_SHARE)
/* If we have 64-bit longs and want to share format files (with 32-bit
   machines), use `int'.  */
#define INTEGER_IS_INT
#endif

#ifdef INTEGER_IS_INT
#define INTEGER_TYPE int
#define INTEGER_MAX INT_MAX
#define INTEGER_MIN INT_MIN
#else
#define INTEGER_TYPE long
#define INTEGER_MAX LONG_MAX
#define INTEGER_MIN LONG_MIN
#endif /* not INTEGER_IS_INT */

#endif /* not INTEGER_TYPE */

typedef INTEGER_TYPE integer;

/* I don't want to write a configure test for remove when all Unix
   machines have unlink.  But, for the sake of non-Unix machines that
   support ANSI C... */
#if !defined (unix) && !defined (__unix__) && defined (__STDC__) && !defined (unlink)
#define unlink remove
#endif

/* Window support on the Amiga is just for the Amiga.  */
#ifdef AMIGA
#define AMIGAWIN
#endif

/* Window support for WIN32 machines. */
#ifdef WIN32
#define WIN32WIN
#endif

/* strtol.c */
#ifndef HAVE_STRTOL
extern long strtol P3H(const char *, char **, int);
#endif

/* From uexit.c.  This is here because the lib/ and web2c/ routines
   themselves can use it, but they don't need cpascal.h.  */
extern void uexit P1H(int status);

/* usage.c */
extern void usage P1H(const_string progname);
extern void usagehelp P2H(const_string *message, const_string bug_email);

#endif /* not CONFIG_H */
