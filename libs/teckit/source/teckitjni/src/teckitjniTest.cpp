/***************************************************************************
 *   Copyright (C) 2004 by Keith Stribley                                  *
 *   keith@myanmarlug.org                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <iconv.h>
#include <errno.h>

#include "TecKitJni.h"


int main(int argc, char *argv[])
{
  if (argc < 3)
  {
  	fprintf(stderr,"Usage: %s -utf82char|-char2utf8 mapFile.enc [input.txt output.txt]\n",argv[0]);
	  fprintf(stderr,"Converts stdin to stdout\n");
      return 1;
  }
  bool toUnicode = true;
  if (strcmp(argv[1],"-utf82char")==0) toUnicode = false;
  FILE * inFile = NULL;
  FILE * outFile = NULL;
  if (argc > 4)
  {
  	inFile = fopen(argv[3],"rb");
	outFile = fopen(argv[4],"wb");
  }
  else
  {
  	inFile = stdin;
	outFile = stdout;
  }
  TecKitJni * jni = new TecKitJni();
  assert(jni);
  /*
  if (toUnicode)
  {
      if (!jni->openMapping(argv[2],toUnicode),kForm_Bytes,kForm_UTF8))
          fprintf(stderr,"Failed to open %s\n", argv[2]);
  }
  else
  {
      if (!jni->openMapping(argv[2],toUnicode,kForm_UTF8,kForm_Bytes))
          fprintf(stderr,"Failed to open %s\n", argv[2]);
  }*/
  if (!jni->openMapping(argv[2],toUnicode))
          fprintf(stderr,"Failed to open %s\n", argv[2]);
      //cerr << "Failed to open " << argv[2] << endl;
  size_t length = 2048;
  char * buffer = (char*)calloc(sizeof(char),length);
  char * output = NULL;
  ssize_t read = 0;
  bool convert2Bytes = toUnicode;
  
  while ((read = getline(&buffer,&length,inFile)) > -1)
  {
        //fprintf(stderr,"Buffer length = %d,%d\n",read,length);
          
  	// remove \r\n
        //char * eol = strchr(buffer,'\r');
        //if (eol) *eol = '\0';
        //eol = strchr(buffer,'\r');
        //if (eol) *eol = '\0';
        //fprintf(stderr,"Processing: <%s>\n",buffer);
        char * inPointer = buffer;
        char preBuffer[2048];
        if (convert2Bytes)
        {
            buffer[read] = '\0';
            iconv_t ic = iconv_open("UTF-8","ISO8859-1");
            char * pa = buffer;
            char * pb = preBuffer;
            size_t la = strlen(buffer);
            size_t lb = 2048;
            size_t status = iconv(ic,&pa,&la,&pb,&lb);
            if (status<0)
                fprintf(stderr,"Error: %s\n",strerror(errno));
            //fprintf(stderr,"Indices: %d %d %d %d\n",la,lb,pa-buffer,pb-preBuffer);
            *pb = '\0';
            //fprintf(stderr,"Preprocessing: <%s> %d %c\n",preBuffer,status,preBuffer[0]);
            inPointer = preBuffer;
        }
	
        if (read > 0)
	{
		output = jni->convert(inPointer);
		//fprintf(stderr,"Output length = %d\n",jni->getOutputLength());
		fwrite(jni->getOutputBuffer(),1,jni->getOutputLength()+1,outFile);
		//fflush(outFile);
	}
	
  } 
  delete jni;
  if (buffer) free(buffer);
  fclose(inFile);
  fclose(outFile);
  //fprintf(stderr,"teckitjniTest finished\n");
  return 0;//EXIT_SUCCESS;
}
