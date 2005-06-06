#include "dvipng.h"
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

void CreateImage(pixels x_width,pixels y_width)
{
  int Background;

  if (page_imagep) 
    gdImageDestroy(page_imagep);
  if (x_width <= 0) x_width=1;
  if (y_width <= 0) y_width=1;
#ifdef HAVE_GDIMAGECREATETRUECOLOR
  if (flags & RENDER_TRUECOLOR) 
    page_imagep=gdImageCreateTrueColor(x_width,y_width);
  else
#endif
    page_imagep=gdImageCreate(x_width,y_width);
  /* Set bg color */
  Background = gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue);
  if (borderwidth<0) {
    gdImageColorTransparent(page_imagep,Background); 
  }
#ifdef HAVE_GDIMAGECREATETRUECOLOR
  if (flags & RENDER_TRUECOLOR) 
    /* Truecolor: there is no background color index, fill image instead. */
    gdImageFilledRectangle(page_imagep, 0, 0, 
			   x_width-1, y_width-1, Background);
#endif
  if (borderwidth>0) {
    int Transparent;
    
    /* Set ANOTHER bg color, transparent this time */
    Transparent = gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue); 
    gdImageColorTransparent(page_imagep,Transparent); 
    gdImageFilledRectangle(page_imagep,0,0,
			   x_width-1,borderwidth-1,Transparent);
    gdImageFilledRectangle(page_imagep,0,0,
			   borderwidth-1,y_width-1,Transparent);
    gdImageFilledRectangle(page_imagep,x_width-borderwidth,0,
			   x_width-1,y_width-1,Transparent);
    gdImageFilledRectangle(page_imagep,0,y_width-borderwidth,
			   x_width-1,y_width-1,Transparent);
  }
}

void WriteImage(char *pngname, int pagenum)
{
  char* pos;
  FILE* outfp=NULL;

  if ((pos=strchr(pngname,'%')) != NULL) {
    if (strchr(++pos,'%'))
      Fatal("too many %%'s in output file name");
    if (*pos == 'd' || strncmp(pos,"03d",3)==0) {
      /* %d -> pagenumber, so add 9 string positions 
	 since pagenumber max +-2^31 or +-2*10^9 */
      char* tempname = alloca(strlen(pngname)+9);
      sprintf(tempname,pngname,pagenum);
      pngname = tempname;
    } else {
      Fatal("unacceptible format spec. in output file name");
    }
  }
  if ((outfp = fopen(pngname,"wb")) == NULL)
      Fatal("Cannot open output file %s",pngname);
#ifdef HAVE_GDIMAGEPNGEX
  gdImagePngEx(page_imagep,outfp,compression);
#else
  gdImagePng(page_imagep,outfp);
#endif
  fclose(outfp);
  DEBUG_PRINT(DEBUG_DVI,("\n  WROTE:   \t%s\n",pngname));
  gdImageDestroy(page_imagep);
  page_imagep=NULL;
}



/*-->SetRule*/
/**********************************************************************/
/*****************************  SetRule  ******************************/
/**********************************************************************/
/*   this routine will draw a rule */
dviunits SetRule(dviunits a, dviunits b, subpixels hh,subpixels vv)
{
  int Color;
  pixels    width=0, height=0;

  if ( a > 0 && b > 0 ) {
    /* Calculate width and height, round up */
    width = (b+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    height = (a+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
  }
  if (page_imagep != NULL) {
    if ((height>0) && (width>0)) {
      /* This code produces too dark rules. But what the hell. Grey
       * rules look fuzzy.
       */
      Color = gdImageColorResolve(page_imagep, Red,Green,Blue);
      /* +1 and -1 are because the Rectangle coords include last pixels */
      gdImageFilledRectangle(page_imagep,hh,vv-height+1,hh+width-1,vv,Color);
      DEBUG_PRINT(DEBUG_DVI,("\n  RULE \t%dx%d at (%d,%d)",
		   width, height, hh, vv));
    }
  } else {
    /* The +1's are because things are cut _at_that_coordinate_. */
    min(x_min,hh);
    min(y_min,vv-height+1);
    max(x_max,hh+width);
    max(y_max,vv+1);
  }
  return(b);
}


#if 0
  if (page_imagep != NULL) {
    if ((a>0) && (b>0)) {
      int width,height,left=-1,right=-1,bottom=-1,top=-1;
      gdImagePtr rule;
      

      width  = (h+b+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor
	- h/dvi->conv/shrinkfactor;
      /* Calculate height, round up on the bottom and down on the top */
      height = (v+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor
	- (v-a)/dvi->conv/shrinkfactor;
      rule = gdImageCreate(width,height);
      /* Set background */
      gdImageColorAllocate(rule,Red,Green,Blue);
      
      /* Calculate blackness of edges of wide rule */
      left   = shrinkfactor - h/dvi->conv%shrinkfactor;
      right  = (h+b+dvi->conv-1)/dvi->conv%shrinkfactor;
      if (right==0)  right  = shrinkfactor;
      if (width==1) {
	/* Adjust blackness of narrow rule */
	left = shrinkfactor;
	/* The (correct) alternative produces "fuzzy" tables */ 
	/* left = ((h+b+dvi->conv-1)/dvi->conv - h/dvi->conv)%shrinkfactor;
	 */
      } 
      if (width==2 && right+left <= shrinkfactor +1) {
	/* Adjust blackness of narrow rule when spread across two columns */
	/* Cheat to make it sharp (+1 above, and wedging it around below) */
	if (right>left) { right=shrinkfactor; left=0; }
	if (right<left) { left=shrinkfactor; right=0; }
	if (right==left) { left=right=(shrinkfactor+1)/2; }
      }

      /* Calculate blackness of edges of tall rule */
      bottom = (v+dvi->conv-1)/dvi->conv%shrinkfactor;
      if (bottom==0) bottom = shrinkfactor;
      top    = shrinkfactor-(v-a)/dvi->conv%shrinkfactor;
      if (height == 1) {
	/* Adjust blackness of short rule */
	bottom = shrinkfactor;
	/* The (correct) alternative produces "fuzzy" tables */ 
	/* bottom = ((v+dvi->conv-1)/dvi->conv - (v-a)/dvi->conv)%shrinkfactor;
	 */
      } 
      if (height==2 && top+bottom <= shrinkfactor +1) {
	/* Adjust blackness of short rule when spread across two columns */
	/* Cheat to make it sharp (+1 above, and wedging it around below) */
	if (top>bottom) { top=shrinkfactor; bottom=0; }
	if (top<bottom) { bottom=shrinkfactor; top=0; }
	if (right==left) { left=right=(shrinkfactor+1)/2; }
      }
	
      Color = gdImageColorResolve(rule,
		  bRed-(bRed-Red)*left*bottom/shrinkfactor/shrinkfactor,
		  bGreen-(bGreen-Green)*left*bottom/shrinkfactor/shrinkfactor,
		  bBlue-(bBlue-Blue)*left*bottom/shrinkfactor/shrinkfactor);
      gdImageSetPixel(rule,0,height-1,Color);
      if (width>1) {
	Color = gdImageColorResolve(rule, 
		  bRed-(bRed-Red)*right*bottom/shrinkfactor/shrinkfactor,
		  bGreen-(bGreen-Green)*right*bottom/shrinkfactor/shrinkfactor,
		  bBlue-(bBlue-Blue)*right*bottom/shrinkfactor/shrinkfactor);
	gdImageSetPixel(rule,width-1,height-1,Color);
      }
      if (height>1) {
	Color = gdImageColorResolve(rule, 
	       	    bRed-(bRed-Red)*left*top/shrinkfactor/shrinkfactor,
		    bGreen-(bGreen-Green)*left*top/shrinkfactor/shrinkfactor,
		    bBlue-(bBlue-Blue)*left*top/shrinkfactor/shrinkfactor);
	gdImageSetPixel(rule,0,0,Color);
      }
      if (height>1 && width>1) {
	Color = gdImageColorResolve(rule, 
		    bRed-(bRed-Red)*right*top/shrinkfactor/shrinkfactor,
		    bGreen-(bGreen-Green)*right*top/shrinkfactor/shrinkfactor,
		    bBlue-(bBlue-Blue)*right*top/shrinkfactor/shrinkfactor);
	gdImageSetPixel(rule,width-1,0,Color);
      }

      if (width>2) {
	Color = gdImageColorResolve(rule, 
				    bRed-(bRed-Red)*bottom/shrinkfactor,
				    bGreen-(bGreen-Green)*bottom/shrinkfactor,
				    bBlue-(bBlue-Blue)*bottom/shrinkfactor);
	gdImageFilledRectangle(rule,1,height-1,width-2,height-1,Color);
      }
      if (height>2) {
	Color = gdImageColorResolve(rule, 
				    bRed-(bRed-Red)*left/shrinkfactor,
				    bGreen-(bGreen-Green)*left/shrinkfactor,
				    bBlue-(bBlue-Blue)*left/shrinkfactor);
	gdImageFilledRectangle(rule,0,1,0,height-2,Color);
      }
      if (height>1 && width>2) {
	Color = gdImageColorResolve(rule, 
				    bRed-(bRed-Red)*top/shrinkfactor,
				    bGreen-(bGreen-Green)*top/shrinkfactor,
				    bBlue-(bBlue-Blue)*top/shrinkfactor);
	gdImageFilledRectangle(rule,1,0,width-2,0,Color);
      }
      if (height>2 && width>1) {
	Color = gdImageColorResolve(rule, 
				    bRed-(bRed-Red)*right/shrinkfactor,
				    bGreen-(bGreen-Green)*right/shrinkfactor,
				    bBlue-(bBlue-Blue)*right/shrinkfactor);
	gdImageFilledRectangle(rule,width-1,1,width-1,height-2,Color);
      }
      gdImageCopy(page_imagep,rule,
		  h/dvi->conv/shrinkfactor,
		  (v-a)/dvi->conv/shrinkfactor,
		  0,0,width,height);
      DEBUG_PRINT(DEBUG_DVI,("\n  RULE \t%dx%d at (%d,%d)",
		   width,height,
		   PIXROUND(h, dvi->conv*shrinkfactor),
		   PIXROUND(v, dvi->conv*shrinkfactor)));
      DEBUG_PRINT(DEBUG_DVI,(" (lrtb %d %d %d %d)",left,right,top,bottom));
    }
  } else {
    min(x_min,hh);
    min(y_min,vv-height+1);
    max(x_max,hh+width);
    max(y_max,vv+1);
  }
#endif


