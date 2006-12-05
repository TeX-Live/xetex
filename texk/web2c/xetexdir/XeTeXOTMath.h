#ifndef __XETEX_OT_MATH__
#define __XETEX_OT_MATH__

#include "MathTable.h"

#include "LEFontInstance.h"

/* public "C" APIs for calling from Web(-to-C) code */
extern "C" {
	int getnativemathsyparam(int f, int n);
	int getnativemathexparam(int f, int n);
	int getotmathconstant(int f, int n);
	int getotmathvariant(int f, int g, int v, int* adv);
	void* getotassemblyptr(int f, int g);
	int getotmathitalcorr(int f, int g);
	int otpartcount(const GlyphAssembly* a);
	int otpartglyph(const GlyphAssembly* a, int i);
	int otpartisextender(const GlyphAssembly* a, int i);
	int otpartstartconnector(int f, const GlyphAssembly* a, int i);
	int otpartendconnector(int f, const GlyphAssembly* a, int i);
	int otpartfulladvance(int f, const GlyphAssembly* a, int i);
	int otminconnectoroverlap(int f);
};


/* internal functions */

/* get a math font constant, scaled according to the font size */
int getMathConstant(LEFontInstance*	fontInst, mathConstantIndex whichConstant);


#endif
