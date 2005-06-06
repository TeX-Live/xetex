/* vhea.c -- Vertical Header Table
 * Copyright (C) 1997 Li-Da Lho, All right reserved 
 */
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "ttf.h"
#include "ttfutil.h"

#ifdef MEMCHECK
#include <dmalloc.h>
#endif

/* 	$Id: vhea.c,v 1.1.1.1 1998/06/05 07:47:52 robert Exp $	 */

#ifndef lint
static char vcid[] = "$Id: vhea.c,v 1.1.1.1 1998/06/05 07:47:52 robert Exp $";
#endif /* lint */


static VHEAPtr ttfAllocVHEA(TTFontPtr font);
static void ttfLoadVHEA(FILE *fp,VHEAPtr vhea,ULONG offset);

void ttfInitVHEA(TTFontPtr font)
{
    ULONG tag = 'v' | 'h' << 8 | 'e' << 16 | 'a' << 24;
    TableDirPtr ptd;
     
    if ((ptd = ttfLookUpTableDir(tag,font)) != NULL)
	{
	    font->vhea = ttfAllocVHEA(font);
	    ttfLoadVHEA(font->fp,font->vhea,ptd->offset);
	}
}
static VHEAPtr ttfAllocVHEA(TTFontPtr font)
{
    VHEAPtr vhea;
    
    if ((vhea = (VHEAPtr) calloc(1,sizeof(VHEA))) == NULL)
	{
	    ttfError("Out of Memory in __FILE__:__LINE__\n");
	    return NULL;
	}
    return vhea;
}
static void ttfLoadVHEA (FILE *fp,VHEAPtr vhea,ULONG offset)
{
    int i;

    if (fseek(fp,offset,SEEK_SET) != 0)
	ttfError("Fseek Failed in ttfLoadVHEA \n");	
    
    vhea->version = ttfGetFixed(fp);
    vhea->ascent  = ttfGetSHORT(fp);
    vhea->descent = ttfGetSHORT(fp);
    vhea->lineGap = ttfGetSHORT(fp);
    vhea->advanceHeightMax = ttfGetSHORT(fp);
    vhea->minTopSideBearing = ttfGetSHORT(fp);
    vhea->minBottomSideBearing = ttfGetSHORT(fp);
    vhea->yMaxExtent = ttfGetSHORT(fp);
    vhea->caretSlopeRise = ttfGetSHORT(fp);
    vhea->caretSlopeRun = ttfGetSHORT(fp);
    vhea->caretOffset = ttfGetSHORT(fp);
    for(i=0;i<4;i++)
	(vhea->reserved)[i] = ttfGetSHORT(fp);
    vhea->metricDataFormat = ttfGetSHORT(fp);
    vhea->numOfLongVerMetrics = ttfGetUSHORT(fp);
}

void ttfPrintVHEA(FILE *fp,VHEAPtr vhea)
{
    int i,b[2];

    fprintf(fp,"'VHEA' - Vertical Header Table\n");
    fprintf(fp,"------------------------------\n");

    FixedSplit(vhea->version,b);
    
    fprintf(fp,"\t version:\t %d.%d\n",b[1],b[0]);
    fprintf(fp,"\t ascent:\t %d\n",vhea->ascent);
    fprintf(fp,"\t descent:\t %d\n",vhea->descent);
    fprintf(fp,"\t lineGap:\t %d\n",vhea->lineGap);
    fprintf(fp,"\t advanceHeightMax: %d\n",vhea->advanceHeightMax);
    fprintf(fp,"\t minTopSideBearing: %d\n",vhea->minTopSideBearing);
    fprintf(fp,"\t minBottomBearing: %d\n",vhea->minBottomSideBearing);
    fprintf(fp,"\t yMaxExtent:\t %d\n",vhea->yMaxExtent);
    fprintf(fp,"\t caretSlopeRise: %d\n", vhea->caretSlopeRise);
    fprintf(fp,"\t caretSlopeRun: %d\n", vhea->caretSlopeRun);
    fprintf(fp,"\t caretOffset:\t %d\n", vhea->caretOffset);
    for(i=0;i<4;i++)
	fprintf(fp,"\t reserved %d:\t %d\n",i,(vhea->reserved)[i]);
    fprintf(fp,"\t metricDataFormat:\t %d\n", vhea->metricDataFormat);
    fprintf(fp,"\t numOfLongVerMetrics: %d\n",vhea->numOfLongVerMetrics);
}

void ttfFreeVHEA(VHEAPtr vhea)
{
    if (vhea != NULL)
	{
	    free(vhea);
	}
}
