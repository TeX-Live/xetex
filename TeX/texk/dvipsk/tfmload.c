/*
 *   Loads a tfm file.  It marks the characters as undefined.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#ifdef KPATHSEA
#include <kpathsea/c-pathmx.h>
#endif
/*
 *   These are the external routines it calls:
 */
#include "protos.h"
/*
 *   Here are the external variables we use:
 */
extern real conv ;
extern real vconv ;
extern real alpha ;
#ifndef KPATHSEA
extern char *tfmpath ;
#endif
extern char errbuf[] ;
extern integer fsizetol ;
/*
 *   Our static variables:
 */
FILE *tfmfile ; 
static char name[50] ;

/*
 *   Tries to open a tfm file.  Uses cmr10.tfm if unsuccessful,
 *   and complains loudly about it.
 */
void
tfmopen P1C(register fontdesctype *, fd)
{
  register char *n;
#ifdef KPATHSEA
  kpse_file_format_type d = tfmpath;
#else
   register char *d;
   d = fd->area ;
   if (*d==0)
     d = tfmpath ;
#endif
   n = fd->name ;
#ifdef MVSXA   /* IBM: MVS/XA */
   (void)sprintf(name, "tfm(%s)", n) ;
#else
   (void)sprintf(name, "%s.tfm", n) ;
#endif
   if ((tfmfile=search(d, name, READBIN))==NULL) {
#ifdef Omega
#ifdef KPATHSEA
   d = ofmpath;
#endif
   (void)sprintf(name, "%s.ofm", n) ;
   if ((tfmfile=search(d, name, READBIN))==NULL) {
#endif
      (void)sprintf(errbuf, "Can't open font metric file %s%s",
             fd->area, name) ;
      error(errbuf) ;
      error("I will use cmr10.tfm instead, so expect bad output.") ;
#ifdef MVSXA   /* IBM: MVS/XA */
      if ((tfmfile=search(d, "tfm(cmr10)", READBIN))==NULL)
#else
      if ((tfmfile=search(d, "cmr10.tfm", READBIN))==NULL)
#endif
         error(
          "! I can't find cmr10.tfm; please reinstall me with proper paths") ;
   }
#ifdef Omega
   }
#endif
}

shalfword
tfmbyte P1H(void)
{
  return(getc(tfmfile)) ;
}

halfword
tfm16 P1H(void)
{
  register halfword a ; 
  a = tfmbyte () ; 
  return ( a * 256 + tfmbyte () ) ; 
} 

integer
tfm32 P1H(void)
{
  register integer a ; 
  a = tfm16 () ; 
  if (a > 32767) a -= 65536 ;
  return ( a * 65536 + tfm16 () ) ; 
} 

int
tfmload P1C(register fontdesctype *, curfnt)
{
#ifdef Omega
   register integer i, j ;
#else
   register shalfword i ;
#endif
   register integer li ;
   integer scaledsize ;
   shalfword nw, hd ;
#ifdef Omega
   integer bc, ec ;
   integer nco, ncw, npc, no_repeats ;
   integer scaled[65536] ;
   integer chardat[65536] ;
   integer font_level ;
   integer pretend_no_chars ;
#else
   shalfword bc, ec ;
   integer scaled[256] ;
   halfword chardat[256] ;
#endif
   int charcount = 0 ;

   tfmopen(curfnt) ;
/*
 *   Next, we read the font data from the tfm file, and store it in
 *   our own arrays.
 */
#ifdef Omega
   li = tfm16() ;
   if (li!=0) {
      font_level = -1 ;
      hd = tfm16() ;
      bc = tfm16() ; ec = tfm16() ;
      nw = tfm16() ;
      li = tfm32() ; li = tfm32() ; li = tfm32() ; li = tfm16() ;
   } else {  /* In an .ofm file */
      font_level = tfm16();
      li = tfm32() ; hd = tfm32() ;
      bc = tfm32() ; ec = tfm32() ;
      nw = tfm32() ;
      for (i=0; i<8; i++) li=tfm32();
      if (font_level==1) {
         nco = tfm32() ;
         ncw = tfm32() ;
         npc = tfm32() ;
         for (i=0; i<12; i++) li=tfm32();
      }
   }
#else
   li = tfm16() ; hd = tfm16() ;
   bc = tfm16() ; ec = tfm16() ;
   nw = tfm16() ;
   li = tfm32() ; li = tfm32() ; li = tfm32() ; li = tfm16() ;
#endif
   li = tfm32() ;
   check_checksum (li, curfnt->checksum, curfnt->name);
   li = (integer)(alpha * (real)tfm32()) ;
   if (li > curfnt->designsize + fsizetol ||
       li < curfnt->designsize - fsizetol) {
      (void)sprintf(errbuf,"Design size mismatch in %s", name) ;
      error(errbuf) ;
   }
#ifdef Omega
   pretend_no_chars=ec+1 ;
   if (pretend_no_chars<256) pretend_no_chars=256 ;
   curfnt->chardesc = (chardesctype *)
         realloc(curfnt->chardesc, sizeof(chardesctype)*pretend_no_chars) ;
   for (i=2; i<((font_level==1)?nco-29:hd); i++)
      li = tfm32() ;
   for (i=0; i<pretend_no_chars; i++)
      chardat[i] = -1 ;
   no_repeats = 0 ;
#else
   for (i=2; i<hd; i++)
      li = tfm32() ;
   for (i=0; i<256; i++)
      chardat[i] = 256 ;
#endif
   for (i=bc; i<=ec; i++) {
#ifdef Omega
      if (no_repeats>0) {
         chardat[i] = chardat[i-1] ;
         no_repeats-- ;
      } else if (font_level>=0) {
         chardat[i] = tfm16() ;
         li = tfm32() ;
         li |= tfm16() ;
         if (font_level==1) {
            no_repeats = tfm16() ;
            for (j=0; j<(npc|1); j++) tfm16() ;
            ncw -= 3 + npc/2 ;
         }
      } else {
         chardat[i] = tfmbyte() ;
         li = tfm16() ;
         li |= tfmbyte() ;
      }
      if (li || chardat[i])
         charcount++ ;
#else
      chardat[i] = tfmbyte() ;
      li = tfm16() ;
      li |= tfmbyte() ;
      if (li || chardat[i])
         charcount++ ;
#endif
   }
#ifdef Omega
   if (font_level==1&&ncw!=0) {
      sprintf(errbuf, "Table size mismatch in %s", curfnt->name) ;
      error(errbuf) ;
   }
#endif
   scaledsize = curfnt->scaledsize ;
   for (i=0; i<nw; i++)
      scaled[i] = scalewidth(tfm32(), scaledsize) ;
   (void)fclose(tfmfile) ;
#ifdef Omega
   for (i=0; i<pretend_no_chars; i++)
      if (chardat[i]!= -1) {
#else
   for (i=0; i<256; i++)
      if (chardat[i]!= 256) {
#endif
         li = scaled[chardat[i]] ;
         curfnt->chardesc[i].TFMwidth = li ;
         if (li >= 0)
            curfnt->chardesc[i].pixelwidth = ((integer)(conv*li+0.5)) ;
         else
            curfnt->chardesc[i].pixelwidth = -((integer)(conv*-li+0.5)) ;
         curfnt->chardesc[i].flags = (curfnt->resfont ? EXISTS : 0) ;
         curfnt->chardesc[i].flags2 = EXISTS ;
      }
#ifdef Omega
   if (ec>=256) curfnt->codewidth = 2 ; /* XXX: 2byte-code can have ec<256 */
#endif
   curfnt->loaded = 1 ;
   return charcount ;
}
