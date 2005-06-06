#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>


#define VF_ID 202
#define LONG_CHAR 242

int32_t SetVF(int32_t c) 
{
  struct font_entry* currentvf;
  unsigned char *command,*end;
  struct vf_char* ptr=currentfont->chr[c];

  currentvf=currentfont;
  BeginVFMacro(currentvf);
  command = ptr->mmap;
  end = command + ptr->length;
  while (command < end)  {
    DEBUG_PRINT(DEBUG_DVI,("\n  VF MACRO:\t%s ", dvi_commands[*command]));
    DrawCommand(command,currentvf);
    command += CommandLength(command);
  } 
  EndVFMacro();
  currentfont=currentvf;
  return(ptr->tfmw);
}



void InitVF(struct font_entry * tfontp)
{
  struct stat stat;
  unsigned char* position;
  int length;
  struct vf_char *tcharptr;  
  uint32_t c=0;
  struct font_num *tfontnump;  /* temporary font_num pointer   */
  
  DEBUG_PRINT((DEBUG_DVI|DEBUG_VF),("\n  OPEN FONT:\t'%s'", tfontp->name));
  Message(BE_VERBOSE,"<%s>", tfontp->name);
  if ((tfontp->filedes = open(tfontp->name,O_RDONLY)) == -1) 
    Warning("font file %s could not be opened", tfontp->name);
  fstat(tfontp->filedes,&stat);
  tfontp->mmap = mmap(NULL,stat.st_size,
      PROT_READ, MAP_SHARED,tfontp->filedes,0);
  if (tfontp->mmap == (unsigned char *)-1) 
    Fatal("cannot mmap VF file <%s> !\n",currentfont->name);
  tfontp->end=tfontp->mmap+stat.st_size;
  if (*(tfontp->mmap) != PRE) 
    Fatal("unknown font format in file <%s> !\n",currentfont->name);
  if (*(tfontp->mmap+1) != VF_ID) 
      Fatal( "wrong version of vf file!  (%d should be 202)\n",
	     (int)*(tfontp->mmap+1));
  DEBUG_PRINT(DEBUG_VF,("\n  VF_PRE:\t'%.*s'", 
		(int)*(tfontp->mmap+2), tfontp->mmap+3));
  position = tfontp->mmap+3 + *(tfontp->mmap+2);
  c=UNumRead(position, 4);
  DEBUG_PRINT(DEBUG_VF,(" %d", c));
  CheckChecksum (tfontp->c, c, tfontp->name);
  tfontp->designsize = UNumRead(position+4,4);
  DEBUG_PRINT(DEBUG_VF,(" %d", tfontp->designsize));
  tfontp->type = FONT_TYPE_VF;
  tfontp->vffontnump=NULL;
  /* Read font definitions */
  position += 8;
  while(*position >= FNT_DEF1 && *position <= FNT_DEF4) {
    DEBUG_PRINT(DEBUG_VF,("\n  @%ld VF:\t%s", 
		  (long)(position - tfontp->mmap), 
		  dvi_commands[*position]));
    FontDef(position,tfontp);	
    length = dvi_commandlength[*position];
    position += length + *(position + length-1) + *(position+length-2);
  }
  /* Default font is the first defined */
  tfontnump = tfontp->vffontnump;
  while (tfontnump->next != NULL) {
    tfontnump = tfontnump->next;
  }
  tfontp->defaultfont=tfontnump->k;
  
  /* Read char definitions */
  while(*position < FNT_DEF1) {
    DEBUG_PRINT(DEBUG_VF,("\n@%ld VF CHAR:\t", 
		 (long)(position - tfontp->mmap)));
    tcharptr=xmalloc(sizeof(struct vf_char));
    switch (*position) {
    case LONG_CHAR:
      tcharptr->length = UNumRead(position+1,4);
      c = UNumRead(position+5,4);
      tcharptr->tfmw = UNumRead(position+9,4);
      position += 13;
      break;
    default:
      tcharptr->length = UNumRead(position,1);
      c = UNumRead(position+1,1);
      tcharptr->tfmw = UNumRead(position+2,3);
      position += 5;
    }
    DEBUG_PRINT(DEBUG_VF,("%d %d %d",tcharptr->length,c,tcharptr->tfmw));
    tcharptr->tfmw = (int32_t) 
      ((int64_t) tcharptr->tfmw * tfontp->s / (1 << 20));
    DEBUG_PRINT(DEBUG_VF,(" (%d)",tcharptr->tfmw));
    if (c > NFNTCHARS) /* Only positive for now */
      Fatal("vf character exceeds numbering limit");
    tfontp->chr[c] = tcharptr;
    tcharptr->mmap=position;
    position += tcharptr->length;
  }
}


void DoneVF(struct font_entry *tfontp)
{
  int c=0;

  if (munmap(tfontp->mmap,tfontp->end-tfontp->mmap))
    Warning("font file %s could not be munmapped", tfontp->name);
  if (close(tfontp->filedes)) 
    Warning("font file %s could not be closed", tfontp->name);
  tfontp->mmap=NULL;
  tfontp->filedes=-1;
  while(c<NFNTCHARS-1) {
    if (tfontp->chr[c]!=NULL) {
      free(tfontp->chr[c]);
      tfontp->chr[c]=NULL;
    }
    c++;
  }
  FreeFontNumP(tfontp->vffontnump);
  tfontp->vffontnump=NULL;
  tfontp->name[0]='\0';
}
