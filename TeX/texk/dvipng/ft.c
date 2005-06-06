#include "dvipng.h"
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

#define GREYS 255

dviunits SetFT(int32_t c, subpixels hh, subpixels vv) 
{
  register struct ft_char *ptr = currentfont->chr[c];
                                      /* temporary ft_char pointer */
  int red,green,blue;
  int *Color=alloca(sizeof(int)*(GREYS+1));
  int x,y;
  int pos=0;
  int bgColor,pixelcolor;
  hh -= ptr->xOffset;
  vv -= ptr->yOffset;
  
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

void LoadFT(int32_t c, struct ft_char * ptr)
{
  FT_Bitmap  bitmap;
  FT_UInt    glyph_i;
  int i,j,k;
  char* bit;

  DEBUG_PRINT(DEBUG_FT,("\n  LOAD FT CHAR\t%d (%d)",c,ptr->tfmw));
  if (currentfont->psfontmap!=NULL
      && currentfont->psfontmap->encoding != NULL)
    glyph_i = FT_Get_Name_Index(currentfont->face,
				currentfont->psfontmap->encoding->charname[c]);
  else 
    glyph_i = FT_Get_Char_Index( currentfont->face, c );
  if (FT_Load_Glyph( currentfont->face,    /* handle to face object */
		     glyph_i,              /* glyph index           */
		     FT_LOAD_RENDER | FT_LOAD_NO_HINTING ))
                                           /* load flags            */
    Fatal("can't load ft_char %d",c);
  ptr->xOffset = -currentfont->face->glyph->bitmap_left;
  ptr->yOffset = currentfont->face->glyph->bitmap_top-1;
  bitmap=currentfont->face->glyph->bitmap;
  DEBUG_PRINT(DEBUG_FT,(" (%dx%d)",bitmap.width,bitmap.rows));
    
  if ((ptr->data 
       = (char*) calloc(bitmap.width*bitmap.rows,sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);
  ptr->w = bitmap.width;
  ptr->h = bitmap.rows;

#define GREYLEVELS 16
  DEBUG_PRINT(DEBUG_GLYPH,("\nDRAW GLYPH %d\n", (int)c));
  bit=ptr->data;
  for(i=0;i<bitmap.rows;i++) {
    for(j=0;j<bitmap.width;j++) {
      k=bitmap.buffer[i*bitmap.pitch+j]/(256/GREYLEVELS)*17;
      //k=(bitmap.buffer[i*bitmap.pitch+j]+1)/16;
      //k= k>0 ? k*16-1 : 0;
      DEBUG_PRINT(DEBUG_GLYPH,("%3u ",k));
      bit[i*bitmap.width+j]=k;
    }
    DEBUG_PRINT(DEBUG_GLYPH,("|\n"));
  }
}

bool InitFT(struct font_entry * tfontp)
{
  int error;

  if (libfreetype==NULL) {
    if (FT_Init_FreeType( &libfreetype )) {
      Warning("an error occured during freetype initialisation, disabling it"); 
      flags &= ~USE_FREETYPE;
      return(false);
    } 
# ifdef DEBUG
    else {
      FT_Int      amajor, aminor, apatch;
      
      FT_Library_Version( libfreetype, &amajor, &aminor, &apatch );
      DEBUG_PRINT(DEBUG_FT,("\n  FREETYPE VERSION: FreeType %d.%d.%d", 
			    amajor, aminor, apatch));
    }
# endif
  }

  DEBUG_PRINT((DEBUG_DVI|DEBUG_FT),("\n  OPEN FT FONT:\t'%s'", tfontp->name));
  error = FT_New_Face( libfreetype, tfontp->name, 0, &tfontp->face );
  if (error == FT_Err_Unknown_File_Format) {
    Warning("font file %s has unknown format", tfontp->name);
    return(false);
  } else if (error) { 
    Warning("font file %s could not be opened", tfontp->name);
    return(false);
  } 
  Message(BE_VERBOSE,"<%s>", tfontp->name);
  if (tfontp->psfontmap == NULL || tfontp->psfontmap->encoding == NULL) {
#ifndef FT_ENCODING_ADOBE_CUSTOM
# define FT_ENCODING_ADOBE_CUSTOM ft_encoding_adobe_custom
# define FT_ENCODING_ADOBE_STANDARD ft_encoding_adobe_standard
#endif
    if (FT_Select_Charmap( tfontp->face, FT_ENCODING_ADOBE_CUSTOM )) {
      Warning("unable to set font encoding FT_ENCODING_ADOBE_CUSTOM for %s", 
	      tfontp->name);
      if(FT_Select_Charmap( tfontp->face, FT_ENCODING_ADOBE_STANDARD )) {
	Warning("unable to set font encoding for %s", tfontp->name);
	return(false);
      }
    }
  } 
  if (FT_Set_Char_Size( tfontp->face, /* handle to face object           */
			0,            /* char_width in 1/64th of points  */
			(tfontp->d*64)/65536,
			/* char_height in 1/64th of points */
			tfontp->dpi/shrinkfactor,   /* horizontal resolution */
			tfontp->dpi/shrinkfactor )) /* vertical resolution   */ {
    Warning("unable to set font size for %s", tfontp->name);
    return(false);
  }
  if (tfontp->psfontmap!=NULL)
    FT_Set_Transform(tfontp->face, tfontp->psfontmap->ft_transformp, NULL);
  tfontp->type = FONT_TYPE_FT;
  return(true);
}


void UnLoadFT(struct ft_char *ptr)
{
  if (ptr->data!=NULL)
    free(ptr->data);
  ptr->data=NULL;
}


void DoneFT(struct font_entry *tfontp)
{
  int c=0;

  int error = FT_Done_Face( tfontp->face );
  if (error)
    Warning("font file %s could not be closed", tfontp->name);
  while(c<NFNTCHARS-1) {
    if (tfontp->chr[c]!=NULL) {
      UnLoadFT((struct ft_char*)tfontp->chr[c]);
      free(tfontp->chr[c]);
      tfontp->chr[c]=NULL;
    }
    c++;
  }
  tfontp->name[0]='\0';
}


