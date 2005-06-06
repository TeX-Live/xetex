#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

struct encoding* encodingp=NULL;

struct encoding* InitEncoding(char* encoding) 
{
  char *pos,*max,*buf,*enc_file =
#ifdef HAVE_KPSE_ENC_FORMATS
    kpse_find_file(encoding,kpse_enc_format,false);
#else
    kpse_find_file(encoding,kpse_tex_ps_header_format,false);
#endif
  int encfd,i;
  struct encoding* encp=NULL;
  struct stat stat;
  unsigned char* encmmap;
  
  if (enc_file == NULL)
    Warning("encoding file %s could not be found",encoding);
  DEBUG_PRINT((DEBUG_FT|DEBUG_ENC),("\n  OPEN ENCODING:\t'%s'", enc_file));
  if ((encfd = open(enc_file,O_RDONLY)) == -1) {
    Warning("encoding file %s could not be opened", enc_file);
    return(NULL);
  }
  fstat(encfd,&stat);
  encmmap = mmap(NULL,stat.st_size, PROT_READ, MAP_SHARED,encfd,0);
  if (encmmap == (unsigned char *)-1) {
    Warning("cannot mmap encoding <%s> !\n",enc_file);
    return(NULL);
  }
  if ((encp = calloc(sizeof(struct encoding)+strlen(encoding)+1
		     +stat.st_size,1))==NULL) {
    Warning("cannot alloc space for encoding",enc_file);
    return(NULL);
  }
  encp->name=(char*)encp+sizeof(struct encoding);
  strcpy(encp->name,encoding);
  pos=encmmap;
  max=encmmap+stat.st_size;
  buf=encp->name+strlen(encoding)+1;
#define SKIPCOMMENT(x) if (*x=='%') while (x<max && *x!='\n') x++;
  while(pos<max && *pos!='/') {
    SKIPCOMMENT(pos);
    pos++;
  }
  pos++;
  encp->charname[256]=buf;
  while(pos<max && *pos!='[' 
	&& *pos!=' ' && *pos!='\t' && *pos!='\n' && *pos!='%') 
    *buf++=*pos++;
  *buf++='\0';
  DEBUG_PRINT(DEBUG_ENC,("\n  PS ENCODING '%s'",
			 encp->charname[256])); 
  while (pos < max && *pos!='[') {
    SKIPCOMMENT(pos);
    pos++;
  }
  while(pos<max && *pos!='/') {
    SKIPCOMMENT(pos);
    pos++;
  }
  i=0;
  while(pos<max && *pos!=']') {
    pos++;
    encp->charname[i++]=buf;
    while(pos<max && *pos!=' ' && *pos!='\t' && *pos!='\n' && *pos!='%') 
      *buf++=*pos++;
    *buf++='\0';
    DEBUG_PRINT(DEBUG_ENC,("\n  PS ENCODING %d '%s'",
		 i-1,encp->charname[i-1])); 
    while(pos<max && *pos!='/' && *pos!=']') {
      SKIPCOMMENT(pos);
      pos++;
    }
  }
  if (munmap(encmmap,stat.st_size))
    Warning("cannot munmap encoding %s!?\n",enc_file);
  if (close(encfd))
    Warning("cannot close encoding file %s!?\n",enc_file);
  return(encp);
}


struct encoding* FindEncoding(char* encoding) 
{
  struct encoding *temp=encodingp;

  //  printf("{%s} \n",encoding);
  while(temp!=NULL && strcmp(encoding,temp->name)!=0) 
    temp=temp->next;
  if (temp==NULL) {
    temp=InitEncoding(encoding);
    if (temp!=NULL) {
      temp->next=encodingp;
      encodingp=temp;
    }
  }
  return(temp);
}
