#include "dvipng.h"
#include <sys/wait.h>
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

gdImagePtr
ps2png(const char *psfile, int hresolution, int vresolution, 
       int urx, int ury, int llx, int lly)
{
  int status, downpipe[2], uppipe[2];
  pid_t pid;
  char resolution[STRSIZE]; 
  char devicesize[STRSIZE]; 
  /* For some reason, png256 gives inferior result */
  char *device="-sDEVICE=png16m";  
  gdImagePtr psimage=NULL;
  static bool showpage;

  status=sprintf(resolution, "-r%dx%d",hresolution,vresolution);
  status=sprintf(devicesize, "-g%dx%d",
		  //(int)((sin(atan(1.0))+1)*
		  (urx - llx)*hresolution/72,//), 
		  //(int)((sin(atan(1.0))+1)*
    (ury - lly)*vresolution/72);//);
  /* png16m being the default, this code is not needed
   * #ifdef HAVE_GDIMAGECREATETRUECOLOR
   * if (flags & RENDER_TRUECOLOR) 
   * device="-sDEVICE=png16m";
   * #endif  
   */
  if (status>0 && status<STRSIZE 
      && pipe(downpipe)==0 && pipe(uppipe)==0) { /* Ready to fork */
    pid = fork ();
    if (pid == 0) { /* Child process.  Execute gs. */       
      close(downpipe[1]);
      dup2(downpipe[0], STDIN_FILENO);
      close(downpipe[0]);
      DEBUG_PRINT(DEBUG_GS,
		  ("\n  GS CALL:\t%s %s %s %s %s %s %s %s %s %s %s ",// %s",
		   GS_PATH, device, resolution, //devicesize,
		   "-dBATCH", "-dNOPAUSE", "-dSAFER", "-q", 
		   "-sOutputFile=-", 
		   "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
		   "-"));
      close(uppipe[0]);
      dup2(uppipe[1], STDOUT_FILENO);
      close(uppipe[1]);
      execl (GS_PATH, GS_PATH, device, resolution, //devicesize,
	     "-dBATCH", "-dNOPAUSE", "-dSAFER", "-q", 
	     "-sOutputFile=-", 
	     "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
	     "-",NULL);
      _exit (EXIT_FAILURE);
    }
    else if (pid > 0) { /* Parent process. */
      FILE *psstream, *pngstream;
      
      close(downpipe[0]);
      psstream=fdopen(downpipe[1],"w");
      if (psstream) {
	DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\t<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice",
	       urx - llx, ury - lly,llx,lly));
       fprintf(psstream, "<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice\n",
               urx - llx, ury - lly,llx,lly);
	if ( bRed < 255 || bGreen < 255 || bBlue < 255 ) {
	  DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\tgsave %f %f %f setrgbcolor clippath fill grestore",
		       bRed/256.0, bGreen/256.0, bBlue/256.0));
	  fprintf(psstream, "gsave %f %f %f setrgbcolor clippath fill grestore",
		  bRed/256.0, bGreen/256.0, bBlue/256.0);
	}
	DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\t(%s) run", psfile));
	fprintf(psstream, "(%s) run\n", psfile);
	if (showpage) {
	  DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\tshowpage"));
	  fprintf(psstream, "showpage\n");
	}
	DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\tquit"));
	fprintf(psstream, "quit\n");
	fclose(psstream);
      }
      close(uppipe[1]);
      pngstream=fdopen(uppipe[0],"r");
      if (pngstream) {
	psimage = gdImageCreateFromPng(pngstream);
	fclose(pngstream);
      }
#ifdef HAVE_GDIMAGETRUECOLORTOPALETTE
#ifdef HAVE_GDIMAGECREATETRUECOLOR
      if (!flags & RENDER_TRUECOLOR)
#endif
	gdImageTrueColorToPalette(psimage,0,256);
#endif
      if (psimage == NULL) {
	DEBUG_PRINT(DEBUG_GS,("\n  GS OUTPUT:\tNO IMAGE "));
	if (!showpage) {
	  showpage=TRUE;
	  DEBUG_PRINT(DEBUG_GS,("(will try adding \"showpage\") "));
	  psimage=ps2png(psfile, hresolution, vresolution, urx, ury, llx, lly);
	  showpage=FALSE;
	}
      } else {
	DEBUG_PRINT(DEBUG_GS,("\n  GS OUTPUT:\t%dx%d image ",
			      gdImageSX(psimage),gdImageSY(psimage)));
      }
    }
  }
  return psimage;
}

/*-->SetSpecial*/
/*********************************************************************/
/****************************  SetSpecial  ***************************/
/*********************************************************************/

void SetSpecial(char * special, int32_t length, int32_t hh, int32_t vv)
/* interpret a \special command, made up of keyword=value pairs */
/* Color specials only for now. Warn otherwise. */
{
  char *buffer, *token;

  DEBUG_PRINT(DEBUG_DVI,(" '%.*s'",length,special));

  buffer = alloca(sizeof(char)*(length+1));
  if (buffer==NULL) 
    Fatal("Cannot allocate space for special string");

  strncpy(buffer,special,length);
  buffer[length]='\0';

  token = strtok(buffer," ");
  /********************** Color specials ***********************/
  if (strcmp(token,"background")==0) {
    token = strtok(NULL,"\0");
    background(token);
    return;
  }
  if (strcmp(token,"color")==0) {
    token = strtok(NULL,"\0");
    if (strncmp(token,"push",4)==0) {
      token = strtok(token," ");
      token = strtok(NULL,"\0");
      pushcolor(token);
    } else {
      if (strncmp(token,"pop",3)==0)
	popcolor();
      else 
	resetcolorstack(token);
    }
    return;
  }

  /******************* Postscript inclusion ********************/
  if (strncmp(token,"PSfile=",7)==0) { /* PSfile */
    char* psname = token+7;
    int llx=0,lly=0,urx=0,ury=0,rwi=0,rhi=0;
    int hresolution,vresolution;

    /* Remove quotation marks around filename */
    if (*psname=='"') {
      char* tmp;
      psname++;
      tmp=strrchr(psname,'"');
      if (tmp!=NULL) *tmp='\0';
    }

    while((token = strtok(NULL," ")) != NULL) {
      if (strncmp(token,"llx=",4)==0) llx = atoi(token+4);
      if (strncmp(token,"lly=",4)==0) lly = atoi(token+4);
      if (strncmp(token,"urx=",4)==0) urx = atoi(token+4);
      if (strncmp(token,"ury=",4)==0) ury = atoi(token+4);
      if (strncmp(token,"rwi=",4)==0) rwi = atoi(token+4);
      if (strncmp(token,"rhi=",4)==0) rhi = atoi(token+4);
    }
    
    /* Calculate resolution, and use our base resolution as a fallback. */
    /* The factor 10 is magic, the dvips graphicx driver needs this.    */
    hresolution = dpi*rwi/(urx - llx)/10;
    vresolution = dpi*rhi/(ury - lly)/10;
    if (vresolution==0) vresolution = hresolution;
    if (hresolution==0) hresolution = vresolution;
    if (hresolution==0) hresolution = vresolution = dpi;
    
    if (page_imagep != NULL) { /* Draw into image */
      char* psfile = kpse_find_file(psname,kpse_pict_format,0);
      char* pngname = NULL;
      gdImagePtr psimage=NULL;

      if (flags & CACHE_IMAGES) {
	char* pngfile;

	pngname = malloc(sizeof(char)*(strlen(psname+5)));
	if (pngname==NULL) 
	  Fatal("Cannot allocate space for cached image filename");
	strcpy(pngname,psname);
	pngfile = strrchr(pngname,'.');
	if (pngfile==NULL)
	  strcat(pngname,".png");
	else {
	  pngfile[1] = 'p';
	  pngfile[2] = 'n';
	  pngfile[3] = 'g';
	  pngfile[4] = '\0';
	}
	pngfile = kpse_find_file(pngname,kpse_pict_format,0);
	if (pngfile!=NULL) {
	  FILE* pngfilep = fopen(pngfile,"rb");

	  if (pngfilep!=NULL) {
	    psimage = gdImageCreateFromPng(pngfilep);
	    fclose(pngfilep);
	  }
	  free(pngfile);
	}
      }
      Message(BE_NONQUIET,"<%s",psname);
      if (psimage==NULL) {
	if (psfile == NULL) {
	  Warning("PS file %s not found, image will be left blank", psname );
	  flags |= PAGE_GAVE_WARN;
	} else {
	  psimage = ps2png(psfile, hresolution, vresolution, 
			   urx, ury, llx, lly);
	  if ( psimage == NULL ) {
	    Warning("Unable to convert %s to PNG, image will be left blank", 
		    psfile );
	    flags |= PAGE_GAVE_WARN;
	  }
	}
      }
      if (pngname !=NULL && psimage != NULL) {
	FILE* pngfilep = fopen(pngname,"wb");
	if (pngfilep!=NULL) {
	  gdImagePng(psimage,pngfilep);
	  fclose(pngfilep);
	} else
	  Warning("Unable to cache %s as PNG", psfile );
      } 
      if (psimage!=NULL) {
	DEBUG_PRINT(DEBUG_DVI,
		    ("\n  PS-PNG INCLUDE \t%s (%d,%d) res %dx%d at (%d,%d)",
		     psfile,
		     gdImageSX(psimage),gdImageSY(psimage),
		     hresolution,vresolution,
		     hh, vv));
	gdImageCopy(page_imagep, psimage, 
		    hh, vv-gdImageSY(psimage),
		    0,0,
		    gdImageSX(psimage),gdImageSY(psimage));
	gdImageDestroy(psimage);
      }
      Message(BE_NONQUIET,">");
    } else { /* Not PASS_DRAW */
      int pngheight,pngwidth;
      
      /* Convert from postscript 72 dpi resolution to our given resolution */
      pngheight = (vresolution*(ury - lly)+71)/72; /* +71: do 'ceil' */
      pngwidth  = (hresolution*(urx - llx)+71)/72;
      DEBUG_PRINT(DEBUG_DVI,("\n  PS-PNG INCLUDE \t(%d,%d)", 
		   pngwidth,pngheight));
      min(x_min,hh);
      min(y_min,vv-pngheight);
      max(x_max,hh+pngwidth);
      max(y_max,vv);
    }
    return;
  }

  /* preview-latex' tightpage option */
  if (strcmp(buffer,"!userdict")==0 
      && strcmp(buffer+10,"begin/bop-hook{7{currentfile token not{stop}if 65781.76 div DVImag mul}repeat 72 add 72 2 copy gt{exch}if 4 2 roll neg 2 copy lt{exch}if dup 0 gt{pop 0 exch}{exch dup 0 lt{pop 0}if}ifelse 720 add exch 720 add 3 1 roll 4{5 -1 roll add 4 1 roll}repeat <</PageSize[5 -1 roll 6 index sub 5 -1 roll 5 index sub]/PageOffset[7 -2 roll [1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice//bop-hook exec}bind def end")==0) {
    if (page_imagep==NULL) 
      Message(BE_NONQUIET,"preview-latex's tightpage option detected, will use its bounding box.\n");
    flags |= PREVIEW_LATEX_TIGHTPAGE;
    return;
  }
  if (strncmp(token,"ps::",4)==0) {
    /* Hokay, decode bounding box */
    dviunits adj_llx,adj_lly,adj_urx,adj_ury,ht,dp,wd;
    adj_llx = atoi(token+4);
    token = strtok(NULL," ");
    adj_lly = atoi(token);
    token = strtok(NULL," ");
    adj_urx = atoi(token);
    token = strtok(NULL," ");
    adj_ury = atoi(token);
    token = strtok(NULL," ");
    ht = atoi(token);
    token = strtok(NULL," ");
    dp = atoi(token);
    token = strtok(NULL," ");
    wd = atoi(token);
    if (wd>0) {
      x_offset_tightpage = 
	(-adj_llx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
      x_width_tightpage  = x_offset_tightpage
	+(wd+adj_urx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    } else {
      x_offset_tightpage = 
	(-wd+adj_urx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
      x_width_tightpage  = x_offset_tightpage
	+(-adj_llx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    }
    /* y-offset = height - 1 */
    y_offset_tightpage = 
      (((ht>0)?ht:0)+adj_ury+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor-1;
    y_width_tightpage  = y_offset_tightpage+1
      +(((dp>0)?dp:0)-adj_lly+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    return;
  }

  if (strncmp(token,"header=",7)==0 || token[0]=='!') { /* header, ignored */
    if ( page_imagep != NULL )
      Warning("at (%ld,%ld) ignored header \\special{%.*s}.",
	      hh, vv, length,special);
    return;
  }
  if (strncmp(token,"src:",4)==0) { /* source special */
    if ( page_imagep != NULL )
      Message(BE_NONQUIET," at (%ld,%ld) source \\special{%.*s}",
	      hh, vv, length,special);
    return;
  }
  if ( page_imagep != NULL || flags & NO_IMAGE_ON_WARN ) {
    Warning("at (%ld,%ld) unimplemented \\special{%.*s}.",
	    hh, vv, length,special);
    flags |= PAGE_GAVE_WARN;
  }
}


