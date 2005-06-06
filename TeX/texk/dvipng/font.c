#include "dvipng.h"

void CheckChecksum(uint32_t c1, uint32_t c2, const char* name)
{
  /* Report a warning if both checksums are nonzero, they don't match,
     and the user hasn't turned it off.  */
  if (c1 && c2 && c1 != c2
#ifdef HAVE_LIBKPATHSEA
      && !kpse_tex_hush ("checksum")
#endif
      ) {
     Warning ("Checksum mismatch in %s", name) ;
   }
}


double ActualFactor(uint32_t unmodsize)
/* compute the actual size factor given the approximation */
/* actually factor * 1000 */
{
  double  realsize;     /* the actual magnification factor */
  realsize = (double)unmodsize / 1000.0;
  if (abs((int)(unmodsize - 1095l))<2)
    realsize = 1.095445115; /*stephalf*/
  else if (abs((int)(unmodsize - 1315l))<2)
    realsize = 1.31453414; /*stepihalf*/
  else if (abs((int)(unmodsize - 1577l))<2)
    realsize = 1.57744097; /*stepiihalf*/
  else if (abs((int)(unmodsize - 1893l))<2)
    realsize = 1.89292916; /*stepiiihalf*/
  else if (abs((int)(unmodsize - 2074l))<2)
    realsize = 2.0736;   /*stepiv*/
  else if (abs((int)(unmodsize - 2488l))<2)
    realsize = 2.48832;  /*stepv*/
  else if (abs((int)(unmodsize - 2986l))<2)
    realsize = 2.985984; /*stepvi*/
  /* the remaining magnification steps are represented with sufficient
     accuracy already */
  return(realsize);
}


void FontDef(unsigned char* command, void* parent)
{
  int32_t k;
  uint32_t   c, s, d;
  uint8_t    a, l;
  unsigned char* current;
  struct font_entry *tfontptr; /* temporary font_entry pointer   */
  struct font_num *tfontnump = NULL;  /* temporary font_num pointer   */
  unsigned short i;

  current = command + 1;
  k = UNumRead(current, (int)*command - FNT_DEF1 + 1);
  current += (int)*command - FNT_DEF1 + 1;
  c = UNumRead(current, 4); /* checksum */
  s = UNumRead(current+4, 4); /* space size */
  d = UNumRead(current+8, 4); /* design size */
  a = UNumRead(current+12, 1); /* length for font name */
  l = UNumRead(current+13, 1); /* device length */
  if (((struct font_entry*)parent)->type==FONT_TYPE_VF) {
    DEBUG_PRINT(DEBUG_VF,(" %d %d %d",k,c,s));
    /* Rescale. s is relative to the actual scale /(1<<20) */
    s = (uint32_t)((uint64_t) s * (((struct font_entry*) parent)->s) 
		   / (1<<20));
    DEBUG_PRINT(DEBUG_VF,(" (%d) %d",s,d));
    /* Oddly, d differs in the DVI and the VF that my system produces */
    d = (uint32_t)((uint64_t) d * ((struct font_entry*)parent)->d
		   / ((struct font_entry*)parent)->designsize);
    DEBUG_PRINT(DEBUG_VF,(" (%d)",d));
    DEBUG_PRINT(DEBUG_VF,(" %d %d '%.*s'",a,l,a+l,current+14));
#ifdef DEBUG
  } else {
    DEBUG_PRINT(DEBUG_DVI,(" %d %d %d %d %d %d '%.*s'",k,c,s,d,a,l,
		 a+l,current+14));
#endif
  }
  if (a+l > STRSIZE-1)
    Fatal("too long font name for font %ld\n",k);

  /* Find entry with this font number in use */
  switch (((struct font_entry*)parent)->type) {
  case FONT_TYPE_VF:
    tfontnump = ((struct font_entry*)parent)->vffontnump;
    break;
  case DVI_TYPE:
    tfontnump = ((struct dvi_data*)parent)->fontnump;
  }
  while (tfontnump != NULL && tfontnump->k != k) {
    tfontnump = tfontnump->next;
  }
  /* If found, return if it is correct */
  if (tfontnump!=NULL 
      && tfontnump->fontp->s == s 
      && tfontnump->fontp->d == d 
      && strncmp(tfontnump->fontp->n,current+14,a+l) == 0) {
    DEBUG_PRINT((DEBUG_DVI|DEBUG_VF),("\n  FONT %d:\tMatch found",k));
    return;
  }
  /* If not found, create new */
  if (tfontnump==NULL) {
    if ((tfontnump=malloc(sizeof(struct font_num)))==NULL) 
      Fatal("cannot allocate memory for new font number");
    tfontnump->k=k;
    switch (((struct font_entry*)parent)->type) {
    case FONT_TYPE_VF:
      tfontnump->next=((struct font_entry*)parent)->vffontnump;
      ((struct font_entry*)parent)->vffontnump=tfontnump;
      break;
    case DVI_TYPE:
      tfontnump->next=((struct dvi_data*)parent)->fontnump;
      ((struct dvi_data*)parent)->fontnump=tfontnump;
    }
  }

  /* Search font list for possible match */
  tfontptr = hfontptr;
  while (tfontptr != NULL 
	 && (tfontptr->s != s 
	     || tfontptr->d != d 
	     || strncmp(tfontptr->n,current+14,a+l) != 0 ) ) {
    tfontptr = tfontptr->next;
  }
  /* If found, set its number and return */
  if (tfontptr!=NULL) {
    DEBUG_PRINT((DEBUG_DVI|DEBUG_VF),("\n  FONT %d:\tMatch found, number set",k));
    tfontnump->fontp = tfontptr; 
    return;
  }

  DEBUG_PRINT((DEBUG_DVI|DEBUG_VF),("\n  FONT %d:\tNew entry created",k));
  /* No fitting font found, create new entry. */
  if ((tfontptr = calloc(1,sizeof(struct font_entry))) == NULL)
    Fatal("can't malloc space for font_entry");
  tfontptr->next = hfontptr;
  hfontptr = tfontptr;
  tfontnump->fontp = tfontptr;
  tfontptr->filedes = 0;
  tfontptr->c = c; /* checksum */
  tfontptr->s = s; /* space size */
  tfontptr->d = d; /* design size */
  tfontptr->a = a; /* length for font name */
  tfontptr->l = l; /* device length */
  strncpy(tfontptr->n,current+14,a+l); /* full font name */
  tfontptr->n[a+l] = '\0';
  
  tfontptr->name[0] = '\0';
  for (i = FIRSTFNTCHAR; i <= LASTFNTCHAR; i++) {
    tfontptr->chr[i] = NULL;
  }

  tfontptr->dpi = 
    (uint32_t)((ActualFactor((uint32_t)(1000.0*tfontptr->s
				  /(double)tfontptr->d+0.5))
	     * ActualFactor(dvi->mag) * dpi*shrinkfactor) + 0.5);
}

#ifdef HAVE_FT2_OR_LIBT1
inline char* kpse_find_t1_or_tt(char* filename) 
{
    char* filepath = kpse_find_file(filename, kpse_type1_format, false);
#ifdef HAVE_FT2
    if ((flags & USE_FREETYPE) && filepath==NULL) 
      filepath = kpse_find_file(filename, kpse_truetype_format, false);
#endif
    return(filepath);
}
#endif

void FontFind(struct font_entry * tfontptr)
{
#ifdef HAVE_LIBKPATHSEA
  kpse_glyph_file_type font_ret;
  char *name;

  //tfontptr->dpi = kpse_magstep_fix (tfontptr->dpi, resolution, NULL);
  DEBUG_PRINT(DEBUG_DVI,("\n  FIND FONT:\t%s %d",tfontptr->n,tfontptr->dpi));

  name = kpse_find_vf (tfontptr->n);
  if (name!=NULL) {
    strcpy (tfontptr->name, name);
    free (name);
    InitVF(tfontptr);
  }
#ifdef HAVE_FT2_OR_LIBT1
  if ((flags & (USE_FREETYPE | USE_LIBT1)) && name==NULL) {
    tfontptr->psfontmap = FindPSFontMap(tfontptr->n);
    if (tfontptr->psfontmap!=NULL) {
      name = kpse_find_t1_or_tt(tfontptr->psfontmap->psfile);
    } else
      name = kpse_find_t1_or_tt(tfontptr->n);
    if (name!=NULL) {
      strcpy (tfontptr->name, name);
      free (name);
      name = kpse_find_file(tfontptr->n, kpse_tfm_format, false);
      if (name!=NULL) {
	if (!ReadTFM(tfontptr,name)) {
	  Warning("unable to read tfm file %s", name);
	  free(name);
	  name=NULL;
	} else 
#ifdef HAVE_FT2
	  if ((flags & USE_FREETYPE)==0 || !InitFT(tfontptr)) {
#endif
#ifdef HAVE_LIBT1
	    if ((flags & USE_LIBT1)==0 || !InitT1(tfontptr)) {
#endif
	      /* if Freetype or T1 loading fails for some reason, fall
		 back to PK font */
	      free(name);
	      name=NULL; 
	    } else
	      free(name);
#ifdef HAVE_FT2
#ifdef HAVE_LIBT1
	  } else
	    free(name);
#endif
#endif
      }
    }
  }
#endif /* HAVE_FT2_OR_LIBT1 */
  if (name==NULL) {
    name = kpse_find_pk (tfontptr->n, tfontptr->dpi, &font_ret);
    if (name!=NULL) {
      strcpy (tfontptr->name, name);
      free (name);
      
      if (!FILESTRCASEEQ (tfontptr->n, font_ret.name)) {
	flags |= PAGE_GAVE_WARN;
	Warning("font %s not found, using %s at %d dpi instead.\n",
		tfontptr->n, font_ret.name, font_ret.dpi);
	tfontptr->c = 0; /* no checksum warning */
      } else if (!kpse_bitmap_tolerance ((double)font_ret.dpi, (double) tfontptr->dpi)) {
	flags |= PAGE_GAVE_WARN;
	Warning("font %s at %d dpi not found, using %d dpi instead.\n",
		tfontptr->name, tfontptr->dpi, font_ret.dpi);
      }
      InitPK(tfontptr);
    } else {
      flags |= PAGE_GAVE_WARN;
      Warning("font %s at %d dpi not found, characters will be left blank.\n",
	      tfontptr->n, tfontptr->dpi);
      strcpy (tfontptr->name, "None");
      tfontptr->filedes = 0;
      tfontptr->magnification = 0;
      tfontptr->designsize = 0;
    }
  }
#else /* not HAVE_LIBKPATHSEA */
      /* Ouch time! findfile not implemented (old cruft from dvilj) */
  /* Total argh, since none of this is adapted to vf and the like */
  if (!(findfile(PXLpath,
		 tfontptr->n,
		 tfontptr->dpi,
		 tfontptr->name,
		 _FALSE,
		 0))) {
    Warning(tfontptr->name); /* contains error messsage */
    tfontptr->filedes = 0;
#ifdef __riscos
    MakeMetafontFile(PXLpath, tfontptr->n, tfontptr->dpi);
#endif
  }
#endif 
}


void DoneFont(struct font_entry *tfontp)
{
  switch (tfontp->type) {
  case FONT_TYPE_PK:
    DonePK(tfontp);
    break;
  case FONT_TYPE_VF:
    DoneVF(tfontp);
    break;
#ifdef HAVE_FT2
  case FONT_TYPE_FT:
    DoneFT(tfontp);
    break;
#endif
#ifdef HAVE_LIBT1
  case FONT_TYPE_T1:
    DoneT1(tfontp);
    break;
#endif
  }
}


void FreeFontNumP(struct font_num *hfontnump)
{
  struct font_num *tmp;
  while(hfontnump!=NULL) {
    tmp=hfontnump->next; 
    free(hfontnump);
    hfontnump=tmp;
  }
}

void ClearFonts(void)
{
  struct font_entry *tmp;

  while(hfontptr!=NULL) {
    tmp=hfontptr->next;
    DoneFont(hfontptr);
    free(hfontptr);
    hfontptr=tmp;
  }
  if (dvi!=NULL)
    FreeFontNumP(dvi->fontnump);
}

/*-->SetFntNum*/
/**********************************************************************/
/****************************  SetFntNum  *****************************/
/**********************************************************************/
void SetFntNum(int32_t k, void* parent /* dvi/vf */)
/*  this routine is used to specify the font to be used in printing future
    characters */
{
  struct font_num *tfontnump=NULL;  /* temporary font_num pointer   */

  switch (((struct font_entry*)parent)->type) {
  case FONT_TYPE_VF:
    tfontnump = ((struct font_entry*)parent)->vffontnump;
    break;
  case DVI_TYPE:
    tfontnump = ((struct dvi_data*)parent)->fontnump;
  }
  while (tfontnump != NULL && tfontnump->k != k)
    tfontnump = tfontnump->next;
  if (tfontnump == NULL)
    Fatal("font %d undefined", k);

  currentfont = tfontnump->fontp;
  if (currentfont->name[0]=='\0')
    FontFind(currentfont);
}


