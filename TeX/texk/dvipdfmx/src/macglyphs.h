/*  $Header: /home/cvsroot/dvipdfmx/src/macglyphs.h,v 1.5 2004/01/30 18:34:21 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _MACGLYPHS_H_
#define _MACGLYPHS_H_

/* Macintosh glyph order - from apple's TTRefMan */
const char *const
macglyphorder[258] = {
  /* 0x0000 */
  ".notdef", ".null", "nonmarkingreturn", "space", "exclam", "quotedbl",
  "numbersign", "dollar", "percent", "ampersand", "quotesingle",
  "parenleft", "parenright", "asterisk", "plus", "comma",
  /* 0x0010 */
  "hyphen", "period", "slash", "zero", "one", "two", "three", "four",
  "five", "six", "seven", "eight", "nine", "colon", "semicolon", "less",
  /* 0x0020 */
  "equal", "greater", "question", "at", "A", "B", "C", "D",
  "E", "F", "G", "H", "I", "J", "K", "L",
  /* 0x0030 */
  "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X",
  "Y", "Z", "bracketleft", "backslash",
  /* 0x0040 */
  "bracketright", "asciicircum", "underscore", "grave",
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",
  /* 0x0050 */
  "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
  "y", "z", "braceleft", "bar",
  /* 0x0060 */
  "braceright", "asciitilde", "Adieresis", "Aring", "Ccedilla",
  "Eacute", "Ntilde", "Odieresis", "Udieresis", "aacute", "agrave",
  "acircumflex", "adieresis", "atilde", "aring", "ccedilla",
  /* 0x0070 */
  "eacute", "egrave", "ecircumflex", "edieresis", "iacute", "igrave",
  "icircumflex", "idieresis", "ntilde", "oacute", "ograve", "ocircumflex",
  "odieresis", "otilde", "uacute", "ugrave",
  /* 0x0080 */
  "ucircumflex", "udieresis", "dagger", "degree", "cent", "sterling",
  "section", "bullet", "paragraph", "germandbls", "registered",
  "copyright", "trademark", "acute", "dieresis", "notequal",
  /* 0x0090 */
  "AE", "Oslash", "infinity", "plusminus", "lessequal",	"greaterequal",
  "yen", "mu", "partialdiff", "summation", "product", "pi", "integral",
  "ordfeminine", "ordmasculine", "Omega",
  /* 0x00a0 */
  "ae", "oslash", "questiondown", "exclamdown", "logicalnot", "radical",
  "florin", "approxequal", "Delta", "guillemotleft", "guillemotright",
  "ellipsis", "nonbreakingspace", "Agrave", "Atilde", "Otilde",
  /* 0x00b0 */
  "OE", "oe", "endash", "emdash", "quotedblleft", "quotedblright",
  "quoteleft", "quoteright", "divide", "lozenge", "ydieresis",
  "Ydieresis", "fraction", "currency", "guilsinglleft", "guilsinglright",
  /* 0x00c0 */
  "fi", "fl", "daggerdbl", "periodcentered", "quotesinglbase",
  "quotedblbase", "perthousand", "Acircumflex",	 "Ecircumflex", "Aacute",
  "Edieresis", "Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave",
  /* 0x00d0 */
  "Oacute", "Ocircumflex", "apple", "Ograve", "Uacute", "Ucircumflex",
  "Ugrave", "dotlessi", "circumflex", "tilde", "macron", "breve",
  "dotaccent", "ring", "cedilla", "hungarumlaut",
  /* 0x00e0 */
  "ogonek", "caron", "Lslash", "lslash", "Scaron", "scaron", "Zcaron",
  "zcaron", "brokenbar", "Eth", "eth", "Yacute", "yacute", "Thorn",
  "thorn", "minus",
  /* 0x00f0 */
  "multiply", "onesuperior", "twosuperior", "threesuperior", "onehalf",
  "onequarter", "threequarters", "franc", "Gbreve", "gbreve", "Idotaccent",
  "Scedilla", "scedilla", "Cacute", "cacute", "Ccaron",
  /* 0x0100 */
  "ccaron", "dcroat"
};

#endif /* _MACGLYPHS_H_ */
