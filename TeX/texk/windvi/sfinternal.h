/*
 * Copyright 1989 Software Research Associates, Inc., Tokyo, Japan
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Software Research Associates not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Software Research Associates
 * makes no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * SOFTWARE RESEARCH ASSOCIATES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL SOFTWARE RESEARCH ASSOCIATES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Erik M. van der Poel
 *         Software Research Associates, Inc., Tokyo, Japan
 *         erik@sra.co.jp
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#undef wchar_t

#define SEL_FILE_CANCEL		-1
#define SEL_FILE_OK		0
#define SEL_FILE_NULL		1
#define SEL_FILE_TEXT		2

#define SF_DO_SCROLL		1
#define SF_DO_NOT_SCROLL	0

typedef struct {
	int	statDone;
	char	*real;
	char	*shown;
} SFEntry;

typedef struct {
	char	*dir;
	char	*path;
	SFEntry	*entries;
	int	nEntries;
	int	vOrigin;
	int	nChars;
	int	hOrigin;
	int	changed;
	int	beginSelection;
	int	endSelection;
	time_t	mtime;
} SFDir;

extern int SFstatus;

extern char SFcurrentPath[], SFstartDir[], SFcurrentDir[];

extern Widget
		selFile,
		selFileCancel,
		selFileField,
		selFileForm,
		selFileHScroll,
		selFileHScrolls[],
		selFileLists[],
		selFileOK,
		selFilePrompt,
		selFileVScrolls[];

extern Display *SFdisplay;

extern int SFcharWidth, SFcharHeight, SFcharAscent;

extern SFDir *SFdirs;

extern int SFdirEnd, SFdirPtr;

extern Pixel SFfore, SFback;

extern Atom SFwmDeleteWindow;

extern XSegment SFsegs[], SFcompletionSegs[];

extern XawTextPosition SFtextPos;

extern void
	SFenterList(),
	SFleaveList(),
	SFmotionList(),
	SFbuttonPressList(),
	SFbuttonReleaseList();

extern void
	SFvSliderMovedCallback(),
	SFvFloatSliderMovedCallback(),
	SFhSliderMovedCallback(),
	SFpathSliderMovedCallback(),
	SFvAreaSelectedCallback(),
	SFhAreaSelectedCallback(),
	SFpathAreaSelectedCallback();

extern int SFupperX, SFlowerY, SFupperY;

extern int SFtextX, SFtextYoffset;

extern int SFentryWidth, SFentryHeight;

extern int SFlineToTextH, SFlineToTextV;

extern int SFbesideText, SFaboveAndBelowText;

extern int SFcharsPerEntry;

extern int SFlistSize;

extern int SFcurrentInvert[];

extern int SFworkProcAdded;

extern Boolean SFworkProc();

extern XtAppContext SFapp;

extern int SFpathScrollWidth, SFvScrollHeight, SFhScrollWidth;

extern char SFtextBuffer[];

extern int SFbuttonPressed;

extern int SFcompareEntries();

extern void SFdirModTimer();

extern char SFstatChar();

extern XtIntervalId SFdirModTimerId;

extern int (*SFfunc)();
