#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "ttf.h"
#include "ttfutil.h"

#define __TTF_OS2
#include "os2_P.h"

#ifdef MEMCHECK
#include <dmalloc.h>
#endif

/* 	$Id: os2.c,v 1.2 1998/07/06 06:07:01 werner Exp $	 */

#ifndef lint
static char vcid[] = "$Id: os2.c,v 1.2 1998/07/06 06:07:01 werner Exp $";
#endif /* lint */

static OS_2Ptr ttfAllocOS2(TTFontPtr font);
static void ttfLoadOS2(FILE *fp, OS_2Ptr os2, ULONG offset);

void ttfInitOS2(TTFontPtr font)
{
    ULONG tag = 'O' | 'S' << 8 | '/' << 16 | '2' << 24;
    TableDirPtr ptd;

    if ((ptd = ttfLookUpTableDir(tag, font)) != NULL)
	{
	    font->os2 = ttfAllocOS2(font);
	    ttfLoadOS2(font->fp, font->os2, ptd->offset);
	}   
}

static OS_2Ptr ttfAllocOS2(TTFontPtr font)
{
    OS_2Ptr os2;

    if ((os2 = (OS_2Ptr) calloc(1, sizeof(OS_2))) == NULL)
	{
	    ttfError("Out of Memofy in __FILE__:__LINE__\n");
	    return NULL;
	}
    return os2;
}

static void ttfLoadOS2(FILE *fp, OS_2Ptr os2, ULONG offset)
{
    if (fseek(fp, offset, SEEK_SET) != 0)
	ttfError("Fseek Failed in ttfLoadGLYF\n");

    os2->version = ttfGetUSHORT(fp);
    os2->xAvgCharWidth = ttfGetSHORT(fp);
    os2->usWeightClass = ttfGetUSHORT(fp);
    os2->usWidthClass = ttfGetUSHORT(fp);
    os2->fsType = ttfGetUSHORT(fp);
    os2->ySubscriptXSize = ttfGetSHORT(fp);
    os2->ySubscriptYSize = ttfGetSHORT(fp);
    os2->ySubscriptXOffset = ttfGetSHORT(fp);
    os2->ySubscriptYOffset = ttfGetSHORT(fp);
    os2->ySuperscriptXSize = ttfGetSHORT(fp);
    os2->ySuperscriptYSize = ttfGetSHORT(fp);
    os2->ySuperscriptXOffset = ttfGetSHORT(fp);
    os2->ySuperscriptYOffset = ttfGetSHORT(fp);
    os2->yStrikeoutSize = ttfGetSHORT(fp);
    os2->yStrikeoutPosition = ttfGetSHORT(fp);
    os2->sFamilyClass = ttfGetSHORT(fp);

    if (fread(os2->panose, sizeof(CHAR), 10, fp) != 10)
	ttfError("Error readind PANOSE\n");

    os2->ulUnicodeRange1 = ttfGetULONG(fp);
    os2->ulUnicodeRange2 = ttfGetULONG(fp);
    os2->ulUnicodeRange3 = ttfGetULONG(fp);
    os2->ulUnicodeRange4 = ttfGetULONG(fp);

    if (fread(os2->achVendID, sizeof(CHAR), 4, fp) != 4)
	ttfError("Error readind achVendID\n");
    os2->achVendID[4] = 0x0;

    os2->fsSelection = ttfGetUSHORT(fp);
    os2->usFirstCharIndex = ttfGetUSHORT(fp);
    os2->usLastCharIndex = ttfGetUSHORT(fp);
    os2->sTypoAscender = ttfGetSHORT(fp);
    os2->sTypoDescender = ttfGetSHORT(fp);
    os2->sTypoLineGap = ttfGetSHORT(fp);
    os2->usWinAscent = ttfGetUSHORT(fp);
    os2->usWinDescent = ttfGetUSHORT(fp);
    os2->ulCodePageRange1 = ttfGetULONG(fp);
    os2->ulCodePageRange2 = ttfGetULONG(fp);
}

void ttfPrintOS2(FILE *fp,OS_2Ptr os2)
{
    char buf[80];
    
    fprintf(fp,"'OS/2' Table - OS/2 and Windows Metrics\n");
    fprintf(fp,"---------------------------------------\n");

    fprintf(fp,"\t 'OS/2' version:\t %d\n",os2->version);
    fprintf(fp,"\t xAvgCharWidth:\t\t %d\n",os2->xAvgCharWidth);
    fprintf(fp,"\t usWeightClass:\t\t %d \t '%s'\n",os2->usWeightClass,
	    WeightClassName[os2->usWeightClass/100 - 1] );
    fprintf(fp,"\t usWidthClass:\t\t %d \t '%s'\n",os2->usWidthClass,
	    WidthClassName[os2->usWidthClass - 1]);
    fprintf(fp,"\t fsType:\t\t %d\n",os2->fsType);
    fprintf(fp,"\t ySubscriptXSize:\t %d\n",os2->ySubscriptXSize);
    fprintf(fp,"\t ySubscriptYSize:\t %d\n",os2->ySubscriptYSize);
    fprintf(fp,"\t ySubscriptXOffset:\t %d\n",os2->ySubscriptXOffset);
    fprintf(fp,"\t ySubscriptYOffset:\t %d\n",os2->ySubscriptYOffset);
    fprintf(fp,"\t ySuperscriptXSize:\t %d\n",os2-> ySuperscriptXSize);
    fprintf(fp,"\t ySuperscriptYSize:\t %d\n",os2->ySuperscriptYSize);
    fprintf(fp,"\t ySuperscriptXOffset:\t %d\n",os2->ySuperscriptXOffset);
    fprintf(fp,"\t ySuperscriptYOffset:\t %d\n",os2->ySuperscriptYOffset);
    fprintf(fp,"\t yStrikeoutSize:\t %d\n",os2->yStrikeoutSize);
    fprintf(fp,"\t yStrikeoutPosition\t %d\n",os2->yStrikeoutPosition);
    fprintf(fp,"\t sFamilyClass:\t %d \t subclass = %d\n",
	    (os2->sFamilyClass) >> 8,(os2->sFamilyClass) & 0x00ff);
    fprintf(fp,"\t PANOSE:\n");
    fprintf(fp,"\t\t Family Kind:\t %d \t '%s'\n",os2->panose[0],
	    PanoseFamily[os2->panose[0]]);
    fprintf(fp,"\t\t Serif Style:\t %d \t '%s'\n",os2->panose[1],
	    PanoseSerif[os2->panose[1]]);
    fprintf(fp,"\t\t Weight:\t %d \t '%s'\n",os2->panose[2],
	    PanoseWeight[os2->panose[2]]);
    fprintf(fp,"\t\t Proportion:\t %d \t '%s'\n",os2->panose[3],
	    PanoseProportion[os2->panose[3]]);
    fprintf(fp,"\t\t Contrast:\t %d \t '%s'\n",os2->panose[4],
	    PanoseContrast[os2->panose[4]]);
    fprintf(fp,"\t\t Stroke:\t %d \t '%s'\n",os2->panose[5],
	    PanoseStroke[os2->panose[5]]);
    fprintf(fp,"\t\t Arm Style:\t %d \t '%s'\n",os2->panose[6],
	    PanoseArm[os2->panose[6]]);
    fprintf(fp,"\t\t Lettreform:\t %d \t '%s'\n",os2->panose[7],
	    PanoseLetterform[os2->panose[7]]);
    fprintf(fp,"\t\t Midline:\t %d \t '%s'\n",os2->panose[8],
	    PanoseMidline[os2->panose[8]]);
    fprintf(fp,"\t\t X-height:\t %d \t '%s'\n",os2->panose[9],
	    PanoseXHeight[os2->panose[9]]);
    fprintf(fp,"\t Unicode Range 1( Bits 0 - 31 ): \t 0x%08x\n",
	    os2->ulUnicodeRange1);
    fprintf(fp,"\t Unicode Range 2( Bits 32 - 63 ): \t 0x%08x\n",
	    os2->ulUnicodeRange2);
    fprintf(fp,"\t Unicode Range 3( Bits 64 - 95 ): \t 0x%08x\n",
	    os2->ulUnicodeRange3);
    fprintf(fp,"\t Unicode Range 4( Bits 96 - 128 ): \t 0x%08x\n",
	    os2->ulUnicodeRange4);
  
    fprintf(fp,"\t achVendID:\t\t '%s'\n",os2->achVendID);

    buf[0] = 0x0;
    if (os2->fsSelection & FS_FLAGS_REGULAR)
	{
	    strcat(buf,"Regular ");
	}
    else
	{
	    if (os2->fsSelection & FS_FLAGS_BOLD)
		strcat(buf,"Bold ");
	    if (os2->fsSelection & FS_FLAGS_ITALIC)
		strcat(buf,"Italic ");
	}
    fprintf(fp,"\t fsSelection:\t\t 0x%04x \t '%s'\n",os2->fsSelection,buf);
    fprintf(fp,"\t usFirstCharIndex:\t 0x%04x\n ",os2->usFirstCharIndex);
    fprintf(fp,"\t usLastCharIndex:\t 0x%04x\n",os2->usLastCharIndex);
    fprintf(fp,"\t sTypoAscender:\t\t %d\n",os2->sTypoAscender);
    fprintf(fp,"\t sTypoDescender:\t %d\n",os2->sTypoDescender);
    fprintf(fp,"\t sTypoLineGap:\t\t %d\n",os2->sTypoLineGap);
    fprintf(fp,"\t usWinAscent:\t\t %d\n",os2->usWinAscent);
    fprintf(fp,"\t usWinDescent:\t\t %d\n",os2->usWinDescent);
    fprintf(fp,"\t CodePage Range 1( Bits 0 - 31 ):\t 0x%08x\n",
	    os2->ulCodePageRange1);
    fprintf(fp,"\t CodePage Range 2( Bits 32- 63 ):\t 0x%08x\n",
	    os2->ulCodePageRange2);
}

void ttfFreeOS2(OS_2Ptr os2)
{
    free(os2);
}
