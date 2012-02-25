//========================================================================
//
// SplashFTFontFile.h
//
//========================================================================

#ifndef SPLASHFTFONTFILE_H
#define SPLASHFTFONTFILE_H

#include <aconf.h>

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include "SplashFontFile.h"

class SplashFontFileID;
class SplashFTFontEngine;

//------------------------------------------------------------------------
// SplashFTFontFile
//------------------------------------------------------------------------

class SplashFTFontFile: public SplashFontFile {
public:

  static SplashFontFile *loadType1Font(SplashFTFontEngine *engineA,
				       SplashFontFileID *idA, char *fileNameA,
				       GBool deleteFileA, const char **encA);
  static SplashFontFile *loadCIDFont(SplashFTFontEngine *engineA,
				     SplashFontFileID *idA, char *fileNameA,
				     GBool deleteFileA,
				     int *codeToGIDA, int codeToGIDLenA);
  static SplashFontFile *loadTrueTypeFont(SplashFTFontEngine *engineA,
					  SplashFontFileID *idA,
					  char *fileNameA,
					  int fontNum,
					  GBool deleteFileA,
					  int *codeToGIDA,
					  int codeToGIDLenA);

  virtual ~SplashFTFontFile();

  // Create a new SplashFTFont, i.e., a scaled instance of this font
  // file.
  virtual SplashFont *makeFont(SplashCoord *mat,
			       SplashCoord *textMat);

private:

  SplashFTFontFile(SplashFTFontEngine *engineA,
		   SplashFontFileID *idA,
		   char *fileNameA, GBool deleteFileA,
		   FT_Face faceA,
		   int *codeToGIDA, int codeToGIDLenA,
		   GBool trueTypeA, GBool type1A);

  SplashFTFontEngine *engine;
  FT_Face face;
  int *codeToGID;
  int codeToGIDLen;
  GBool trueType;
  GBool type1;

  friend class SplashFTFont;
};

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#endif
