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

#ifdef HAVE_CONFIG_H
#include "c-auto.h"
#endif

#if !defined (NOSELFILE) && !defined (NOTOOL) /* for xdvik */

#include <kpathsea/c-auto.h>
#include <kpathsea/config.h>
#include <kpathsea/c-stat.h>

#include <stdio.h>
#include "sfinternal.h"
#include <X11/StringDefs.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Cardinals.h>

#define SF_DEFAULT_FONT "9x15"

#ifdef ABS
#undef ABS
#endif
#define ABS(x) (((x) < 0) ? (-(x)) : (x))

typedef struct {
	char *fontname;
} TextData, *textPtr;

int SFcharWidth, SFcharAscent, SFcharHeight;

int SFcurrentInvert[3] = { -1, -1, -1 };

static GC SFlineGC, SFscrollGC, SFinvertGC, SFtextGC;

static XtResource textResources[] = {
	{XtNfont, XtCFont, XtRString, sizeof (char *),
		XtOffset(textPtr, fontname), XtRString, SF_DEFAULT_FONT},
};

static XFontStruct *SFfont;

static int SFcurrentListY;

static XtIntervalId SFscrollTimerId;

void
SFinitFont()
{
	TextData	*data;

	data = XtNew(TextData);

	XtGetApplicationResources(selFileForm, (XtPointer) data, textResources,
		XtNumber(textResources), (Arg *) NULL, ZERO);

	SFfont = XLoadQueryFont(SFdisplay, data->fontname);
	if (!SFfont) {
		SFfont = XLoadQueryFont(SFdisplay, SF_DEFAULT_FONT);
		if (!SFfont) {
			char	sbuf[256];

			(void) sprintf(sbuf, "XsraSelFile: can't get font %s",
				SF_DEFAULT_FONT);

			XtAppError(SFapp, sbuf);
		}
	}

	SFcharWidth = (SFfont->max_bounds.width + SFfont->min_bounds.width) / 2;
	SFcharAscent = SFfont->max_bounds.ascent;
	SFcharHeight = SFcharAscent + SFfont->max_bounds.descent;
}

void
SFcreateGC()
{
	XGCValues	gcValues;
	XRectangle	rectangles[1];

	gcValues.foreground = SFfore;

	SFlineGC = XtGetGC(
		selFileLists[0],
		(XtGCMask)
			GCForeground		|
			0,
		&gcValues
	);

	SFscrollGC = XtGetGC(
		selFileLists[0],
		(XtGCMask)
			0,
		&gcValues
	);

	gcValues.function = GXinvert;
	gcValues.plane_mask = (SFfore ^ SFback);

	SFinvertGC = XtGetGC(
		selFileLists[0],
		(XtGCMask)
			GCFunction		|
			GCPlaneMask		|
			0,
		&gcValues
	);

	gcValues.foreground = SFfore;
	gcValues.background = SFback;
	gcValues.font = SFfont->fid;

	SFtextGC = XCreateGC(
		SFdisplay,
		XtWindow(selFileLists[0]),
		(unsigned long)
			GCForeground		|
			GCBackground		|
			GCFont			|
			0,
		&gcValues
	);

	rectangles[0].x = SFlineToTextH + SFbesideText;
	rectangles[0].y = 0;
	rectangles[0].width = SFcharsPerEntry * SFcharWidth;
	rectangles[0].height = SFupperY + 1;

	XSetClipRectangles(
		SFdisplay,
		SFtextGC,
		0,
		0,
		rectangles,
		1,
		Unsorted
	);
}

void
SFclearList(n, doScroll)
	int	n;
	int	doScroll;
{
	SFDir	*dir;

	SFcurrentInvert[n] = -1;

	XClearWindow(SFdisplay, XtWindow(selFileLists[n]));

	XDrawSegments(SFdisplay, XtWindow(selFileLists[n]), SFlineGC, SFsegs,
		2);

	if (doScroll) {
		dir = &(SFdirs[SFdirPtr + n]);

		if ((SFdirPtr + n < SFdirEnd) && dir->nEntries && dir->nChars) {
			XawScrollbarSetThumb(
				selFileVScrolls[n],
				(float) (((double) dir->vOrigin) /
					dir->nEntries),
				(float) (((double) ((dir->nEntries < SFlistSize)
					? dir->nEntries : SFlistSize)) /
					dir->nEntries)
			);

			XawScrollbarSetThumb(
				selFileHScrolls[n],
				(float) (((double) dir->hOrigin) / dir->nChars),
				(float) (((double) ((dir->nChars <
					SFcharsPerEntry) ? dir->nChars :
					SFcharsPerEntry)) / dir->nChars)
			);
		} else {
			XawScrollbarSetThumb(selFileVScrolls[n], (float) 0.0,
				(float) 1.0);
			XawScrollbarSetThumb(selFileHScrolls[n], (float) 0.0,
				(float) 1.0);
		}
	}
}

static void
SFdeleteEntry(dir, entry)
	SFDir	*dir;
	SFEntry	*entry;
{
	register SFEntry	*e;
	register SFEntry	*end;
	int			n;
	int			idx;

	idx = entry - dir->entries;

	if (idx < dir->beginSelection) {
		dir->beginSelection--;
	}
	if (idx <= dir->endSelection) {
		dir->endSelection--;
	}
	if (dir->beginSelection > dir->endSelection) {
		dir->beginSelection = dir->endSelection = -1;
	}

	if (idx < dir->vOrigin) {
		dir->vOrigin--;
	}

	XtFree(entry->real);

	end = &(dir->entries[dir->nEntries - 1]);

	for (e = entry; e < end; e++) {
		*e = *(e + 1);
	}

	if (!(--dir->nEntries)) {
		return;
	}

	n = dir - &(SFdirs[SFdirPtr]);
	if ((n < 0) || (n > 2)) {
		return;
	}

	XawScrollbarSetThumb(
		selFileVScrolls[n],
		(float) (((double) dir->vOrigin) / dir->nEntries),
		(float) (((double) ((dir->nEntries < SFlistSize) ?
			dir->nEntries : SFlistSize)) / dir->nEntries)
	);
}

static void
SFwriteStatChar(name, last, statBuf)
	char		*name;
	int		last;
	struct stat	*statBuf;
{
	name[last] = SFstatChar(statBuf);
}

static int
SFstatAndCheck(dir, entry)
	SFDir	*dir;
	SFEntry	*entry;
{
	struct stat	statBuf;
	char		save;
	int		last;
	extern int SFchdir ();
	
	/*
	 * must be restored before returning
	 */
	save = *(dir->path);
	*(dir->path) = 0;

	if (!SFchdir(SFcurrentPath)) {
		last = strlen(entry->real) - 1;
		entry->real[last] = 0;
		entry->statDone = 1;
		if (
			(!stat(entry->real, &statBuf))

#ifdef S_IFLNK

		     || (!lstat(entry->real, &statBuf))

#endif /* ndef S_IFLNK */

		) {
			if (SFfunc) {
				char *shown;

				shown = NULL;
				if (SFfunc(entry->real, &shown, &statBuf)) {
					if (shown) {
						int len;

						len = strlen(shown);
						entry->shown = XtMalloc(
							(unsigned) (len + 2)
						);
						(void) strcpy(entry->shown,
							shown);
						SFwriteStatChar(
							entry->shown,
							len,
							&statBuf
						);
						entry->shown[len + 1] = 0;
					}
				} else {
					SFdeleteEntry(dir, entry);

					*(dir->path) = save;
					return 1;
				}
			}
			SFwriteStatChar(entry->real, last, &statBuf);
		} else {
			entry->real[last] = ' ';
		}
	}

	*(dir->path) = save;
	return 0;
}

static void
SFdrawStrings(w, dir, from, to)
	register Window	w;
	register SFDir	*dir;
	register int	from;
	register int	to;
{
	register int		i;
	register SFEntry	*entry;
	int			x;

	x = SFtextX - dir->hOrigin * SFcharWidth;

	if (dir->vOrigin + to >= dir->nEntries) {
		to = dir->nEntries - dir->vOrigin - 1;
	}
	for (i = from; i <= to; i++) {
		entry = &(dir->entries[dir->vOrigin + i]);
		if (!(entry->statDone)) {
			if (SFstatAndCheck(dir, entry)) {
				if (dir->vOrigin + to >= dir->nEntries) {
					to = dir->nEntries - dir->vOrigin - 1;
				}
				i--;
				continue;
			}
		}
		XDrawImageString(
			SFdisplay,
			w,
			SFtextGC,
			x,
			SFtextYoffset + i * SFentryHeight,
			entry->shown,
			strlen(entry->shown)
		);
		if (dir->vOrigin + i == dir->beginSelection) {
			XDrawLine(
				SFdisplay,
				w,
				SFlineGC,
				SFlineToTextH + 1,
				SFlowerY + i * SFentryHeight,
				SFlineToTextH + SFentryWidth - 2,
				SFlowerY + i * SFentryHeight
			);
		}
		if (
			(dir->vOrigin + i >= dir->beginSelection) &&
			(dir->vOrigin + i <= dir->endSelection)
		) {
			SFcompletionSegs[0].y1 = SFcompletionSegs[1].y1 =
				SFlowerY + i * SFentryHeight;
			SFcompletionSegs[0].y2 = SFcompletionSegs[1].y2 =
				SFlowerY + (i + 1) * SFentryHeight - 1;
			XDrawSegments(
				SFdisplay,
				w,
				SFlineGC,
				SFcompletionSegs,
				2
			);
		}
		if (dir->vOrigin + i == dir->endSelection) {
			XDrawLine(
				SFdisplay,
				w,
				SFlineGC,
				SFlineToTextH + 1,
				SFlowerY + (i + 1) * SFentryHeight - 1,
				SFlineToTextH + SFentryWidth - 2,
				SFlowerY + (i + 1) * SFentryHeight - 1
			);
		}
	}
}

void
SFdrawList(n, doScroll)
	int	n;
	int	doScroll;
{
	SFDir	*dir;
	Window	w;

	SFclearList(n, doScroll);

	if (SFdirPtr + n < SFdirEnd) {
		dir = &(SFdirs[SFdirPtr + n]);
		w = XtWindow(selFileLists[n]);
		XDrawImageString(
			SFdisplay,
			w,
			SFtextGC,
			SFtextX - dir->hOrigin * SFcharWidth,
			SFlineToTextV + SFaboveAndBelowText + SFcharAscent,
			dir->dir,
			strlen(dir->dir)
		);
		SFdrawStrings(w, dir, 0, SFlistSize - 1);
	}
}

void
SFdrawLists(doScroll)
	int	doScroll;
{
	int	i;

	for (i = 0; i < 3; i++) {
		SFdrawList(i, doScroll);
	}
}

static void
SFinvertEntry(n)
	register int	n;
{
	XFillRectangle(
		SFdisplay,
		XtWindow(selFileLists[n]),
		SFinvertGC,
		SFlineToTextH,
		SFcurrentInvert[n] * SFentryHeight + SFlowerY,
		SFentryWidth,
		SFentryHeight
	);
}

static unsigned long
SFscrollTimerInterval()
{
	static int	maxVal = 200;
	static int	varyDist = 50;
	static int	minDist = 50;
	int		t;
	int		dist;

	if (SFcurrentListY < SFlowerY) {
		dist = SFlowerY - SFcurrentListY;
	} else if (SFcurrentListY > SFupperY) {
		dist = SFcurrentListY - SFupperY;
	} else {
		return (unsigned long) 1;
	}

	t = maxVal - ((maxVal / varyDist) * (dist - minDist));

	if (t < 1) {
		t = 1;
	}

	if (t > maxVal) {
		t = maxVal;
	}

	return (unsigned long) t;
}

static void
SFscrollTimer(p, id)
	XtPointer	p;
        XtIntervalId    *id;
{
	SFDir	*dir;
	int	save;
	int     n;

        n = (int) p;

	dir = &(SFdirs[SFdirPtr + n]);
	save = dir->vOrigin;

	if (SFcurrentListY < SFlowerY) {
		if (dir->vOrigin > 0) {
			SFvSliderMovedCallback(selFileVScrolls[n], n,
				dir->vOrigin - 1);
		}
	} else if (SFcurrentListY > SFupperY) {
		if (dir->vOrigin < dir->nEntries - SFlistSize) {
			SFvSliderMovedCallback(selFileVScrolls[n], n,
				dir->vOrigin + 1);
		}
	}

	if (dir->vOrigin != save) {
		if (dir->nEntries) {
		    XawScrollbarSetThumb(
			selFileVScrolls[n],
			(float) (((double) dir->vOrigin) / dir->nEntries),
			(float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries)
		    );
		}
	}

	if (SFbuttonPressed) {
		SFscrollTimerId = XtAppAddTimeOut(SFapp,
			SFscrollTimerInterval(), SFscrollTimer, (XtPointer) n);
	}
}

static int
SFnewInvertEntry(n, event)
	register int		n;
	register XMotionEvent	*event;
{
	register int	x, y;
	register int	new;
	static int	SFscrollTimerAdded = 0;

	x = event->x;
	y = event->y;

	if (SFdirPtr + n >= SFdirEnd) {
		return -1;
	} else if (
		(x >= 0)	&& (x <= SFupperX) &&
		(y >= SFlowerY)	&& (y <= SFupperY)
	) {
		register SFDir *dir = &(SFdirs[SFdirPtr + n]);

		if (SFscrollTimerAdded) {
			SFscrollTimerAdded = 0;
			XtRemoveTimeOut(SFscrollTimerId);
		}

		new = (y - SFlowerY) / SFentryHeight;
		if (dir->vOrigin + new >= dir->nEntries) {
			return -1;
		}
		return new;
	} else {
		if (SFbuttonPressed) {
			SFcurrentListY = y;
			if (!SFscrollTimerAdded) {
				SFscrollTimerAdded = 1;
				SFscrollTimerId = XtAppAddTimeOut(SFapp,
					SFscrollTimerInterval(), SFscrollTimer,
					(XtPointer) n);
			}
		}

		return -1;
	}
}

/* ARGSUSED */
void
SFenterList(w, n, event)
	Widget				w;
	register int			n;
	register XEnterWindowEvent	*event;
{
	register int	new;

	/* sanity */
	if (SFcurrentInvert[n] != -1) {
		SFinvertEntry(n);
		SFcurrentInvert[n] = -1;
	}

	new = SFnewInvertEntry(n, (XMotionEvent *) event);
	if (new != -1) {
		SFcurrentInvert[n] = new;
		SFinvertEntry(n);
	}
}

/* ARGSUSED */
void
SFleaveList(w, n, event)
	Widget		w;
	register int	n;
	XEvent		*event;
{
	if (SFcurrentInvert[n] != -1) {
		SFinvertEntry(n);
		SFcurrentInvert[n] = -1;
	}
}

/* ARGSUSED */
void
SFmotionList(w, n, event)
	Widget			w;
	register int		n;
	register XMotionEvent	*event;
{
	register int	new;

	new = SFnewInvertEntry(n, event);

	if (new != SFcurrentInvert[n]) {
		if (SFcurrentInvert[n] != -1) {
			SFinvertEntry(n);
		}
		SFcurrentInvert[n] = new;
		if (new != -1) {
			SFinvertEntry(n);
		}
	}
}

/* ARGSUSED */
void
SFvFloatSliderMovedCallback(w, n, fnew)
	Widget	w;
	int	n;
	float	*fnew;
{
	int	new;

	new = (*fnew) * SFdirs[SFdirPtr + n].nEntries;

	SFvSliderMovedCallback(w, n, new);
}

/* ARGSUSED */
void
SFvSliderMovedCallback(w, n, new)
	Widget	w;
	int	n;
	int	new;
{
	int		old;
	register Window	win;
	SFDir		*dir;

	dir = &(SFdirs[SFdirPtr + n]);

	old = dir->vOrigin;
	dir->vOrigin = new;

	if (old == new) {
		return;
	}

	win = XtWindow(selFileLists[n]);

	if (ABS(new - old) < SFlistSize) {
		if (new > old) {
			XCopyArea(
				SFdisplay,
				win,
				win,
				SFscrollGC,
				SFlineToTextH,
				SFlowerY + (new - old) * SFentryHeight,
				SFentryWidth + SFlineToTextH,
				(SFlistSize - (new - old)) * SFentryHeight,
				SFlineToTextH,
				SFlowerY
			);
			XClearArea(
				SFdisplay,
				win,
				SFlineToTextH,
				SFlowerY + (SFlistSize - (new - old)) *
					SFentryHeight,
				SFentryWidth + SFlineToTextH,
				(new - old) * SFentryHeight,
				False
			);
			SFdrawStrings(win, dir, SFlistSize - (new - old),
				SFlistSize - 1);
		} else {
			XCopyArea(
				SFdisplay,
				win,
				win,
				SFscrollGC,
				SFlineToTextH,
				SFlowerY,
				SFentryWidth + SFlineToTextH,
				(SFlistSize - (old - new)) * SFentryHeight,
				SFlineToTextH,
				SFlowerY + (old - new) * SFentryHeight
			);
			XClearArea(
				SFdisplay,
				win,
				SFlineToTextH,
				SFlowerY,
				SFentryWidth + SFlineToTextH,
				(old - new) * SFentryHeight,
				False
			);
			SFdrawStrings(win, dir, 0, old - new);
		}
	} else {
		XClearArea(
			SFdisplay,
			win,
			SFlineToTextH,
			SFlowerY,
			SFentryWidth + SFlineToTextH,
			SFlistSize * SFentryHeight,
			False
		);
		SFdrawStrings(win, dir, 0, SFlistSize - 1);
	}
}

/* ARGSUSED */
void
SFvAreaSelectedCallback(w, n, pnew)
	Widget	w;
	int	n;
	int	pnew;
{
	SFDir	*dir;
	int	new;

	dir = &(SFdirs[SFdirPtr + n]);

	new = dir->vOrigin +
		(((double) pnew) / SFvScrollHeight) * dir->nEntries;

	if (new > dir->nEntries - SFlistSize) {
		new = dir->nEntries - SFlistSize;
	}

	if (new < 0) {
		new = 0;
	}

	if (dir->nEntries) {
		float	f;

		f = ((double) new) / dir->nEntries;

		XawScrollbarSetThumb(
			w,
			f,
			(float) (((double) ((dir->nEntries < SFlistSize) ?
				dir->nEntries : SFlistSize)) / dir->nEntries)
		);
	}

	SFvSliderMovedCallback(w, n, new);
}

/* ARGSUSED */
void
SFhSliderMovedCallback(w, n, new)
	Widget	w;
	int	n;
	float	*new;
{
	SFDir	*dir;
	int	save;

	dir = &(SFdirs[SFdirPtr + n]);
	save = dir->hOrigin;
	dir->hOrigin = (*new) * dir->nChars;
	if (dir->hOrigin == save) {
		return;
	}

	SFdrawList(n, SF_DO_NOT_SCROLL);
}

/* ARGSUSED */
void
SFhAreaSelectedCallback(w, n, pnew)
	Widget	w;
	int	n;
	int	pnew;
{
	SFDir	*dir;
	int	new;

	dir = &(SFdirs[SFdirPtr + n]);

	new = dir->hOrigin +
		(((double) pnew) / SFhScrollWidth) * dir->nChars;

	if (new > dir->nChars - SFcharsPerEntry) {
		new = dir->nChars - SFcharsPerEntry;
	}

	if (new < 0) {
		new = 0;
	}

	if (dir->nChars) {
		float	f;

		f = ((double) new) / dir->nChars;

		XawScrollbarSetThumb(
			w,
			f,
			(float) (((double) ((dir->nChars < SFcharsPerEntry) ?
				dir->nChars : SFcharsPerEntry)) / dir->nChars)
		);

		SFhSliderMovedCallback(w, n, &f);
	}
}

/* ARGSUSED */
void
SFpathSliderMovedCallback(w, client_data, new)
	Widget		w;
	XtPointer	client_data;
	float	*new;
{
	SFDir		*dir;
	int		n;
	XawTextPosition	pos;
	int	SFdirPtrSave;

	SFdirPtrSave = SFdirPtr;
	SFdirPtr = (*new) * SFdirEnd;
	if (SFdirPtr == SFdirPtrSave) {
		return;
	}

	SFdrawLists(SF_DO_SCROLL);

	n = 2;
	while (SFdirPtr + n >= SFdirEnd) {
		n--;
	}

	dir = &(SFdirs[SFdirPtr + n]);

	pos = dir->path - SFcurrentPath;

	if (!strncmp(SFcurrentPath, SFstartDir, strlen(SFstartDir))) {
		pos -= strlen(SFstartDir);
		if (pos < 0) {
			pos = 0;
		}
	}

	XawTextSetInsertionPoint(selFileField, pos);
}

/* ARGSUSED */

void
SFpathAreaSelectedCallback(w, client_data, pnew)
	Widget		w;
	XtPointer	client_data;
	int		pnew;
{
	int	new;
	float	f;

	new = SFdirPtr + (((double) pnew) / SFpathScrollWidth) * SFdirEnd;

	if (new > SFdirEnd - 3) {
		new = SFdirEnd - 3;
	}

	if (new < 0) {
		new = 0;
	}

	f = ((double) new) / SFdirEnd;

	XawScrollbarSetThumb(
		w,
		f,
		(float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) /
			SFdirEnd)
	);

	SFpathSliderMovedCallback(w, (XtPointer) NULL, &f);
}

Boolean
SFworkProc()
{
	register SFDir		*dir;
	register SFEntry	*entry;

	for (dir = &(SFdirs[SFdirEnd - 1]); dir >= SFdirs; dir--) {
		if (!(dir->nEntries)) {
			continue;
		}
		for (
			entry = &(dir->entries[dir->nEntries - 1]);
			entry >= dir->entries;
			entry--
		) {
			if (!(entry->statDone)) {
				(void) SFstatAndCheck(dir, entry);
				return False;
			}
		}
	}

	SFworkProcAdded = 0;

	return True;
}

#endif /* not NOSELFILE and not NOTOOL */

