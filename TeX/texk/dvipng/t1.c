#include "dvipng.h"
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

#define GREYS 255

dviunits SetT1(int32_t c, subpixels hh, subpixels vv) 
{
  register struct t1_char *ptr = currentfont->chr[c];
                                      /* temporary t1_char pointer */
  int red,green,blue;
  int *Color=alloca(sizeof(int)*(GREYS+1));
  int x,y;
  int pos=0;
  int bgColor,pixelcolor;

  hh -= ptr->xOffset/shrinkfactor;
  vv -= ptr->yOffset/shrinkfactor;

  Color[0] = gdImageColorResolve(page_imagep,bRed,bGreen,bBlue);
  for( x=1; x<=GREYS ; x++) 
    Color[x] = -1;
  for( y=0; y<ptr->h; y++) {
    for( x=0; x<ptr->w; x++) {
      if (ptr->data[pos]>0) {
	bgColor = gdImageGetPixel(page_imagep, hh + x, vv + y);
	if (bgColor == Color[0]) {
	  /* Standard background: use cached value if present */
	  pixelcolor=Color[(int)ptr->data[pos]];
	  if (pixelcolor==-1) {
	    red = bRed-(bRed-Red)*ptr->data[pos]/GREYS;
	    green = bGreen-(bGreen-Green)*ptr->data[pos]/GREYS;
	    blue = bBlue-(bBlue-Blue)*ptr->data[pos]/GREYS;
	    Color[ptr->data[pos]] = 
	      gdImageColorResolve(page_imagep,red,green,blue);
	    pixelcolor=Color[ptr->data[pos]];
	  }
	} else {
	  /* Overstrike: No cache */
	  red=gdImageRed(page_imagep, bgColor);
	  green=gdImageGreen(page_imagep, bgColor);
	  blue=gdImageBlue(page_imagep, bgColor);
	  red = red-(red-Red)*ptr->data[pos]/GREYS;
	  green = green-(green-Green)*ptr->data[pos]/GREYS;
	  blue = blue-(blue-Blue)*ptr->data[pos]/GREYS;
	  pixelcolor = gdImageColorResolve(page_imagep, red, green, blue);
	}
	gdImageSetPixel(page_imagep, hh + x, vv + y, pixelcolor);
      }
      pos++;
    }
  }
  return(ptr->tfmw);
}

void LoadT1(int32_t c, struct t1_char * ptr)
{
  GLYPH *glyph;
  int original_width,original_height;
  int i,j,k,width,height;
  unsigned short   shrunk_width,shrunk_height;
  short   xoffset,yoffset;
  unsigned short   i_offset,j_offset;

  DEBUG_PRINT(DEBUG_T1,("\n  LOAD T1 CHAR\t%d",c));
  if ((glyph=
       T1_SetChar( currentfont->T1id, c, (float)currentfont->dpi*currentfont->d/65536/72 ,
		   currentfont->psfontmap==NULL ? NULL : currentfont->psfontmap->t1_transformp))
      ==NULL)
      Fatal("can't load t1_char %d",c);

  DEBUG_PRINT(DEBUG_T1,(" (%d)",ptr->tfmw));
  
  original_width = glyph->metrics.rightSideBearing 
    - glyph->metrics.leftSideBearing;
  original_height  = glyph->metrics.ascent - glyph->metrics.descent;
  DEBUG_PRINT(DEBUG_T1,(" %dx%d",original_width,original_height));

  if (original_width > 0x7fff || original_height > 0x7fff)
    Fatal("Character %d too large in file %s", c, currentfont->name);

  /* 
   * Hotspot issues: Shrinking to the topleft corner rather than the
     hotspot will displace glyphs a fraction of a pixel. We deal with
     this in as follows: The glyph is shrunk to its hotspot by
     offsetting the bitmap somewhat to put the hotspot in the lower
     left corner of a "shrink square". Shrinking to the topleft corner
     will then act as shrinking to the hotspot. This may enlarge the
     bitmap somewhat, of course.  (Also remember that the below
     calculation of i/j_offset is in integer arithmetics.)
     
     There will still be a displacement from rounding the dvi
     position, but vertically it will be equal for all glyphs on a
     line, so we displace a whole line vertically by fractions of a
     pixel. This is acceptible, IMHO. Sometime there will be support
     for subpixel positioning, horizontally. Will do for now, I
     suppose.
   */
  xoffset = -glyph->metrics.leftSideBearing;
  //printf("xoffset: %d\n",xoffset);
  i_offset = ( shrinkfactor - xoffset % shrinkfactor ) % shrinkfactor;
  width = original_width+i_offset;
  ptr->xOffset = xoffset+i_offset;

  yoffset = glyph->metrics.ascent-1;
  //printf("yoffset: %d\n",yoffset);
  j_offset = ( shrinkfactor - (yoffset-(shrinkfactor-1)) % shrinkfactor )
    % shrinkfactor;
  height = original_height+j_offset;
  ptr->yOffset = yoffset+j_offset;

  DEBUG_PRINT(DEBUG_T1,(" (%dx%d)",width,height));
  /* 
     Extra marginal so that we do not crop the image when shrinking.
  */
  shrunk_width = (width + shrinkfactor - 1) / shrinkfactor;
  shrunk_height = (height + shrinkfactor - 1) / shrinkfactor;
  ptr->w = shrunk_width;
  ptr->h = shrunk_height;

  //printf("(%d,%d) ",ptr->w,ptr->h);
  DEBUG_PRINT(DEBUG_GLYPH,("\nDRAW GLYPH %d\n", (int)c));

  /*
    Shrink raster while doing antialiasing. 
  */
  if ((ptr->data = calloc(shrunk_width*shrunk_height,sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);
  for (j = 0; j < original_height; j++) {	
    for (i = 0; i < (original_width+7)/8 ; i++) {    
      for (k = 0; k < 8 ; k++) {
	DEBUG_PRINT(DEBUG_GLYPH,
		    ("%c",
		     (glyph->bits[i+j*((original_width+7)/8)] & (1<<k)) ?
		      'x' : ' '));
	ptr->data[(i*8+k+i_offset)/shrinkfactor
		  +(j+j_offset)/shrinkfactor*shrunk_width] += 
	  (glyph->bits[i+j*((original_width+7)/8)] & (1<<k) ? 1 : 0);
      }
    }    
    DEBUG_PRINT(DEBUG_GLYPH,("|\n"));
  }	
  for (j = 0; j < shrunk_height; j++) {	
    for (i = 0; i < shrunk_width; i++) {    
      ptr->data[i+j*shrunk_width] = ptr->data[i+j*shrunk_width]
	*255/shrinkfactor/shrinkfactor;
      DEBUG_PRINT(DEBUG_GLYPH,("%3u ",ptr->data[i+j*shrunk_width]));
    }
    DEBUG_PRINT(DEBUG_GLYPH,("|\n"));
  }	 
}

bool InitT1(struct font_entry * tfontp)
{
  if (libt1==NULL) {
    if ((libt1=T1_InitLib( NO_LOGFILE | IGNORE_CONFIGFILE
			   | IGNORE_FONTDATABASE | T1_NO_AFM)) == NULL) {
      Warning("an error occured during t1lib initialisation, disabling it"); 
      flags &= ~USE_LIBT1;
      return(false);
    }
# ifdef DEBUG
    else 
      DEBUG_PRINT(DEBUG_T1,("\n  T1LIB VERSION: %s", T1_GetLibIdent()));
# endif
  }

  DEBUG_PRINT((DEBUG_DVI|DEBUG_T1),("\n  OPEN T1 FONT:\t'%s'", tfontp->name));
  tfontp->T1id = T1_AddFont( tfontp->name );
  if (tfontp->T1id < 0) {
    Warning("t1lib could not open font file %s", tfontp->name);
    return(false);
  } 
  if (T1_LoadFont(tfontp->T1id)) {
    Warning("t1lib could not load font file %s", tfontp->name);
    return(false);
  } 
  Message(BE_VERBOSE,"<%s>", tfontp->name);
  if (tfontp->psfontmap!=NULL && tfontp->psfontmap->encoding != NULL) {
    DEBUG_PRINT(DEBUG_T1,("\n  USE ENCODING:\t'%s'", tfontp->psfontmap->encoding->name));
    if (T1_ReencodeFont(tfontp->T1id,tfontp->psfontmap->encoding->charname)) {
      Warning("unable to use font encoding '%s' for %s", 
	      tfontp->psfontmap->encoding->name,tfontp->name);
      return(false);
    }
  }
  tfontp->type = FONT_TYPE_T1;
  return(true);
}


void UnLoadT1(struct t1_char *ptr)
{
  if (ptr->data!=NULL)
    free(ptr->data);
  ptr->data=NULL;
}


void DoneT1(struct font_entry *tfontp)
{
  int c=0;

  int error = T1_DeleteFont( tfontp->T1id );
  if (error)
    Warning("font file %s could not be closed", tfontp->name);
  while(c<NFNTCHARS-1) {
    if (tfontp->chr[c]!=NULL) {
      UnLoadT1((struct t1_char*)tfontp->chr[c]);
      free(tfontp->chr[c]);
      tfontp->chr[c]=NULL;
    }
    c++;
  }
  tfontp->name[0]='\0';
}


