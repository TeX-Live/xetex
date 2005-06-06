#include "dvipng.h"
#include <libgen.h>
#include <sys/stat.h>

bool followmode=0;

bool DVIFollowToggle()
{
  return followmode = ! followmode;
}

static unsigned char fgetc_follow(FILE* fp)
{
  int got=fgetc(fp);

  while(followmode && got==EOF) {
    sleep(1);
    got=fgetc(fp);
  }
  if (got==EOF)
    Fatal("DVI file ends prematurely");
  return (unsigned char)got;
}

void DVIInit(struct dvi_data* dvi)
{
  int     k;
  unsigned char* pre;
  struct stat stat;

  fseek(dvi->filep,0,SEEK_SET);
  pre=DVIGetCommand(dvi);
  if (*pre != PRE) {
    Fatal("PRE doesn't occur first--are you sure this is a DVI file?\n\n");
  }
  k = UNumRead(pre+1,1);
  DEBUG_PRINT(DEBUG_DVI,("DVI START:\tPRE %d",k));
  if (k != DVIFORMAT) {
    Fatal("DVI format = %d, can only process DVI format %d files\n\n",
	  k, DVIFORMAT);
  }
  dvi->num = UNumRead(pre+2, 4);
  dvi->den = UNumRead(pre+6, 4);
  DEBUG_PRINT(DEBUG_DVI,(" %d/%d",dvi->num,dvi->den));
  dvi->mag = UNumRead(pre+10, 4); /*FIXME, see font.c*/
  DEBUG_PRINT(DEBUG_DVI,(" %d",dvi->mag));
  if ( usermag > 0 && usermag != dvi->mag ) {
    Warning("DVI magnification of %d over-ridden by user (%ld)",
	    (long)dvi->mag, usermag );
    dvi->mag = usermag;
  }
  dvi->conv = (1.0/(((double)dvi->num / (double)dvi->den) *
		    ((double)dvi->mag / 1000.0) *
		    ((double)dpi*shrinkfactor/254000.0)))+0.5;
  DEBUG_PRINT(DEBUG_DVI,(" (%d)",dvi->conv));
  k = UNumRead(pre+14,1);
  DEBUG_PRINT(DEBUG_DVI,(" '%.*s'",k,pre+15));
  Message(BE_VERBOSE,"'%.*s' -> %s\n",k,pre+15,dvi->outname);
  fstat(fileno(dvi->filep), &stat);
  dvi->mtime = stat.st_mtime;
}

struct dvi_data* DVIOpen(char* dviname,char* outname)
{
  char* tmpstring;
  struct dvi_data* dvi;

  if ((dvi = malloc(sizeof(struct dvi_data)))==NULL)
    Fatal("cannot allocate memory for dvi struct");

  dvi->type = DVI_TYPE;
  dvi->fontnump=NULL;

  dvi->name = malloc(strlen(dviname)+5);
  if (dvi->name==NULL)
    Fatal("cannot allocate space for DVI filename");
  strcpy(dvi->name, dviname);
  tmpstring = strrchr(dvi->name, '.');
  if (tmpstring == NULL || strcmp(tmpstring,".dvi") != 0) 
    strcat(dvi->name, ".dvi");
  
  if (outname==NULL) { 
    dvi->outname = malloc(strlen(basename(dviname))+7);
    if (dvi->outname==NULL)
      Fatal("cannot allocate space for output filename");
    strcpy(dvi->outname,basename(dviname));
    tmpstring = strrchr(dvi->outname, '.');
    if (tmpstring != NULL) 
      *tmpstring = '\0';
    strcat(dvi->outname, "%d.png");
  } else {
    dvi->outname = malloc(strlen(outname)+1);
    if (dvi->outname==NULL)
      Fatal("cannot allocate space for output filename");
    strcpy(dvi->outname,outname);
  }

  if ((dvi->filep = fopen(dvi->name,"rb")) == NULL) {
    /* do not insist on .dvi */
    tmpstring = strrchr(dvi->name, '.');
    *tmpstring='\0';
    dvi->filep = fopen(dvi->name,"rb");
  }
  while((dvi->filep == NULL) && followmode) {
    sleep(1);
    *tmpstring='.';
    if ((dvi->filep = fopen(dvi->name,"rb")) == NULL) {
      /* do not insist on .dvi */
      *tmpstring='\0';
      dvi->filep = fopen(dvi->name,"rb");
    }
  }
  if (dvi->filep == NULL) {
    perror(dviname);
    exit (EXIT_FAILURE);
  }
  DEBUG_PRINT(DEBUG_DVI,("OPEN FILE\t%s", dvi->name));
  DVIInit(dvi);
  return(dvi);
}

unsigned char* DVIGetCommand(struct dvi_data* dvi)
     /* This function reads in and stores the next dvi command. */
     /* Mmap is not appropriate here, we may want to read from
	half-written files. */
{ 
  static unsigned char command[STRSIZE];
  static unsigned char* lcommand = command;
  unsigned char *current = command;
  int length;
  uint32_t strlength=0;

  DEBUG_PRINT(DEBUG_DVI,("\n@%ld ", ftell(dvi->filep)));
  *(current++) = fgetc_follow(dvi->filep);
  length = dvi_commandlength[*command];
  if (length < 0)
    Fatal("undefined DVI op-code %d",*command);
  while(current < command+length) 
    *(current++) = fgetc_follow(dvi->filep);
  switch (*command)  {
  case XXX4:
    strlength =                   *(current - 4);
  case XXX3:
    strlength = strlength * 256 + *(current - 3);
  case XXX2: 
    strlength = strlength * 256 + *(current - 2);
  case XXX1:
    strlength = strlength * 256 + *(current - 1);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
    strlength = *(current - 1) + *(current - 2);
    break;
  case PRE: 
    strlength = *(current - 1);
    break;
  }
  if (strlength > 0) { /* Read string */
    if (lcommand!=command) {
      free(lcommand);
      lcommand=command;
    }
    if (strlength + (uint32_t)length >  (uint32_t)STRSIZE) {
      /* string + command length exceeds that of buffer */
      if ((lcommand=malloc(length+strlength+1))==NULL) 
	Fatal("cannot allocate memory for DVI command");
      memcpy(lcommand,command,length);
      current = lcommand + length;
    }
    while(current < lcommand+length+strlength) 
      *(current++) = fgetc_follow(dvi->filep);
    return(lcommand);
  } else
    return(command);
}


uint32_t CommandLength(unsigned char* command)
{ 
  /* generally 2^32+5 bytes max, but in practice 32 bit numbers suffice */
  uint32_t length=0;

  length = dvi_commandlength[*command];
  switch (*command)  {
  case XXX1: case XXX2: case XXX3: case XXX4: 
    length += UNumRead((char*)command + 1,length - 1);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4: 
    length += *(command + length - 1) + *(command + length - 2);
    break;
  case PRE: 
    length += *(command + length - 1);
    break;
  }
  return(length);
}

void SkipPage(struct dvi_data* dvi)
{ 
  /* Skip present page */
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  while (*command != EOP)  {
    switch (*command)  {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      DEBUG_PRINT(DEBUG_DVI,("NOSKIP CMD:\t%s", dvi_commands[*command]));
      FontDef(command,dvi);
      break;
    case BOP: case PRE: case POST: case POST_POST:
      Fatal("%s occurs within page", dvi_commands[*command]);
      break;
#ifdef DEBUG
    default:
      DEBUG_PRINT(DEBUG_DVI,("SKIP CMD:\t%s", dvi_commands[*command]));
#endif
    }
    command=DVIGetCommand(dvi);
  } /* while */
  DEBUG_PRINT(DEBUG_DVI,("SKIP CMD:\t%s", dvi_commands[*command]));
}

struct page_list* InitPage(struct dvi_data* dvi)
{
  /* Find page start, return pointer to page_list entry if found */
  struct page_list* tpagelistp=NULL;
  unsigned char* command;           

  command=DVIGetCommand(dvi);
  /* Skip until page start or postamble */
  while((*command != BOP) && (*command != POST)) {
    switch(*command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      DEBUG_PRINT(DEBUG_DVI,("NOPAGE CMD:\t%s", dvi_commands[*command]));
      FontDef(command,dvi);
      break;
    case NOP:
      DEBUG_PRINT(DEBUG_DVI,("NOPAGE CMD:\tNOP"));
      break;
    default:
      Fatal("%s occurs outside page", dvi_commands[*command]);
    }
    command=DVIGetCommand(dvi);
  }
  if ((tpagelistp = malloc(sizeof(struct page_list)))==NULL)
    Fatal("cannot allocate memory for new page entry");
  tpagelistp->next = NULL;
  if ( *command == BOP ) {  /*  Init page */
    int i;
    DEBUG_PRINT(DEBUG_DVI,("PAGE START:\tBOP"));
    tpagelistp->offset = ftell(dvi->filep)-45;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = UNumRead(command + 1 + i*4, 4);
      DEBUG_PRINT(DEBUG_DVI,(" %d",tpagelistp->count[i]));
    }
    if (dvi->pagelistp==NULL)
      tpagelistp->count[10] = 1;
    else
      tpagelistp->count[10] = dvi->pagelistp->count[10]+1;
    DEBUG_PRINT(DEBUG_DVI,(" (%d)", tpagelistp->count[10]));
  } else {
    DEBUG_PRINT(DEBUG_DVI,("DVI END:\tPOST"));
    tpagelistp->offset = ftell(dvi->filep)-1;
    tpagelistp->count[0] = PAGE_POST; /* POST */
  }
  return(tpagelistp);
}

int SeekPage(struct dvi_data* dvi, struct page_list* page)
{
  return(fseek(dvi->filep, 
	       page->offset+((page->count[0]==PAGE_POST) ? 1L : 45L),
	       SEEK_SET));
}

struct page_list* NextPage(struct dvi_data* dvi, struct page_list* page)
{
  struct page_list* tpagelistp;

  /* if page points to POST there is no next page */
  if (page!=NULL && page->count[0]==PAGE_POST) 
    return(NULL);

  /* If we have read past the last page in our current list or the
   *  list is empty, sneak a look at the next page
   */
  if (dvi->pagelistp==NULL
      || dvi->pagelistp->offset+45L < ftell(dvi->filep)) {
    tpagelistp=dvi->pagelistp;
    dvi->pagelistp=InitPage(dvi);
    dvi->pagelistp->next=tpagelistp;
  }

  if (page!=dvi->pagelistp) {
    /* also works if page==NULL, we'll get the first page then */
    tpagelistp=dvi->pagelistp;
    while(tpagelistp!=NULL && tpagelistp->next!=page)
      tpagelistp=tpagelistp->next;
  } else {
    /* dvi->pagelistp points to the last page we've read so far,
     * the last page that we know where it is, so to speak
     * So look at the next
     */
    (void)SeekPage(dvi,dvi->pagelistp);
    SkipPage(dvi);
    tpagelistp=dvi->pagelistp;
    dvi->pagelistp=InitPage(dvi);
    dvi->pagelistp->next=tpagelistp;
    tpagelistp=dvi->pagelistp;
  }
  return(tpagelistp);
}

struct page_list* PrevPage(struct dvi_data* dvi, struct page_list* page)
{
  return(page->next);
}


struct page_list* FindPage(struct dvi_data* dvi, int32_t pagenum, bool abspage)
     /* Find first page of certain number, 
	absolute number if abspage is set */
{
  struct page_list* page=NextPage(dvi, NULL);
  
  if (pagenum==PAGE_LASTPAGE || pagenum==PAGE_POST) {
    while(page!=NULL && page->count[0]!=PAGE_POST)
      page=NextPage(dvi,page);
    if (pagenum==PAGE_LASTPAGE)
      page=PrevPage(dvi,page);
  } else
    if (pagenum!=PAGE_FIRSTPAGE) 
      while(page != NULL && pagenum != page->count[abspage ? 0 : 10])
	page=NextPage(dvi,page);
  return(page);
}


void DelPageList(struct dvi_data* dvi)
{
  struct page_list* temp;
  
  /* Delete the page list */

  temp=dvi->pagelistp;
  while(temp!=NULL) {
    dvi->pagelistp=dvi->pagelistp->next;
    free(temp);
    temp=dvi->pagelistp;
  }
}

void DVIClose(struct dvi_data* dvi)
{
  fclose(dvi->filep);
  DelPageList(dvi);
  free(dvi->outname);
  free(dvi->name);
  free(dvi);
}

bool DVIReOpen(struct dvi_data* dvi)
{
  struct stat stat;
  fstat(fileno(dvi->filep), &stat);
  if (dvi->mtime != stat.st_mtime) {
    fclose(dvi->filep);
    dvi->filep=NULL;
    DelPageList(dvi);
    while(((dvi->filep = fopen(dvi->name,"rb")) == NULL) && followmode) {
      sleep(1);
    }
    if (dvi->filep == NULL) {
      perror(dvi->name);
      exit(EXIT_FAILURE);
    }
    Message(PARSE_STDIN,"Reopened file\n");
    DEBUG_PRINT(DEBUG_DVI,("\nREOPEN FILE\t%s", dvi->name));
    DVIInit(dvi);
    return(true);
  }
  return(false);
}
