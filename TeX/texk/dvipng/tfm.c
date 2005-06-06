#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

bool ReadTFM(struct font_entry * tfontp, char* tfmname)
{
  struct stat stat;
  struct ft_char *tcharptr;  
  int filedes;
  unsigned char* filemmap,*position; 
  int lh,bc,ec,nw, c;
  dviunits* width;

  DEBUG_PRINT((DEBUG_DVI|DEBUG_FT|DEBUG_TFM),
	      ("\n  OPEN METRICS:\t'%s'", tfmname));
  if ((filedes = open(tfmname,O_RDONLY)) == -1) {
    Warning("metric file %s could not be opened", tfmname);
    return(false);
  }
  fstat(filedes,&stat);
  filemmap = mmap(NULL,stat.st_size, PROT_READ, MAP_SHARED,filedes,0);
  if (filemmap == (unsigned char *)-1) {
    Warning("cannot mmap metric file %s",tfmname);
    close(filedes);
    return(false);
  }
  lh = UNumRead(filemmap+2,2);
  bc = UNumRead(filemmap+4,2);
  ec = UNumRead(filemmap+6,2);
  nw = UNumRead(filemmap+8,2);
  DEBUG_PRINT(DEBUG_TFM,(" %d %d %d %d",lh,bc,ec,nw));
  width=alloca(nw*sizeof(dviunits));  
  c=0;
  position=filemmap+24+(lh+ec-bc+1)*4;
  while( c < nw ) {
    width[c] = SNumRead(position,4);
    c++;
    position += 4; 
  }
  
  /* Read char widths */
  c=bc;
  position=filemmap+24+lh*4;
  while(c <= ec) {
    DEBUG_PRINT(DEBUG_TFM,("\n@%ld TFM METRICS:\t", 
		 (long)(position - filemmap)));
    tcharptr=xmalloc(sizeof(struct ft_char));
    tcharptr->data=NULL;
    tcharptr->tfmw=width[*position];
    DEBUG_PRINT(DEBUG_TFM,("%d [%d] %d",c,*position,tcharptr->tfmw));
    tcharptr->tfmw = (dviunits) 
      ((int64_t) tcharptr->tfmw * tfontp->s / (1 << 20));
    DEBUG_PRINT(DEBUG_TFM,(" (%d)",tcharptr->tfmw));
    if (c > NFNTCHARS) /* Only positive for now */
      Fatal("tfm character exceeds numbering limit");
    tfontp->chr[c] = tcharptr;
    c++;
    position += 4;
  }
  return(true);
}
  

