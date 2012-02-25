//========================================================================
//
// Annot.h
//
// Copyright 2000-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef ANNOT_H
#define ANNOT_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

class XRef;
class Catalog;
class Gfx;
class GfxFontDict;
class PDFDoc;

//------------------------------------------------------------------------
// AnnotBorderStyle
//------------------------------------------------------------------------

enum AnnotBorderType {
  annotBorderSolid,
  annotBorderDashed,
  annotBorderBeveled,
  annotBorderInset,
  annotBorderUnderlined
};

class AnnotBorderStyle {
public:

  AnnotBorderStyle(AnnotBorderType typeA, double widthA,
		   double *dashA, int dashLengthA,
		   double rA, double gA, double bA);
  ~AnnotBorderStyle();

  AnnotBorderType getType() { return type; }
  double getWidth() { return width; }
  void getDash(double **dashA, int *dashLengthA)
    { *dashA = dash; *dashLengthA = dashLength; }
  void getColor(double *rA, double *gA, double *bA)
    { *rA = r; *gA = g; *bA = b; }

private:

  AnnotBorderType type;
  double width;
  double *dash;
  int dashLength;
  double r, g, b;
};

//------------------------------------------------------------------------
// Annot
//------------------------------------------------------------------------

class Annot {
public:

  Annot(PDFDoc *docA, Dict *dict, Ref *refA);
  ~Annot();
  GBool isOk() { return ok; }

  void draw(Gfx *gfx, GBool printing);

  GString *getType() { return type; }
  double getXMin() { return xMin; }
  double getYMin() { return yMin; }
  double getXMax() { return xMax; }
  double getYMax() { return yMax; }
  Object *getObject(Object *obj);

  // Get appearance object.
  Object *getAppearance(Object *obj) { return appearance.fetch(xref, obj); }

  AnnotBorderStyle *getBorderStyle() { return borderStyle; }

  GBool match(Ref *refA)
    { return ref.num == refA->num && ref.gen == refA->gen; }

  void generateFieldAppearance(Dict *field, Dict *annot, Dict *acroForm);

private:
 
  void setColor(Array *a, GBool fill, int adjust);
  void drawText(GString *text, GString *da, GfxFontDict *fontDict,
		GBool multiline, int comb, int quadding,
		GBool txField, GBool forceZapfDingbats, int rot);
  void drawListBox(GString **text, GBool *selection,
		   int nOptions, int topIdx,
		   GString *da, GfxFontDict *fontDict, GBool quadding);
  void getNextLine(GString *text, int start,
		   GfxFont *font, double fontSize, double wMax,
		   int *end, double *width, int *next);
  void drawCircle(double cx, double cy, double r, GBool fill);
  void drawCircleTopLeft(double cx, double cy, double r);
  void drawCircleBottomRight(double cx, double cy, double r);
  Object *fieldLookup(Dict *field, Dict *acroForm,
		      const char *key, Object *obj);

  PDFDoc *doc;
  XRef *xref;			// the xref table for this PDF file
  Ref ref;			// object ref identifying this annotation
  GString *type;		// annotation type
  GString *appearanceState;	// appearance state name
  Object appearance;		// a reference to the Form XObject stream
				//   for the normal appearance
  GString *appearBuf;
  double xMin, yMin,		// annotation rectangle
         xMax, yMax;
  Guint flags;
  AnnotBorderStyle *borderStyle;
  Object ocObj;			// optional content entry
  GBool ok;
};

//------------------------------------------------------------------------
// Annots
//------------------------------------------------------------------------

class Annots {
public:

  // Build a list of Annot objects.
  Annots(PDFDoc *docA, Object *annotsObj);

  ~Annots();

  // Iterate through list of annotations.
  int getNumAnnots() { return nAnnots; }
  Annot *getAnnot(int i) { return annots[i]; }

  // (Re)generate the appearance streams for all annotations belonging
  // to a form field.
  void generateAppearances();

private:

  void scanFieldAppearances(Dict *node, Ref *ref, Dict *parent,
			    Dict *acroForm);
  Annot *findAnnot(Ref *ref);

  PDFDoc *doc;
  Annot **annots;
  int nAnnots;
};

#endif
