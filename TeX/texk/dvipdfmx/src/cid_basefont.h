/*  $Header: /home/cvsroot/dvipdfmx/src/cid_basefont.h,v 1.10 2004/01/30 18:34:19 hirata Exp $

    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>

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

#ifndef _CID_BASEFONT_H_
#define _CID_BASEFONT_H_

/*
 * /BaseFont (Font Dict), /FontName (FontDescriptor) ommited. (due to style)
 */

const struct {
  const char *fontname;
  const char *fontdict;
  const char *descriptor;
} cid_basefont[] = {
  /* Well-known fonts found in PostScript printers. */
  {
    "Ryumin-Light",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Japan1) /Supplement 2 >> \
>>",
    "<< \
/CapHeight 709 /Ascent 723 /Descent -241 /StemV 69 \
/FontBBox [-170 -331 1024 903] \
/ItalicAngle 0 /Flags 6 \
/Style << /Panose <010502020300000000000000> >> \
>>"
  },
  {
    "GothicBBB-Medium",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo <<  /Registry (Adobe) /Ordering (Japan1) /Supplement 2 >> \
>>",
    "<< \
/CapHeight 737 /Ascent 752 /Descent -271 /StemV 99 \
/FontBBox [-174 -268 1001 944] \
/ItalicAngle 0 /Flags 4 \
/Style << /Panose <0801020b0500000000000000> >> \
>>"
  },

  /* Adobe Asian Font Packs for Acrobat Reader 4 */
  {
    "MHei-Medium-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (CNS1) /Supplement 0 >> \
>>",
    "<< \
/Ascent 752 /CapHeight 737 /Descent -271 /StemV 58 \
/FontBBox [-45 -250 1015 887] \
/ItalicAngle 0 /Flags 4 /XHeight 553 \
/Style << /Panose <000001000600000000000000> >> \
>>"
  },
  {
    "MSung-Light-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (CNS1) /Supplement 0 >> \
>>",
    "<< \
/Ascent 752 /CapHeight 737 /Descent -271 /StemV 58 \
/FontBBox [-160 -259 1015 888] \
/ItalicAngle 0 /Flags 6 /XHeight 553 \
/Style << /Panose <000000000400000000000000> >> \
>>"
  },
  {
    "STSong-Light-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (GB1) /Supplement 2 >> \
>>",
    "<< \
/Ascent 857 /CapHeight 857 /Descent -143 /StemV 91 \
/FontBBox [-250 -143 600 857] \
/ItalicAngle 0 /Flags 6 /XHeight 599 \
/Style << /Panose <000000000400000000000000> >> \
>>"
  },
  {
    "HeiseiKakuGo-W5-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Japan1) /Supplement  2 >> \
>>",
    "<< \
/Ascent 752 /CapHeight 737 /Descent -221 /StemV 114 \
/FontBBox [-92 -250 1010 922] \
/ItalicAngle 0 /Flags 4 /XHeight 553 \
/Style << /Panose <000001000500000000000000> >> \
>>"
  },
  {
    "HeiseiMin-W3-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Japan1) /Supplement 2 >> \
>>",
    "<< \
/Ascent 723 /CapHeight 709 /Descent -241 /StemV 69 \
/FontBBox [-123 -257 1001 910] \
/ItalicAngle 0 /Flags 6 /XHeight 450 \
/Style << /Panose <000002020500000000000000> >> \
>>"
  },
  {
    "HYGoThic-Medium-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Korea1) /Supplement 1 >> \
>>",
    "<< \
/Ascent 752 /CapHeight 737 /Descent -271 /StemV 58 \
/FontBBox [-6 -145 1003 880] \
/ItalicAngle 0 /Flags 4 /XHeight 553 \
/Style << /Panose <000001000600000000000000> >> \
>>"
  },
  {
    "HYSMyeongJo-Medium-Acro",
    "<< \
/Subtype /CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Korea1) /Supplement 1 >> \
>>",
    "<< \
/Ascent 752 /CapHeight 737 /Descent -271 /StemV 58 \
/FontBBox [-0 -148 1001 880] \
/ItalicAngle 0 /Flags 6 /XHeight 553 \
/Style << /Panose <000000000600000000000000> >> \
>>"
  },

  /* Adobe Asian Font Packs for Acrobat Reader 6 */
  {
    "AdobeMingStd-Light-Acro",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (CNS1) /Supplement 4 >> \
>>",
    "<< \
/Ascent 880 /Descent -120 /StemV 66 /CapHeight 731 \
/FontBBox [-38 -121 1002 918] \
/ItalicAngle 0 /Flags 6 /XHeight 466 /AvgWidth 995 \
/Style << /Panose <000002020300000000000000> >> \
>>"
  },
  {
    "AdobeSongStd-Light-Acro",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (GB1) /Supplement 4 >> \
>>",
    "<< \
/Ascent 880 /Descent -120 /StemV 66 /CapHeight 626 \
/FontBBox [-134 -254 1001 905] \
/ItalicAngle 0 /Flags 6 /XHeight 416 /AvgWidth 996 \
/Style << /Panose <000002020300000000000000> >> \
>>"
  },
  {
    "KozMinPro-Regular-Acro",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Japan1) /Supplement 4 >> \
>>",
    "<< \
/Ascent 880 /Descent -120 /StemV 86 /CapHeight 742 \
/FontBBox [-148 -268 1104 987] \
/ItalicAngle 0 /Flags 6 /XHeight 503 /AvgWidth 890 \
/Style << /Panose <000002020400000000000000> >> \
>>"
  },
  {
    "KozGoPro-Medium-Acro",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering(Japan1) /Supplement 4 >> \
>>",
    "<< \
/Ascent 880 /Descent -120 /StemV 99 /CapHeight 763 \
/FontBBox [-149 -374 1254 999] \
/ItalicAngle 0 /Flags 4 /XHeight 549 /AvgWidth 890 \
/Style << /Panose <0000020b0700000000000000> >> \
>>"
  },
  {
    "AdobeMyungjoStd-Medium-Acro",
    "<< \
/Subtype/CIDFontType0 \
/DW 1000 \
/CIDSystemInfo << /Registry (Adobe) /Ordering (Korea1) /Supplement 2 >> \
>>",
    "<< \
/Ascent 880 /Descent -120 /StemV 99 /CapHeight 719 \
/FontBBox [-28 -148 1001 880] \
/ItalicAngle 0 /Flags 6 /XHeight 478 /AvgWidth 995 \
/Style << /Panose <000002020600000000000000> >> \
>>"
  },
  /* END */
  {NULL, NULL, NULL}
};

#endif /* _CID_BASEFONT_H_ */
