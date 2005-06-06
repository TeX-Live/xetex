#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

static unsigned char* psfont_mmap = (unsigned char *)-1;
static int psfont_filedes = -1;
static struct stat psfont_stat;
static struct psfontmap *psfontmap=NULL;

inline char* newword(char** buffer, char* end) 
{
  char *word,*pos=*buffer;

  while(pos<end && *pos!=' ' && *pos!='\t' && *pos!='"') pos++;
  if ((word=malloc(pos-*buffer+1))==NULL)
    Fatal("cannot malloc space for string");
  strncpy(word,*buffer,pos-*buffer);
  word[pos-*buffer]='\0';
  if (*pos=='"') pos++;
  *buffer=pos;
  return(word);
}

char* find_format(char* name)
{
  /* Cater for both new (first case) and old (second case) kpathsea */
  char* format =
     kpse_find_file(name,kpse_fontmap_format,false);

  if (format==NULL)
    format = kpse_find_file(name,kpse_dvips_config_format,false);
  return(format);
}

void InitPSFontMap(void)
{
  char *pos,*end,*psfont_name = find_format("ps2pk.map");
  struct psfontmap *entry;

  if (psfont_name==NULL)
    psfont_name = find_format("psfonts.map");

  if (psfont_name==NULL) {
    Warning("cannot find ps2pk.map, nor psfonts.map");
    return;
  }
  DEBUG_PRINT(DEBUG_FT,("\n  OPEN PSFONT MAP:\t'%s'", psfont_name));  
  if ((psfont_filedes = open(psfont_name,O_RDONLY)) == -1) { 
    Warning("psfonts map %s could not be opened", psfont_name);
    return;
  }
  fstat(psfont_filedes,&psfont_stat);
  psfont_mmap = mmap(NULL,psfont_stat.st_size,
		     PROT_READ, MAP_SHARED,psfont_filedes,0);
  if (psfont_mmap == (unsigned char *)-1) 
    Warning("cannot mmap psfonts map %s !\n",psfont_name);
  else {
    pos = psfont_mmap;
    end = psfont_mmap+psfont_stat.st_size;
    while(pos<end) {
      while(pos < end && (*pos=='\n' || *pos==' ' || *pos=='\t' 
			  || *pos=='%' || *pos=='*' || *pos==';' || *pos=='#')) {
	while(pos < end && *pos!='\n') pos++; /* skip comments/empty rows */
	pos++;
      }
      if (pos < end) {
	if ((entry=malloc(sizeof(struct psfontmap)))==NULL)
	  Fatal("cannot malloc psfontmap space");
	entry->line = pos;
	/* skip <something and quoted entries */
	while(pos < end && (*pos=='<' || *pos=='"')) {
	  if (*pos=='<') 
	    while(pos < end && *pos!=' ' && *pos!='\t') pos++;
	  else 
	    while(pos < end && *pos!='"') pos++;
	  while(pos < end && (*pos==' ' || *pos=='\t')) pos++;
	}
	/* first word is font name */
 	entry->tfmname = newword(&pos,end);
	entry->psfile = NULL;
	entry->encname = NULL;
	while(pos < end && *pos!='\n') pos++;
	entry->end = pos;
	entry->next=psfontmap;
	psfontmap=entry;
      }
      pos++;
    }
  }
}


struct psfontmap* FindPSFontMap(char* fontname)
{
  char *pos,*tfmname,*psname;
  struct psfontmap *entry;
  double cxx=0.0,cxy=0.0;
	
  entry=psfontmap;
  while(entry!=NULL && strcmp(entry->tfmname,fontname)!=0)
    entry=entry->next;
  if (entry!=NULL && entry->psfile==NULL) {
    int nameno = 0;
    
    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\n  PSFONTMAP: %s ",fontname));
    pos=entry->line;
#if HAVE_LIBT1
    entry->t1_transformp=NULL;
#endif
#if HAVE_FT2
    entry->ft_transformp=NULL;
#endif
    while(pos < entry->end) { 
      if (*pos=='<') {                               /* filename follows */
	pos++;
	if (pos<entry->end && *pos=='<') {           /* <<download.font */
	  pos++;
	  entry->psfile = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<%s ",entry->psfile));
	} else if (pos<entry->end && *pos=='[') {    /* <[encoding.file */
	  pos++;
	  entry->encname = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
	} else {                                     /* <some.file      */
	  char* word =newword((char**)&pos,entry->end); 
	  if (strncmp(word+strlen(word)-4,".enc",4)==0) {/* <some.enc */
	    entry->encname=word;
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
	  } else {                                   /* <font    */  
	    entry->psfile=word;
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<%s ",entry->psfile));
	  }
	}
      } else if (*pos=='"') { /* PS code: reencoding/tranformation exists */
	double val=0.0;
	pos++;
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\""));
	while(pos < entry->end && *pos!='"') {
	  val=strtod(pos,(char**)&pos); 
	  /* Small buffer overrun risk, but we're inside double quotes */
	  while(pos < entry->end && (*pos==' ' || *pos=='\t')) pos++;
	  if (pos<entry->end-10 && strncmp(pos,"ExtendFont",10)==0) {
	    cxx=val;
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),
			("%f ExtendFont ",cxx));
	  }
	  if (pos<entry->end-9 && strncmp(pos,"SlantFont",9)==0) {
	    cxy=val;
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),
			("%f SlantFont ",cxy));
	  }
	  while(pos<entry->end && *pos!=' ' && *pos!='\t' && *pos!='"') 
	    pos++;
	}
#if HAVE_FT2
	entry->ft_transformp=&(entry->ft_transform);
	if (cxx!=0.0)
	  entry->ft_transform.xx=(FT_Fixed)(cxx*0x10000);
	else
	  entry->ft_transform.xx=0x10000;
	entry->ft_transform.xy=(FT_Fixed)(cxy*0x10000);
	entry->ft_transform.yx=0;
	entry->ft_transform.yy=0x10000;
#endif
#if HAVE_LIBT1
	entry->t1_transformp=&(entry->t1_transform);
	if (cxx!=0)
	  entry->t1_transform.cxx=cxx;
	else
	  entry->t1_transform.cxx=1.0;
	entry->t1_transform.cxy=cxy;
	entry->t1_transform.cyx=0.0;
	entry->t1_transform.cyy=1.0;
#endif
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\" "));
	pos++;
      } else {                                      /* bare word */
	switch (++nameno) {
	case 1:            /* first word is tfmname and perhaps psname */
	  while(pos<entry->end && *pos!=' ' && *pos!='\t') pos++;
	  psname=entry->tfmname;
	  break;
	case 2:                              /* second word is psname */
	  psname = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("(%s) ",psname));
	  free(psname);
	  break;
	case 3:                             /* third word is encoding */
	  entry->encname = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
	  break;
	default:
	  Warning("more than three bare words in font map entry");
	}
      }
      while(pos < entry->end && (*pos==' ' || *pos=='\t')) pos++;
    }
    if (entry->psfile==NULL) { 
      /* No psfile-name given, use (free-able copy of) tfmname */
      entry->psfile=newword(&tfmname,tfmname+strlen(tfmname));
      DEBUG_PRINT((DEBUG_FT|DEBUG_T1),(" <%s ",entry->psfile));
    }
    if (entry->encname!=NULL 
	&& (entry->encoding=FindEncoding(entry->encname))==NULL) 
      Warning("unable to load font encoding '%s' for %s",
	      entry->encname,entry->tfmname);
  }
  if (entry != NULL && entry->encname!=NULL && entry->encoding==NULL) 
    /* Encoding given but it cannot be found. Unusable font */
    return(NULL);
  
  return(entry);
}
