/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#ifndef __XeTeXswap_H
#define __XeTeXswap_H

#include "platform.h"	// ICU's platform.h defines U_IS_BIG_ENDIAN for us

static inline UInt16
SWAP16(const UInt16 p)
{
#if U_IS_BIG_ENDIAN
	return p;
#else
	return (p >> 8) + (p << 8);
#endif
}

static inline UInt32
SWAP32(const UInt32 p)
{
#if U_IS_BIG_ENDIAN
	return p;
#else
	return (p >> 24) + ((p >> 8) & 0x0000ff00) + ((p << 8) & 0x00ff0000) + (p << 24);
#endif
}

#ifdef __cplusplus
static inline UInt16
SWAP(UInt16 p)
{
	return SWAP16(p);
}

static inline UInt32
SWAP(UInt32 p)
{
	return SWAP32(p);
}

static inline SInt16
SWAP(SInt16 p)
{
	return (SInt16)SWAP16((UInt16)p);
}

static inline SInt32
SWAP(SInt32 p)
{
	return (SInt32)SWAP32((UInt32)p);
}
#endif

#endif
