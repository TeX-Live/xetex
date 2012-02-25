//========================================================================
//
// SplashFTFontFile.cc
//
//========================================================================

#include <aconf.h>

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "gmem.h"
#include "SplashFTFontEngine.h"
#include "SplashFTFont.h"
#include "SplashFTFontFile.h"

//------------------------------------------------------------------------
// SplashFTFontFile
//------------------------------------------------------------------------

SplashFontFile *SplashFTFontFile::loadType1Font(SplashFTFontEngine *engineA,
						SplashFontFileID *idA,
						char *fileNameA,
						GBool deleteFileA,
						const char **encA) {
  FT_Face faceA;
  int *codeToGIDA;
  const char *name;
  int i;

  if (FT_New_Face(engineA->lib, fileNameA, 0, &faceA)) {
    return NULL;
  }
  codeToGIDA = (int *)gmallocn(256, sizeof(int));
  for (i = 0; i < 256; ++i) {
    codeToGIDA[i] = 0;
    if ((name = encA[i])) {
      codeToGIDA[i] = (int)FT_Get_Name_Index(faceA, (char *)name);
    }
  }

  return new SplashFTFontFile(engineA, idA, fileNameA, deleteFileA,
			      faceA, codeToGIDA, 256, gFalse, gTrue);
}

SplashFontFile *SplashFTFontFile::loadCIDFont(SplashFTFontEngine *engineA,
					      SplashFontFileID *idA,
					      char *fileNameA,
					      GBool deleteFileA,
					      int *codeToGIDA,
					      int codeToGIDLenA) {
  FT_Face faceA;

  if (FT_New_Face(engineA->lib, fileNameA, 0, &faceA)) {
    return NULL;
  }

  return new SplashFTFontFile(engineA, idA, fileNameA, deleteFileA,
			      faceA, codeToGIDA, codeToGIDLenA,
			      gFalse, gFalse);
}

SplashFontFile *SplashFTFontFile::loadTrueTypeFont(SplashFTFontEngine *engineA,
						   SplashFontFileID *idA,
						   char *fileNameA,
						   int fontNum,
						   GBool deleteFileA,
						   int *codeToGIDA,
						   int codeToGIDLenA) {
  FT_Face faceA;

  if (FT_New_Face(engineA->lib, fileNameA, fontNum, &faceA)) {
    return NULL;
  }

  return new SplashFTFontFile(engineA, idA, fileNameA, deleteFileA,
			      faceA, codeToGIDA, codeToGIDLenA,
			      gTrue, gFalse);
}

SplashFTFontFile::SplashFTFontFile(SplashFTFontEngine *engineA,
				   SplashFontFileID *idA,
				   char *fileNameA, GBool deleteFileA,
				   FT_Face faceA,
				   int *codeToGIDA, int codeToGIDLenA,
				   GBool trueTypeA, GBool type1A):
  SplashFontFile(idA, fileNameA, deleteFileA)
{
  engine = engineA;
  face = faceA;
  codeToGID = codeToGIDA;
  codeToGIDLen = codeToGIDLenA;
  trueType = trueTypeA;
  type1 = type1A;
}

SplashFTFontFile::~SplashFTFontFile() {
  if (face) {
    FT_Done_Face(face);
  }
  if (codeToGID) {
    gfree(codeToGID);
  }
}

SplashFont *SplashFTFontFile::makeFont(SplashCoord *mat,
				       SplashCoord *textMat) {
  SplashFont *font;

  font = new SplashFTFont(this, mat, textMat);
  font->initCache();
  return font;
}

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
