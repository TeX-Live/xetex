/*
******************************************************************************
*
*   Copyright (C) 1997-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : putil.c (previously putil.cpp and ptypes.cpp)
*
*   Date        Name        Description
*   04/14/97    aliu        Creation.
*   04/24/97    aliu        Added getDefaultDataDirectory() and
*                            getDefaultLocaleID().
*   04/28/97    aliu        Rewritten to assume Unix and apply general methods
*                            for assumed case.  Non-UNIX platforms must be
*                            special-cased.  Rewrote numeric methods dealing
*                            with NaN and Infinity to be platform independent
*                             over all IEEE 754 platforms.
*   05/13/97    aliu        Restored sign of timezone
*                            (semantics are hours West of GMT)
*   06/16/98    erm         Added IEEE_754 stuff, cleaned up isInfinite, isNan,
*                             nextDouble..
*   07/22/98    stephen     Added remainder, max, min, trunc
*   08/13/98    stephen     Added isNegativeInfinity, isPositiveInfinity
*   08/24/98    stephen     Added longBitsFromDouble
*   09/08/98    stephen     Minor changes for Mac Port
*   03/02/99    stephen     Removed openFile().  Added AS400 support.
*                            Fixed EBCDIC tables
*   04/15/99    stephen     Converted to C.
*   06/28/99    stephen     Removed mutex locking in u_isBigEndian().
*   08/04/99    jeffrey R.  Added OS/2 changes
*   11/15/99    helena      Integrated S/390 IEEE support.
*   04/26/01    Barry N.    OS/400 support for uprv_getDefaultLocaleID
*   08/15/01    Steven H.   OS/400 support for uprv_getDefaultCodepage
******************************************************************************
*/

#ifndef PTX

/* Define _XOPEN_SOURCE for Solaris and friends. */
/* NetBSD needs it to be >= 4 */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 4
#endif

/* Define __USE_POSIX and __USE_XOPEN for Linux and glibc. */
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#endif /* PTX */

/* include ICU headers */
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "putilimp.h"
#include "uassert.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "locmap.h"
#include "ucln_cmn.h"

/* Include standard headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <float.h>
#include <time.h>

/* include system headers */
#ifdef U_WINDOWS
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#elif defined(U_CYGWIN) && defined(__STRICT_ANSI__)
/* tzset isn't defined in strict ANSI on Cygwin. */
#   undef __STRICT_ANSI__
#elif defined(OS400)
#   include <float.h>
#   include <qusec.h>       /* error code structure */
#   include <qusrjobi.h>
#   include <qliept.h>      /* EPT_CALL macro  - this include must be after all other "QSYSINCs" */
#elif defined(XP_MAC)
#   include <Files.h>
#   include <IntlResources.h>
#   include <Script.h>
#   include <Folders.h>
#   include <MacTypes.h>
#   include <TextUtils.h>
#   define ICU_NO_USER_DATA_OVERRIDE 1
#elif defined(OS390)
#include "unicode/ucnv.h"   /* Needed for UCNV_SWAP_LFNL_OPTION_STRING */
#elif defined(U_AIX)
#elif defined(U_SOLARIS) || defined(U_LINUX)
#elif defined(U_HPUX)
#elif defined(U_DARWIN)
#include <sys/file.h>
#include <sys/param.h>
#elif defined(U_QNX)
#include <sys/neutrino.h>
#endif

#ifndef U_WINDOWS
#include <sys/time.h> 
#endif

/*
 * Only include langinfo.h if we have a way to get the codeset. If we later
 * depend on more feature, we can test on U_HAVE_NL_LANGINFO.
 *
 */

#if U_HAVE_NL_LANGINFO_CODESET
#include <langinfo.h>
#endif

/* Define the extension for data files, again... */
#define DATA_TYPE "dat"

/* Leave this copyright notice here! */
static const char copyright[] = U_COPYRIGHT_STRING;

/* floating point implementations ------------------------------------------- */

/* We return QNAN rather than SNAN*/
#define SIGN 0x80000000U
#if defined(__GNUC__) || defined(_MSC_VER)
/*
    This is an optimization for when u_topNBytesOfDouble
    and u_bottomNBytesOfDouble can't be properly optimized by the compiler
    or when faster infinity and NaN usage is helpful.
*/
#define USE_64BIT_DOUBLE_OPTIMIZATION 1
#else
#define USE_64BIT_DOUBLE_OPTIMIZATION 0
#endif

#if USE_64BIT_DOUBLE_OPTIMIZATION
/* gcc 3.2 has an optimization bug */
static const int64_t gNan64 = INT64_C(0x7FF8000000000000);
static const int64_t gInf64 = INT64_C(0x7FF0000000000000);
static const double * const fgNan = (const double * const)(&gNan64);
static const double * const fgInf = (const double * const)(&gInf64);
#else

#if IEEE_754
#define NAN_TOP ((int16_t)0x7FF8)
#define INF_TOP ((int16_t)0x7FF0)
#elif defined(OS390)
#define NAN_TOP ((int16_t)0x7F08)
#define INF_TOP ((int16_t)0x3F00)
#endif

/* statics */
static UBool fgNaNInitialized = FALSE;
static UBool fgInfInitialized = FALSE;
static double gNan;
static double gInf;
static double * fgNan = &gNan;
static double * fgInf = &gInf;
#endif

/*---------------------------------------------------------------------------
  Platform utilities
  Our general strategy is to assume we're on a POSIX platform.  Platforms which
  are non-POSIX must declare themselves so.  The default POSIX implementation
  will sometimes work for non-POSIX platforms as well (e.g., the NaN-related
  functions).
  ---------------------------------------------------------------------------*/

#if defined(U_WINDOWS) || defined(XP_MAC) || defined(OS400)
#   undef U_POSIX_LOCALE
#else
#   define U_POSIX_LOCALE    1
#endif

/* Utilities to get the bits from a double */
#if !USE_64BIT_DOUBLE_OPTIMIZATION
static char*
u_topNBytesOfDouble(double* d, int n)
{
#if U_IS_BIG_ENDIAN
    return (char*)d;
#else
    return (char*)(d + 1) - n;
#endif
}
#endif

static char*
u_bottomNBytesOfDouble(double* d, int n)
{
#if U_IS_BIG_ENDIAN
    return (char*)(d + 1) - n;
#else
    return (char*)d;
#endif
}

#if defined(U_WINDOWS)
typedef union {
    int64_t int64;
    FILETIME fileTime;
} FileTimeConversion;   /* This is like a ULARGE_INTEGER */

/* Number of 100 nanoseconds from 1/1/1601 to 1/1/1970 */
#define EPOCH_BIAS  INT64_C(116444736000000000)
#define HECTONANOSECOND_PER_MILLISECOND   10000

#endif

/*---------------------------------------------------------------------------
  Universal Implementations
  These are designed to work on all platforms.  Try these, and if they
  don't work on your platform, then special case your platform with new
  implementations.
---------------------------------------------------------------------------*/

/* Return UTC (GMT) time measured in milliseconds since 0:00 on 1/1/70.*/
U_CAPI UDate U_EXPORT2
uprv_getUTCtime()
{
#ifdef XP_MAC
    time_t t, t1, t2;
    struct tm tmrec;

    uprv_memset( &tmrec, 0, sizeof(tmrec) );
    tmrec.tm_year = 70;
    tmrec.tm_mon = 0;
    tmrec.tm_mday = 1;
    t1 = mktime(&tmrec);    /* seconds of 1/1/1970*/

    time(&t);
    uprv_memcpy( &tmrec, gmtime(&t), sizeof(tmrec) );
    t2 = mktime(&tmrec);    /* seconds of current GMT*/
    return (UDate)(t2 - t1) * U_MILLIS_PER_SECOND;         /* GMT (or UTC) in seconds since 1970*/
#elif defined(U_WINDOWS)

    FileTimeConversion winTime;
    GetSystemTimeAsFileTime(&winTime.fileTime);
    return (UDate)((winTime.int64 - EPOCH_BIAS) / HECTONANOSECOND_PER_MILLISECOND);
#else
/*
    struct timeval posixTime;
    gettimeofday(&posixTime, NULL);
    return (UDate)(((int64_t)posixTime.tv_sec * U_MILLIS_PER_SECOND) + (posixTime.tv_usec/1000));
*/
    time_t epochtime;
    time(&epochtime);
    return (UDate)epochtime * U_MILLIS_PER_SECOND;
#endif
}

/*-----------------------------------------------------------------------------
  IEEE 754
  These methods detect and return NaN and infinity values for doubles
  conforming to IEEE 754.  Platforms which support this standard include X86,
  Mac 680x0, Mac PowerPC, AIX RS/6000, and most others.
  If this doesn't work on your platform, you have non-IEEE floating-point, and
  will need to code your own versions.  A naive implementation is to return 0.0
  for getNaN and getInfinity, and false for isNaN and isInfinite.
  ---------------------------------------------------------------------------*/

U_CAPI UBool U_EXPORT2
uprv_isNaN(double number)
{
#if IEEE_754
#if USE_64BIT_DOUBLE_OPTIMIZATION
    /* gcc 3.2 has an optimization bug */
    /* Infinity is 0x7FF0000000000000U. Anything greater than that is a NaN */
    return (UBool)(((*((int64_t *)&number)) & U_INT64_MAX) > gInf64);

#else
    /* This should work in theory, but it doesn't, so we resort to the more*/
    /* complicated method below.*/
    /*  return number != number;*/

    /* You can't return number == getNaN() because, by definition, NaN != x for*/
    /* all x, including NaN (that is, NaN != NaN).  So instead, we compare*/
    /* against the known bit pattern.  We must be careful of endianism here.*/
    /* The pattern we are looking for id:*/

    /*   7FFy yyyy yyyy yyyy  (some y non-zero)*/

    /* There are two different kinds of NaN, but we ignore the distinction*/
    /* here.  Note that the y value must be non-zero; if it is zero, then we*/
    /* have infinity.*/

    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                              sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                             sizeof(uint32_t));

    return (UBool)(((highBits & 0x7FF00000L) == 0x7FF00000L) &&
      (((highBits & 0x000FFFFFL) != 0) || (lowBits != 0)));
#endif

#elif defined(OS390)
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                        sizeof(uint32_t));

    return ((highBits & 0x7F080000L) == 0x7F080000L) &&
      (lowBits == 0x00000000L);

#else
    /* If your platform doesn't support IEEE 754 but *does* have an NaN value,*/
    /* you'll need to replace this default implementation with what's correct*/
    /* for your platform.*/
    return number != number;
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isInfinite(double number)
{
#if IEEE_754
#if USE_64BIT_DOUBLE_OPTIMIZATION
    /* gcc 3.2 has an optimization bug */
    return (UBool)(((*((int64_t *)&number)) & U_INT64_MAX) == gInf64);
#else

    /* We know the top bit is the sign bit, so we mask that off in a copy of */
    /* the number and compare against infinity. [LIU]*/
    /* The following approach doesn't work for some reason, so we go ahead and */
    /* scrutinize the pattern itself. */
    /*  double a = number; */
    /*  *(int8_t*)u_topNBytesOfDouble(&a, 1) &= 0x7F;*/
    /*  return a == uprv_getInfinity();*/
    /* Instead, We want to see either:*/

    /*   7FF0 0000 0000 0000*/
    /*   FFF0 0000 0000 0000*/

    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                        sizeof(uint32_t));

    return (UBool)(((highBits  & ~SIGN) == 0x7FF00000U) &&
      (lowBits == 0x00000000U));
#endif

#elif defined(OS390)
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    uint32_t lowBits  = *(uint32_t*)u_bottomNBytesOfDouble(&number,
                        sizeof(uint32_t));

    return ((highBits  & ~SIGN) == 0x70FF0000L) && (lowBits == 0x00000000L);

#else
    /* If your platform doesn't support IEEE 754 but *does* have an infinity*/
    /* value, you'll need to replace this default implementation with what's*/
    /* correct for your platform.*/
    return number == (2.0 * number);
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isPositiveInfinity(double number)
{
#if IEEE_754 || defined(OS390)
    return (UBool)(number > 0 && uprv_isInfinite(number));
#else
    return uprv_isInfinite(number);
#endif
}

U_CAPI UBool U_EXPORT2
uprv_isNegativeInfinity(double number)
{
#if IEEE_754 || defined(OS390)
    return (UBool)(number < 0 && uprv_isInfinite(number));

#else
    uint32_t highBits = *(uint32_t*)u_topNBytesOfDouble(&number,
                        sizeof(uint32_t));
    return((highBits & SIGN) && uprv_isInfinite(number));

#endif
}

U_CAPI double U_EXPORT2
uprv_getNaN()
{
#if IEEE_754 || defined(OS390)
#if !USE_64BIT_DOUBLE_OPTIMIZATION
    if (!fgNaNInitialized) {
        /* This variable is always initialized with the same value,
        so a mutex isn't needed. */
        int i;
        int8_t* p = (int8_t*)fgNan;
        for(i = 0; i < sizeof(double); ++i)
            *p++ = 0;
        *(int16_t*)u_topNBytesOfDouble(fgNan, sizeof(NAN_TOP)) = NAN_TOP;
        fgNaNInitialized = TRUE;
    }
#endif
    return *fgNan;
#else
    /* If your platform doesn't support IEEE 754 but *does* have an NaN value,*/
    /* you'll need to replace this default implementation with what's correct*/
    /* for your platform.*/
    return 0.0;
#endif
}

U_CAPI double U_EXPORT2
uprv_getInfinity()
{
#if IEEE_754 || defined(OS390)
#if !USE_64BIT_DOUBLE_OPTIMIZATION
    if (!fgInfInitialized)
    {
        /* This variable is always initialized with the same value,
        so a mutex isn't needed. */
        int i;
        int8_t* p = (int8_t*)fgInf;
        for(i = 0; i < sizeof(double); ++i)
            *p++ = 0;
        *(int16_t*)u_topNBytesOfDouble(fgInf, sizeof(INF_TOP)) = INF_TOP;
        fgInfInitialized = TRUE;
    }
#endif
    return *fgInf;
#else
    /* If your platform doesn't support IEEE 754 but *does* have an infinity*/
    /* value, you'll need to replace this default implementation with what's*/
    /* correct for your platform.*/
    return 0.0;
#endif
}

U_CAPI double U_EXPORT2
uprv_floor(double x)
{
    return floor(x);
}

U_CAPI double U_EXPORT2
uprv_ceil(double x)
{
    return ceil(x);
}

U_CAPI double U_EXPORT2
uprv_round(double x)
{
    return uprv_floor(x + 0.5);
}

U_CAPI double U_EXPORT2
uprv_fabs(double x)
{
    return fabs(x);
}

U_CAPI double U_EXPORT2
uprv_modf(double x, double* y)
{
    return modf(x, y);
}

U_CAPI double U_EXPORT2
uprv_fmod(double x, double y)
{
    return fmod(x, y);
}

U_CAPI double U_EXPORT2
uprv_pow(double x, double y)
{
    /* This is declared as "double pow(double x, double y)" */
    return pow(x, y);
}

U_CAPI double U_EXPORT2
uprv_pow10(int32_t x)
{
    return pow(10.0, (double)x);
}

U_CAPI double U_EXPORT2
uprv_fmax(double x, double y)
{
#if IEEE_754
    int32_t lowBits;

    /* first handle NaN*/
    if(uprv_isNaN(x) || uprv_isNaN(y))
        return uprv_getNaN();

    /* check for -0 and 0*/
    lowBits = *(uint32_t*) u_bottomNBytesOfDouble(&x, sizeof(uint32_t));
    if(x == 0.0 && y == 0.0 && (lowBits & SIGN))
        return y;

#endif

    /* this should work for all flt point w/o NaN and Infpecial cases */
    return (x > y ? x : y);
}

U_CAPI int32_t U_EXPORT2
uprv_max(int32_t x, int32_t y)
{
    return (x > y ? x : y);
}

U_CAPI double U_EXPORT2
uprv_fmin(double x, double y)
{
#if IEEE_754
    int32_t lowBits;

    /* first handle NaN*/
    if(uprv_isNaN(x) || uprv_isNaN(y))
        return uprv_getNaN();

    /* check for -0 and 0*/
    lowBits = *(uint32_t*) u_bottomNBytesOfDouble(&y, sizeof(uint32_t));
    if(x == 0.0 && y == 0.0 && (lowBits & SIGN))
        return y;

#endif

    /* this should work for all flt point w/o NaN and Inf special cases */
    return (x > y ? y : x);
}

U_CAPI int32_t U_EXPORT2
uprv_min(int32_t x, int32_t y)
{
    return (x > y ? y : x);
}

/**
 * Truncates the given double.
 * trunc(3.3) = 3.0, trunc (-3.3) = -3.0
 * This is different than calling floor() or ceil():
 * floor(3.3) = 3, floor(-3.3) = -4
 * ceil(3.3) = 4, ceil(-3.3) = -3
 */
U_CAPI double U_EXPORT2
uprv_trunc(double d)
{
#if IEEE_754
    int32_t lowBits;

    /* handle error cases*/
    if(uprv_isNaN(d))
        return uprv_getNaN();
    if(uprv_isInfinite(d))
        return uprv_getInfinity();

    lowBits = *(uint32_t*) u_bottomNBytesOfDouble(&d, sizeof(uint32_t));
    if( (d == 0.0 && (lowBits & SIGN)) || d < 0)
        return ceil(d);
    else
        return floor(d);

#else
    return d >= 0 ? floor(d) : ceil(d);

#endif
}

/**
 * Return the largest positive number that can be represented by an integer
 * type of arbitrary bit length.
 */
U_CAPI double U_EXPORT2
uprv_maxMantissa(void)
{
    return pow(2.0, DBL_MANT_DIG + 1.0) - 1.0;
}

/**
 * Return the floor of the log base 10 of a given double.
 * This method compensates for inaccuracies which arise naturally when
 * computing logs, and always give the correct value.  The parameter
 * must be positive and finite.
 * (Thanks to Alan Liu for supplying this function.)
 */
U_CAPI int16_t U_EXPORT2
uprv_log10(double d)
{
#ifdef OS400
    /* We don't use the normal implementation because you can't underflow */
    /* a double otherwise an underflow exception occurs */
    return log10(d);
#else
    /* The reason this routine is needed is that simply taking the*/
    /* log and dividing by log10 yields a result which may be off*/
    /* by 1 due to rounding errors.  For example, the naive log10*/
    /* of 1.0e300 taken this way is 299, rather than 300.*/
    double alog10 = log(d) / log(10.0);
    int16_t ailog10 = (int16_t) floor(alog10);

    /* Positive logs could be too small, e.g. 0.99 instead of 1.0*/
    if (alog10 > 0 && d >= pow(10.0, (double)(ailog10 + 1)))
        ++ailog10;

    /* Negative logs could be too big, e.g. -0.99 instead of -1.0*/
    else if (alog10 < 0 && d < pow(10.0, (double)(ailog10)))
        --ailog10;

    return ailog10;
#endif
}

U_CAPI double U_EXPORT2
uprv_log(double d)
{
    return log(d);
}

#if 0
/* This isn't used. If it's readded, readd putiltst.c tests */
U_CAPI int32_t U_EXPORT2
uprv_digitsAfterDecimal(double x)
{
    char buffer[20];
    int32_t numDigits, bytesWritten;
    char *p = buffer;
    int32_t ptPos, exponent;

    /* cheat and use the string-format routine to get a string representation*/
    /* (it handles mathematical inaccuracy better than we can), then find out */
    /* many characters are to the right of the decimal point */
    bytesWritten = sprintf(buffer, "%+.9g", x);
    while (isdigit(*(++p))) {
    }

    ptPos = (int32_t)(p - buffer);
    numDigits = (int32_t)(bytesWritten - ptPos - 1);

    /* if the number's string representation is in scientific notation, find */
    /* the exponent and take it into account*/
    exponent = 0;
    p = uprv_strchr(buffer, 'e');
    if (p != 0) {
        int16_t expPos = (int16_t)(p - buffer);
        numDigits -= bytesWritten - expPos;
        exponent = (int32_t)(atol(p + 1));
    }

    /* the string representation may still have spurious decimal digits in it, */
    /* so we cut off at the ninth digit to the right of the decimal, and have */
    /* to search backward from there to the first non-zero digit*/
    if (numDigits > 9) {
        numDigits = 9;
        while (numDigits > 0 && buffer[ptPos + numDigits] == '0')
            --numDigits;
    }
    numDigits -= exponent;
    if (numDigits < 0) {
        return 0;
    }
    return numDigits;
}
#endif

/*---------------------------------------------------------------------------
  Platform-specific Implementations
  Try these, and if they don't work on your platform, then special case your
  platform with new implementations.
  ---------------------------------------------------------------------------*/

/* Win32 time zone detection ------------------------------------------------ */

#ifdef U_WINDOWS

/*
  This code attempts to detect the Windows time zone, as set in the
  Windows Date and Time control panel.  It attempts to work on
  multiple flavors of Windows (9x, Me, NT, 2000, XP) and on localized
  installs.  It works by directly interrogating the registry and
  comparing the data there with the data returned by the
  GetTimeZoneInformation API, along with some other strategies.  The
  registry contains time zone data under one of two keys (depending on
  the flavor of Windows):

    HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Time Zones\
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones\

  Under this key are several subkeys, one for each time zone.  These
  subkeys are named "Pacific" on Win9x/Me and "Pacific Standard Time"
  on WinNT/2k/XP.  There are some other wrinkles; see the code for
  details.  The subkey name is NOT LOCALIZED, allowing us to support
  localized installs.

  Under the subkey are data values.  We care about:

    Std   Standard time display name, localized
    TZI   Binary block of data

  The TZI data is of particular interest.  It contains the offset, two
  more offsets for standard and daylight time, and the start and end
  rules.  This is the same data returned by the GetTimeZoneInformation
  API.  The API may modify the data on the way out, so we have to be
  careful, but essentially we do a binary comparison against the TZI
  blocks of various registry keys.  When we find a match, we know what
  time zone Windows is set to.  Since the registry key is not
  localized, we can then translate the key through a simple table
  lookup into the corresponding ICU time zone.

  This strategy doesn't always work because there are zones which
  share an offset and rules, so more than one TZI block will match.
  For example, both Tokyo and Seoul are at GMT+9 with no DST rules;
  their TZI blocks are identical.  For these cases, we fall back to a
  name lookup.  We attempt to match the display name as stored in the
  registry for the current zone to the display name stored in the
  registry for various Windows zones.  By comparing the registry data
  directly we avoid conversion complications.

  Author: Alan Liu
  Since: ICU 2.6
  Based on original code by Carl Brown <cbrown@xnetinc.com>
*/

/**
 * Layout of the binary registry data under the "TZI" key.
 */
typedef struct {
   LONG       Bias;
   LONG       StandardBias;
   LONG       DaylightBias; /* Tweaked by GetTimeZoneInformation */
   SYSTEMTIME StandardDate;
   SYSTEMTIME DaylightDate;
} TZI;

typedef struct {
    const char* icuid;
    const char* winid;
} WindowsICUMap;

/**
 * Mapping between Windows zone IDs and ICU zone IDs.  This list has
 * been mechanically checked; all zone offsets match (most important)
 * and city names match the display city names (where possible).  The
 * presence or absence of DST differs in some cases, but this is
 * acceptable as long as the zone is semantically the same (which has
 * been manually checked).
 *
 * Windows 9x/Me zone IDs are listed as "Pacific" rather than "Pacific
 * Standard Time", which is seen in NT/2k/XP.  This is fixed-up at
 * runtime as needed.  The one exception is "Mexico Standard Time 2",
 * which is not present on Windows 9x/Me.
 *
 * Zones that are not unique under Offset+Rules should be grouped
 * together for efficiency (see code below).  In addition, rules MUST
 * be grouped so that all zones of a single offset are together.
 *
 * Comments list S(tandard) or D(aylight), as declared by Windows,
 * followed by the display name (data from Windows XP).
 *
 * NOTE: Etc/GMT+12 is CORRECT for offset GMT-12:00.  Consult
 * documentation elsewhere for an explanation.
 */
static const WindowsICUMap ZONE_MAP[] = {
    "Etc/GMT+12",           "Dateline", /* S (GMT-12:00) International Date Line West */

    "Pacific/Apia",         "Samoa", /* S (GMT-11:00) Midway Island, Samoa */

    "Pacific/Honolulu",     "Hawaiian", /* S (GMT-10:00) Hawaii */

    "America/Anchorage",    "Alaskan", /* D (GMT-09:00) Alaska */

    "America/Los_Angeles",  "Pacific", /* D (GMT-08:00) Pacific Time (US & Canada); Tijuana */

    "America/Phoenix",      "US Mountain", /* S (GMT-07:00) Arizona */
    "America/Denver",       "Mountain", /* D (GMT-07:00) Mountain Time (US & Canada) */
    "America/Chihuahua",    "Mexico Standard Time 2", /* D (GMT-07:00) Chihuahua, La Paz, Mazatlan */

    "America/Managua",      "Central America", /* S (GMT-06:00) Central America */
    "America/Regina",       "Canada Central", /* S (GMT-06:00) Saskatchewan */
    "America/Mexico_City",  "Mexico", /* D (GMT-06:00) Guadalajara, Mexico City, Monterrey */
    "America/Chicago",      "Central", /* D (GMT-06:00) Central Time (US & Canada) */

    "America/Indianapolis", "US Eastern", /* S (GMT-05:00) Indiana (East) */
    "America/Bogota",       "SA Pacific", /* S (GMT-05:00) Bogota, Lima, Quito */
    "America/New_York",     "Eastern", /* D (GMT-05:00) Eastern Time (US & Canada) */

    "America/Caracas",      "SA Western", /* S (GMT-04:00) Caracas, La Paz */
    "America/Santiago",     "Pacific SA", /* D (GMT-04:00) Santiago */
    "America/Halifax",      "Atlantic", /* D (GMT-04:00) Atlantic Time (Canada) */

    "America/St_Johns",     "Newfoundland", /* D (GMT-03:30) Newfoundland */

    "America/Buenos_Aires", "SA Eastern", /* S (GMT-03:00) Buenos Aires, Georgetown */
    "America/Godthab",      "Greenland", /* D (GMT-03:00) Greenland */
    "America/Sao_Paulo",    "E. South America", /* D (GMT-03:00) Brasilia */

    "America/Noronha",      "Mid-Atlantic", /* D (GMT-02:00) Mid-Atlantic */

    "Atlantic/Cape_Verde",  "Cape Verde", /* S (GMT-01:00) Cape Verde Is. */
    "Atlantic/Azores",      "Azores", /* D (GMT-01:00) Azores */

    "Africa/Casablanca",    "Greenwich", /* S (GMT) Casablanca, Monrovia */
    "Europe/London",        "GMT", /* D (GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London */

    "Africa/Lagos",         "W. Central Africa", /* S (GMT+01:00) West Central Africa */
    "Europe/Berlin",        "W. Europe", /* D (GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna */
    "Europe/Paris",         "Romance", /* D (GMT+01:00) Brussels, Copenhagen, Madrid, Paris */
    "Europe/Sarajevo",      "Central European", /* D (GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb */
    "Europe/Belgrade",      "Central Europe", /* D (GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague */

    "Africa/Johannesburg",  "South Africa", /* S (GMT+02:00) Harare, Pretoria */
    "Asia/Jerusalem",       "Israel", /* S (GMT+02:00) Jerusalem */
    "Europe/Istanbul",      "GTB", /* D (GMT+02:00) Athens, Istanbul, Minsk */
    "Europe/Helsinki",      "FLE", /* D (GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius */
    "Africa/Cairo",         "Egypt", /* D (GMT+02:00) Cairo */
    "Europe/Bucharest",     "E. Europe", /* D (GMT+02:00) Bucharest */

    "Africa/Nairobi",       "E. Africa", /* S (GMT+03:00) Nairobi */
    "Asia/Riyadh",          "Arab", /* S (GMT+03:00) Kuwait, Riyadh */
    "Europe/Moscow",        "Russian", /* D (GMT+03:00) Moscow, St. Petersburg, Volgograd */
    "Asia/Baghdad",         "Arabic", /* D (GMT+03:00) Baghdad */

    "Asia/Tehran",          "Iran", /* D (GMT+03:30) Tehran */

    "Asia/Muscat",          "Arabian", /* S (GMT+04:00) Abu Dhabi, Muscat */
    "Asia/Tbilisi",         "Caucasus", /* D (GMT+04:00) Baku, Tbilisi, Yerevan */

    "Asia/Kabul",           "Afghanistan", /* S (GMT+04:30) Kabul */

    "Asia/Karachi",         "West Asia", /* S (GMT+05:00) Islamabad, Karachi, Tashkent */
    "Asia/Yekaterinburg",   "Ekaterinburg", /* D (GMT+05:00) Ekaterinburg */

    "Asia/Calcutta",        "India", /* S (GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi */

    "Asia/Katmandu",        "Nepal", /* S (GMT+05:45) Kathmandu */

    "Asia/Colombo",         "Sri Lanka", /* S (GMT+06:00) Sri Jayawardenepura */
    "Asia/Dhaka",           "Central Asia", /* S (GMT+06:00) Astana, Dhaka */
    "Asia/Novosibirsk",     "N. Central Asia", /* D (GMT+06:00) Almaty, Novosibirsk */

    "Asia/Rangoon",         "Myanmar", /* S (GMT+06:30) Rangoon */

    "Asia/Bangkok",         "SE Asia", /* S (GMT+07:00) Bangkok, Hanoi, Jakarta */
    "Asia/Krasnoyarsk",     "North Asia", /* D (GMT+07:00) Krasnoyarsk */

    "Australia/Perth",      "W. Australia", /* S (GMT+08:00) Perth */
    "Asia/Taipei",          "Taipei", /* S (GMT+08:00) Taipei */
    "Asia/Singapore",       "Singapore", /* S (GMT+08:00) Kuala Lumpur, Singapore */
    "Asia/Hong_Kong",       "China", /* S (GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi */
    "Asia/Irkutsk",         "North Asia East", /* D (GMT+08:00) Irkutsk, Ulaan Bataar */

    "Asia/Tokyo",           "Tokyo", /* S (GMT+09:00) Osaka, Sapporo, Tokyo */
    "Asia/Seoul",           "Korea", /* S (GMT+09:00) Seoul */
    "Asia/Yakutsk",         "Yakutsk", /* D (GMT+09:00) Yakutsk */

    "Australia/Darwin",     "AUS Central", /* S (GMT+09:30) Darwin */
    "Australia/Adelaide",   "Cen. Australia", /* D (GMT+09:30) Adelaide */

    "Pacific/Guam",         "West Pacific", /* S (GMT+10:00) Guam, Port Moresby */
    "Australia/Brisbane",   "E. Australia", /* S (GMT+10:00) Brisbane */
    "Asia/Vladivostok",     "Vladivostok", /* D (GMT+10:00) Vladivostok */
    "Australia/Hobart",     "Tasmania", /* D (GMT+10:00) Hobart */
    "Australia/Sydney",     "AUS Eastern", /* D (GMT+10:00) Canberra, Melbourne, Sydney */

    "Asia/Magadan",         "Central Pacific", /* S (GMT+11:00) Magadan, Solomon Is., New Caledonia */

    "Pacific/Fiji",         "Fiji", /* S (GMT+12:00) Fiji, Kamchatka, Marshall Is. */
    "Pacific/Auckland",     "New Zealand", /* D (GMT+12:00) Auckland, Wellington */

    "Pacific/Tongatapu",    "Tonga", /* S (GMT+13:00) Nuku'alofa */
    NULL,                   NULL
};

typedef struct {
    const char* winid;
    const char* altwinid;
} WindowsZoneRemap;

/**
 * If a lookup fails, we attempt to remap certain Windows ids to
 * alternate Windows ids.  If the alternate listed here begins with
 * '-', we use it as is (without the '-').  If it begins with '+', we
 * append a " Standard Time" if appropriate.
 */
static const WindowsZoneRemap ZONE_REMAP[] = {
    "Central European",     "-Warsaw",
    "Central Europe",       "-Prague Bratislava",
    "China",                "-Beijing",
                                               
    "Greenwich",            "+GMT",
    "GTB",                  "+GFT",
    "Arab",                 "+Saudi Arabia",
    "SE Asia",              "+Bangkok",
    "AUS Eastern",          "+Sydney",
    NULL,                   NULL,
};

/**
 * Various registry keys and key fragments.
 */
static const char CURRENT_ZONE_REGKEY[] = "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation\\";
static const char STANDARD_NAME_REGKEY[] = "StandardName";
static const char STANDARD_TIME_REGKEY[] = " Standard Time";
static const char TZI_REGKEY[] = "TZI";
static const char STD_REGKEY[] = "Std";

/**
 * HKLM subkeys used to probe for the flavor of Windows.  Note that we
 * specifically check for the "GMT" zone subkey; this is present on
 * NT, but on XP has become "GMT Standard Time".  We need to
 * discriminate between these cases.
 */
static const char* const WIN_TYPE_PROBE_REGKEY[] = {
    /* WIN_9X_ME_TYPE */
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones",

    /* WIN_NT_TYPE */
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\GMT"

    /* otherwise: WIN_2K_XP_TYPE */
};

/**
 * The time zone root subkeys (under HKLM) for different flavors of
 * Windows.
 */
static const char* const TZ_REGKEY[] = {
    /* WIN_9X_ME_TYPE */
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones\\",

    /* WIN_NT_TYPE | WIN_2K_XP_TYPE */
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\"
};

/**
 * Flavor of Windows, from our perspective.  Not a real OS version,
 * but rather the flavor of the layout of the time zone information in
 * the registry.
 */
enum {
    WIN_9X_ME_TYPE = 0,
    WIN_NT_TYPE = 1,
    WIN_2K_XP_TYPE = 2
};

/**
 * Auxiliary Windows time zone function.  Attempts to open the given
 * Windows time zone ID as a registry key.  Returns ERROR_SUCCESS if
 * successful.  Caller must close the registry key.  Handles
 * variations in the resource layout in different flavors of Windows.
 *
 * @param hkey output parameter to receive opened registry key
 * @param winid Windows zone ID, e.g., "Pacific", without the
 * " Standard Time" suffix (if any).  Special case "Mexico Standard Time 2"
 * allowed.
 * @param winType Windows flavor (WIN_9X_ME_TYPE, etc.)
 * @return ERROR_SUCCESS upon success
 */
static LONG openTZRegKey(HKEY *hkey, const char* winid, int winType) {
    LONG result;
    char subKeyName[96];
    char* name;
    int i;

    uprv_strcpy(subKeyName, TZ_REGKEY[(winType == WIN_9X_ME_TYPE) ? 0 : 1]);
    name = &subKeyName[strlen(subKeyName)];
    uprv_strcat(subKeyName, winid);
    if (winType != WIN_9X_ME_TYPE) {
        /* Don't modify "Mexico Standard Time 2", which does not occur
           on WIN_9X_ME_TYPE.  Also, if the type is WIN_NT_TYPE, then
           in practice this means the GMT key is not followed by
           " Standard Time", so don't append in that case. */
        int isMexico2 = (winid[uprv_strlen(winid)- 1] == '2');
        if (!isMexico2 &&
            !(winType == WIN_NT_TYPE && uprv_strcmp(winid, "GMT") == 0)) {
            uprv_strcat(subKeyName, STANDARD_TIME_REGKEY);
        }
    }
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          subKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          hkey);

    if (result != ERROR_SUCCESS) {
        /* If the primary lookup fails, try to remap the Windows zone
           ID, according to the remapping table. */
        for (i=0; ZONE_REMAP[i].winid; ++i) {
            if (uprv_strcmp(winid, ZONE_REMAP[i].winid) == 0) {
                uprv_strcpy(name, ZONE_REMAP[i].altwinid + 1);
                if (*(ZONE_REMAP[i].altwinid) == '+' &&
                    winType != WIN_9X_ME_TYPE) {
                    uprv_strcat(subKeyName, STANDARD_TIME_REGKEY);                
                }
                result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      subKeyName,
                                      0,
                                      KEY_QUERY_VALUE,
                                      hkey);
                break;
            }
        }
    }

    return result;
}

/**
 * Main Windows time zone detection function.  Returns the Windows
 * time zone, translated to an ICU time zone, or NULL upon failure.
 */
static const char* detectWindowsTimeZone() {
    int winType;
    LONG result;
    HKEY hkey;
    TZI tziKey;
    TZI tziReg;
    DWORD cbData = sizeof(TZI);
    TIME_ZONE_INFORMATION apiTZI;
    char stdName[32];
    DWORD stdNameSize;
    char stdRegName[64];
    DWORD stdRegNameSize;
    int firstMatch, lastMatch;
    int j;

    /* Detect the version of windows by trying to open a sequence of
       probe keys.  We don't use the OS version API because what we
       really want to know is how the registry is laid out.
       Specifically, is it 9x/Me or not, and is it "GMT" or "GMT
       Standard Time". */
    for (winType=0; winType<2; ++winType) {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              WIN_TYPE_PROBE_REGKEY[winType],
                              0,
                              KEY_QUERY_VALUE,
                              &hkey);
        RegCloseKey(hkey);
        if (result == ERROR_SUCCESS) {
            break;
        }
    }

    /* Obtain TIME_ZONE_INFORMATION from the API, and then convert it
       to TZI.  We could also interrogate the registry directly; we do
       this below if needed. */
    uprv_memset(&apiTZI, 0, sizeof(apiTZI));
    GetTimeZoneInformation(&apiTZI);
    tziKey.Bias = apiTZI.Bias;
    uprv_memcpy((char *)&tziKey.StandardDate, (char*)&apiTZI.StandardDate,
           sizeof(apiTZI.StandardDate));
    uprv_memcpy((char *)&tziKey.DaylightDate, (char*)&apiTZI.DaylightDate,
           sizeof(apiTZI.DaylightDate));

    /* For each zone that can be identified by Offset+Rules, see if we
       have a match.  Continue scanning after finding a match,
       recording the index of the first and the last match.  We have
       to do this because some zones are not unique under
       Offset+Rules. */
    firstMatch = lastMatch = -1;
    for (j=0; ZONE_MAP[j].icuid; j++) {
        result = openTZRegKey(&hkey, ZONE_MAP[j].winid, winType);
        if (result == ERROR_SUCCESS) {
            result = RegQueryValueEx(hkey,
                                     TZI_REGKEY,
                                     NULL,
                                     NULL,
                                     (LPBYTE)&tziReg,
                                     &cbData);
        }
        RegCloseKey(hkey);
        if (result == ERROR_SUCCESS) {
            /* Assume that offsets are grouped together, and bail out
               when we've scanned everything with a matching
               offset. */
            if (firstMatch >= 0 && tziKey.Bias != tziReg.Bias) {
                break;
            }
            /* Windows alters the DaylightBias in some situations.
               Using the bias and the rules suffices, so overwrite
               these unreliable fields. */
            tziKey.StandardBias = tziReg.StandardBias;
            tziKey.DaylightBias = tziReg.DaylightBias;
            if (uprv_memcmp((char *)&tziKey, (char*)&tziReg,
                       sizeof(tziKey)) == 0) {
                if (firstMatch < 0) {
                    firstMatch = j;
                }
                lastMatch = j;
            }
        }
    }

    /* This should never happen; if it does it means our table doesn't
       match Windows AT ALL, perhaps because this is post-XP? */
    if (firstMatch < 0) {
        return NULL;
    }
    
    if (firstMatch != lastMatch) {
        /* Offset+Rules lookup yielded >= 2 matches.  Try to match the
           localized display name.  Get the name from the registry
           (not the API). This avoids conversion issues.  Use the
           standard name, since Windows modifies the daylight name to
           match the standard name if there is no DST. */
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              CURRENT_ZONE_REGKEY,
                              0,
                              KEY_QUERY_VALUE,
                              &hkey);
        if (result == ERROR_SUCCESS) {
            stdNameSize = sizeof(stdName);
            result = RegQueryValueEx(hkey,
                                     (LPTSTR)STANDARD_NAME_REGKEY,
                                     NULL,
                                     NULL,
                                     (LPBYTE)stdName,
                                     &stdNameSize);
            RegCloseKey(hkey);

            /* Scan through the Windows time zone data in the registry
               again (just the range of zones with matching TZIs) and
               look for a standard display name match. */
            for (j=firstMatch; j<=lastMatch; j++) {
                result = openTZRegKey(&hkey, ZONE_MAP[j].winid, winType);
                if (result == ERROR_SUCCESS) {
                    stdRegNameSize = sizeof(stdRegName);
                    result = RegQueryValueEx(hkey,
                                             (LPTSTR)STD_REGKEY,
                                             NULL,
                                             NULL,
                                             (LPBYTE)stdRegName,
                                             &stdRegNameSize);
                }
                RegCloseKey(hkey);
                if (result == ERROR_SUCCESS &&
                    stdRegNameSize == stdNameSize &&
                    uprv_memcmp(stdName, stdRegName, stdNameSize) == 0) {
                    firstMatch = j; /* record the match */
                    break;
                }
            }
        } else {
            RegCloseKey(hkey); /* should never get here */
        }
    }

    return ZONE_MAP[firstMatch].icuid;
}

#endif /*U_WINDOWS*/

/* Generic time zone layer -------------------------------------------------- */

/* Time zone utilities */
U_CAPI void U_EXPORT2
uprv_tzset()
{
#ifdef U_TZSET
    U_TZSET();
#else
    /* no initialization*/
#endif
}

U_CAPI int32_t U_EXPORT2
uprv_timezone()
{
#ifdef U_TIMEZONE
    return U_TIMEZONE;
#else
    time_t t, t1, t2;
    struct tm tmrec;
    UBool dst_checked;
    int32_t tdiff = 0;

    time(&t);
    uprv_memcpy( &tmrec, localtime(&t), sizeof(tmrec) );
    dst_checked = (tmrec.tm_isdst != 0); /* daylight savings time is checked*/
    t1 = mktime(&tmrec);                 /* local time in seconds*/
    uprv_memcpy( &tmrec, gmtime(&t), sizeof(tmrec) );
    t2 = mktime(&tmrec);                 /* GMT (or UTC) in seconds*/
    tdiff = t2 - t1;
    /* imitate NT behaviour, which returns same timezone offset to GMT for
       winter and summer*/
    if (dst_checked)
        tdiff += 3600;
    return tdiff;
#endif
}

/* Note that U_TZNAME does *not* have to be tzname, but if it is,
   some platforms need to have it declared here. */ 

#if defined(U_TZNAME) && (defined(U_IRIX) || defined(U_DARWIN) || defined(U_CYGWIN))
/* RS6000 and others reject char **tzname.  */
extern U_IMPORT char *U_TZNAME[];
#endif

#if defined(U_DARWIN)   /* For Mac OS X */
#define TZZONELINK      "/etc/localtime"
#define TZZONEINFO      "/usr/share/zoneinfo/"
static char *gTimeZoneBuffer = NULL; /* Heap allocated */
#endif

U_CAPI const char* U_EXPORT2
uprv_tzname(int n)
{
#ifdef U_WINDOWS
    char* id = (char*) detectWindowsTimeZone();
    if (id != NULL) {
        return id;
    }
#endif

#if defined(U_DARWIN)
    int ret;

    char *tzenv;

    tzenv = getenv("TZFILE");
    if (tzenv != NULL) {
        return tzenv;
    }

#if 0
    /* TZ is often set to "PST8PDT" or similar, so we cannot use it. Alan */
    tzenv = getenv("TZ");
    if (tzenv != NULL) {
        return tzenv;
    }
#endif
    
    /* Caller must handle threading issues */
    if (gTimeZoneBuffer == NULL) {
        gTimeZoneBuffer = (char *) uprv_malloc(MAXPATHLEN + 2);

        ret = readlink(TZZONELINK, gTimeZoneBuffer, MAXPATHLEN + 2);
        if (0 < ret) {
            gTimeZoneBuffer[ret] = '\0';
            if (uprv_strncmp(gTimeZoneBuffer, TZZONEINFO, sizeof(TZZONEINFO) - 1) == 0) {
                return (gTimeZoneBuffer += sizeof(TZZONEINFO) - 1);
            }
        }

        uprv_free(gTimeZoneBuffer);
        gTimeZoneBuffer = NULL;
    }
#endif

#ifdef U_TZNAME
    return U_TZNAME[n];
#else
    return "";
#endif
}

/* Get and set the ICU data directory --------------------------------------- */

static char *gDataDirectory = NULL;
#if U_POSIX_LOCALE
 static char *gCorrectedPOSIXLocale = NULL; /* Heap allocated */
#endif

static UBool U_CALLCONV putil_cleanup(void)
{
    if (gDataDirectory && *gDataDirectory) {
        uprv_free(gDataDirectory);
    }
    gDataDirectory = NULL;
#if U_POSIX_LOCALE
    if (gCorrectedPOSIXLocale) {
        uprv_free(gCorrectedPOSIXLocale);
        gCorrectedPOSIXLocale = NULL;
    }
#endif
    return TRUE;
}

/*
 * Set the data directory.
 *    Make a copy of the passed string, and set the global data dir to point to it.
 *    TODO:  see bug #2849, regarding thread safety.
 */
U_CAPI void U_EXPORT2
u_setDataDirectory(const char *directory) {
    char *newDataDir;
    int32_t length;

    if(directory==NULL || *directory==0) {
        /* A small optimization to prevent the malloc and copy when the
        shared library is used, and this is a way to make sure that NULL
        is never returned.
        */
        newDataDir = (char *)"";
    }
    else {
        length=(int32_t)uprv_strlen(directory);
        newDataDir = (char *)uprv_malloc(length + 2);
        uprv_strcpy(newDataDir, directory);

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
        {
            char *p;
            while(p = uprv_strchr(newDataDir, U_FILE_ALT_SEP_CHAR)) {
                *p = U_FILE_SEP_CHAR;
            }
        }
#endif
    }

    umtx_lock(NULL);
    if (gDataDirectory && *gDataDirectory) {
        uprv_free(gDataDirectory);
    }
    gDataDirectory = newDataDir;
    ucln_common_registerCleanup(UCLN_COMMON_PUTIL, putil_cleanup);
    umtx_unlock(NULL);
}

U_CAPI UBool U_EXPORT2
uprv_pathIsAbsolute(const char *path) 
{
  if(!path || !*path) { 
    return FALSE; 
  }

  if(*path == U_FILE_SEP_CHAR) {
    return TRUE;
  }

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
  if(*path == U_FILE_ALT_SEP_CHAR) {
    return TRUE;
  }
#endif

#if defined(U_WINDOWS)
  if( (((path[0] >= 'A') && (path[0] <= 'Z')) ||
       ((path[0] >= 'a') && (path[0] <= 'z'))) &&
      path[1] == ':' ) {
    return TRUE;
  }
#endif

  return FALSE;
}

U_CAPI const char * U_EXPORT2
u_getDataDirectory(void) {
    const char *path = NULL;

    /* if we have the directory, then return it immediately */
    umtx_lock(NULL);
    path = gDataDirectory;
    umtx_unlock(NULL);

    if(path) {
        return path;
    }

    /*
    When ICU_NO_USER_DATA_OVERRIDE is defined, users aren't allowed to
    override ICU's data with the ICU_DATA environment variable. This prevents
    problems where multiple custom copies of ICU's specific version of data
    are installed on a system. Either the application must define the data
    directory with u_setDataDirectory, define ICU_DATA_DIR when compiling
    ICU, set the data with udata_setCommonData or trust that all of the
    required data is contained in ICU's data library that contains
    the entry point defined by U_ICUDATA_ENTRY_POINT.

    There may also be some platforms where environment variables
    are not allowed.
    */
#   if !defined(ICU_NO_USER_DATA_OVERRIDE) && (!defined(UCONFIG_NO_FILE_IO) || !UCONFIG_NO_FILE_IO)
    /* First try to get the environment variable */
    path=getenv("ICU_DATA");
#   endif

    /* ICU_DATA_DIR may be set as a compile option */
#   ifdef ICU_DATA_DIR
    if(path==NULL || *path==0) {
        path=ICU_DATA_DIR;
    }
#   endif

    if(path==NULL) {
        /* It looks really bad, set it to something. */
        path = "";
    }

    u_setDataDirectory(path);
    return gDataDirectory;
}





/* Macintosh-specific locale information ------------------------------------ */
#ifdef XP_MAC

typedef struct {
    int32_t script;
    int32_t region;
    int32_t lang;
    int32_t date_region;
    const char* posixID;
} mac_lc_rec;

/* Todo: This will be updated with a newer version from www.unicode.org web
   page when it's available.*/
#define MAC_LC_MAGIC_NUMBER -5
#define MAC_LC_INIT_NUMBER -9

static const mac_lc_rec mac_lc_recs[] = {
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 0, "en_US",
    /* United States*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 1, "fr_FR",
    /* France*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 2, "en_GB",
    /* Great Britain*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 3, "de_DE",
    /* Germany*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 4, "it_IT",
    /* Italy*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 5, "nl_NL",
    /* Metherlands*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 6, "fr_BE",
    /* French for Belgium or Lxembourg*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 7, "sv_SE",
    /* Sweden*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 9, "da_DK",
    /* Denmark*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 10, "pt_PT",
    /* Portugal*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 11, "fr_CA",
    /* French Canada*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 13, "is_IS",
    /* Israel*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 14, "ja_JP",
    /* Japan*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 15, "en_AU",
    /* Australia*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 16, "ar_AE",
    /* the Arabic world (?)*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 17, "fi_FI",
    /* Finland*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 18, "fr_CH",
    /* French for Switzerland*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 19, "de_CH",
    /* German for Switzerland*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 20, "el_GR",
    /* Greece*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 21, "is_IS",
    /* Iceland ===*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 22, "",*/
    /* Malta ===*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 23, "",*/
    /* Cyprus ===*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 24, "tr_TR",
    /* Turkey ===*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 25, "sh_YU",
    /* Croatian system for Yugoslavia*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 33, "",*/
    /* Hindi system for India*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 34, "",*/
    /* Pakistan*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 41, "lt_LT",
    /* Lithuania*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 42, "pl_PL",
    /* Poland*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 43, "hu_HU",
    /* Hungary*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 44, "et_EE",
    /* Estonia*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 45, "lv_LV",
    /* Latvia*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 46, "",*/
    /* Lapland  [Ask Rich for the data. HS]*/
    /*MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 47, "",*/
    /* Faeroe Islands*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 48, "fa_IR",
    /* Iran*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 49, "ru_RU",
    /* Russia*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 50, "en_IE",
    /* Ireland*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 51, "ko_KR",
    /* Korea*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 52, "zh_CN",
    /* People's Republic of China*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 53, "zh_TW",
    /* Taiwan*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, 54, "th_TH",
    /* Thailand*/

    /* fallback is en_US*/
    MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER, MAC_LC_MAGIC_NUMBER,
    MAC_LC_MAGIC_NUMBER, "en_US"
};

#endif

#if U_POSIX_LOCALE
/* Return just the POSIX id, whatever happens to be in it */
static const char *uprv_getPOSIXID(void)
{
    static const char* posixID = NULL;
    if (posixID == 0) {
        posixID = getenv("LC_ALL");
        if (posixID == 0) {
            posixID = getenv("LANG");
            if (posixID == 0) {
                /*
                * On Solaris two different calls to setlocale can result in 
                * different values. Only get this value once.
                */
                posixID = setlocale(LC_ALL, NULL);
            }
        }
    }

    if (posixID==0)
    {
        /* Nothing worked.  Give it a nice value. */
        posixID = "en_US";
    }
    else if ((uprv_strcmp("C", posixID) == 0)
        || (uprv_strchr(posixID, ' ') != NULL)
        || (uprv_strchr(posixID, '/') != NULL))
    {   /* HPUX returns 'C C C C C C C' */
        /* Solaris can return /en_US/C/C/C/C/C on the second try. */
        /* Maybe we got some garbage.  Give it a nice value. */
        posixID = "en_US_POSIX";
    }
    return posixID;
}
#endif

/* NOTE: The caller should handle thread safety */
U_CAPI const char* U_EXPORT2
uprv_getDefaultLocaleID()
{
#if U_POSIX_LOCALE
/*
  Note that:  (a '!' means the ID is improper somehow)
     LC_ALL  ---->     default_loc          codepage
--------------------------------------------------------
     ab.CD             ab                   CD
     ab@CD             ab__CD               -
     ab@CD.EF          ab__CD               EF

     ab_CD.EF@GH       ab_CD_GH             EF

Some 'improper' ways to do the same as above:
  !  ab_CD@GH.EF       ab_CD_GH             EF
  !  ab_CD.EF@GH.IJ    ab_CD_GH             EF
  !  ab_CD@ZZ.EF@GH.IJ ab_CD_GH             EF

     _CD@GH            _CD_GH               -
     _CD.EF@GH         _CD_GH               EF

The variant cannot have dots in it.
The 'rightmost' variant (@xxx) wins.
The leftmost codepage (.xxx) wins.
*/
    char *correctedPOSIXLocale = 0;
    const char* posixID = uprv_getPOSIXID();
    const char *p;
    const char *q;
    int32_t len;

    /* Format: (no spaces)
    ll [ _CC ] [ . MM ] [ @ VV]

      l = lang, C = ctry, M = charmap, V = variant
    */

    if (gCorrectedPOSIXLocale != NULL) {
        return gCorrectedPOSIXLocale; 
    }

    if ((p = uprv_strchr(posixID, '.')) != NULL) {
        /* assume new locale can't be larger than old one? */
        correctedPOSIXLocale = uprv_malloc(uprv_strlen(posixID));
        uprv_strncpy(correctedPOSIXLocale, posixID, p-posixID);
        correctedPOSIXLocale[p-posixID] = 0;

        /* do not copy after the @ */
        if ((p = uprv_strchr(correctedPOSIXLocale, '@')) != NULL) {
            correctedPOSIXLocale[p-correctedPOSIXLocale] = 0;
        }
    }

    /* Note that we scan the *uncorrected* ID. */
    if ((p = uprv_strrchr(posixID, '@')) != NULL) {
        if (correctedPOSIXLocale == NULL) {
            correctedPOSIXLocale = uprv_malloc(uprv_strlen(posixID));
            uprv_strncpy(correctedPOSIXLocale, posixID, p-posixID);
            correctedPOSIXLocale[p-posixID] = 0;
        }
        p++;

        /* Take care of any special cases here.. */
        if (!uprv_strcmp(p, "nynorsk")) {
            p = "NY";

            /*      Should we assume no_NO_NY instead of possible no__NY?
            * if (!uprv_strcmp(correctedPOSIXLocale, "no")) {
            *     uprv_strcpy(correctedPOSIXLocale, "no_NO");
            * }
            */
        }

        if (uprv_strchr(correctedPOSIXLocale,'_') == NULL) {
            uprv_strcat(correctedPOSIXLocale, "__"); /* aa@b -> aa__b */
        }
        else {
            uprv_strcat(correctedPOSIXLocale, "_"); /* aa_CC@b -> aa_CC_b */
        }

        if ((q = uprv_strchr(p, '.')) != NULL) {
            /* How big will the resulting string be? */
            len = (int32_t)(uprv_strlen(correctedPOSIXLocale) + (q-p));
            uprv_strncat(correctedPOSIXLocale, p, q-p);
            correctedPOSIXLocale[len] = 0;
        }
        else {
            /* Anything following the @ sign */
            uprv_strcat(correctedPOSIXLocale, p);
        }

        /* Should there be a map from 'no@nynorsk' -> no_NO_NY here?
         * How about 'russian' -> 'ru'?
         */
    }

    /* Was a correction made? */
    if (correctedPOSIXLocale != NULL) {
        posixID = correctedPOSIXLocale;
    }
    else {
        /* copy it, just in case the original pointer goes away.  See j2395 */
        correctedPOSIXLocale = (char *)uprv_malloc(uprv_strlen(posixID) + 1);
        posixID = uprv_strcpy(correctedPOSIXLocale, posixID);
    }

    if (gCorrectedPOSIXLocale == NULL) {
        gCorrectedPOSIXLocale = correctedPOSIXLocale;
        ucln_common_registerCleanup(UCLN_COMMON_PUTIL, putil_cleanup);
        correctedPOSIXLocale = NULL;
    }

    if (correctedPOSIXLocale != NULL) {  /* Was already set - clean up. */
        uprv_free(correctedPOSIXLocale); 
    }

    return posixID;

#elif defined(U_WINDOWS)
    UErrorCode status = U_ZERO_ERROR;
    LCID id = GetThreadLocale();
    const char* locID = uprv_convertToPosix(id, &status);

    if (U_FAILURE(status)) {
        locID = "en_US";
    }
    return locID;

#elif defined(XP_MAC)
    int32_t script = MAC_LC_INIT_NUMBER;
    /* = IntlScript(); or GetScriptManagerVariable(smSysScript);*/
    int32_t region = MAC_LC_INIT_NUMBER;
    /* = GetScriptManagerVariable(smRegionCode);*/
    int32_t lang = MAC_LC_INIT_NUMBER;
    /* = GetScriptManagerVariable(smScriptLang);*/
    int32_t date_region = MAC_LC_INIT_NUMBER;
    const char* posixID = 0;
    int32_t count = sizeof(mac_lc_recs) / sizeof(mac_lc_rec);
    int32_t i;
    Intl1Hndl ih;

    ih = (Intl1Hndl) GetIntlResource(1);
    if (ih)
        date_region = ((uint16_t)(*ih)->intl1Vers) >> 8;

    for (i = 0; i < count; i++) {
        if (   ((mac_lc_recs[i].script == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].script == script))
            && ((mac_lc_recs[i].region == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].region == region))
            && ((mac_lc_recs[i].lang == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].lang == lang))
            && ((mac_lc_recs[i].date_region == MAC_LC_MAGIC_NUMBER)
             || (mac_lc_recs[i].date_region == date_region))
            )
        {
            posixID = mac_lc_recs[i].posixID;
            break;
        }
    }

    return posixID;

#elif defined(OS400)
    /* locales are process scoped and are by definition thread safe */
    static char correctedLocale[64];
    const  char *localeID = getenv("LC_ALL");
           char *p;

    if (localeID == NULL)
        localeID = getenv("LANG");
    if (localeID == NULL)
        localeID = setlocale(LC_ALL, NULL);
    /* Make sure we have something... */
    if (localeID == NULL)
        return "en_US_POSIX";

    /* Extract the locale name from the path. */
    if((p = uprv_strrchr(localeID, '/')) != NULL)
    {
        /* Increment p to start of locale name. */
        p++;
        localeID = p;
    }

    /* Copy to work location. */
    uprv_strcpy(correctedLocale, localeID);

    /* Strip off the '.locale' extension. */
    if((p = uprv_strchr(correctedLocale, '.')) != NULL) {
        *p = 0;
    }

    /* Upper case the locale name. */
    T_CString_toUpperCase(correctedLocale);

    /* See if we are using the POSIX locale.  Any of the
    * following are equivalent and use the same QLGPGCMA
    * (POSIX) locale.
    * QLGPGCMA2 means UCS2
    * QLGPGCMA_4 means UTF-32
    * QLGPGCMA_8 means UTF-8
    */
    if ((uprv_strcmp("C", correctedLocale) == 0) ||
        (uprv_strcmp("POSIX", correctedLocale) == 0) ||
        (uprv_strncmp("QLGPGCMA", correctedLocale, 8) == 0))
    {
        uprv_strcpy(correctedLocale, "en_US_POSIX");
    }
    else
    {
        int16_t LocaleLen;

        /* Lower case the lang portion. */
        for(p = correctedLocale; *p != 0 && *p != '_'; p++)
        {
            *p = uprv_tolower(*p);
        }

        /* Adjust for Euro.  After '_E' add 'URO'. */
        LocaleLen = uprv_strlen(correctedLocale);
        if (correctedLocale[LocaleLen - 2] == '_' &&
            correctedLocale[LocaleLen - 1] == 'E')
        {
            uprv_strcat(correctedLocale, "URO");
        }

        /* If using Lotus-based locale then convert to
         * equivalent non Lotus.
         */
        else if (correctedLocale[LocaleLen - 2] == '_' &&
            correctedLocale[LocaleLen - 1] == 'L')
        {
            correctedLocale[LocaleLen - 2] = 0;
        }

        /* There are separate simplified and traditional
         * locales called zh_HK_S and zh_HK_T.
         */
        else if (uprv_strncmp(correctedLocale, "zh_HK", 5) == 0)
        {
            uprv_strcpy(correctedLocale, "zh_HK");
        }

        /* A special zh_CN_GBK locale...
        */
        else if (uprv_strcmp(correctedLocale, "zh_CN_GBK") == 0)
        {
            uprv_strcpy(correctedLocale, "zh_CN");
        }

    }

    return correctedLocale;
#endif

}


static const char*  
int_getDefaultCodepage()
{
#if defined(OS400)
    uint32_t ccsid = 37; /* Default to ibm-37 */
    static char codepage[64];
    Qwc_JOBI0400_t jobinfo;
    Qus_EC_t error = { sizeof(Qus_EC_t) }; /* SPI error code */

    EPT_CALL(QUSRJOBI)(&jobinfo, sizeof(jobinfo), "JOBI0400",
        "*                         ", "                ", &error);

    if (error.Bytes_Available == 0) {
        if (jobinfo.Coded_Char_Set_ID != 0xFFFF) {
            ccsid = (uint32_t)jobinfo.Coded_Char_Set_ID;
        }
        else if (jobinfo.Default_Coded_Char_Set_Id != 0xFFFF) {
            ccsid = (uint32_t)jobinfo.Default_Coded_Char_Set_Id;
        }
        /* else use the default */
    }
    sprintf(codepage,"ibm-%d", ccsid);
    return codepage;

#elif defined(OS390)
    static char codepage[64];
    sprintf(codepage,"%s" UCNV_SWAP_LFNL_OPTION_STRING, nl_langinfo(CODESET));
    return codepage;

#elif defined(XP_MAC)
    return "macintosh"; /* TODO: Macintosh Roman. There must be a better way. fixme! */

#elif defined(U_WINDOWS)
    static char codepage[64];
    sprintf(codepage, "windows-%d", GetACP());
    return codepage;

#elif U_POSIX_LOCALE
    static char codesetName[100];
    char *name = NULL;
    char *euro = NULL;
    const char *localeName = NULL;

    uprv_memset(codesetName, 0, sizeof(codesetName));

    /* Check setlocale before the environment variables
       because the application may have set it first */

    /* Use setlocale in a nice way.
       Maybe the application used setlocale already.
       Normally this won't work. */
    localeName = setlocale(LC_CTYPE, NULL);
    if (localeName != NULL && (name = (uprv_strchr(localeName, '.'))) != NULL) {
        /* strip the locale name and look at the suffix only */
        name = uprv_strncpy(codesetName, name+1, sizeof(codesetName));
        codesetName[sizeof(codesetName)-1] = 0;
        if ((euro = (uprv_strchr(name, '@'))) != NULL) {
           *euro = 0;
        }
        /* if we can find the codset name from setlocale, return that. */
        if (*name) {
            return name;
        }
    }
    /* else "C" was probably returned. That's underspecified. */

    /* Use setlocale a little more forcefully.
       The application didn't use setlocale */
    localeName = setlocale(LC_CTYPE, "");
    if (localeName != NULL && (name = (uprv_strchr(localeName, '.'))) != NULL) {
        /* strip the locale name and look at the suffix only */
        name = uprv_strncpy(codesetName, name+1, sizeof(codesetName));
        codesetName[sizeof(codesetName)-1] = 0;
        if ((euro = (uprv_strchr(name, '@'))) != NULL) {
           *euro = 0;
        }
        /* if we can find the codset name from setlocale, return that. */
        if (*name) {
            return name;
        }
    }
    /* else "C" or something like it was returned. That's still underspecified. */

#if U_HAVE_NL_LANGINFO_CODESET
    if (*codesetName) {
        uprv_memset(codesetName, 0, sizeof(codesetName));
    }
    /* When available, check nl_langinfo first because it usually gives more
       useful names. It depends on LC_CTYPE and not LANG or LC_ALL */
    {
        const char *codeset = nl_langinfo(U_NL_LANGINFO_CODESET);
        if (codeset != NULL) {
            uprv_strncpy(codesetName, codeset, sizeof(codesetName));
            codesetName[sizeof(codesetName)-1] = 0;
            return codesetName;
        }
    }
#endif

    /* Try a locale specified by the user.
       This is usually underspecified and usually checked by setlocale already. */
    if (*codesetName) {
        uprv_memset(codesetName, 0, sizeof(codesetName));
    }
    localeName = uprv_getPOSIXID();
    if (localeName != NULL && (name = (uprv_strchr(localeName, '.'))) != NULL) {
        /* strip the locale name and look at the suffix only */
        name = uprv_strncpy(codesetName, name+1, sizeof(codesetName));
        codesetName[sizeof(codesetName)-1] = 0;
        if ((euro = (uprv_strchr(name, '@'))) != NULL) {
           *euro = 0;
        }
        /* if we can find the codset name, return that. */
        if (*name) {
            return name;
        }
    }

    if (*codesetName == 0)
    {
        /* if the table lookup failed, return US ASCII (ISO 646). */
        uprv_strcpy(codesetName, "US-ASCII");
    }
    return codesetName;
#else
    return "US-ASCII";
#endif
}


U_CAPI const char*  U_EXPORT2
uprv_getDefaultCodepage()
{
    static char const  *name = NULL;
    umtx_lock(NULL);
    if (name == NULL) {
        name = int_getDefaultCodepage();
    }
    umtx_unlock(NULL);
    return name;
}


/* end of platform-specific implementation -------------- */

/* version handling --------------------------------------------------------- */

U_CAPI void U_EXPORT2
u_versionFromString(UVersionInfo versionArray, const char *versionString) {
    char *end;
    uint16_t part=0;

    if(versionArray==NULL) {
        return;
    }

    if(versionString!=NULL) {
        for(;;) {
            versionArray[part]=(uint8_t)uprv_strtoul(versionString, &end, 10);
            if(end==versionString || ++part==U_MAX_VERSION_LENGTH || *end!=U_VERSION_DELIMITER) {
                break;
            }
            versionString=end+1;
        }
    }

    while(part<U_MAX_VERSION_LENGTH) {
        versionArray[part++]=0;
    }
}

U_CAPI void U_EXPORT2
u_versionToString(UVersionInfo versionArray, char *versionString) {
    uint16_t count, part;
    uint8_t field;

    if(versionString==NULL) {
        return;
    }

    if(versionArray==NULL) {
        versionString[0]=0;
        return;
    }

    /* count how many fields need to be written */
    for(count=4; count>0 && versionArray[count-1]==0; --count) {
    }

    if(count <= 1) {
        count = 2;
    }

    /* write the first part */
    /* write the decimal field value */
    field=versionArray[0];
    if(field>=100) {
        *versionString++=(char)('0'+field/100);
        field%=100;
    }
    if(field>=10) {
        *versionString++=(char)('0'+field/10);
        field%=10;
    }
    *versionString++=(char)('0'+field);

    /* write the following parts */
    for(part=1; part<count; ++part) {
        /* write a dot first */
        *versionString++=U_VERSION_DELIMITER;

        /* write the decimal field value */
        field=versionArray[part];
        if(field>=100) {
            *versionString++=(char)('0'+field/100);
            field%=100;
        }
        if(field>=10) {
            *versionString++=(char)('0'+field/10);
            field%=10;
        }
        *versionString++=(char)('0'+field);
    }

    /* NUL-terminate */
    *versionString=0;
}

U_CAPI void U_EXPORT2
u_getVersion(UVersionInfo versionArray) {
    u_versionFromString(versionArray, U_ICU_VERSION);
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
