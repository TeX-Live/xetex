/*======================================================================*\

Copyright (c) 1990-2004  Paul Vojta and others

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
PAUL VOJTA OR ANY OTHER AUTHOR OF THIS SOFTWARE BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

NOTE:
	xdvi is based on prior work, as noted in the modification history
	in xdvi.c.

\*========================================================================*/

#include "xdvi-config.h"

#ifdef STDC_HEADERS
# include <unistd.h>
# include <fcntl.h>
#endif
#include <signal.h>
#include <sys/file.h>	/* this defines FASYNC */
#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>  /* Or this might define FASYNC */
#endif
#include <sys/ioctl.h>	/* this defines SIOCSPGRP and FIOASYNC */
#include <sys/wait.h>	/* ignore HAVE_SYS_WAIT_H -- we always need WNOHANG */

#include "xdvi.h" /* this includes Xlib and Xutil are already included */
#include "xdvi-debug.h"


#ifndef MOTIF
#include <X11/Xaw/Form.h> /* for XtNresizable */
#endif

#include <setjmp.h>

#include <X11/IntrinsicP.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>	/* needed for def. of XtNiconX(??) */

#include "pagesel.h"
#include "filehist.h"
#include "special.h"
#include "psgs.h"


#include <errno.h>

#include <ctype.h>

#include "util.h"
#include "x_util.h"
#include "string-utils.h"
#include "print-dialog.h"
#include "search-dialog.h"
#include "sfSelFile.h"
#include "mag.h"
#include "help-window.h"
#include "message-window.h"
#include "dvi-draw.h"
#include "statusline.h"
#include "hypertex.h"
#include "dvi-init.h"
#include "Tip.h"
#include "browser.h"
#include "search-internal.h"
#include "my-snprintf.h"

#include "events.h"
#include "selection.h"
#include "encodings.h"
#include "pagehist.h"
#include "xm_colorsel.h"
#include "xm_toolbar.h"
#include "xaw_menu.h"
#include "xm_menu.h"
#include "xm_prefs.h"

#include "xm_prefs_appearance.h" /* for update_preferences_expert() */
#include "xm_prefs_fonts.h"	 /* for update_preferences_color() */
#include "xm_prefs_page.h"	 /* for update_preferences_shrink() */

#ifdef	X_NOT_STDC_ENV
extern int errno;
#endif /* X_NOT_STDC_ENV */

/* Linux prefers O_ASYNC over FASYNC; SGI IRIX does the opposite.  */
#if !defined(FASYNC) && defined(O_ASYNC)
# define FASYNC	O_ASYNC
#endif

#if !defined(FLAKY_SIGPOLL) && !HAVE_STREAMS && !defined(FASYNC)
# if !defined(SIOCSPGRP) || !defined(FIOASYNC)
#  define FLAKY_SIGPOLL	1
# endif
#endif

#ifndef FLAKY_SIGPOLL

# ifndef SIGPOLL
#  define SIGPOLL	SIGIO
# endif

# ifndef SA_RESTART
#  define SA_RESTART 0
# endif

# if HAVE_STREAMS
#  include <stropts.h>

#  ifndef S_RDNORM
#   define S_RDNORM S_INPUT
#  endif

#  ifndef S_RDBAND
#   define S_RDBAND  0
#  endif

#  ifndef S_HANGUP
#   define S_HANGUP  0
#  endif

#  ifndef S_WRNORM
#   define S_WRNORM  S_OUTPUT
#  endif
# endif /* HAVE_STREAMS */

#endif /* not FLAKY_SIGPOLL */

#if HAVE_SIGACTION && !defined SA_RESETHAND
# ifdef SA_ONESHOT
#  define SA_RESETHAND SA_ONESHOT
# else
#  undef HAVE_SIGACTION         /* Needed for Mac OS X < 10.2 (9/2002) */
# endif
#endif

#if HAVE_POLL
# include <poll.h>
#else
# if HAVE_SYS_SELECT_H
#  include <sys/select.h>
# else
#  if HAVE_SELECT_H
#   include <select.h>
#  endif
# endif
# define XIO_IN 1
# define XIO_OUT 2
#endif /* HAVE_POLL */

static sigset_t all_signals;

/*
 *	Interrupt system for receiving events.  The program sets a flag
 *	whenever an event comes in, so that at the proper time (i.e., when
 *	reading a new dvi item), we can check incoming events to see if we
 *	still want to go on printing this page.  This way, one can stop
 *	displaying a page if it is about to be erased anyway.  We try to read
 *	as many events as possible before doing anything and base the next
 *	action on all events read.
 *	Note that the Xlib and Xt routines are not reentrant, so the most we
 *	can do is set a flag in the interrupt routine and check it later.
 *	Also, sometimes the interrupts are not generated (some systems only
 *	guarantee that SIGIO is generated for terminal files, and on the system
 *	I use, the interrupts are not generated if I use "(xdvi foo &)" instead
 *	of "xdvi foo").  Therefore, there is also a mechanism to check the
 *	event queue every 70 drawing operations or so.  This mechanism is
 *	disabled if it turns out that the interrupts do work.
 *	For a fuller discussion of some of the above, see xlife in
 *	comp.sources.x.
 */

/*
 *	Signal flags
 */

/* This could be static volatile, but we want to avoid any optimizer bugs.  */
VOLATILE unsigned int sig_flags	= 0;

#define	SF_USR	1
#define	SF_ALRM	2
#define	SF_POLL	4
#define	SF_CHLD	8
#define	SF_TERM	16
#define	SF_SEGV	32

static void do_sigusr(void);
static void do_sigalrm(void);
static void do_sigpoll(void);
static void do_sigchld(void);
static void do_sigterm(void);
static void do_sigsegv(void);

/* these must be in the same order as SF_*.
   The higher flags have higher priority, since these are
   checked first when resolving flags_to_sigproc[sig_flags].

   Example: flag := SF_TERM | SF_CHLD = 24
   
   flags_to_sigproc[24] =>
       do_sigterm, which sets flag to 24 & ~SF_TERM == 8
   then, in next check:       
   flags_to_sigproc[8] =>
       do_sigchld, which sets flag 8 & ~SF_CHLD == 0.
 */
#define	SP0	do_sigusr
#define	SP1	do_sigalrm
#define	SP2	do_sigpoll
#define	SP3	do_sigchld
#define	SP4	do_sigterm
#define	SP5	do_sigsegv

typedef	void	(*signalproc)(void);

static const signalproc flags_to_sigproc[64] = {
    NULL,
    /* 1 */
    SP0,
    /* 2 - 3 */
    SP1, SP1,
    /* 4 - 7 */
    SP2, SP2, SP2, SP2,
    /* 8 - 15 */
    SP3, SP3, SP3, SP3, SP3, SP3, SP3, SP3,
    /* 16 - 31 */
    SP4, SP4, SP4, SP4, SP4, SP4, SP4, SP4,
    SP4, SP4, SP4, SP4, SP4, SP4, SP4, SP4,
    /* 32 - 63 */
    SP5, SP5, SP5, SP5, SP5, SP5, SP5, SP5,
    SP5, SP5, SP5, SP5, SP5, SP5, SP5, SP5,
    SP5, SP5, SP5, SP5, SP5, SP5, SP5, SP5,
    SP5, SP5, SP5, SP5, SP5, SP5, SP5, SP5
};

#undef	SP0
#undef	SP2
#undef	SP3
#undef	SP4
#undef	SP5

/* file-static variables for prefix argument mechanism */
static Boolean m_have_arg = False; /* flag whether we have a possible prefix arg */
static int m_number = 0;        /* prefix arg value, without the sign */
static int m_sign = 1;          /* is prefix arg negative? */


/* to remember the scrollbar positions so that we can restore them
   when `keep position' is active and window is resized to full size
   (removing scrollbars) and then back (#810501) */
static int m_x_scroll = 0, m_y_scroll = 0;

static int source_reverse_x, source_reverse_y;
static int source_show_all;

/* globals for color stuff */
Pixel plane_masks[4];
XColor color_data[2];
struct pagecolor_info page_colors = { 0, NULL };

struct rgb *color_bottom;
unsigned int color_bot_size;		/* number of entries */
struct colorframe *rcs_top;
struct rgb fg_initial;			/* Initial fg (from command line) */
struct rgb bg_initial;			/* Initial bg */
struct bgrec *bg_head = NULL;		/* head of list */
struct bgrec *bg_current = NULL;	/* current bg value */
struct fgrec *fg_current;		/* current fg value */
struct fgrec *fg_active = NULL;		/* where the GCs are */
Pixel *color_list;			/* list of colors */
unsigned int color_list_len = 0;	/* current len of list*/
unsigned int color_list_max = 0;	/* allocated size */
Boolean color_warned = False;
/*
  FIXME: the following file-static flag is used to control whether
  a mouse-related Act_* should block further actions that have
  been registered for the same event.
  
  It is used for the Act_href_* routines.
  
  This used to be a
     longjmp(somewehre_in_the_event_loop);
  but that was pretty dreadful as well. Surely there must
  be some other (X(t)-specific?) way to do this?? (Apart from
  de-registering the action and re-registering it again.)
*/
static Boolean m_block_mouse_event = False; 

/* for calling it from mag.c */
void
block_next_mouse_event(void) {
    m_block_mouse_event = True;
}

Boolean
block_this_mouse_event(void) {
    if (m_block_mouse_event == True) {
	m_block_mouse_event = False;
	return True;
    }
    return False;
}


void null_mouse(XEvent *event)
{
    UNUSED(event);
}

mouse_proc mouse_motion = null_mouse;
mouse_proc mouse_release = null_mouse;

static void Act_digit(Widget, XEvent *, String *, Cardinal *);
static void Act_find(Widget, XEvent *, String *, Cardinal *);
static void Act_find_next(Widget, XEvent *, String *, Cardinal *);
static void Act_minus(Widget, XEvent *, String *, Cardinal *);
static void Act_quit(Widget, XEvent *, String *, Cardinal *);
static void Act_quit_confirm(Widget, XEvent *, String *, Cardinal *);
static void Act_print(Widget, XEvent *, String *, Cardinal *);
static void Act_save(Widget, XEvent *, String *, Cardinal *);
static void Act_help(Widget, XEvent *, String *, Cardinal *);
static void Act_goto_page(Widget, XEvent *, String *, Cardinal *);
static void Act_declare_page_number(Widget, XEvent *, String *, Cardinal *);
static void Act_toggle_mark(Widget, XEvent *, String *, Cardinal *);
static void Act_home(Widget, XEvent *, String *, Cardinal *);
static void Act_home_or_top(Widget, XEvent *, String *, Cardinal *);
static void Act_end_or_bottom(Widget, XEvent *, String *, Cardinal *);
static void Act_center(Widget, XEvent *, String *, Cardinal *);
static void Act_left(Widget, XEvent *, String *, Cardinal *);
static void Act_right(Widget, XEvent *, String *, Cardinal *);
static void Act_up(Widget, XEvent *, String *, Cardinal *);
static void Act_down(Widget, XEvent *, String *, Cardinal *);
static void Act_up_or_previous(Widget, XEvent *, String *, Cardinal *);
static void Act_down_or_next(Widget, XEvent *, String *, Cardinal *);
static void Act_set_margins(Widget, XEvent *, String *, Cardinal *);
static void Act_show_display_attributes(Widget, XEvent *, String *, Cardinal *);
static void Act_set_density(Widget, XEvent *, String *, Cardinal *);
static void Act_change_density(Widget, XEvent *, String *, Cardinal *);
static void Act_fullscreen(Widget, XEvent *, String *, Cardinal *);
#ifdef GREY
static void Act_set_greyscaling(Widget, XEvent *, String *, Cardinal *);
#endif
#if COLOR
static void Act_set_color(Widget, XEvent *, String *, Cardinal *);
#endif

static void Act_htex_anchorinfo(Widget, XEvent *, String *, Cardinal *);

static void Act_reread_dvi_file(Widget, XEvent *, String *, Cardinal *);
static void Act_select_dvi_file(Widget, XEvent *, String *, Cardinal *);
static void Act_discard_number(Widget, XEvent *, String *, Cardinal *);
static void Act_drag(Widget, XEvent *, String *, Cardinal *);
static void Act_wheel(Widget, XEvent *, String *, Cardinal *);
static void Act_motion(Widget, XEvent *, String *, Cardinal *);
static void Act_release(Widget, XEvent *, String *, Cardinal *);
static void Act_source_special(Widget, XEvent *, String *, Cardinal *);
static void Act_show_source_specials(Widget, XEvent *, String *, Cardinal *);
static void Act_source_what_special(Widget, XEvent *, String *, Cardinal *);
static void Act_unpause_or_next(Widget, XEvent *, String *, Cardinal *);
static void Act_ruler_snap_origin(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void Act_text_mode(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void Act_load_url(Widget w, XEvent *event, String *params, Cardinal *num_params);
#ifdef MOTIF
static void Act_prefs_dialog(Widget w, XEvent *event, String *params, Cardinal *num_params);
#endif

static XtActionsRec m_actions[] = {
    {"digit", Act_digit},
    {"switch-mode", Act_switch_mode},
    {"ruler-mode", Act_ruler_mode},
    {"text-mode", Act_text_mode},
    {"minus", Act_minus},
    {"recent-files", Act_recent_files},
    {"quit", Act_quit},
    {"quit-confirm", Act_quit_confirm},
    {"print", Act_print},
    {"save", Act_save},
    {"find", Act_find},
    {"find-next", Act_find_next},
    {"help", Act_help},
    {"goto-page", Act_goto_page},
    {"use-tex-pages", Act_use_tex_pages},
    {"forward-page", Act_forward_page},
    {"back-page", Act_back_page},
    {"toggle-mark", Act_toggle_mark},
    {"declare-page-number", Act_declare_page_number},
    {"home", Act_home},
    {"home-or-top", Act_home_or_top},
    {"end-or-bottom", Act_end_or_bottom},
    {"center", Act_center},
    {"set-keep-flag", Act_set_keep_flag},
    {"ruler-snap-origin", Act_ruler_snap_origin},
    {"left", Act_left},
    {"right", Act_right},
    {"up", Act_up},
    {"down", Act_down},
    {"up-or-previous", Act_up_or_previous},
    {"down-or-next", Act_down_or_next},
    {"set-margins", Act_set_margins},
    {"show-display-attributes", Act_show_display_attributes},
    {"set-shrink-factor", Act_set_shrink_factor},
    {"shrink-to-dpi", Act_shrink_to_dpi},
    {"set-density", Act_set_density},
    {"change-density", Act_change_density},
    {"fullscreen", Act_fullscreen},
#ifdef GREY
    {"set-greyscaling", Act_set_greyscaling},
#endif
#if COLOR
    {"set-color", Act_set_color},
#endif
#ifdef PS
    {"set-ps", Act_set_ps},
#endif
    {"htex-back", Act_htex_back},
    {"htex-forward", Act_htex_forward},
    {"htex-anchorinfo", Act_htex_anchorinfo},
#ifdef PS_GS
    {"set-gs-alpha", Act_set_gs_alpha},
#endif
    {"set-expert-mode", Act_set_expert_mode},
    {"reread-dvi-file", Act_reread_dvi_file},
    {"select-dvi-file", Act_select_dvi_file},
    {"discard-number", Act_discard_number},
    {"drag", Act_drag},
    {"wheel", Act_wheel},
    {"motion", Act_motion},
    {"release", Act_release},
    {"source-special", Act_source_special},
    {"show-source-specials", Act_show_source_specials},
    {"source-what-special", Act_source_what_special},
    {"unpause-or-next", Act_unpause_or_next},
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    {"set-papersize", Act_set_papersize},
    {"set-paper-landscape", Act_set_paper_landscape},
#endif /* NEW_MENU_CREATION */
    {"load-url", Act_load_url},
    {"pagehistory-clear", Act_pagehistory_clear},
    {"pagehistory-back", Act_pagehistory_back},
    {"pagehistory-forward", Act_pagehistory_forward},
    {"pagehistory-delete-backward", Act_pagehistory_delete_backward},
    {"pagehistory-delete-forward", Act_pagehistory_delete_forward},    
#ifdef MOTIF
    {"prefs-dialog", Act_prefs_dialog},    
#endif
};

/*
 * Access to m_actions
 */
int get_num_actions(void)
{
    return XtNumber(m_actions);
}

XtActionsRec *get_actions(void)
{
    return m_actions;
}

/*
 *	Data for buffered events.
 */

#ifndef FLAKY_SIGPOLL
static VOLATILE int event_freq = 70;
#else
#define	event_freq	70
#endif

static void can_exposures(struct WindowRec *windowrec);


/*
 *	Set the flag so that termination occurs, via the above routine.
 *	This should be used in place of xdvi_exit() when there may be a
 *	non-killable process running (e.g., anytime within read_events()).
 */

static void
xdvi_normal_exit(void)
{
    sig_flags |= SF_TERM;
}

static void
xdvi_normal_exit_cb(XtPointer arg)
{
    UNUSED(arg);
    sig_flags |= SF_TERM;
}

void
xdvi_exit_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    UNUSED(w);
    UNUSED(client_data);
    UNUSED(call_data);

    sig_flags |= SF_TERM;
}

/*
 * Event-handling routines.
 */


void
expose(struct WindowRec *windowrec,
       int x, int y,
       unsigned int w, unsigned int h)
{
    if (windowrec->min_x > x)
	windowrec->min_x = x;
    if (windowrec->max_x < (int)(x + w))
	windowrec->max_x = x + w;
    if (windowrec->min_y > y)
	windowrec->min_y = y;
    if (windowrec->max_y < (int)(y + h))
	windowrec->max_y = y + h;

    globals.ev.flags |= EV_EXPOSE;
}

void
clearexpose(struct WindowRec *windowrec,
	    int x, int y,
	    unsigned w, unsigned h)
{
    XClearArea(DISP, windowrec->win, x, y, w, h, False);
    expose(windowrec, x, y, w, h);
}

/*
 *	Routines for X11 toolkit.
 */

static Position window_x, window_y;
static Arg arg_xy[] = {
    {XtNx, (XtArgVal) &window_x},
    {XtNy, (XtArgVal) &window_y},
};

#define	get_xy() XtGetValues(globals.widgets.draw_widget, arg_xy, XtNumber(arg_xy))


static void
warn_num_params(const char *act_name, String *params, int num_params, int max_params)
{
    if (num_params > max_params) {
	XDVI_WARNING((stderr, "Too many parameters (%d) for action \"%s\", ignoring all after \"%s\"",
		      num_params, act_name, params[max_params - 1]));
    }
}


struct xdvi_action *
compile_action(const char *str)
{
    const char *p, *p1, *p2;
    XtActionsRec *actp;
    struct xdvi_action *ap;

    while (*str == ' ' || *str == '\t')
	++str;

    if (*str == '\0' || *str == '\n')
	return NULL;

    p = str;

    /* find end of command name */
    while (isalnum(*p) || *p == '-' || *p == '_')
	++p;

    for (actp = m_actions; ; ++actp) {
	if (actp >= m_actions + XtNumber(m_actions)) {
	    const char *tmp = strchr(str, '\0');
	    if (tmp == NULL) {
		tmp = p;
	    }
	    XDVI_WARNING((stderr, "Cannot compile action \"%.*s\".", tmp - str, str));

	    return NULL;
	}
	if (memcmp(str, actp->string, p - str) == 0 && actp->string[p - str] == '\0')
	    break;
    }

    while (*p == ' ' || *p == '\t')
	++p;
    if (*p != '(') {
	while (*p != '\0' && *p != '\n')
	    ++p;
	XDVI_WARNING((stderr, "Syntax error in action %.*s.", p - str, str));

	return NULL;
    }
    ++p;
    while (*p == ' ' || *p == '\t')
	++p;
    for (p1 = p;; ++p1) {
	if (*p1 == '\0' || *p1 == '\n') {
	    XDVI_WARNING((stderr, "Syntax error in action %.*s.", p1 - str, str));
	    return NULL;
	}
	if (*p1 == ')')
	    break;
    }

    ap = xmalloc(sizeof *ap);
    ap->proc = actp->proc;
    for (p2 = p1;; --p2) {
	if (p2 <= p) {	/* if no args */
	    ap->num_params = 0;
	    ap->param = NULL;
	    break;
	}
	else if (p2[-1] != ' ' && p2[-1] != '\t') {
	    char *arg;

	    arg = xmalloc(p2 - p + 1);
	    bcopy(p, arg, p2 - p);
	    arg[p2 - p] = '\0';
	    ap->num_params = 1;
	    ap->param = arg;
	    break;
	}
    }
    
    ap->next = compile_action(p1 + 1);

    return ap;
}

void
handle_command(Widget widget, XtPointer client_data, XtPointer call_data)
{
    struct xdvi_action *actp;

    UNUSED(call_data);

    /* call all actions registered for this event */
    for (actp = (struct xdvi_action *)client_data; actp != NULL; actp = actp->next) {
	if (globals.debug & DBG_EVENT)
	    fprintf(stderr, "calling action with param: %s\n", actp->param);
	(actp->proc) (widget, NULL, &actp->param, &actp->num_params);
    }
}

#ifdef MOTIF
int
set_bar_value(Widget bar, int value, int max)
{
    XmScrollBarCallbackStruct call_data;

#ifdef TEST_SCROLLING
    fprintf(stderr, "set_bar_value: val %d, max %d\n", value, max);
#endif
    if (value > max)
	value = max;
    if (value < 0)
	value = 0;
    call_data.value = value;
    XtVaSetValues(bar, XmNvalue, value, NULL);
    XtCallCallbacks(bar, XmNvalueChangedCallback, &call_data);
    return value;
}
#endif

void
home(wide_bool scrl)
{
    if (!scrl)
	XUnmapWindow(DISP, mane.win);
# ifdef MOTIF
    {
	int value;

	value = (globals.page.w - mane.width) / 2;
	if (value > resource.sidemargin_int / mane.shrinkfactor)
	    value = resource.sidemargin_int / mane.shrinkfactor;
	(void)set_bar_value(globals.widgets.x_bar, value, (int)(globals.page.w - mane.width));

	value = (globals.page.h - mane.height) / 2;
	if (value > resource.topmargin_int / mane.shrinkfactor)
	    value = resource.topmargin_int / mane.shrinkfactor;
	(void)set_bar_value(globals.widgets.y_bar, value, (int)(globals.page.h - mane.height));
    }
# else
    get_xy();
    if (globals.widgets.x_bar != NULL) {
	int coord = (globals.page.w - mane.width) / 2;

	if (coord > resource.sidemargin_int / mane.shrinkfactor)
	    coord = resource.sidemargin_int / mane.shrinkfactor;
	XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer) (window_x + coord));
    }
    if (globals.widgets.y_bar != NULL) {
	int coord = (globals.page.h - mane.height) / 2;

	if (coord > resource.topmargin_int / mane.shrinkfactor)
	    coord = resource.topmargin_int / mane.shrinkfactor;
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc, (XtPointer) (window_y + coord));
    }
# endif /* MOTIF */
    if (!scrl) {
	XMapWindow(DISP, mane.win);
	/* Wait for the server to catch up---this eliminates flicker. */
	XSync(DISP, False);
    }
    handle_x_scroll(NULL, NULL, NULL, NULL);
    handle_y_scroll(NULL, NULL, NULL, NULL);
}

/*
 *	Same as home(), except move to the bottom of the page.
 */

static void
home_bottom(wide_bool scrl)
{
    UNUSED(scrl);
    XUnmapWindow(DISP, mane.win);
#ifdef MOTIF
    {
	int value;

	value = (globals.page.w - mane.width) / 2;
	if (value > resource.sidemargin_int / mane.shrinkfactor)
	    value = resource.sidemargin_int / mane.shrinkfactor;
	(void)set_bar_value(globals.widgets.x_bar, value, (int)(globals.page.w - mane.width));

	(void)set_bar_value(globals.widgets.y_bar, (int)(globals.page.h - mane.height), (int)(globals.page.h - mane.height));
    }
#else /* MOTIF */
    get_xy();
    if (globals.widgets.x_bar != NULL) {
	int coord = (globals.page.w - mane.width) / 2;

	if (coord > resource.sidemargin_int / mane.shrinkfactor)
	    coord = resource.sidemargin_int / mane.shrinkfactor;
	XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer) (window_x + coord));
    }
    if (globals.widgets.y_bar != NULL)
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc, (XtPointer)(window_y + (globals.page.h - mane.height)));
#endif /* MOTIF */
    XMapWindow(DISP, mane.win);
    /* Wait for the server to catch up---this eliminates flicker. */
    XSync(DISP, False);

    handle_x_scroll(NULL, NULL, NULL, NULL);
    handle_y_scroll(NULL, NULL, NULL, NULL);
}


#ifndef MOTIF
static void
handle_destroy_bar(Widget w, XtPointer client_data, XtPointer call_data)
{
    UNUSED(w);
    UNUSED(call_data);
    *(Widget *) client_data = NULL;
}
#endif

static Boolean resized = False;

static void
get_geom(void)
{
    static Dimension new_clip_w, new_clip_h;
    
    static Arg arg_wh_clip[] = {
	{XtNwidth, (XtArgVal) &new_clip_w},
	{XtNheight, (XtArgVal) &new_clip_h},
    };

    static Dimension window_w, window_h;

    static Arg arg_wh[] = {
	{XtNwidth, (XtArgVal) &window_w},
	{XtNheight, (XtArgVal) &window_h},
    };

    
    int old_clip_w;

#ifdef MOTIF
    /* event handlers for Motif scrollbars have already been added
       in create_initialize_widgets(), xdvi.c */
    XtGetValues(globals.widgets.main_window, arg_wh, XtNumber(arg_wh));
#else
    XtGetValues(globals.widgets.vport_widget, arg_wh, XtNumber(arg_wh));
    /* Note:  widgets may be destroyed but not forgotten */
    if (globals.widgets.x_bar == NULL) {
	globals.widgets.x_bar = XtNameToWidget(globals.widgets.vport_widget, "horizontal");
	if (globals.widgets.x_bar != NULL) {
	    XtAddCallback(globals.widgets.x_bar, XtNdestroyCallback, handle_destroy_bar,
			  (XtPointer)&globals.widgets.x_bar);
	    XtAddEventHandler(globals.widgets.x_bar, ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
			      False, handle_x_scroll, NULL);
	}
    }
    if (globals.widgets.y_bar == NULL) {
	globals.widgets.y_bar = XtNameToWidget(globals.widgets.vport_widget, "vertical");
	if (globals.widgets.y_bar != NULL) {
	    XtAddCallback(globals.widgets.y_bar, XtNdestroyCallback, handle_destroy_bar,
			  (XtPointer)&globals.widgets.y_bar);
	    XtAddEventHandler(globals.widgets.y_bar, ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
			      False, handle_y_scroll, NULL);
	}
    }
#endif
    XtGetValues(globals.widgets.clip_widget, arg_wh_clip, XtNumber(arg_wh_clip));

    old_clip_w = mane.width;

    /* we need to do this because 
       sizeof(Dimension) != sizeof(int)
    */
    mane.width = new_clip_w;
    mane.height = new_clip_h;
    if (old_clip_w == 0) {
	globals.ev.flags |= EV_NEWPAGE;
    }

    if (resource.keep_flag) {
#ifndef MOTIF
	Dimension d;
	int curr_scroll;
	if ((globals.widgets.x_bar != NULL && m_x_scroll != 0) || (globals.widgets.y_bar != NULL && m_y_scroll != 0)) {
	    get_xy();
	}
#endif
	if (globals.widgets.x_bar != NULL && m_x_scroll != 0) {
#ifdef MOTIF
	    if (m_x_scroll > 0)
		(void)set_bar_value(globals.widgets.x_bar, m_x_scroll, (int)(globals.page.w - mane.width));
#else
	    XtVaGetValues(globals.widgets.clip_widget, XtNy, &d, NULL);
	    curr_scroll = d - window_x;
	    if (m_x_scroll > curr_scroll) {
		TRACE_GUI((stderr, "======== diff: %d", m_x_scroll - curr_scroll));
		XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer)(m_x_scroll - curr_scroll));
	    }
#endif
	}
	if (globals.widgets.y_bar != NULL && m_y_scroll != 0) {
#ifdef MOTIF
	    if (m_y_scroll > 0)
		(void)set_bar_value(globals.widgets.y_bar, m_y_scroll, (int)(globals.page.h - mane.height));
#else
	    XtVaGetValues(globals.widgets.clip_widget, XtNy, &d, NULL);
	    curr_scroll = d - window_y;
	    if (m_y_scroll > curr_scroll) {
		TRACE_GUI((stderr, "======== diff: %d", m_y_scroll - curr_scroll));
		XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc, (XtPointer)(m_y_scroll - curr_scroll));
	    }
#endif
	}
    }
    /*     home(False); */
    resized = False;
}

/*
 * Callback routines
 */

void
handle_resize(Widget widget, XtPointer junk, XEvent *event, Boolean *cont)
{
    UNUSED(widget);
    UNUSED(junk);
    UNUSED(event);
    UNUSED(cont);
    
    resized = True;
#ifndef MOTIF
    handle_statusline_resize();
    handle_pagelist_resize();
#endif
}

void
reconfig(void)
{
/*      Dimension x, y; */

    if (globals.dvi_file.bak_fp == NULL)
	return;
    
#ifndef MOTIF
    XtVaSetValues(globals.widgets.vport_widget, XtNresizable, (XtArgVal)False, NULL);
#endif
    TRACE_GUI((stderr, "globals.widgets.draw_widget: w %d, h %d", globals.page.w, globals.page.h));
    XtVaSetValues(globals.widgets.draw_widget, XtNwidth, (XtArgVal)globals.page.w, XtNheight, (XtArgVal)globals.page.h, NULL);
    
#ifdef TEST_SCROLLING
/*     XtVaSetValues(globals.widgets.draw_background, XtNwidth, (XtArgVal)globals.page.w, XtNheight, (XtArgVal)globals.page.h, NULL); */
#endif
    
#ifndef MOTIF
    handle_statusline_resize(); /* without this, statusline will disappear */
    /* following not needed? */
    /*     handle_pagelist_resize(); */
#endif
    
    get_geom();

/*     set_windowsize(&x, &y */
/* #ifndef MOTIF */
/* 		   , get_panel_width() */
/* #endif */
/* 		   ); */
/*     XResizeWindow(DISP, XtWindow(globals.widgets.top_level), x, y); */
/*      reconfigure_window(False, x, y, True); */
/*      reconfig_window(); */
}


int
check_goto_page(int pageno)
{
    int retval;
    if (pageno < 0) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Can't go to page %d, going to first page instead", pageno + 1);
	retval = 0;
    }
    else if (pageno >= total_pages) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT,
			 "Can't go to page %d, going to last page (%d) instead",
			 pageno + 1, total_pages);
	retval = total_pages - 1;
    }
    else
	retval = pageno;

    page_history_insert(retval);
    return retval;
}


static int
check_goto_tex_page(int pageno)
{
    /*
      Translate from TeX page number to `real' page number if
      needed. Note that pageno is a C-style 0-based number, hence we
      add 1 for the argument of pageinfo_get_index_of_number().
    */
    int retval;
    if (resource.use_tex_pages) {
	int res = pageinfo_get_index_of_number(pageno + 1);
	if (res >= 0)
	    retval = res;
	else {
	    XBell(DISP, 0);
	    if (pageno < 1) {
		statusline_print(STATUS_SHORT, "Can't go to page %d, going to first page instead", pageno + 1);
		retval = 0;
	    }
	    else {
		/* there is no quick way to determine the last page number in the TeX page index */
		statusline_print(STATUS_SHORT,
				 "Can't go to page %d, going to last page instead",
				 pageno + 1);
		retval = total_pages - 1;
	    }
	}
	page_history_insert(retval);
    }
    else {
	retval = check_goto_page(pageno);
    }
    return retval;
}

/* |||
 *	Currently the event handler does not coordinate XCopyArea requests
 *	with GraphicsExpose events.  This can lead to problems if the window
 *	is partially obscured and one, for example, drags a scrollbar.
 */

/*
 *	Actions for the translation mechanism.
 */

/* if there are global prefixes, return them in res and reset them to defaults */
static Boolean
get_prefix_arg(int *res)
{
    Boolean ret;

    *res = m_sign * m_number;
    ret = m_have_arg;
    /* reset global flags */
    m_have_arg = False;
    m_number = 0;
    m_sign = 1;
    return ret;
}

Boolean
get_int_arg(String *param, Cardinal *num_params, int *res)
{
    if (*num_params > 0) {
	*res = atoi(*param);
	return True;
    }
    else {
	if (get_prefix_arg(res)) { /* prefix argument? */
	    return True;
	}
    }
    return False;
}


Boolean
toggle_arg(int arg, String *param, Cardinal *num_params)
{
    if (*num_params > 0) {
	if (**param != 't' && (atoi(*param) != 0) == arg)
	    return False;
    }
    else {
	if (m_have_arg) {
	    int	tmparg = m_number;

	    m_have_arg = False;
	    m_number = 0;
	    m_sign = 1;

	    if ((tmparg != 0) == arg)
		return False;
	}
    }
    return True;
}

#if defined(NEW_MENU_CREATION) || defined(MOTIF)
Boolean
check_resource_expert(void *val, const char *param)
{
    int j = strtol(param, (char **)NULL, 10);
    /* check if the j-1th bit is set: */
    return (*(int *)val >> (j - 1)) & 1;
}

Boolean
check_papersize(void *val, const char *param)
{
    UNUSED(val);
    UNUSED(param);
    return False; /* TODO */
}

Boolean
check_paper_landscape(void *val, const char *param)
{
    UNUSED(val);
    UNUSED(param);
    return False; /* TODO */
}

/* comparison functions for the menu setting code */
Boolean
check_toggle(void *val, const char *param)
{
    Boolean *on = val;
    if (strcmp(param, "toggle") == 0) {
	return *on;
    }
    else {
	fprintf(stderr, "TODO: check_toggle: arg |%s|, curr: %d\n", param, *(int *)val);
	return *on;
    }
}

Boolean
check_int(void *val, const char *param)
{
    int i = strtol(param, (char **)NULL, 10);
    return i == *(int *)val;
}
#endif /* NEW_MENU_CREATION */


static void
Act_digit(Widget w, XEvent *event,
	  String *params, Cardinal *num_params)
{
    int digit;
    /* for overflow checks */
    static const int MAXINT_QUOT = INT_MAX / 10;
    static const int MAXINT_MOD = INT_MAX % 10;


    UNUSED(w);
    UNUSED(event);

    if (*num_params != 1 || (digit = **params - '0') > 9) {
	XBell(DISP, 0);
	return;
    }
    m_have_arg = True;

    /* don't increment m_number if it would overflow */
    if (m_number < MAXINT_QUOT || (m_number == MAXINT_QUOT && digit <= MAXINT_MOD)) {
	m_number = m_number * 10 + digit;
	if (resource.expert_mode & XPRT_SHOW_STATUSLINE) /* too distracting for stdout */
	    statusline_print(STATUS_SHORT, "numerical prefix: %s%d", m_sign < 0 ? "-" : "", m_number);
    }
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "numerical prefix: %s%d: no larger value possible", m_sign < 0 ? "-" : "", m_number);
    }
}

static void
Act_minus(Widget w, XEvent *event,
	  String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    
    m_have_arg = True;
    m_sign = -m_sign;
    if (m_number > 0) {
	if (resource.expert_mode & XPRT_SHOW_STATUSLINE) /* too distracting for stdout */
	    statusline_print(STATUS_SHORT, "numerical prefix: %s%d", m_sign < 0 ? "-" : "", m_number);
    }
    else {
	if (resource.expert_mode & XPRT_SHOW_STATUSLINE) /* too distracting for stdout */
	    statusline_print(STATUS_SHORT, "numerical prefix: %s", m_sign < 0 ? "-" : "");
    }
}

static void
Act_quit(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    
#ifndef FLAKY_SIGPOLL
    if (globals.debug & DBG_EVENT)
	puts(event_freq < 0
	     ? "SIGPOLL is working"
	     : "no SIGPOLL signals received");
#endif
    xdvi_normal_exit();
}

static void
Act_quit_confirm(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    static Widget dialog = 0;
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

#ifndef FLAKY_SIGPOLL
    if (globals.debug & DBG_EVENT)
	puts(event_freq < 0
	     ? "SIGPOLL is working"
	     : "no SIGPOLL signals received");
#endif

    /* already a quit dialog open? */
    if (dialog != 0) {
	/* HACK ALERT: use brute force, since tests for XtIsRealized()
	   or XtIsMapped() don't work?? Grabbing the server apparently
	   has problems with Xaw, and it's not a nice solution anyway ... */
	if (kill_message_window(dialog))
	    XBell(DISP, 0);
    }

    dialog = choice_dialog_sized(globals.widgets.top_level,
				 MSG_QUESTION,
				 SIZE_SMALL,
				 NULL,
#ifndef MOTIF
				 "quit",
#endif
				 NULL, NULL, /* no pre_callbacks */
				 "OK", xdvi_normal_exit_cb, NULL,
				 "Cancel", NULL, NULL,
				 "Really quit xdvi?");
}

static void
Act_load_url(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(num_params);
    launch_browser(*params);
}

static void
Act_print(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    static struct save_or_print_info info;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    
    info.act = FILE_PRINT;
    
    save_or_print_callback(&info);
}

static void
Act_save(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    static struct save_or_print_info info;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    
    info.act = FILE_SAVE;
    
    save_or_print_callback(&info);
}

static void
Act_find(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    dvi_find_string(NULL, False);
}

static void
Act_find_next(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    dvi_find_string(NULL, True);
}

static void
Act_help(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    char *arg = NULL;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    
    if (*num_params > 0)
	arg = *params;
    
    show_help(globals.widgets.top_level, arg);
}

static home_proc home_action = NULL;

void
goto_page(int new_page, home_proc proc, Boolean force)
{
    /* SU: added clearing the window here, else old window content
       will survive switching pages as long as globals.pausing.flag is active
       (i.e. when inadvertedly changing page instead of unpausing)
    */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
	    
    if (globals.dvi_file.bak_fp == NULL) {
	current_page = new_page; /* so that xdvi gets the page change */
	return;
    }

    ASSERT(new_page >= 0 && new_page < total_pages, "new_page in goto_page() out of range");   
    if (current_page != new_page || force) {
	globals.cursor.flags &= ~CURSOR_LINK; /* disable link cursor if needed */

	current_page = new_page;
	home_action = proc;
	globals.warn_spec_now = resource.warn_spec;
    /* this seems unneccessary */
/* 	if (!resource.keep_flag) */
/* 	    home(False); */
#if defined(MOTIF) && HAVE_XPM
	tb_check_navigation_sensitivity(current_page);
/*  	page_history_update_toolbar_navigation(); */
#endif
	maybe_scroll_pagelist(current_page, False);

	if (globals.pausing.num_save)
	    globals.pausing.num = globals.pausing.num_save[new_page];
	else
	    globals.pausing.num = 0;
	/* Control-L (and changing the page) clears this box */
	globals.src.fwd_box_page = -1;

	globals.ev.flags |= EV_NEWPAGE;
	XFlush(DISP);       
    }
}


static void
Act_goto_page(Widget w, XEvent *event,
	      String *params, Cardinal *num_params)
{
    int arg;
    Boolean clear_statusline = False;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    warn_num_params("goto-page()", params, *num_params, 1);
    
    if (*num_params > 0) {
	if (**params == 'e') {
	    arg = total_pages - 1;
	}
	else
	    arg = atoi(*params) - globals.pageno_correct;
    }
    else {
	if (get_prefix_arg(&arg)) {
	    clear_statusline = True;
	    arg -= globals.pageno_correct;
	}
	else {
	    arg = total_pages - 1;
	}
    }
    if (arg == total_pages - 1) {
	/* with TeX numbers, total_pages - 1 might not be found in the page list,
	   so don't use check_goto_tex_page() in this case */
	goto_page(check_goto_page(arg), resource.keep_flag ? NULL : home, False);
    }
    else {
	goto_page(check_goto_tex_page(arg), resource.keep_flag ? NULL : home, False);
    }
    search_signal_page_changed();
    if (clear_statusline)
	statusline_clear();
}

void
Act_recent_files(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
}

void
Act_use_tex_pages(Widget w, XEvent *event,
		  String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);

    if (block_this_mouse_event())
	return;
    
    if (*num_params == 0) {
	if (m_have_arg) {
	    resource.use_tex_pages = (m_number != 0);
	    m_have_arg = False;
	    m_number = 0;
	    m_sign = 1;
	}
	else {
	    resource.use_tex_pages = !resource.use_tex_pages;
	}
    }
    else
	resource.use_tex_pages = (**params == 't' ? !resource.use_tex_pages : atoi(*params));
    
    if (resource.use_tex_pages) {
	statusline_print(STATUS_SHORT, "Using TeX page numbers for \"g\", goto-page()");
    }
    else {
	statusline_print(STATUS_SHORT, "Using physical page numbers for \"g\", goto-page()");
    }

    store_preference(NULL, "useTeXPages", "%s", resource.use_tex_pages ? "True" : "False");
    
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.use_tex_pages, Act_use_tex_pages, check_toggle);
#else
#ifdef MOTIF
   set_use_tex_option();
#else
    toggle_menu(resource.use_tex_pages,  Act_use_tex_pages);
#endif
#endif /* NEW_MENU_CREATION */
    refresh_pagelist(total_pages, current_page);
}

void
Act_forward_page(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    int arg;
    Boolean clear_statusline = False;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg))
	arg = 1;
    else
	clear_statusline = True;
    
    arg += current_page;

    if (arg == current_page) { /* zero argument -> redraw page */
	globals.src.fwd_box_page = -1;
	search_reset_info();
	globals.ev.flags |= EV_NEWPAGE;
	XFlush(DISP);
	statusline_print(STATUS_SHORT, "Page redrawn.");
	return;
    }
    else if (current_page >= total_pages - 1) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Last page of DVI file");
	return;
    }
    
    goto_page(check_goto_page(arg), resource.keep_flag ? NULL : home, False);
    if (clear_statusline)
	statusline_clear();
    search_signal_page_changed();
}

void
Act_back_page(Widget w, XEvent *event,
	      String *params, Cardinal *num_params)
{
    int arg;
    Boolean clear_statusline = False;
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg))
	arg = 1;
    else
	clear_statusline = True;
    
    arg = current_page - arg;

    if (current_page == 0) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "First page of DVI file");
	return;
    }

    goto_page(check_goto_page(arg), resource.keep_flag ? NULL : home, False);
    if (clear_statusline)
	statusline_clear();
    search_signal_page_changed();
}

static void
Act_declare_page_number(Widget w, XEvent *event,
			String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (resource.mouse_mode == MOUSE_RULER_MODE) {
	show_distance_from_ruler(event, True);
	return;
    }
    
    if (!get_int_arg(params, num_params, &arg)) {
	arg = 0;
    }
    globals.pageno_correct = arg - current_page;
    statusline_print(STATUS_SHORT, "Current page number set to %d", arg);
}

static void
Act_toggle_mark(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (globals.dvi_file.bak_fp == NULL)
	return;
    
    if (
#ifdef MOTIF
	(resource.expert_mode & XPRT_SHOW_PAGELIST) == 0
#else
	(resource.expert_mode & XPRT_SHOW_BUTTONS) == 0
#endif
	)
	return;
    
    if (*num_params > 0) {
	arg = atoi(*params);
	if (arg < -1 || arg > 2) {
	    XBell(DISP, 0);
	    statusline_print(STATUS_SHORT,
			     "Possible arguments: none (toggle current), "
			     "-1 (mark all), 0 (unmark all), 1 (toggle odd), 2 (toggle even)");
	}
	list_toggle_marks(arg);
    }
    else if (get_prefix_arg(&arg)) {
	list_toggle_marks(arg);
	statusline_clear();
    }
    else {
	arg = current_page;
	list_toggle_current(arg);
    }
}

static void
Act_home(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    home(True);
}

static void
Act_home_or_top(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);
    
    if (block_this_mouse_event())
	return;
    
    if (resource.keep_flag) {
	String args[1];
	args[0] = "10"; /* should be large enough ... */
	XtCallActionProc(globals.widgets.top_level, "up", NULL, args, 1);
    }
    else {
	home(True);
    }
}

static void
Act_end_or_bottom(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    String args[1];

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    args[0] = "10"; /* should be large enough ... */
    XtCallActionProc(globals.widgets.top_level, "down", NULL, args, 1);
    
    if (!resource.keep_flag
#ifndef MOTIF
	&& globals.widgets.x_bar != NULL
#endif
	) {
	XtCallActionProc(globals.widgets.top_level, "right", NULL, args, 1);
    }
}

static void
Act_center(Widget w, XEvent *event,
	   String *params, Cardinal *num_params)
{
    int x, y;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

#ifdef MOTIF
    /* The clip widget gives a more exact value. */
    x = event->xkey.x - mane.width / 2;
    y = event->xkey.y - mane.height / 2;

    x = set_bar_value(globals.widgets.x_bar, x, (int)(globals.page.w - mane.width));
    y = set_bar_value(globals.widgets.y_bar, y, (int)(globals.page.h - mane.height));
    get_xy();
    XWarpPointer(DISP, None, None, 0, 0, 0, 0, -x - window_x, -y - window_y);
#else

    if (event == NULL)
	return;	/* button actions do not provide events */

    x = event->xkey.x - mane.width / 2;
    y = event->xkey.y - mane.height / 2;
    /* The clip widget gives a more exact value. */
    if (globals.widgets.x_bar != NULL)
	XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer) x);
    if (globals.widgets.y_bar != NULL)
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc, (XtPointer) y);
    XWarpPointer(DISP, None, None, 0, 0, 0, 0, -x, -y);
#endif
    handle_x_scroll(NULL, NULL, NULL, NULL);
    handle_y_scroll(NULL, NULL, NULL, NULL);
}

void
Act_ruler_snap_origin(Widget w, XEvent *event,
		      String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (resource.mouse_mode == MOUSE_RULER_MODE)
	ruler_snap_origin(event);
}

#if defined(NEW_MENU_CREATION) || defined(MOTIF)
void
Act_set_papersize(Widget w, XEvent *event,
		  String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(num_params);

    fprintf(stderr, "set_paperformat: %s\n", *params);
    resource.paper = *params;
    set_menu((void *)resource.paper, Act_set_papersize, check_papersize);
}

void
Act_set_paper_landscape(Widget w, XEvent *event,
			String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(num_params);

    resource.paper_landscape = !resource.paper_landscape;
    fprintf(stderr, "set_paper_landscape: %s\n", *params);

    set_menu((char *)resource.paper,  Act_set_paper_landscape, check_paper_landscape);
}
#endif /* NEW_MENU_CREATION */

void
Act_set_keep_flag(Widget w, XEvent *event,
		  String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(event);
    UNUSED(w);
    
    if (*num_params == 0) {
	if (m_have_arg) {
	    resource.keep_flag = (m_number != 0);
	    m_have_arg = False;
	    m_number = 0;
	    m_sign = 1;
	}
	else {
	    resource.keep_flag = !resource.keep_flag;
	}
    }
    else
	resource.keep_flag = (**params == 't'
			      ? !resource.keep_flag : atoi(*params));
    if (resource.keep_flag) {
	statusline_print(STATUS_SHORT, "Keeping position when switching pages");
    }
    else {
	statusline_print(STATUS_SHORT,
			 "Not keeping position when switching pages");
    }

    store_preference(NULL, "keepPosition", "%s", resource.keep_flag ? "True" : "False");
    
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.keep_flag, Act_set_keep_flag, check_toggle);
#else
    toggle_menu(resource.keep_flag,  Act_set_keep_flag);
#endif /* NEW_MENU_CREATION */
}

static void
Act_left(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;
    
    warn_num_params("left()", params, *num_params, 1);

#ifdef MOTIF
    get_xy();
#ifdef TEST_SCROLLING
    fprintf(stderr, "left for %p\n", (void *)w);
    fprintf(stderr, "window x: %d, y: %d\n", window_x, window_y);
#endif

    (void)set_bar_value(globals.widgets.x_bar, (*num_params == 0 ? (-2 * (int)mane.width / 3)
				: (int)(-my_atof(*params) * mane.width)) - window_x,
			(int)(globals.page.w - mane.width));
#else
    if (globals.widgets.x_bar != NULL)
	XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc,
			(XtPointer) (*num_params == 0 ? (-2 * (int)mane.width / 3)
				     : (int)(-my_atof(*params) * mane.width)));
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Horizontal scrolling not possible");
    }
#endif
    handle_x_scroll(NULL, NULL, NULL, NULL);
}

static void
Act_right(Widget w, XEvent *event,
	  String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;
    
    warn_num_params("right()", params, *num_params, 1);

#ifdef MOTIF
    get_xy();
#ifdef TEST_SCROLLING
    fprintf(stderr, "right for %p\n", (void *)w);
    fprintf(stderr, "window x: %d, y: %d\n", window_x, window_y);
#endif

    (void)set_bar_value(globals.widgets.x_bar, (*num_params == 0 ? (2 * (int)mane.width / 3)
				: (int)(my_atof(*params) * mane.width)) - window_x,
			(int)(globals.page.w - mane.width));
#else
    if (globals.widgets.x_bar != NULL)
	XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc,
			(XtPointer) (*num_params == 0 ? (2 * (int)mane.width / 3)
				     : (int)(my_atof(*params) * mane.width)));
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Horizontal scrolling not possible");
    }
#endif
    handle_x_scroll(NULL, NULL, NULL, NULL);
}

static void
Act_up(Widget w, XEvent *event,
       String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;
    
    warn_num_params("up()", params, *num_params, 1);
    
#ifdef MOTIF
#ifdef TEST_SCROLLING
    fprintf(stderr, "up for %p\n", (void *)w);
#endif
    get_xy();
    (void)set_bar_value(globals.widgets.y_bar, (*num_params == 0 ? (-2 * (int)mane.height / 3)
				: (int)(-my_atof(*params) * mane.height)) - window_y,
			(int)(globals.page.h - mane.height));
#else
    if (globals.widgets.y_bar != NULL)
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc,
			(XtPointer) (*num_params == 0 ? (-2 * (int)mane.height / 3)
				     : (int)(-my_atof(*params) * mane.height)));
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Vertical scrolling not possible");
    }
#endif
    handle_y_scroll(NULL, NULL, NULL, NULL);
}

static void
Act_down(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;

    warn_num_params("down()", params, *num_params, 1);

#ifdef MOTIF
#ifdef TEST_SCROLLING
    fprintf(stderr, "down for %p\n", (void *)w);
#endif
    get_xy();
    (void)set_bar_value(globals.widgets.y_bar, (*num_params == 0 ? (2 * (int)mane.height / 3)
				: (int)(my_atof(*params) * mane.height)) - window_y,
			(int)(globals.page.h - mane.height));
#else
    if (globals.widgets.y_bar != NULL)
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc,
			(XtPointer) (*num_params == 0 ? (2 * (int)mane.height / 3)
				     : (int)(my_atof(*params) * mane.height)));
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Vertical scrolling not possible");
    }
#endif
    handle_y_scroll(NULL, NULL, NULL, NULL);
}

static void
Act_down_or_next(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;

    warn_num_params("down-or-next()", params, *num_params, 1);

#ifdef TEST_SCROLLING
    fprintf(stderr, "down-or-next for %p\n", (void *)w);
#endif

#ifdef MOTIF
    get_xy();
    if (window_y > (int)mane.height - (int)globals.page.h) {
	(void)set_bar_value(globals.widgets.y_bar, (*num_params == 0 ? (2 * (int)mane.height / 3)
				    : (int)(my_atof(*params) * mane.height)) -
			    window_y, (int)(globals.page.h - mane.height));
	return;
    }
#else
    if (globals.widgets.y_bar != NULL) {
	get_xy();
	if (window_y > (int)mane.height - (int)globals.page.h) {
	    XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc,
			    (XtPointer) (*num_params ==
					 0 ? (2 * (int)mane.height / 3)
					 : (int)(my_atof(*params) * mane.height)));
	    return;
	}
    }
#endif

    if (current_page < total_pages - 1) {
	goto_page(current_page + 1, home, False);
	search_signal_page_changed();
    }
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "At bottom of last page");
    }
    handle_y_scroll(NULL, NULL, NULL, NULL);
}


static void
Act_up_or_previous(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    do_autoscroll = False;

    warn_num_params("up-or-previous()", params, *num_params, 1);

#ifdef TEST_SCROLLING
    fprintf(stderr, "up-or-previous for %p\n", (void *)w);
#endif
#ifdef MOTIF
    get_xy();
    if (window_y < 0) {
	(void)set_bar_value(globals.widgets.y_bar,
			    (*num_params == 0 ? (-2 * (int)mane.height / 3)
			     : (int)(-my_atof(*params) * mane.height)) - window_y,
			    (int)(globals.page.h - mane.height));
	return;
    }
#else
    if (globals.widgets.y_bar != NULL) {
	get_xy();
	if (window_y < 0) {
	    XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc,
			    (XtPointer) (*num_params ==
					 0 ? (-2 * (int)mane.height / 3)
					 : (int)(-my_atof(*params) * mane.height)));
	    return;
	}
    }
#endif

    if (current_page > 0) {
	goto_page(current_page - 1, home_bottom, False);
	search_signal_page_changed();
    }
    else {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "At top of first page");
    }
    handle_y_scroll(NULL, NULL, NULL, NULL);
}


static void
Act_set_margins(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    Window dummy;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

#ifndef MOTIF
    if (event == NULL)
	return;	/* button actions do not provide events */
#endif

    (void)XTranslateCoordinates(DISP, event->xkey.window, mane.win,
				event->xkey.x, event->xkey.y, &resource.sidemargin_int, &resource.topmargin_int, &dummy);	/* throw away last argument */
    statusline_print(STATUS_SHORT, "Margins set to cursor position (%d, %d)", resource.sidemargin_int, resource.topmargin_int);
    resource.sidemargin_int *= mane.shrinkfactor;
    resource.topmargin_int *= mane.shrinkfactor;
}

static void
Act_show_display_attributes(Widget w, XEvent *event,
			    String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    statusline_print(STATUS_SHORT, "Unit = %d, bitord = %d, byteord = %d",
		     BitmapUnit(DISP), BitmapBitOrder(DISP),
		     ImageByteOrder(DISP));
}

int
shrink_to_fit(void)
{
    int value1;
    int value2;
    static Dimension window_w;
#ifndef MOTIF
    static Dimension window_h;
#endif
    
    static Arg arg_wh[] = {
	{XtNwidth, (XtArgVal) &window_w}
#ifndef MOTIF
	, {XtNheight, (XtArgVal) &window_h}
#endif
    };

    
#ifdef MOTIF
    XtGetValues(globals.widgets.main_window, arg_wh, XtNumber(arg_wh));
#else
    XtGetValues(globals.widgets.vport_widget, arg_wh, XtNumber(arg_wh));
#endif
    
    value1 = ROUNDUP(globals.page.unshrunk_w, window_w - 2);

#ifdef  MOTIF
    {	/* account for menubar */
	static Dimension new_h;

	/* get rid of scrollbar */
	XtVaSetValues(globals.widgets.draw_widget, XtNwidth, (XtArgVal)1, XtNheight, (XtArgVal)1, NULL);
	XtVaGetValues(globals.widgets.clip_widget, XtNheight, &new_h, NULL);
	value2 = ROUNDUP(globals.page.unshrunk_h, new_h - 2);
    }
#else
    /* FIXME: value seems too small here! */
    value2 = ROUNDUP(globals.page.unshrunk_h, window_h - 2);
#endif

    return value1 > value2 ? value1 : value2;
}

void
do_set_shrinkfactor(int arg, Boolean set_resource)
{
    static int shrink_bak = -1;
    
    /* We don't store this as preference since it may affect initial window geometry. */
    /*  store_preference(NULL, "shrinkFactor", "%d", arg); */
    
    if (resource.mouse_mode == MOUSE_TEXT_MODE) {
	text_change_region(TEXT_SEL_CLEAR, NULL);
    }
    
    mane.shrinkfactor = arg;
    if (set_resource)
	resource.shrinkfactor = mane.shrinkfactor;
    
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&arg, Act_set_shrink_factor, check_int);
#if HAVE_XMP
    tb_set_zoom_sensitivity(arg != 1);
#endif
#else
    toggle_menu(arg, Act_set_shrink_factor);
#endif /* NEW_MENU_CREATION */
    if (arg != 1 && arg != shrink_bak) {
	shrink_bak = arg;
#if GREY
#if COLOR
	if (resource.use_grey)
	    fg_active = NULL;
#else 
	if (resource.use_grey)
	    init_pix();
#endif
#endif /* GREY */
	reset_fonts();
    }
    init_page();
    reconfig();

    htex_resize_page();

    /* this seems unneccessary */
/*     if (!resource.keep_flag) */
/* 	home(False); */

    globals.ev.flags |= EV_NEWPAGE;
    XFlush(DISP);
}

void
Act_set_shrink_factor(Widget w, XEvent *event,
		      String *params, Cardinal *num_params)
{
    int arg;
    /* Set a limit to the shrink factors allowed; otherwise xdvi may
       consume a lot of memory (even run out of memory in malloc) in
       the allocation for the subpixel lookup table pixeltbl,
       dvi-draw.c. The `correct' solution would be to recognize that at a
       certain shrink, the only pixel for a character will be always
       set to `on' anyway, so that we could do without the lookup table for
       very large shrink factors ... */
    static const int SHRINK_MAX = 999;
     
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    warn_num_params("set-shrink-factor()", params, *num_params, 1);

    if (*num_params > 0) {
	if (**params == 'a')
	    arg = shrink_to_fit();
	else if (**params == '-')
	    arg = mane.shrinkfactor + 1;
	else if (**params == '+')
	    arg = mane.shrinkfactor - 1;
	else
	    arg = atoi(*params);
    }
    else if (!get_prefix_arg(&arg)) {
	arg = shrink_to_fit();
    }

    if (arg <= 0) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT,
			 "No more enlarging possible");
	return;
    }
    
    if (arg > SHRINK_MAX) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT,
			 "Shrink factor %d too large (maximum: %d)", arg, SHRINK_MAX);
	return;
    }
    
    /* SU: added clearing the window here, else old window content
       will survive changing shrink
    */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
    
    statusline_print(STATUS_SHORT, "shrink factor: %d", arg);
#if 0
    /* Uncommented this, otherwise selecting `shrink to fit' with same value as
       current mane.shrinkfactor will clear the canvas (only fixed by changing shrink
       factor again). That bug is also present in xdvik-22.48-alpha3. */
    if (arg == mane.shrinkfactor) {
	return;
    }
#endif

    do_set_shrinkfactor(arg, True);

#if MOTIF    
    /* note: mustn't do that inside do_set_shrinkfactor(),
       or we get into an endless loop! */
    update_preferences_shrink();
#endif
}

void
Act_shrink_to_dpi(Widget w, XEvent *event,
		  String *params, Cardinal *num_params)
{
    int arg, arg_bak;
    Boolean clear_statusline = False;
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    warn_num_params("shrink-to-dpi()", params, *num_params, 1);
    
    if (!get_int_arg(params, num_params, &arg))
	arg = 0;
    else
	clear_statusline = True;
    
    arg_bak = arg;
    
    if (arg > 0)
	arg = (double)resource.pixels_per_inch / arg + 0.5;

    if (arg <= 0) {
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT,
			 "shrink-to-dpi requires a positive argument");
	return;
    }

    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
#if 0
    /* Uncommented this, otherwise selecting `shrink to fit' with same value as
       current mane.shrinkfactor will clear the canvas (only fixed by changing shrink
       factor again). That bug is also present in xdvik-22.48-alpha3. */
    if (arg == mane.shrinkfactor)
	return;
#endif
    do_set_shrinkfactor(arg, True);

#if MOTIF    
    /* note: mustn't do that inside do_set_shrinkfactor(),
       or we get into an endless loop! */
    update_preferences_shrink();
#endif
}

void
do_set_density(double newgamma, Boolean force, Boolean update_resource)
{
    
    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }

#ifdef GREY
    if (resource.use_grey) {
	if (newgamma == resource.gamma && !force) {
	    statusline_print(STATUS_SHORT, "density value: %.3f", newgamma);
	    return;
	}
	resource.gamma = newgamma;
	if (update_resource)
	    globals.curr_gamma = resource.gamma;
	
#if COLOR
	fg_active = NULL;
	reset_colors();
#else
	init_pix();
	if (G_visual->class != TrueColor) {
	    return;
	}
	reset_fonts();
#endif /* COLOR */
	statusline_print(STATUS_SHORT, "density value: %.3f", newgamma);
    } else
#endif /* GREY */
    {
	reset_fonts();
	if (mane.shrinkfactor == 1) {
	    statusline_print(STATUS_SHORT,
		    "set-density ignored at magnification 1");
	    return;
	}
	statusline_print(STATUS_SHORT, "density value: %.3f", newgamma);
    }

/*      store_preference(NULL, "gamma", "%f", resource.gamma); */

    globals.ev.flags |= EV_NEWPAGE;
    XFlush(DISP);
}

static void
Act_set_density(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    warn_num_params("set-density()", params, *num_params, 1);
    
    if (!get_int_arg(params, num_params, &arg)) {
	XBell(DISP, 0);
	return;
    }
/*     if (arg < 0) { */
/* 	XBell(DISP, 0); */
/* 	statusline_print(STATUS_SHORT, */
/* 			 "set-density requires a positive value"); */
/* 	return; */
/*     } */
    do_set_density(arg != 0 ? arg / 100.0 : 1.0, False, True);
#if MOTIF
    update_preferences_darkness();
#endif
}


static void
Act_change_density(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{
    int arg;
    double diff;
    double oldgamma = resource.gamma;
    UNUSED(w);
    UNUSED(event);

    if (block_this_mouse_event())
	return;
    
    warn_num_params("change-density()", params, *num_params, 1);
    
    if (!get_int_arg(params, num_params, &arg)) {
	XBell(DISP, 0);
	return;
    }
    diff = oldgamma / arg;
    if (oldgamma + diff <= 0.0)
	oldgamma = 0.0;
    else
	oldgamma += diff;
    do_set_density(oldgamma, False, True);
#if MOTIF
    update_preferences_darkness();
#endif
}

static void
Act_fullscreen(Widget w, XEvent *event,
	       String *params, Cardinal *num_params)
{
    Dimension main_win_w, main_win_h;
    static int orig_x = 0, orig_y = 0;
#ifndef MOTIF
    static Dimension get_x, get_y;
#endif
    static Dimension old_w = 0, old_h = 0;
    Dimension panel_width = 0;
#if 0
    static Dimension w_old = 0, h_old = 0;
#endif

    if (block_this_mouse_event())
	return;

    UNUSED(w);
    UNUSED(event);
    
    if (!toggle_arg(resource.fullscreen, params, num_params)) {
	return;
    }
    
    resource.fullscreen = !resource.fullscreen;
    
#ifndef MOTIF
    panel_width = get_panel_width();
#endif
    
    if (resource.fullscreen) {
	/* when toggling to fullscreen, save current scrollbar values
	   and window geometry */
	XtVaGetValues(globals.widgets.top_level, XtNwidth, &old_w, XtNheight, &old_h, NULL);
	/*  	fprintf(stderr, "saved geometry: %dx%d\n", old_w, old_h); */
#ifdef MOTIF
	if (globals.widgets.x_bar != NULL)
	    XtVaGetValues(globals.widgets.x_bar, XmNvalue, &orig_x, NULL);
	if (globals.widgets.y_bar != NULL)
	    XtVaGetValues(globals.widgets.y_bar, XmNvalue, &orig_y, NULL);
#else
	get_xy();
	if (globals.widgets.x_bar != NULL)
	    XtVaGetValues(globals.widgets.clip_widget, XtNx, &get_x, NULL);
	if (globals.widgets.y_bar != NULL)
	    XtVaGetValues(globals.widgets.clip_widget, XtNy, &get_y, NULL);
	orig_x = -(get_x - window_x);
	orig_y = -(get_y - window_y);
#endif /* MOTIF */
	/*  	fprintf(stderr, "Current offsets: %d, %d, %d, %d\n", orig_x, window_x, orig_y, window_y); */
    }
#if 0 /* TODO: don't hardcode w/h values -
	 what's wrong with the set_windowsize() call below? */    
    if (resource.fullscreen) {
	XWindowAttributes attr;
	if (XGetWindowAttributes(DISP, XtWindow(globals.widgets.top_level), &attr)) {
	    w_old = attr.width;
	    h_old = attr.height;
	}
	main_win_w = WidthOfScreen(SCRN);
	main_win_h = HeightOfScreen(SCRN);
    }
    else if (w_old > 0 && h_old > 0) {
	main_win_w = w_old;
	main_win_h = h_old;
    }
    else {
	main_win_w = 800;
	main_win_h = 600;
    }
#else
    if (resource.fullscreen) {
	set_windowsize(&main_win_w, &main_win_h, panel_width, 0, False);
    }
    else { /* re-use old window sizes, or initialize them if started in fullscreen mode */
	if (old_w == 0 || old_h == 0) /* started in fullscreen mode */
	    set_windowsize(&old_w, &old_h, panel_width, 0, False);
	else
	    set_windowsize(&old_w, &old_h, panel_width, 0, True);
    }
#endif

/*      fprintf(stderr, "reconfigure window!\n"); */
    if (resource.fullscreen)
	reconfigure_window(resource.fullscreen, main_win_w, main_win_h, True);
    else {
	reconfigure_window(resource.fullscreen, old_w, old_h, True);
    }

    /*      fprintf(stderr, "orig values: %d, %d\n", orig_x, orig_y); */
    /* restore horizontal scroll position */
    if (resource.keep_flag && orig_x != 0 && globals.widgets.x_bar != NULL) {
#ifdef MOTIF
	if (!resource.fullscreen) {
	    /* only scroll back in non-fullscreen mode. This was:
	       if (resource.fullscreen)
	           (void)set_bar_value(globals.widgets.y_bar, orig_y, (int)(globals.page.h - mane.height));
	       else ...
	       but that was broken (bogus values in globals.page.h - mane.height).
	    */
	    (void)set_bar_value(globals.widgets.x_bar, orig_x, INT_MAX);
	}
#else
	int curr_x;
	get_xy();
	XtVaGetValues(globals.widgets.clip_widget, XtNx, &get_x, NULL);
	curr_x = -(get_x - window_x);
	if (curr_x - orig_x > 0)
	    XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer)(curr_x - orig_x));
#endif /* MOTIF */
    }

    /* restore vertical scroll position */
    if (resource.keep_flag && orig_y != 0 && globals.widgets.y_bar != NULL) {
#ifdef MOTIF
	if (!resource.fullscreen) {
	    /* only scroll back in non-fullscreen mode. This was:
	       if (resource.fullscreen)
	           (void)set_bar_value(globals.widgets.y_bar, orig_y, (int)(globals.page.h - mane.height));
	       else ...
	       but that was broken (bogus values in globals.page.h - mane.height).
	    */
	    (void)set_bar_value(globals.widgets.y_bar, orig_y, INT_MAX);
	}
#else
	int curr_y;
	get_xy();
	XtVaGetValues(globals.widgets.clip_widget, XtNy, &get_y, NULL);
	curr_y = -(get_y - window_y);
	if (curr_y - orig_y > 0)
	    XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc, (XtPointer)(curr_y - orig_y));
#endif /* MOTIF */
    }
    handle_x_scroll(NULL, NULL, NULL, NULL);
    handle_y_scroll(NULL, NULL, NULL, NULL);
}

#ifdef GREY

static void
Act_set_greyscaling(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    warn_num_params("set-greyscaling()", params, *num_params, 1);
    
    if (!get_int_arg(params, num_params, &arg)) { /* no arg, toggle */
	if (!toggle_arg(resource.use_grey, params, num_params)) {
	    return;
	}
	resource.use_grey = !resource.use_grey;
	if (resource.use_grey) {
	    statusline_print(STATUS_SHORT, "greyscaling on");
	}
	else {
	    statusline_print(STATUS_SHORT, "greyscaling off");
	}
	globals.ev.flags |= EV_NEWPAGE;
	XFlush(DISP);
	return;
    }

    switch (arg) {
    case 0:
	resource.use_grey = False;
	statusline_print(STATUS_SHORT, "greyscaling off");
	break;
    case 1:
	resource.use_grey = True;
	statusline_print(STATUS_SHORT, "greyscaling on");
	break;
    default:
	{
	    float newgamma = arg != 0 ? arg / 100.0 : 1.0;
	    resource.use_grey = newgamma;
	    statusline_print(STATUS_SHORT, "greyscale value: %.1f", newgamma);
	}
    }

    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
    
    if (resource.use_grey) {
	if (G_visual->class != TrueColor)
	    init_plane_masks();
#if COLOR
	fg_active = NULL;
#else
	init_pix();
#endif
    }
    reset_fonts();
    globals.ev.flags |= EV_NEWPAGE;
    XFlush(DISP);
}

#endif

#if COLOR

void
do_toggle_color(Boolean update_resource)
{
    if (resource.use_color) {
	resource.use_color = False;
	full_reset_colors();
	scanned_page_color = total_pages;
#if PS
	if (ignore_papersize_specials || scanned_page_ps <= total_pages) {
	    scanned_page = scanned_page_ps;
	}
#endif
	statusline_print(STATUS_SHORT, "color specials off");
    }
    else {
	resource.use_color = True;
	scanned_page = scanned_page_color = scanned_page_reset;
	statusline_print(STATUS_SHORT, "color specials on");
    }
    if (update_resource)
	globals.curr_use_color = resource.use_color;
    
#ifdef MOTIF
    update_preferences_color();
#endif
    globals.ev.flags |= EV_NEWPAGE;
}

static void
Act_set_color(Widget w, XEvent *event,
	      String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);

    if (block_this_mouse_event())
	return;
    
    if (!toggle_arg(resource.use_color, params, num_params)) {
	return;
    };
	
    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }

    do_toggle_color(True);
}

#endif /* COLOR */

#if PS

void
Act_set_ps(Widget w, XEvent *event,
	   String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(event);
    UNUSED(w);
    
    if (!get_int_arg(params, num_params, &arg))
	resource.postscript++;
    else
	resource.postscript = arg;

    if (resource.postscript > 2)
	resource.postscript = 0;

#ifdef PS_GS
    if (!resource.useGS) {
	if (resource.postscript > 0) {
	    popup_message(globals.widgets.top_level,
			  MSG_WARN,
			  "This version of xdvi depends on ghostscript for rendering Postscript images. "
			  "Postscript rendering cannot be activated if the option ``-noghostscript'' is used "
			  "or if the resource ``Ghostscript'' is set to false.",
			  /* message */
			  "Option ``-noghostscript'' is active; "
			  "cannot enable Postscript rendering without ghostscript.");
	}
	resource.postscript = 0;
    }
#endif
    if (resource.postscript > 0) {
	scanned_page_ps = scanned_page_ps_bak;
	if (scanned_page > scanned_page_ps)
	    scanned_page = scanned_page_ps;
	if (resource.postscript == 1) {
	    statusline_print(STATUS_SHORT, "Postscript rendering on");
	}
	else {
	    statusline_print(STATUS_SHORT, "Postscript rendering on (with bounding box)");
	}
    }
    else {
	scanned_page_ps_bak = scanned_page_ps;
	scanned_page_ps = total_pages;
#if COLOR
	if (ignore_papersize_specials || scanned_page_color <= total_pages) {
	    scanned_page = scanned_page_color;
	}
#endif
	statusline_print(STATUS_SHORT, "Postscript rendering off; displaying bounding box instead");
    }

    store_preference(NULL, "postscript", "%d", resource.postscript);
    
    psp.toggle(resource.postscript);
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.postscript, Act_set_ps, check_int);
#else
    toggle_menu(resource.postscript, Act_set_ps);
#endif /* NEW_MENU_CREATION */
    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
    
    globals.ev.flags |= EV_PS_TOGGLE;
    XFlush(DISP);
}

#endif /* PS */

void
Act_htex_back(Widget w, XEvent *event,
	      String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    htex_back();
}

void
Act_htex_forward(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    htex_forward();
}

static void
Act_htex_anchorinfo(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    int x, y;
    Window dummy;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    (void)XTranslateCoordinates(DISP, event->xkey.window, mane.win,
				event->xkey.x, event->xkey.y, &x, &y, &dummy);
    htex_displayanchor(x, y);
}

#ifdef PS_GS
void
Act_set_gs_alpha(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);

    if (block_this_mouse_event())
	return;
    
    if (!toggle_arg(resource.gs_alpha, params, num_params)) {
	return;
    }
    resource.gs_alpha = !resource.gs_alpha;

    if (resource.gs_alpha) {
	statusline_print(STATUS_SHORT, "ghostscript alpha active");
    }
    else {
	statusline_print(STATUS_SHORT, "ghostscript alpha inactive");
    }

    store_preference(NULL, "gsAlpha", "%s", resource.gs_alpha ? "True" : "False");
    
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.gs_alpha, Act_set_gs_alpha, check_toggle);
#else
    toggle_menu(resource.gs_alpha, Act_set_gs_alpha);
#endif /* NEW_MENU_CREATION */
#if GS_PIXMAP_CLEARING_HACK
    had_ps_specials = False; /* else infinite loop and/or crash in redraw! */
#endif
    /* like elsewhere */
    if (globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }
    
    globals.ev.flags |= EV_PS_TOGGLE;
    XFlush(DISP);
}
#endif

static void
Act_reread_dvi_file(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
/*      fprintf(stderr, "reread file!\n"); */
    globals.ev.flags |= EV_RELOAD;
}

static void
Act_discard_number(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    m_have_arg = False;
    m_number = 0;
    m_sign = 1;
    if (resource.expert_mode & XPRT_SHOW_STATUSLINE) /* too distracting for stdout */
	statusline_print(STATUS_SHORT, "numerical prefix discarded");
}

/*
 * Actions to support dragging the image.
 */

static int drag_last_x, drag_last_y;	/* last position of cursor */

static int text_last_x = -1;
static int text_last_y = -1;
static int text_last_page = -1;

static void drag_motion(XEvent *);
static void drag_release(XEvent *);

/* access for hyperref.c */
Boolean dragging_text_selection(void)
{
    return mouse_motion == text_motion;
}

static void
text_release(XEvent * event)
{
    int ulx, uly, w, h;
    static char *text;
    int text_len = 0;
    
    UNUSED(event);

    if (mouse_motion == null_mouse)
	return;
    mouse_motion = mouse_release = null_mouse;

    w = text_last_x - drag_last_x;
    h = text_last_y - drag_last_y;

    if (w == 0 || h == 0) {
	text_last_x = text_last_y = -1;
	drag_last_x = drag_last_y = -1;
	text_last_page = -1;
	return;
    }

    if (w < 0) {
	ulx = drag_last_x + w;
	w = -w;
    }
    else
	ulx = drag_last_x;

    if (h < 0) {
	uly = drag_last_y + h;
	h = -h;
    }
    else
	uly = drag_last_y;

    free(text);
    /* this allocates text */
    text = get_text_selection(&text_len,
			      ulx * currwin.shrinkfactor,
			      uly * currwin.shrinkfactor,
			      (ulx + w) * currwin.shrinkfactor,
			      (uly + h) * currwin.shrinkfactor);
    if (text[0] == '\0') {
	/* fprintf(stdout, "Text selection empty.\n"); */
	return;
    }

    TRACE_GUI((stderr, "Selected `%s'", text));

    if (text_len > 4 * XMaxRequestSize(DISP) - 32) {
	XBell(DISP, 10);
	statusline_print(STATUS_MEDIUM, "Selection too large (%d bytes, maximum %d bytes)",
			 text_len, 4 * XMaxRequestSize(DISP) - 32);
	return;
    }
    
    if (!set_selection(text, globals.widgets.top_level)) {
	XBell(DISP, 10);
	statusline_print(STATUS_MEDIUM, "Could not set primary selection!");
	text_change_region(TEXT_SEL_CLEAR, NULL);
    }
}

void
text_motion(XEvent *event)
{
    text_change_region(TEXT_SEL_MOVE, event);
}

static void
get_rectangle(XRectangle *rect, int x, int y, int last_x, int last_y)
{
    rect->width = ABS(x - last_x);
    rect->height = ABS(y - last_y);
    rect->x = (x < last_x) ? x : last_x;
    rect->y = (y < last_y) ? y : last_y;
}

static void
crop_to_window(int *x, int *y)
{
    if (*x < -currwin.base_x + 1)
	*x = -currwin.base_x + 1;
    else if (*x > -currwin.base_x + (int)ROUNDUP(pageinfo_get_page_width(current_page), currwin.shrinkfactor) + 1)
	*x = -currwin.base_x + ROUNDUP(pageinfo_get_page_width(current_page), currwin.shrinkfactor) + 1;
    
    if (*y < -currwin.base_y + 1)
	*y = -currwin.base_y + 1;
    else if (*y > -currwin.base_y + (int)ROUNDUP(pageinfo_get_page_height(current_page), currwin.shrinkfactor) + 1)
	*y = -currwin.base_y + (int)ROUNDUP(pageinfo_get_page_height(current_page), currwin.shrinkfactor) + 1;
}

void
text_change_region(textSelectionT mode, XEvent *event)
{
    static GC bboxGC = 0;
    static GC redrawGC = 0; /* needed since bboxGC is clipped to diff of old and new region */

    if (bboxGC == 0) {
	XGCValues values;
	unsigned long valuemask;

	values.function = GXinvert; /* as in search code */
	if (values.function == GXinvert) {
	    valuemask = GCFunction;
	}
	else {
	    values.foreground = WhitePixelOfScreen(SCRN) ^ BlackPixelOfScreen(SCRN);
	    /* 		fprintf(stderr, "foreground: 0x%lx, white pixel: 0x%lx, black pixel: 0x%lx\n", */
	    /* 			values.foreground, WhitePixelOfScreen(SCRN), BlackPixelOfScreen(SCRN)); */
	    valuemask = GCFunction | GCForeground;
	}
	values.line_width = 1;
	valuemask |= GCLineWidth;
	bboxGC = XCreateGC(DISP, XtWindow(globals.widgets.top_level), valuemask, &values);
	redrawGC = XCreateGC(DISP, XtWindow(globals.widgets.top_level), valuemask, &values);
    }

    switch (mode) {
    case TEXT_SEL_MOVE:
	{
	    int x, y;
	    
	    Window dummy;

	    XRectangle redraw = { -1, -1, 0, 0 };

	    ASSERT(event != NULL, "event in text_change_region() musn't be NULL for TEXT_SEL_MOVE");

	    (void)XTranslateCoordinates(DISP, event->xkey.window, mane.win,
					event->xkey.x, event->xkey.y,
					&x, &y,
					&dummy);

	    crop_to_window(&x, &y);

	    get_rectangle(&redraw, x, y, drag_last_x, drag_last_y);

	    /*
	     * If we have an old region, we want to clip the GC to the area:
	     *
	     * (clip \cup redraw) - (clip \cap redraw)
	     *
	     * and redraw both rectangle areas; otherwise, just draw the redraw area.
	     */

	    if (text_last_x != -1 && text_last_y != -1) {

		XRectangle clip = { -1, -1, 0, 0 };
		
		Region clip_region = XCreateRegion();
		Region redraw_region = XCreateRegion();
		Region union_region = XCreateRegion();
		Region intersect_region = XCreateRegion();

		get_rectangle(&clip, text_last_x, text_last_y, drag_last_x, drag_last_y);

		XUnionRectWithRegion(&clip, clip_region, clip_region);
		XUnionRectWithRegion(&redraw, redraw_region, redraw_region);

		XUnionRegion(clip_region, redraw_region, union_region);
		XIntersectRegion(clip_region, redraw_region, intersect_region);
		
		XSubtractRegion(union_region, intersect_region, redraw_region);

		XSetRegion(DISP, bboxGC, redraw_region);

		XDestroyRegion(clip_region);
		XDestroyRegion(redraw_region); 
		XDestroyRegion(union_region); 
		XDestroyRegion(intersect_region); 
		
		XFillRectangle(DISP, mane.win, bboxGC, clip.x, clip.y, clip.width, clip.height);
	    }
	    
	    XFillRectangle(DISP, mane.win, bboxGC, redraw.x, redraw.y, redraw.width, redraw.height);

	    text_last_x = x;
	    text_last_y = y;
	    text_last_page = current_page;
	}
	break;
    case TEXT_SEL_CLEAR:
	unset_selection(globals.widgets.top_level);
    case TEXT_SEL_ERASE:
    case TEXT_SEL_REDRAW:
	if (text_last_page == current_page
	    && text_last_x != -1 && text_last_y != -1
	    && drag_last_x != -1 && drag_last_y != -1) {

	    XRectangle clear = { -1, -1, 0, 0 };

	    get_rectangle(&clear, text_last_x, text_last_y, drag_last_x, drag_last_y);
	    
	    if (mode == TEXT_SEL_CLEAR) {
		text_last_x = text_last_y = drag_last_x = drag_last_y = -1;
		text_last_page = -1;
		/* Note ZLB: the region is erased instead of inverted to avoid
		 * multiple inverting problem. An exposure is generated to
		 * make the region redrawn */
 		clearexpose(&mane, clear.x, clear.y, clear.width, clear.height);
	    }
	    else if (clip_region_to_rect(&clear)) {
		if (mode == TEXT_SEL_ERASE) {
		    /* If width or height are 0, XClearArea will clear entire window
		     * from coordinates x, y, so check for that: */
		    if (clear.width > 0 && clear.height > 0)
		    	XClearArea(DISP, mane.win, clear.x, clear.y, clear.width, clear.height, False);
		}
		else
		    XFillRectangle(DISP, mane.win, redrawGC, clear.x, clear.y, clear.width, clear.height);
	    }
	}
	break;
    }
}

void
text_selection_start(XEvent *event)
{
    int x, y;
    Window dummy;

    /* erase existing region */
    text_change_region(TEXT_SEL_CLEAR, NULL);

    (void)XTranslateCoordinates(DISP, event->xkey.window, mane.win,
				event->xkey.x, event->xkey.y,
				&x, &y,
				&dummy);	/* throw away last argument */

    crop_to_window(&x, &y);
    
    drag_last_x = x;
    drag_last_y = y;

    text_last_x = text_last_y = -1;

    if (mouse_release == null_mouse) {
	mouse_motion = text_motion;
	mouse_release = text_release;
    }
}

/*
 * If action list for <Btn1Down> does NOT contain a magnifier() binding,
 * return that action list; else return NULL.
 */
static char *
btn1_no_magnifier_binding(void)
{
    static Boolean initialized = False;
    static char *btn1_binding = NULL;

    if (!initialized) { /* cache value of previous calls */
	if (resource.main_translations != NULL) {
	    if ((btn1_binding = strstr(resource.main_translations, "<Btn1Down>")) != NULL) {
		char *eol, *test;
		btn1_binding = strchr(btn1_binding, ':');
		if (btn1_binding != NULL)
		    btn1_binding++;
		while (isspace(*btn1_binding))
		    btn1_binding++;
		eol = strchr(btn1_binding, '\n');
		if (eol == NULL)
		    eol = btn1_binding + strlen(btn1_binding);
		if ((test = strstr(btn1_binding, "magnifier(")) == NULL || test > eol) {
		    /* binding not found */
		    return btn1_binding;
		}
	    }
	}
/* 	fprintf(stderr, "MAG: %d\n", btn1_has_magnifier); */
	initialized = True;
    }
    return btn1_binding;
}

void
Act_switch_mode(Widget w, XEvent *event,
		String *params, Cardinal *num_params)
{
    mouseModeT prev_mode = resource.mouse_mode;
    int arg = -1;
    char *ptr;
#if defined(LESSTIF_VERSION)
    static Boolean warned_about_lesstif = False;
#endif
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);

    if ((ptr = btn1_no_magnifier_binding()) != NULL) {
	XBell(DISP, 0);
	statusline_print(STATUS_MEDIUM,
			 "Cannot switch modes: Action list for Btn1Down (%s) "
			 "does not contain magnifier() action.", ptr);
	return;
    }
    
/*      fprintf(stderr, "prev mode: %d\n", prev_mode); */
    
    if (get_int_arg(params, num_params, &arg)) {
	if (arg < MOUSE_MAGNIFIER_MODE || arg >= MOUSE_MAX_MODE) {
	    statusline_print(STATUS_SHORT, "Argument for Act_switch_mode outside of range from %d to %d",
			     MOUSE_MAGNIFIER_MODE, MOUSE_MAX_MODE - 1);
	    resource.mouse_mode++;
	}
	else
	    resource.mouse_mode = (mouseModeT)arg;
    }
    else
	resource.mouse_mode++;

    /* check if wrapped */
    if (resource.mouse_mode >= MOUSE_MAX_MODE)
	resource.mouse_mode = MOUSE_MAGNIFIER_MODE;

#if defined(LESSTIF_VERSION)
    if (!warned_about_lesstif && resource.mouse_mode != MOUSE_MAGNIFIER_MODE) {
	popup_message(globals.widgets.top_level,
		      MSG_WARN,
		      NULL,
		      "Note: modes other than magnifier mode may not work properly with LessTif.");
	warned_about_lesstif = True;
    }
#endif
    
    /* undo effects of previous mode */
    switch (prev_mode) {
    case MOUSE_RULER_MODE:
	clear_ruler();
	mouse_motion = mouse_release = null_mouse;
	break;
    case MOUSE_TEXT_MODE:
	text_change_region(TEXT_SEL_CLEAR, NULL);
	mouse_motion = mouse_release = null_mouse;
	break;
    default: /* magnifier, nothing to do */
	    break;
    }
    
    if (resource.mouse_mode == MOUSE_RULER_MODE) {
	XDefineCursor(DISP, CURSORWIN, globals.cursor.rule);
	statusline_print(STATUS_SHORT, "Ruler mode; click Mouse-1 to set ruler");
	show_ruler(event);
	show_distance_from_ruler(event, False);
    }
    else if (resource.mouse_mode == MOUSE_TEXT_MODE) {
	statusline_print(STATUS_SHORT, "Text selection mode; click and drag Mouse-1 to select a region");
    }
    else { /* default: MOUSE_MAGNIFIER_MODE */
	statusline_print(STATUS_SHORT, "Magnifier mode; click Mouse-1 to magnify text");
	mouse_motion = mouse_release = null_mouse;
    }
    
    globals.ev.flags |= EV_CURSOR;
    XFlush(DISP);
    
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.mouse_mode, Act_switch_mode, check_int);
#else
    toggle_menu(resource.mouse_mode, Act_switch_mode);
#endif /* NEW_MENU_CREATION */
    store_preference(NULL, "mouseMode", "%d", resource.mouse_mode);
}

static void
Act_text_mode(Widget w, XEvent *event,
	      String *params, Cardinal *num_params)
{
    char *ptr;
#if defined(LESSTIF_VERSION)
    static Boolean warned_about_lesstif = False;
#endif
    
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;

    if ((ptr = btn1_no_magnifier_binding()) != NULL) {
	XBell(DISP, 0);
	statusline_print(STATUS_MEDIUM,
			 "Cannot switch modes: Action list for Btn1Down (%s) "
			 "does not contain magnifier() action.", ptr);
	return;
    }
    
    if (resource.mouse_mode == MOUSE_TEXT_MODE) {
	resource.mouse_mode++;
	if (resource.mouse_mode >= MOUSE_MAX_MODE)
	    resource.mouse_mode = MOUSE_MAGNIFIER_MODE;
    }
    else
	resource.mouse_mode = MOUSE_TEXT_MODE;

#if defined(LESSTIF_VERSION)
    if (!warned_about_lesstif && resource.mouse_mode != MOUSE_MAGNIFIER_MODE) {
	popup_message(globals.widgets.top_level,
		      MSG_WARN,
		      NULL,
		      "Note: modes other than magnifier mode may not work properly with LessTif.");
	warned_about_lesstif = True;
    }
#endif
    
    if (resource.mouse_mode == MOUSE_TEXT_MODE) {
	if (mouse_release != null_mouse && mouse_release != text_release)
	    return;
	
	globals.ev.flags |= EV_CURSOR;
	
        XFlush(DISP);
    }
    else {
	mouse_motion = mouse_release = null_mouse;
	globals.ev.flags |= EV_CURSOR;

	text_change_region(TEXT_SEL_CLEAR, NULL);
	
	XFlush(DISP);
    }
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.mouse_mode, Act_switch_mode, check_int);
#else
    toggle_menu(resource.mouse_mode, Act_switch_mode);
#endif /* NEW_MENU_CREATION || MOTIF */
}

static void
Act_drag(Widget w, XEvent *event,
	 String *params, Cardinal *num_params)
{
    UNUSED(w);

    if (block_this_mouse_event())
	return;
    
    if (mouse_release != null_mouse && mouse_release != drag_release)
	return;

    if (resource.mouse_mode == MOUSE_TEXT_MODE && text_last_page != -1
	/* this was:
	   && (text_last_x != drag_last_x || text_last_y != drag_last_y)
	   but that isn't restrictive enough */
	) { /* dragging would mess up region drawing */
	XBell(DISP, 0);
	statusline_print(STATUS_SHORT, "Dragging page not possible while selection is active");
	return;
    }
    
    if (*num_params != 1) {
	XDVI_WARNING((stderr, "drag() requires 1 argument (got %d)", *num_params));
	return;
    }
    switch (**params) {
    case '|':
	globals.cursor.flags |= CURSOR_DRAG_V;
	break;
    case '-':
	globals.cursor.flags |= CURSOR_DRAG_H;
	break;
    case '+':
	globals.cursor.flags |= CURSOR_DRAG_A;
	break;
    default:
	XDVI_WARNING((stderr, "drag(): Valid arguments are `+', `|' or `-'"));
    }
    
    globals.ev.flags |= EV_CURSOR;
    
    if (mouse_release == null_mouse) {
	mouse_motion = drag_motion;
	mouse_release = drag_release;
	drag_last_x = event->xbutton.x_root;
	drag_last_y = event->xbutton.y_root;
    }
    else
	drag_motion(event);

    XFlush(DISP);
}


static void
drag_motion(XEvent * event)
{
#ifdef MOTIF
    get_xy();
#endif

    if (globals.cursor.flags & (CURSOR_DRAG_H | CURSOR_DRAG_A)) { /* horizontal motion */
#ifdef MOTIF
	(void)set_bar_value(globals.widgets.x_bar,
			    drag_last_x - event->xbutton.x_root - window_x,
			    (int)(globals.page.w - mane.width));
#else
	if (globals.widgets.x_bar != NULL) {
	    XtCallCallbacks(globals.widgets.x_bar, XtNscrollProc,
			    (XtPointer) (drag_last_x - event->xbutton.x_root));
	}
#endif
	drag_last_x = event->xbutton.x_root;
    }

    if (globals.cursor.flags & (CURSOR_DRAG_V | CURSOR_DRAG_A)) { /* vertical motion */
#ifdef MOTIF
	(void)set_bar_value(globals.widgets.y_bar,
			    drag_last_y - event->xbutton.y_root - window_y,
			    (int)(globals.page.h - mane.height));
#else
	if (globals.widgets.y_bar != NULL) {
	    XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc,
			    (XtPointer) (drag_last_y - event->xbutton.y_root));
	}
#endif
	drag_last_y = event->xbutton.y_root;
    }
    handle_x_scroll(NULL, NULL, NULL, NULL);
    handle_y_scroll(NULL, NULL, NULL, NULL);
}


static void
drag_release(XEvent * event)
{
    drag_motion(event);
    mouse_motion = mouse_release = null_mouse;

    globals.cursor.flags &= ~(CURSOR_DRAG_H | CURSOR_DRAG_V | CURSOR_DRAG_A);
    globals.ev.flags |= EV_CURSOR;
    XFlush(DISP);
}



/* Wheel mouse support.  */

static int wheel_button = -1;

static void
Act_wheel(Widget w, XEvent *event,
	  String *params, Cardinal *num_params)
{
    int dist;

    UNUSED(w);

    if (block_this_mouse_event())
	return;

    if (*num_params != 1) {
	XDVI_WARNING((stderr, "wheel() requires 1 argument (got %d)", *num_params));
	return;
    }
    dist = (strchr(*params, '.') == NULL) ? atoi(*params)
	: (int)(my_atof(*params) * resource.wheel_unit);
#ifdef MOTIF
    get_xy();
    set_bar_value(globals.widgets.y_bar, dist - window_y, (int)(globals.page.h - mane.height));
#else
    if (globals.widgets.y_bar != NULL)
	XtCallCallbacks(globals.widgets.y_bar, XtNscrollProc, (XtPointer) dist);
#endif

    wheel_button = event->xbutton.button;

    handle_y_scroll(NULL, NULL, NULL, NULL);
}


/* Internal mouse actions.  */

static void
Act_motion(Widget w, XEvent *event,
	   String *params, Cardinal *num_params)
{
    /* remember last position, to do this only when pointer is actually moved */
    static int old_x = -1, old_y = -1;
    int x, y;

    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;

    if (resource.mouse_mode == MOUSE_RULER_MODE) {
	show_distance_from_ruler(event, False);
    }
    /* This used to be:
          (abs(x - old_x) > x_threshold || abs(y - old_y) > y_threshold))
       but that didn't work too well either. Just change it whenever user
       moves the mouse. */       
    if (!MAGNIFIER_ACTIVE && resource.mouse_mode != MOUSE_RULER_MODE
	&& pointerlocate(&x, &y) && (x != old_x || y != old_y)) {
	htex_displayanchor(x, y);
	old_x = x;
	old_y = y;
    }

    if ((int)(event->xbutton.button) != wheel_button) {
	mouse_motion(event);
    }
}


static void
Act_release(Widget w, XEvent *event,
	    String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    if ((int)(event->xbutton.button) == wheel_button) {
	wheel_button = -1;
	return;
    }

    mouse_release(event);
}

void
Act_ruler_mode(Widget w, XEvent *event,
	       String *params, Cardinal *num_params)
{
    char *ptr;
#if defined(LESSTIF_VERSION)
    static Boolean warned_about_lesstif = False;
#endif
    
    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;

    if ((ptr = btn1_no_magnifier_binding()) != NULL) {
	XBell(DISP, 0);
	statusline_print(STATUS_MEDIUM,
			 "Cannot switch modes: Action list for Btn1Down (%s) "
			 "does not contain magnifier() action.", ptr);
	return;
    }
    
    if (resource.mouse_mode == MOUSE_RULER_MODE) {
	resource.mouse_mode++;
	if (resource.mouse_mode >= MOUSE_MAX_MODE)
	    resource.mouse_mode = MOUSE_MAGNIFIER_MODE;
    }
    else
	resource.mouse_mode = MOUSE_RULER_MODE;
    
#if defined(LESSTIF_VERSION)
    if (!warned_about_lesstif && resource.mouse_mode != MOUSE_MAGNIFIER_MODE) {
	popup_message(globals.widgets.top_level,
		      MSG_WARN,
		      NULL,
		      "Note: modes other than magnifier mode may not work properly with LessTif.");
	warned_about_lesstif = True;
    }
#endif
    
    if (resource.mouse_mode == MOUSE_RULER_MODE) {
	XDefineCursor(DISP, CURSORWIN, globals.cursor.rule);
	statusline_print(STATUS_SHORT, "Ruler mode on; use Mouse-1 to set/drag ruler");
	show_ruler(event);
	show_distance_from_ruler(event, False);
    }
    else {
	if (globals.cursor.flags & CURSOR_LINK) {
	    XDefineCursor(DISP, CURSORWIN, globals.cursor.link);
	}
	else {
	    XDefineCursor(DISP, CURSORWIN, globals.cursor.ready);
	}
	statusline_print(STATUS_SHORT, "Ruler mode off");
	clear_ruler();
	mouse_motion = mouse_release = null_mouse;
    }
#if defined(NEW_MENU_CREATION) || defined(MOTIF)
    set_menu(&resource.mouse_mode, Act_switch_mode, check_int);
#else
    toggle_menu(resource.mouse_mode, Act_switch_mode);
#endif /* NEW_MENU_CREATION || MOTIF */
}


/* Actions for source specials.  */

void
Act_source_special(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{
    Window dummy;

    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    if ((event->type == ButtonPress && mouse_release != null_mouse)
	|| MAGNIFIER_ACTIVE) {
	XBell(DISP, 0);
	return;
    }

    source_reverse_x = event->xbutton.x;
    source_reverse_y = event->xbutton.y;
    if (event->xbutton.window != mane.win)
	(void)XTranslateCoordinates(DISP,
				    RootWindowOfScreen(SCRN), mane.win,
				    event->xbutton.x_root,
				    event->xbutton.y_root,
				    &source_reverse_x,
				    &source_reverse_y,
				    &dummy);	/* throw away last argument */

    source_reverse_x = (source_reverse_x + mane_base_x) * mane.shrinkfactor;
    source_reverse_y = (source_reverse_y + mane_base_y) * mane.shrinkfactor;

    globals.ev.flags |= EV_SRC;
}

void
Act_show_source_specials(Widget w, XEvent *event,
			 String *params, Cardinal *num_params)
{
    int arg;
    Boolean clear_statusline = False;
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);

    if (!get_int_arg(params, num_params, &arg))
	arg = -1;
    else
	clear_statusline = True;

    if ((event->type == ButtonPress && mouse_release != null_mouse)
	|| magnifier.win != (Window) 0) {
	XBell(DISP, 0);
	return;
    }

    if (!(globals.ev.flags & EV_SRC)) {
	source_reverse_x = -1;
	source_show_all = (arg == 1 ? True : False);

	globals.ev.flags |= EV_SRC;
    }
    if (clear_statusline)
	statusline_clear();
}

void
Act_source_what_special(Widget w, XEvent *event,
			String *params, Cardinal *num_params)
{
    int my_x, my_y;
    Window dummy;

    UNUSED(w);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    (void)XTranslateCoordinates(DISP, event->xkey.window, mane.win,
				event->xkey.x, event->xkey.y, &my_x, &my_y, &dummy);	/* throw away last argument */
    my_x = (my_x + mane_base_x) * mane.shrinkfactor;
    my_y = (my_y + mane_base_y) * mane.shrinkfactor;
    source_reverse_search(my_x, my_y, False);
}

static void
select_cb(const char *filename, void *data)
{
    UNUSED(data);
    if (filename != NULL) {
	TRACE_FILES((stderr, "new filename: |%s|", filename));
		
	set_dvi_name(filename);
	current_page = 0; /* switch to first page */
	close_old_filep();
	globals.ev.flags |= EV_NEWDOC;
	globals.ev.flags |= EV_PAGEHIST_INSERT;
    }
}

static void
Act_select_dvi_file(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    static struct filesel_callback cb; /* static so that we can pass its address */
    cb.title = "Xdvi: Open file";
    cb.prompt = "Open file:";
    cb.ok = "OK";
    cb.cancel = "Cancel";
    cb.init_path = NULL;
    cb.filemask = "*.dvi";
    cb.must_exist = True;
    cb.exit_on_cancel = False;
    cb.func_ptr = select_cb;
    cb.data = NULL;

    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;

    file_history_set_page(current_page);
    
    XsraSelFile(globals.widgets.top_level, &cb);
}

/*
 * If all GUI elements have been turned on/off, make this synonymous
 * with expert mode off/on, so that next `x' keystroke does something
 * reasonable.
 */
void
update_expert_mode(void)
{
    if ((resource.expert_mode & (XPRT_SHOW_STATUSLINE | XPRT_SHOW_SCROLLBARS
#ifdef MOTIF
				 | XPRT_SHOW_PAGELIST | XPRT_SHOW_TOOLBAR | XPRT_SHOW_MENUBAR
#else
				 | XPRT_SHOW_BUTTONS
#endif
				 )) == XPRT_SHOW_ALL) {
	resource.expert = False;
    }
    else if ((resource.expert_mode & (XPRT_SHOW_STATUSLINE
#ifdef MOTIF
				      | XPRT_SHOW_SCROLLBARS | XPRT_SHOW_PAGELIST
				      | XPRT_SHOW_TOOLBAR | XPRT_SHOW_MENUBAR
#else
				      | BROKEN_RECONFIG ? 0 : XPRT_SHOW_SCROLLBARS | XPRT_SHOW_BUTTONS
#endif
				      )) == XPRT_SHOW_NONE) {
	resource.expert = True;
    }
}

void
Act_set_expert_mode(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    int arg;
    Boolean clear_statusline = False;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg))
	arg = -1;
    else
	clear_statusline = True;
    
    switch(arg) {
    case 1:
	resource.expert_mode ^= XPRT_SHOW_STATUSLINE;
	toggle_statusline();
	update_expert_mode();
	break;
    case 2:
#ifndef MOTIF
	/* show this warning only when not toggling global expert mode
	   (that's why it can't be inside toggle_scrollbars) */
	if (BROKEN_RECONFIG) {
	    popup_message(globals.widgets.top_level,
			  MSG_WARN,
			  NULL,
			  "Sorry - cannot toggle scrollbars with this X Version.\n"
			  "This version of XFree has a broken implementation of the viewportWidget, "
			  "which would break the layout if the scrollbars are toggled. "
			  "Versions that are known to work have a VendorRelease version below 4000 or above 4002. "
			  "You will need to update your XFree server to fix this.");
	    return;
	}
#endif
	resource.expert_mode ^= XPRT_SHOW_SCROLLBARS;
	toggle_scrollbars();
	update_expert_mode();
	break;
	
#ifdef MOTIF
    case 3:
	resource.expert_mode ^= XPRT_SHOW_PAGELIST;
	toggle_pagelist();
	update_expert_mode();
	break;
	
    case 4: /* toolbar */
	resource.expert_mode ^= XPRT_SHOW_TOOLBAR;
	toggle_toolbar();
	update_expert_mode();
	break;
	
    case 5:
	resource.expert_mode ^= XPRT_SHOW_MENUBAR;
	toggle_menubar();
	update_expert_mode();
	break;
#else
    case 3:
/*  	fprintf(stderr, "sidebar\n"); */
	resource.expert_mode ^= XPRT_SHOW_BUTTONS;
	toggle_buttons();
	update_expert_mode();
	break;
#endif
    default:
	/* warn 'em */
	if (
#ifdef MOTIF
	    arg > 5
#else
	    arg > 3
#endif
	    ) {
	    statusline_print(STATUS_SHORT, "Number %d too large for `set-expert-mode', using 0 (= toggle) instead.",
			     arg);
	}
	/* toggle all items */
	resource.expert = !resource.expert;
	if (resource.expert)
	    resource.expert_mode = XPRT_SHOW_NONE;
	else
	    resource.expert_mode = XPRT_SHOW_ALL;
	
	toggle_statusline();
#ifndef MOTIF
	if (!BROKEN_RECONFIG)
	    toggle_scrollbars();
#else
	toggle_scrollbars();
#endif
	
#ifdef MOTIF
	toggle_pagelist();
	toggle_toolbar();
	toggle_menubar();
#else
	toggle_buttons();
#endif
    }

#ifdef MOTIF
    update_preferences_expert();
#endif

    store_preference(NULL, "expertMode", "%d", resource.expert_mode);
    
    if (clear_statusline)
	statusline_clear();
}

Boolean have_src_specials = False;
static Boolean do_update_property = False;

void
handle_x_scroll(Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
#ifndef MOTIF
    Dimension get_x = 0;
#endif

    UNUSED(w);
    UNUSED(closure);
    UNUSED(ev);
    UNUSED(cont);

    if (/* !resource.keep_flag || */ globals.widgets.x_bar == NULL)
	return;

#ifdef MOTIF
    XtVaGetValues(globals.widgets.x_bar, XmNvalue, &m_x_scroll, NULL);
#else
    get_xy();
    XtVaGetValues(globals.widgets.clip_widget, XtNx, &get_x, NULL);
    m_x_scroll = get_x - window_x;
    scroll_x_panner(m_x_scroll);
#endif
}

void
handle_y_scroll(Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
#ifndef MOTIF
    Dimension get_y = 0;
#endif
    
    UNUSED(w);
    UNUSED(closure);
    UNUSED(ev);
    UNUSED(cont);
    
    if (/* !resource.keep_flag || */ globals.widgets.y_bar == NULL)
	return;
    
#ifdef MOTIF
    XtVaGetValues(globals.widgets.y_bar, XmNvalue, &m_y_scroll, NULL);
#else
    get_xy();
    XtVaGetValues(globals.widgets.clip_widget, XtNy, &get_y, NULL);
    m_y_scroll = get_y - window_y;
    scroll_y_panner(m_y_scroll);
#endif
}

void
handle_expose(Widget w, XtPointer closure, XEvent *ev, Boolean *cont)
{
    struct WindowRec *windowrec = (struct WindowRec *)closure;

    UNUSED(w);
    UNUSED(cont);

    if (windowrec == &magnifier) {
	if (magnifier_stat < 0) { /* destroy upon exposure */
	    magnifier_stat = 0;
	    mag_release(ev);
	    return;
	}
	else
	    magnifier_stat = 0;
    }

    do_update_property = False;

    if (have_src_specials && !MAGNIFIER_ACTIVE) {
	do_update_property = True;
    }

    expose(windowrec, (&(ev->xexpose))->x, (&(ev->xexpose))->y,
	   (unsigned int)(&(ev->xexpose))->width, (unsigned int)(&(ev->xexpose))->height);
}


void
handle_property_change(Widget w, XtPointer junk,
		       XEvent *ev, Boolean *cont)
{
    char *prop_ret;
    size_t prop_len;

    UNUSED(w);
    UNUSED(junk);
    UNUSED(cont);

    if ((&(ev->xproperty))->window != XtWindow(globals.widgets.top_level)) /* if spurious event */
	return;
    
    if ((&(ev->xproperty))->atom == atom_src_goto()) {
	/* forward search requested */
	if ((prop_len = property_get_data(XtWindow(globals.widgets.top_level), atom_src_goto(),
					  &prop_ret,
					  XGetWindowProperty)) == 0) {
	    TRACE_CLIENT((stderr, "property_get_data() failed for atom_src_goto()!"));
	    return;
	}
	TRACE_CLIENT((stderr, "got back atom_src_goto: |%s|", prop_ret));
	globals.src.fwd_string = prop_ret;
	globals.ev.flags |= EV_SRC;
    }
    else if ((&(ev->xproperty))->atom == atom_find_string()) {
	/* string search requested */
	if ((prop_len = property_get_data(XtWindow(globals.widgets.top_level), atom_find_string(),
					  &prop_ret,
					  XGetWindowProperty)) == 0) {
	    TRACE_CLIENT((stderr, "property_get_data() failed for atom_find_string()!"));
	    return;
	}
	TRACE_FIND((stderr, "got back atom_find_string: |%s|", prop_ret));
	resource.find_string = prop_ret;
	globals.ev.flags |= EV_FIND;
    }
    else if ((&(ev->xproperty))->atom == atom_reload()) {
	/* like do_sigusr(); there's no data in this case. */
	TRACE_CLIENT((stderr, "atom_reload()"));
	globals.ev.flags |= EV_RELOAD;
    }
    else if ((&(ev->xproperty))->atom == atom_newdoc()) {
	/* loading a new file */
	FILE *new_fp;
	if ((prop_len = property_get_data(XtWindow(globals.widgets.top_level), atom_newdoc(),
					  &prop_ret,
					  XGetWindowProperty)) == 0) {
	    TRACE_CLIENT((stderr, "property_get_data() returned zero length for atom_newdoc()"));
	    /* just raise it in this case */
	    XMapRaised(XtDisplay(globals.widgets.top_level), XtWindow(globals.widgets.top_level));
	    raise_message_windows();
	    return;
	}
	TRACE_CLIENT((stderr, "got back atom_newdoc: |%s|", prop_ret));
	if ((new_fp = XFOPEN(prop_ret, "r")) == NULL) {
	    popup_message(globals.widgets.top_level,
			  MSG_ERR, NULL, "Loading %s failed: %s",
			  prop_ret, strerror(errno));
	    return;
	}
	set_dvi_name(prop_ret);
	globals.ev.flags |= EV_NEWDOC;
    }
    else if ((&(ev->xproperty))->atom == atom_newpage()) {
	/* jumping to a different page */
	int newpage;
	char *testptr;
	if ((prop_len = property_get_data(XtWindow(globals.widgets.top_level), atom_newpage(),
					  &prop_ret,
					  XGetWindowProperty)) == 0) {
	    TRACE_CLIENT((stderr, "property_get_data() failed for atom_newpage(): |%s|", prop_ret));
	    return;
	}
	TRACE_CLIENT((stderr, "got back atom_newpage: |%s|", prop_ret));
	if (strcmp(prop_ret, "+") == 0) { /* special case: treat `+' as last page */
	    newpage = total_pages - 1;
	}
	else {
	    newpage = strtol(prop_ret, &testptr, 10) - 1;
	    if (*testptr != '\0') {
		XDVI_FATAL((stderr, "Invalid page number: `%s'.", prop_ret));
	    }
	}

	if (newpage == total_pages - 1) { /* as in Act_goto_page() */
	    goto_page(check_goto_page(newpage), resource.keep_flag ? NULL : home, False);
	    search_signal_page_changed();
	}
	else {
	    goto_page(check_goto_tex_page(newpage), resource.keep_flag ? NULL : home, False);
	    search_signal_page_changed();
	}
    }
    else if ((&(ev->xproperty))->atom == atom_raise()) {
	XMapRaised(XtDisplay(globals.widgets.top_level), XtWindow(globals.widgets.top_level));
	raise_message_windows();
    }
    else if ((&(ev->xproperty))->atom == atom_reread_prefs()) {
	read_user_preferences(globals.widgets.top_level, ".xdvirc.tmp");
    }
}


/*
 *	Signal routines.  At the signal level, all we do is set flags.
 */

#ifndef FLAKY_SIGPOLL
static RETSIGTYPE
handle_sigpoll(int signo)
{
    UNUSED(signo);
    
    globals.ev.ctr = 1;
    event_freq = -1;	/* forget Plan B */
    sig_flags |= SF_POLL;
# ifndef HAVE_SIGACTION
    (void) signal(SIGPOLL, handle_sigpoll);	/* reset the signal */
# endif
}
#endif

static RETSIGTYPE
handle_sigterm(int signo)
{
    UNUSED(signo);
    
    sig_flags |= SF_TERM;
}

static RETSIGTYPE
handle_sigchld(int signo)
{
    UNUSED(signo);

    sig_flags |= SF_CHLD;
}

static RETSIGTYPE
handle_sigalrm(int signo)
{
    UNUSED(signo);

    sig_flags |= SF_ALRM;
}

static RETSIGTYPE
handle_sigusr(int signo)
{
    UNUSED(signo);
    
    globals.ev.ctr = 1;
    sig_flags |= SF_USR;
}

static RETSIGTYPE
handle_sigsegv(int signo)
{
    UNUSED(signo);
    
    XDVI_ABORT((stderr, "Segmentation fault - trying to clean up and aborting ..."));
}

static Boolean sigalarm_initialized = False;

void
setup_sigalarm(void)
{
#if HAVE_SIGACTION
    struct sigaction a;

    a.sa_handler = handle_sigalrm;
    (void) sigemptyset(&a.sa_mask);
    (void) sigaddset(&a.sa_mask, SIGALRM);
    a.sa_flags = 0;
    sigaction(SIGALRM, &a, NULL);
#else /* not HAVE_SIGACTION */
    (void) signal(SIGALRM, handle_sigalrm);
#endif /* not HAVE_SIGACTION */

    sigalarm_initialized = True;
}

/*
 * Called from main to set up the signal handlers.
 */
void
setup_signal_handlers(void)
{
#ifndef FLAKY_SIGPOLL
    int	sock_fd	= ConnectionNumber(DISP);
#endif
#if HAVE_SIGACTION
    struct sigaction a;
#endif

#ifndef FLAKY_SIGPOLL
#if HAVE_SIGACTION
    /* Subprocess handling, e.g., MakeTeXPK, fails on the Alpha without
       this, because SIGPOLL interrupts the call of system(3), since OSF/1
       doesn't retry interrupted wait calls by default.  From code by
       maj@cl.cam.ac.uk.  */
    a.sa_handler = handle_sigpoll;
    (void) sigemptyset(&a.sa_mask);
    (void) sigaddset(&a.sa_mask, SIGPOLL);
    a.sa_flags = SA_RESTART;
    sigaction(SIGPOLL, &a, NULL);
#else /* not HAVE_SIGACTION */
    (void) signal(SIGPOLL, handle_sigpoll);
#endif /* not HAVE_SIGACTION */

    prep_fd(sock_fd, False);
#endif	/* not FLAKY_SIGPOLL */

#if HAVE_SIGACTION
    a.sa_handler = handle_sigterm;
    (void) sigemptyset(&a.sa_mask);
    (void) sigaddset(&a.sa_mask, SIGINT);
    (void) sigaddset(&a.sa_mask, SIGQUIT);
    (void) sigaddset(&a.sa_mask, SIGTERM);
    (void) sigaddset(&a.sa_mask, SIGHUP);
    a.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &a, NULL);
    sigaction(SIGQUIT, &a, NULL);
    sigaction(SIGTERM, &a, NULL);
    sigaction(SIGHUP, &a, NULL);
    a.sa_handler = handle_sigsegv;
    (void)sigemptyset(&a.sa_mask);
    (void)sigaddset(&a.sa_mask, SIGSEGV);
    a.sa_flags = 0;
    sigaction(SIGSEGV, &a, NULL);
#else /* not HAVE_SIGACTION */
    (void) signal(SIGINT, handle_sigterm);
    (void) signal(SIGQUIT, handle_sigterm);
    (void) signal(SIGTERM, handle_sigterm);
    (void) signal(SIGHUP, handle_sigterm);
    (void)signal(SIGSEGV, handle_sigsegv);
#endif /* not HAVE_SIGACTION */

#if HAVE_SIGACTION
    a.sa_handler = handle_sigchld;
    (void) sigemptyset(&a.sa_mask);
    (void) sigaddset(&a.sa_mask, SIGCHLD);
    a.sa_flags = 0;
    sigaction(SIGCHLD, &a, NULL);
#else /* not HAVE_SIGACTION */
    (void) signal(SIGCHLD, handle_sigchld);
#endif /* not HAVE_SIGACTION */

#if HAVE_SIGACTION
    a.sa_handler = handle_sigusr;
    (void) sigemptyset(&a.sa_mask);
    (void) sigaddset(&a.sa_mask, SIGUSR1);
    a.sa_flags = 0;
    sigaction(SIGUSR1, &a, NULL);
#else /* not HAVE_SIGACTION */
    (void) signal(SIGUSR1, handle_sigusr);
#endif /* not HAVE_SIGACTION */

    (void)sigemptyset(&all_signals);
    (void)sigaddset(&all_signals, SIGPOLL);
    (void)sigaddset(&all_signals, SIGINT);
    (void)sigaddset(&all_signals, SIGQUIT);
    (void)sigaddset(&all_signals, SIGTERM);
    (void)sigaddset(&all_signals, SIGHUP);
    (void)sigaddset(&all_signals, SIGCHLD);
    (void)sigaddset(&all_signals, SIGALRM);
    (void)sigaddset(&all_signals, SIGUSR1);
    (void)sigaddset(&all_signals, SIGSEGV);

}


/*
 *	Mid-level signal handlers.  These are called from within read_events(),
 *	and do the actual work appropriate for the given signal.
 */

/*
 *	Process-related routines.  Call set_chld(xchild *) to indicate
 *	that a given child process should be watched for when it
 *	terminates.  Call clear_chld() to remove the process from the
 *	list.  When the child terminates, the record is removed from
 *	the list and xchild->proc is called.
 *	The caller can set the `io' member in xchild to a pointer to
 *	an xio structure for reading from the file descriptor; see
 *	psgs.c and util.c for examples.
 */

static struct xchild *child_recs = NULL;	/* head of child process list */

void
set_chld(struct xchild *cp)
{
    cp->next = child_recs;
    child_recs = cp;
}

void
clear_chld(struct xchild *cp)
{
    struct xchild	**cpp;
    struct xchild	*cp2;

    if (child_recs == NULL) {
	if (globals.debug & DBG_EVENT)
	    fprintf(stderr, "child_recs: %p\n", (void *)child_recs);
	return;
    }
    for (cpp = &child_recs;;) {
	cp2 = *cpp;
	if (cp2 == cp)
	    break;
	cpp = &cp2->next;
    }
    *cpp = cp->next;
}

static void
do_sigchld(void)
{
    pid_t	pid;
    int	status;

    sig_flags &= ~SF_CHLD;

#if ! HAVE_SIGACTION
    (void) signal(SIGCHLD, handle_sigchld);	/* reset the signal */
#endif
    for (;;) {
#if HAVE_WAITPID
	pid = waitpid(-1, &status, WNOHANG);
#else
	pid = wait3(&status, WNOHANG, NULL);
#endif
	if (pid == 0) break;

	if (pid != -1) {
	    struct xchild	**cpp;
	    struct xchild	*cp;

	    for (cpp = &child_recs;;) {
		cp = *cpp;
		if (cp == NULL)
		    break;
		if (cp->pid == pid) {
		    *cpp = cp->next;	/* unlink it */
		    /* call exit reporting procedure for this child */
		    (cp->proc)(status, cp);
		    break;
		}
		cpp = &cp->next;
	    }
	    break;
	}

	if (errno == EINTR) continue;
	if (errno == ECHILD) break;
#if HAVE_WAITPID
	perror("xdvi: waitpid");
#else
	perror("xdvi: wait3");
#endif
	break;
    }
}


/*
 *	File-related routines.  Call set_io() to indicate that a given fd
 *	should be watched for ability to input or output data.  Call clear_io()
 *	to remove it from the list.  When poll()/select() indicates that the fd
 *	is available for the indicated type of i/o, the corresponding routine
 *	is called.  Call clear_io() to remove an fd from the list.
 *	Both set_io() and clear_io() can be called from within read_proc or
 *	write_proc (although turning an io descriptor on or off is better
 *	accomplished by setting the events flag in the xio structure, and
 *	in the corresponding pollfd structure if the pfd pointer is not NULL
 *	(it is always non-NULL when read_proc and write_proc are called)).
 *	We allocate space for one additional record in the pollfd array, to
 *	accommodate the fd for the X connection; this is done by initializing
 *	num_fds to 1 instead of zero.
 */

static	struct xio	*iorecs	= NULL;	/* head of xio list */

#if HAVE_POLL
static	struct pollfd	*fds	= NULL;
static	int		num_fds	= 1;	/* current number of fds */
static	int		max_fds	= 0;	/* max allocated number of fds */
static	Boolean		io_dirty= True;	/* need to recompute fds[] array */
#else
static	int		numfds	= 0;
static	fd_set		readfds;
static	fd_set		writefds;
#endif

void
set_io(struct xio *ip)
{
    ip->next = iorecs;
    iorecs = ip;

#if HAVE_POLL
    ++num_fds;
    if (!io_dirty && num_fds <= max_fds) {
	fds[num_fds - 1].fd = ip->fd;
	fds[num_fds - 1].events = ip->xio_events;
	ip->pfd = &fds[num_fds - 1];
    }
    else {
	ip->pfd = NULL;
	io_dirty = True;
    }
#else
    if (numfds <= ip->fd) numfds = ip->fd + 1;
#endif
}

void
clear_io(struct xio *ip)
{
    struct xio	**ipp;

    for (ipp = &iorecs;;) {
	struct xio *ip2;

	ip2 = *ipp;
	if (ip2 == ip) break;
	ipp = &ip2->next;
    }
    *ipp = ip->next;

#if HAVE_POLL
    --num_fds;
    io_dirty = True;
#else
# if FLAKY_SIGPOLL
    numfds = ConnectionNumber(DISP);
# else
    numfds = (event_freq < 0 ? -1 : ConnectionNumber(DISP));
# endif
    for (ip = iorecs; ip != NULL; ip = ip->next)
	if (ip->fd > numfds)
	    numfds = ip->fd;
    ++numfds;
#endif /* !HAVE_POLL */
}

static void
do_sigpoll(void)
{
#if FLAKY_SIGPOLL
    sig_flags &= ~SF_POLL;
#else
    struct xio	*ip;

    sig_flags &= ~SF_POLL;

# if HAVE_POLL

    if (io_dirty) {
	struct pollfd *fp;

	if (num_fds > max_fds) {
	    if (fds != NULL) free(fds);
	    fds = xmalloc(num_fds * sizeof *fds);
	    max_fds = num_fds;
	    fds->fd = ConnectionNumber(DISP);
	    fds->events = POLLIN;
	}
	fp = fds + 1;
	for (ip = iorecs; ip != NULL; ip = ip->next) {
	    fp->fd = ip->fd;
	    fp->events = ip->xio_events;
	    ip->pfd = fp;
	    ++fp;
	}
	io_dirty = False;
    }

    for (;;) {
	if (poll(fds + 1, num_fds - 1, 0) >= 0)
	    break;

	if (errno != EAGAIN && errno != EINTR) {
	    perror("xdvi: poll");
	    return;
	}
    }

    for (ip = iorecs; ip != NULL; ip = ip->next) {
	int revents = ip->pfd->revents;
	if (revents & POLLIN && ip->read_proc != NULL)
	    (void)(ip->read_proc)(ip->fd);
	if (revents & POLLOUT && ip->write_proc != NULL)
	    (ip->write_proc)(ip->fd);
    }

# else

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    for (ip = iorecs; ip != NULL; ip = ip->next) {
	if (ip->xio_events & XIO_IN)
	    FD_SET(ip->fd, &readfds);
	if (ip->xio_events & XIO_OUT)
	    FD_SET(ip->fd, &writefds);
    }

    for (;;) {
	struct timeval tv;

	tv.tv_sec = tv.tv_usec = 0;
	if (select(numfds, &readfds, &writefds, (fd_set *) NULL, &tv) >= 0)
	    break;

	if (errno != EAGAIN && errno != EINTR) {
	    perror("select (xdvi read_events)");
	    return;
	}
    }

    for (ip = iorecs; ip != NULL; ip = ip->next) {
	if (FD_ISSET(ip->fd, &readfds) && ip->read_proc != NULL)
	    (void)(ip->read_proc)(ip->fd);
	if (FD_ISSET(ip->fd, &writefds) && ip->write_proc != NULL)
	    (ip->write_proc)(ip->fd);
    }

# endif
#endif /* not FLAKY_SIGPOLL */
}


/*
 *	Timer-related routines.  Call set_timer() to set a timer a given number
 *	of milliseconds in the future.  At that time, the timer will be cleared
 *	and the given procedure will be called with argument set to the struct
 *	passed to set_timer().  The timer routine may call set_timer() or
 *	cancel_timer().
 */


static	struct xtimer		*timers	= NULL;	/* head of timer list */

static	struct itimerval	itv	= {{0, 0}, {0, 0}};

#ifndef	timercmp
#define	timercmp(a, b, cmp)	((a)->tv_sec cmp (b)->tv_sec || \
	((a)->tv_sec == (b)->tv_sec && (a)->tv_usec cmp (b)->tv_usec))
#endif	/* timercmp */


static void
show_timers(char *what)
{
    struct xtimer *tp;
    fprintf(stderr, "=======%s; timers:\n", what);
    for (tp = timers; tp != NULL; tp = tp->next) {
	fprintf(stderr, "timer %p: %lu\n", (void *)tp, tp->when.tv_sec);
    }
    fprintf(stderr, "=======\n");
}

void
set_timer(struct xtimer *tp, int ms)
{
    struct xtimer	**tpp;
    struct xtimer	*tp2;

    if (globals.debug & DBG_EVENT)
	fprintf(stderr, "%s:%d: set_timer\n", __FILE__, __LINE__);

    gettimeofday(&tp->when, NULL);
    itv.it_value.tv_sec = ms / 1000;
    itv.it_value.tv_usec = (ms % 1000) * 1000;
    tp->when.tv_sec += itv.it_value.tv_sec;
    tp->when.tv_usec += itv.it_value.tv_usec;
    if (tp->when.tv_usec >= 1000000) {
	tp->when.tv_usec -= 1000000;
	++tp->when.tv_sec;
    }

    for (tpp = &timers;;) {		/* add timer to list */
	tp2 = *tpp;
	if (tp2 == NULL || timercmp(&tp->when, &tp2->when, <))
	    break;
	tpp = &tp2->next;
    }
    tp->next = tp2;
    *tpp = tp;

    if (tpp == &timers) {
	setitimer(ITIMER_REAL, &itv, NULL);
	if (ms == 0)
	    sig_flags |= SF_ALRM;
    }
    if (globals.debug & DBG_EVENT)
	show_timers("after set_timer");
}

void
cancel_timer(struct xtimer *tp)
{
    struct xtimer	**tpp;

    if (globals.debug & DBG_EVENT)
	show_timers("beginning of cancel_timer");
    
    if (timers == NULL) {
	fprintf(stderr, "%s:%d: BUG? timers == NULL!\n", __FILE__, __LINE__);
	return;
    }

    if (globals.debug & DBG_EVENT)
	fprintf(stderr, "%s:%d: cancel_timer %p from %p\n", __FILE__, __LINE__, (void *)&timers, (void *)tp);

    ASSERT(timers != NULL, "timers in cancel_timer() mustn't be NULL");
    for (tpp = &timers; ; ) {		/* remove from list */
	
	if (*tpp == tp)
	    break;
	tpp = &(*tpp)->next;
    }

    *tpp = (*tpp)->next;	/* unlink it */

    if (timers == NULL) {	/* cancel SIGALRM */
	itv.it_value.tv_sec = itv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itv, NULL);
    }
}

#if TIMER_HACK
/*
 * Original comment by Paul:
 *	Newer versions of the Motif toolkit use the timer facility
 *	(XtAppAddTimeOut(), etc.) in the X Toolkit.  Proper functioning of
 *	this mechanism, however, requires that the X Toolkit be in charge of
 *	blocking.  Since xdvi does its own blocking, this means that we need
 *	to provide working alternatives to these X Toolkit routines (by
 *	redefining them ...).
 *	One symptom of the above-mentioned bug is that the printlog window
 *	eventually stops showing dvips progress until you move the mouse.
 *
 * Comment SU:
 *	Xdvik also uses XtAppAddTimeOut() in other places (hyperref/statusline/...),
 *	so we also need these redefinitions also in the Xaw version.
 *
 */

static void xt_alarm (struct xtimer *);

static struct xtimer *xt_free_timers = NULL;

XtIntervalId
XtAddTimeOut(unsigned long interval, XtTimerCallbackProc proc, XtPointer closure)
{
    return XtAppAddTimeOut(NULL, interval, proc, closure);
}

XtIntervalId
XtAppAddTimeOut(XtAppContext app, unsigned long interval, XtTimerCallbackProc proc, XtPointer closure)
{
    struct xtimer *tp;
    
    UNUSED(app);

    /* FIXME: better way of checking this instead of static boolean sigalarm_initialized?
       The following doesn't work, even after
       sigaddset(&all_signals, SIGALRM);
       
          static sigset_t sig_set;
          (void)sigprocmask(0, NULL, &sig_set);
	  if (sigismember(&sig_set, SIGALRM))
	      ... OK ...
	  else
	      ... NOT OK ...
    */
    ASSERT(sigalarm_initialized, "Shouldn't invoke XtAppAddTimeOut() before setup_sigalarm()");
    
    if (globals.debug & DBG_EVENT)
	fprintf(stderr, "XtAppAddTimeOut: %lu msecs\n", interval);
    
    if (xt_free_timers == NULL)
	tp = xmalloc(sizeof *tp);
    else {
	tp = xt_free_timers;
	xt_free_timers = xt_free_timers->next;
    }

    tp->proc = xt_alarm;
    tp->xt_proc = proc;
    tp->closure = closure;
    
    set_timer(tp, interval);

    if (globals.debug & DBG_EVENT)
	show_timers("XtAppAddTimeOut");

    return (XtIntervalId)tp;
}

void
XtRemoveTimeOut(XtIntervalId id)
{
    struct xtimer *tp;

    ASSERT(id != 0, "XtIntervalId argument in XtRemoveTimeOut() mustn't be NULL");
    tp = (struct xtimer *)id;

    /* Motif (2.1 on Solaris 9, 2003) sometimes calls XtRemoveTimeOut() after
       the timer event has occurred, so we need to be sure not to remove
       the timer record twice.  */
    if (tp->proc == NULL)
	return;
    
    cancel_timer(tp);
    
    tp->next = xt_free_timers;
    xt_free_timers = tp;

    if (globals.debug & DBG_EVENT)
	show_timers("XtRemoveTimeOut");
}

void
xt_alarm(struct xtimer *tp)
{
    XtIntervalId id;

    tp->proc = NULL;	/* flag timer as used-up */
    id = (XtIntervalId) tp;
    (tp->xt_proc)(tp->closure, &id);
    
    tp->next = xt_free_timers;
    xt_free_timers = tp;

    if (globals.debug & DBG_EVENT)
	show_timers("xt_alarm");
}
#endif /* TIMER_HACK */

static	void
do_sigalrm(void)
{
    struct timeval	now;

    sig_flags &= ~SF_ALRM;
#ifndef HAVE_SIGACTION
    (void) signal(SIGALRM, handle_sigalrm);	/* reset the signal */
#endif

    gettimeofday(&now, NULL);

    while (timers != NULL && timercmp(&timers->when, &now, <=)) {
	struct xtimer *tp = timers;
	timers = timers->next;	/* unlink it _first_ */
	(tp->proc)(tp);
    }

    if (timers != NULL) {		/* set next timer */
	int i;
	itv.it_value.tv_sec = timers->when.tv_sec - now.tv_sec;
	i = timers->when.tv_usec - now.tv_usec;
	if (i < 0) {
	    --itv.it_value.tv_sec;
	    i += 1000000;
	}
	itv.it_value.tv_usec = i;

	setitimer(ITIMER_REAL, &itv, NULL);
    }
}



/*
 *	Handle SIGUSR1 signal.  Pretty straightforward.
 */

static void
do_sigusr(void)
{
    sig_flags &= ~SF_USR;
#ifndef HAVE_SIGACTION
    (void) signal(SIGUSR1, handle_sigusr);	/* reset the signal */
#endif
    globals.ev.flags |= EV_RELOAD;
}


static void
do_sigsegv(void)
{
    sig_flags &= ~SF_SEGV;
#ifndef HAVE_SIGACTION
    (void) signal(SIGSEGV, handle_sigsegv);	/* reset the signal */
#endif
    handle_sigsegv(SIGSEGV);
}


/*
 *	Handle termination signals.  Kill child processes, if permitted.
 *	Otherwise, leave it up to the caller.  This is the only place where
 *	the EV_TERM event flag is set, and it can only happen if there's an
 *	active non-killable process.  This should only happen if read_events()
 *	is called from one of a very few select locations.
 */

static void
do_sigterm(void)
{
    struct xchild   *cp;

    sig_flags &= ~SF_TERM;

    /* loop through child processes */
    for (cp = child_recs; cp != NULL; cp = cp->next) {
	if (cp->killable)
	    kill(cp->pid, SIGKILL);
    }

     /* SU: Unlike non-k xdvi, we don't care about whether all children have been
	killed, since processes forked via fork_process() (e.g. the web browser)
	may still continue running. So we just exit here.
     */
    xdvi_exit(EXIT_SUCCESS);

    /* since xdvi_exit() may return, we mustn't do the following which
       is in non-k xdvi, else we'll end up in a busy loop! I don't know
       what's meant by `caller' here anyway ...

       globals.ev.flags |= EV_TERM; /\* otherwise, let the caller handle it *\/
    */
}

/*
 *	Since redrawing the screen is (potentially) a slow task, xdvi checks
 *	for incoming events while this is occurring.  It does not register
 *	a work proc that draws and returns every so often, as the toolkit
 *	documentation suggests.  Instead, it checks for events periodically
 *	(or not, if SIGPOLL can be used instead) and processes them in
 *	a subroutine called by the page drawing routine.  This routine (below)
 *	checks to see if anything has happened and processes those events and
 *	signals.  (Or, if it is called when there is no redrawing that needs
 *	to be done, it blocks until something happens.)
 *
 *	Ultimately, the goal is to have this be the only place in xdvi where
 *	blocking occurs.
 *
 *	The argument to this function should be a mask of event types (EV_*)
 *	indicating which event types should cause read_events to return instead
 *	of waiting for more events.  This function will always process all
 *	pending events and signals before returning.
 *	The return value is the value of globals.ev.flags.
 */

unsigned int
read_events(unsigned int ret_mask)
{
    XEvent event;

#if !HAVE_POLL
    if (numfds == 0)
	numfds = ConnectionNumber(DISP) + 1;
#endif

    if (globals.debug & DBG_EVENT)
	fprintf(stderr, "%s:%d: read_events %u\n", __FILE__, __LINE__, ret_mask);
    for (;;) {
	globals.ev.ctr = event_freq;
	/*
	 * The above line clears the flag indicating that an event is
	 * pending.  So if an event comes in right now, the flag will be
	 * set again needlessly, but we just end up making an extra call.
	 * Also, be careful about destroying the magnifying glass while
	 * drawing on it.
	 */

#if !FLAKY_SIGPOLL

	if (event_freq < 0) {	/* if SIGPOLL works */
	    if (!XtPending()) {
		sigset_t oldsig;

		(void) sigprocmask(SIG_BLOCK, &all_signals, &oldsig);
		for (;;) {
#ifdef SHOW_SIG_FLAGS
		    /* this gives HUGE output ... */
		    if (globals.debug & DBG_EVENT)
			fprintf(stderr, "%s:%d: sig_flags = %d\n",
				__FILE__, __LINE__, sig_flags);
#endif
		    while (sig_flags) {
			flags_to_sigproc[sig_flags]();
		    }

		    if (XtPending())
			break;

		    if (globals.ev.flags & ret_mask) {
			(void) sigprocmask(SIG_SETMASK, &oldsig, (sigset_t *) NULL);
			return globals.ev.flags;
		    }
		    (void) sigsuspend(&oldsig);
		}
		(void) sigprocmask(SIG_SETMASK, &oldsig, (sigset_t *) NULL);
	    }
	}
	else

#endif /* not FLAKY_SIGPOLL */

	    {
		for (;;) {
		    struct xio	*ip;

		    if (globals.debug & DBG_EVENT)
			fprintf(stderr, "%s:%d: (flaky) sig_flags = %d\n",
				__FILE__, __LINE__, sig_flags);
		    while (sig_flags) {
			sigset_t oldsig;

			(void) sigprocmask(SIG_BLOCK, &all_signals, &oldsig);

			while (sig_flags) {
			    flags_to_sigproc[sig_flags]();
			}

			(void) sigprocmask(SIG_SETMASK, &oldsig,
					   (sigset_t *) NULL);
		    }

		    if (XtPending())
			break;

		    if (globals.ev.flags & ret_mask)
			return globals.ev.flags;

		    /* If a SIGUSR1 signal comes right now, then it will wait
		       until an X event or another SIGUSR1 signal arrives. */

#if HAVE_POLL
		    if (globals.debug & DBG_EVENT)
			fprintf(stderr, "%s:%d: have_poll!\n",
				__FILE__, __LINE__);
		    if (io_dirty) {
			struct pollfd *fp;

			if (num_fds > max_fds) {
			    if (fds != NULL) free(fds);
			    fds = xmalloc(num_fds * sizeof *fds);
			    max_fds = num_fds;
			    fds->fd = ConnectionNumber(DISP);
			    fds->events = POLLIN;
			}
			fp = fds + 1;
			for (ip = iorecs; ip != NULL; ip = ip->next) {
			    fp->fd = ip->fd;
			    fp->events = ip->xio_events;
			    ip->pfd = fp;
			    ++fp;
			}
			io_dirty = False;
		    }

		    for (;;) {
			if (poll(fds, num_fds, -1) >= 0) {
			    for (ip = iorecs; ip != NULL; ip = ip->next) {
				int revents = ip->pfd->revents;

				if (revents & POLLIN && ip->read_proc != NULL)
				    (ip->read_proc)(ip->fd);
				if (revents & POLLOUT && ip->write_proc != NULL)
				    (ip->write_proc)(ip->fd);
			    }
			    break;
			}

			if (errno == EINTR)
			    break;

			if (errno != EAGAIN) {
			    perror("xdvi: poll");
			    break;
			}
		    }
#else /* HAVE_POLL */
		    if (globals.debug & DBG_EVENT)
			fprintf(stderr, "%s:%d: NOT have_poll!\n",
				__FILE__, __LINE__);
		    FD_ZERO(&readfds);
		    FD_ZERO(&writefds);
		    FD_SET(ConnectionNumber(DISP), &readfds);
		    for (ip = iorecs; ip != NULL; ip = ip->next) {
			if (ip->xio_events & XIO_IN)
			    FD_SET(ip->fd, &readfds);
			if (ip->xio_events & XIO_OUT)
			    FD_SET(ip->fd, &writefds);
		    }

		    for (;;) {
			if (select(numfds, &readfds, &writefds, (fd_set *) NULL,
				   (struct timeval *) NULL) >= 0) {
			    for (ip = iorecs; ip != NULL; ip = ip->next) {
				if (FD_ISSET(ip->fd, &readfds) && ip->read_proc != NULL) {
				    if (globals.debug & DBG_EVENT)
					fprintf(stderr, "%s:%d: reading from %d\n",
						__FILE__, __LINE__, ip->fd);
				    (ip->read_proc)(ip->fd);
				}
				if (FD_ISSET(ip->fd, &writefds) && ip->write_proc != NULL) {
				    if (globals.debug & DBG_EVENT)
					fprintf(stderr, "%s:%d: writing to %d\n",
						__FILE__, __LINE__, ip->fd);
				    (ip->write_proc)(ip->fd);
				}
			    }
			    break;
			}

			if (errno == EINTR)
			    break;

			if (errno != EAGAIN) {
			    perror("xdvi: select");
			    break;
			}
		    }
#endif /* HAVE_POLL */
		}
	    }
	XtAppNextEvent(app, &event);
#ifdef MOTIF
	if ((resource.expert_mode & XPRT_SHOW_TOOLBAR) != 0)
	    TipAppHandle(app, &event);
#endif

	if (resized)
	    get_geom();

	if (event.xany.window == magnifier.win && event.type == Expose) {
	    handle_expose((Widget) NULL, (XtPointer) &magnifier, &event,
			  (Boolean *) NULL);
	    continue;
	}

#ifdef MOTIF
	if (XtIsRealized(globals.widgets.top_level)
	    && event.xany.window == XtWindow(globals.widgets.clip_widget)
	    && event.type == KeyPress) { /* workaround for #610206 */
	    motif_translations_hack();
	}
#else
	if (resource.expert_mode & XPRT_SHOW_BUTTONS)
	    SubMenuHandleEvent(app, &event);
#endif
	XtDispatchEvent(&event);
    }
}


/*
 * Higher-level routines for managing events.
 */

static void
can_exposures(struct WindowRec *windowrec)
{
    windowrec->min_x = windowrec->min_y = MAXDIM;
    windowrec->max_x = windowrec->max_y = 0;
}

void
redraw(struct WindowRec *windowrec)
{
    currwin = *windowrec;
    globals.win_expose.min_x = currwin.min_x + currwin.base_x;
    globals.win_expose.min_y = currwin.min_y + currwin.base_y;
    globals.win_expose.max_x = currwin.max_x + currwin.base_x;
    globals.win_expose.max_y = currwin.max_y + currwin.base_y;
    can_exposures(windowrec);

    /* fix for bug #619070 - the complicated flags (and do_update_property)
       are needed to avoid too many updates at exposures, especially for
       a window of another xdvi instance when the magnifier intersects with
       that window.
    */
    if (have_src_specials && do_update_property
	&& globals.win_expose.min_x != 1 && globals.win_expose.max_y - globals.win_expose.min_y != 1
	&& currwin.base_x == 0 && currwin.base_y == 0) {
	update_window_property(XtWindow(globals.widgets.top_level), True);
    }

    TRACE_EVENTS((stderr, "Redraw %d x %d at (%d, %d) (base=%d,%d)",
		  globals.win_expose.max_x - globals.win_expose.min_x,
		  globals.win_expose.max_y - globals.win_expose.min_y,
		  globals.win_expose.min_x, globals.win_expose.min_y,
		  currwin.base_x, currwin.base_y));

    /* can't use ev_cursor here, since the event loop might not see this change quick enough */
    if (!(globals.ev.flags & EV_CURSOR)) {
	TRACE_EVENTS((stderr, "Cursor: %ld", globals.cursor.flags));
	if (!(globals.cursor.flags & (CURSOR_DRAG_H | CURSOR_DRAG_V | CURSOR_DRAG_A))) {
	    if (resource.mouse_mode == MOUSE_RULER_MODE)
		XDefineCursor(DISP, CURSORWIN, globals.cursor.rule);
	    else
		XDefineCursor(DISP, CURSORWIN, globals.cursor.wait);
	    XFlush(DISP);
	}
	globals.ev.flags |= EV_CURSOR;
    }

    /* No longer needed since windows are correctly transient now */
    /*      raise_message_windows(); */
    raise_file_selector();
    draw_page();
    globals.warn_spec_now = False;
}


void
redraw_page(void)
{
#if COLOR
    const struct rgb *rgbp;
#endif
    TRACE_FILES((stderr, "Redraw page on %p", (void *)globals.dvi_file.bak_fp));

    if (globals.dvi_file.bak_fp == NULL)
	return;

    if (scanned_page < current_page) {
	TRACE_FILES((stderr, "redraw_page: scanned_page = %d, current_page = %d, prescanning %p\n",
		     scanned_page, current_page, (void *)globals.dvi_file.bak_fp));

	prescan(globals.dvi_file.bak_fp);

	if (globals.ev.flags & EV_GE_NEWPAGE) {	/* if we need to re-prescan */
	    return;
	}
    }

    TRACE_FILES((stderr, "redraw_page: current_page = %d", current_page));
    if (pageinfo_get_window_width(current_page) != globals.page.unshrunk_w
	|| pageinfo_get_window_height(current_page) != globals.page.unshrunk_h) {
	TRACE_FILES((stderr, "NEW SIZE: %dx%d",
		     pageinfo_get_window_width(current_page), pageinfo_get_window_height(current_page)));
	init_page();
	reconfig();
    }
    
    /* We can't call home() without proper unshrunk_page_*, which requires
     * prescan(), which can't be done from within read_events() */

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! BUG ALERT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
       There's some complicated interaction with Postscript specials
       here: if home_action check comes before the gs stuff, psp.drawfile
       might not get initialized correctly, resulting in empty PS figures
       (bounding box instead of figure). This is different in xdvi, due to
       different handling of the home_action stuff, but at the moment I can't
       remember the reason for this ...
       !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! BUG ALERT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    */
#if PS_GS
    if (gs_postpone_prescan) {
	if (!setjmp(globals.ev.canit)) {
	    gs_resume_prescan();
	}
	else
	    return;
    }
#endif
    if (home_action != NULL) {
	home_action(False);
	home_action = NULL;
	/* This discards the expose event generated by home()
	   (1 for each page) */
	if (read_events(EV_NOWAIT) & EV_GE_NEWPAGE) {
	    return;
	}

	can_exposures(&mane);
    }

#if COLOR
    rgbp = &bg_initial;
    if (page_colors.stack != NULL) {
	ASSERT(current_page < (int)page_colors.size, "page_colors.size too small");
	rgbp = &page_colors.stack[current_page].bg;
    }

    /* Set background color */
    if (bg_current == NULL
	|| rgbp->r != bg_current->color.r
	|| rgbp->g != bg_current->color.g
	|| rgbp->b != bg_current->color.b) {
	struct bgrec **bgpp;

	for (bgpp = &bg_head;;) {
	    bg_current = *bgpp;
	    if (bg_current == NULL) {	/* if bg is not in list */
		bg_current = *bgpp = xmalloc(sizeof *bg_current);
		bg_current->next = NULL;
		bg_current->color = *rgbp;
		bg_current->fg_head = NULL;
		bg_current->pixel_good = False;
		break;
	    }
	    if (bg_current->color.r == rgbp->r
		&& bg_current->color.g == rgbp->g
		&& bg_current->color.b == rgbp->b)
		break;
	    bgpp = &bg_current->next;
	}
	fg_current = NULL;	/* force change of foreground color */
	/* globals.gc.high is only used in XDrawRectangle, so its background color
	   doesn't need to be changed.  */
	if (globals.debug & DBG_DVI)
	    printf("Changing background color to %5d %5d %5d\n",
		   bg_current->color.r, bg_current->color.g,
		   bg_current->color.b);

	if (!bg_current->pixel_good) {
	    bg_current->pixel = alloc_color(&bg_current->color,
					    color_data[1].pixel);
	    bg_current->pixel_good = True;
	}
	XSetWindowBackground(DISP, mane.win, bg_current->pixel);
#if MOTIF && !FIXED_FLUSHING_PAGING
	XtVaSetValues(XtParent(globals.widgets.draw_widget), XtNbackground, bg_current->pixel, NULL);
#endif
/* 	XSetWindowBackground(DISP, mane.win, bg_current->pixel); */
/*  	XClearWindow(DISP, mane.win); */
#if 0 /* don't recolor the cursor - gives too low contrast on color backgrounds,
	 and bad appearance when part of the background is white and cursor mask
	 is colored */
	{
	    XColor	bg_Color;
	    bg_Color.pixel = bg_current->pixel;
	    XQueryColor(DISP, G_colormap, &bg_Color);
	    XRecolorCursor(DISP, globals.cursor.ready, &globals.cr_color, &bg_Color);
	    XRecolorCursor(DISP, globals.cursor.wait, &globals.cr_color, &bg_Color);
	}
#endif
    }
#endif /* COLOR */
    
    if (!globals.pausing.flag) {
	XClearWindow(DISP, mane.win);
    }

    if (G_backing_store != NotUseful) {
	mane.min_x = mane.min_y = 0;
	mane.max_x = globals.page.w;
	mane.max_y = globals.page.h;
    }
    else {
	get_xy();
	mane.min_x = -window_x;
	mane.max_x = -window_x + mane.width;
	mane.min_y = -window_y;
	mane.max_y = -window_y + mane.height;
    }

/*      update_TOC(); */
    redraw(&mane);
}

void
do_pages(void)
{
    if (globals.debug & DBG_BATCH) {
	
	(void)read_events(EV_GT_IDLE);
	for (current_page = 0; current_page < total_pages; ++current_page) {
	    if (resource.keep_flag) {
		home_action = NULL;
	    }
	    else {
		home_action = home;
	    }
	    globals.warn_spec_now = resource.warn_spec;
#if PS_GS
	    for (;;) {
		redraw_page();
		(void) read_events(EV_NOWAIT);
		if (!(globals.ev.flags & (EV_NEWPAGE | EV_NEWDOC | EV_RELOAD)))
		    break;
		globals.ev.flags = EV_IDLE;
	    }
#else
	    redraw_page();
#endif
	}
	xdvi_exit(EXIT_SUCCESS);
    }
    else {
	for (;;) {	/* normal operation */
	    (void) read_events(EV_GT_IDLE);
/*  	    fprintf(stderr, "globals.ev.flags: %d; ev_newpage: %d, ev_newdoc: %d\n", globals.ev.flags, EV_NEWPAGE, EV_NEWDOC); */
	    /* NOTE: reloading must be checked first! */
	    if (globals.ev.flags & (EV_NEWPAGE | EV_NEWDOC | EV_RELOAD | EV_PS_TOGGLE)) {
		TRACE_EVENTS((stderr, "EV_NEWPAGE | ..."));
		globals.ev.flags &= ~(EV_NEWPAGE | EV_EXPOSE | EV_PS_TOGGLE);
		if (globals.ev.flags & EV_RELOAD) {
		    dviErrFlagT errflag;

		    globals.ev.flags &= ~EV_RELOAD;
		    if (load_dvi_file(True, &errflag)) {
#if PS
			ps_clear_cache();
#if PS_GS
			if (resource.gs_alpha) {
			    /* restart gs so that user has a method for fixing GS artifacts with gs_alpha
			       by using `reload' (see also GS_PIXMAP_CLEARING_HACK) */
			    ps_destroy();
			}
#endif
#endif
			statusline_print(STATUS_SHORT, "File reloaded.");
		    }
		    else {
			statusline_print(STATUS_MEDIUM, "File corrupted, not reloading.");
		    }
		}
		if (globals.ev.flags & EV_NEWDOC) {
		    dviErrFlagT errflag;
		    TRACE_EVENTS((stderr, "EV_NEWDOC!"));
/*  		    fprintf(stderr, "newdoc!\n"); */
		    TRACE_FILES((stderr, "current page: %d", current_page));
/*  		    file_history_set_page(current_page); */
		    globals.ev.flags &= ~EV_NEWDOC;
		    if (load_dvi_file(True, &errflag)) {
			statusline_append(STATUS_SHORT, "Opened ", "Opened \"%s\"", globals.dvi_name);
/* 			statusline_print(STATUS_SHORT, "Opened \"%s\"", globals.dvi_name); */
			TRACE_FILES((stderr, "Adding to history: |%s|\n", globals.dvi_name));
			if (file_history_push(globals.dvi_name)) { /* it's a new file, add to history */
			    TRACE_FILES((stderr, "New entry!"));
			    filehist_menu_add_entry(globals.dvi_name);
			}
			else { /* only need to move existing elements to new positions */
			    TRACE_FILES((stderr, "Existing entry!\n"));
			    filehist_menu_refresh();
			}
		    }
		}

		can_exposures(&mane);
		can_exposures(&magnifier);

#if PS && PS_GS && GS_PIXMAP_CLEARING_HACK
		if (had_ps_specials && !MAGNIFIER_ACTIVE) {
		    erasepage_gs();
		    had_ps_specials = False;
		}
#endif /* PS && PS_GS && GS_PIXMAP_CLEARING_HACK */

		if (globals.dvi_file.bak_fp != NULL) {
		    TRACE_EVENTS((stderr, "redraw_page()"));
		    redraw_page();
		}
		else {
		    TRACE_EVENTS((stderr, "dvi_file_changed()"));
		    (void)dvi_file_changed();
		}
	    }
	    else if (globals.ev.flags & EV_FILEHIST_GOTO_PAGE) {
		int pageno;
		globals.ev.flags &= ~EV_FILEHIST_GOTO_PAGE;
		pageno = file_history_get_page();
		goto_page(pageno, resource.keep_flag ? NULL : home, False);
		TRACE_FILES((stderr, "got page: %d", pageno));
	    }
	    else if (globals.ev.flags & EV_PAGEHIST_INSERT) {
		globals.ev.flags &= ~EV_PAGEHIST_INSERT;
		page_history_insert(current_page);
	    }
	    else if (globals.ev.flags & EV_FIND_CANCEL) {
		/* NOTE: This must be done before checking for expose() */
		globals.ev.flags &= ~EV_FIND_CANCEL;
	    }
	    else if (globals.ev.flags & EV_ANCHOR) {
		/*
		 * Similar to forward search: search for a htex anchor.
		 * This needs to come before the next case which does the redraw_page(),
		 * otherwise anchors for the current page might not be drawn at all:
		 * anchor_search() sets the info later used by htex_draw_anchormarkers(),
		 * which is invoked by redraw_page().
		 */
		
		/* switch off the link cursor */
		globals.cursor.flags &= ~CURSOR_LINK;

		if (dvi_file_changed())
		    continue;
		
		anchor_search(g_anchor_pos);
		globals.ev.flags &= ~EV_ANCHOR;
	    }
	    else if (globals.ev.flags & EV_SRC) {
		/*
		 * Source special operations are deferred to here because
		 * they call geom_scan(), which may call define_font(),
		 * which may call makefont(), which may call read_events()
		 * recursively.
		 */
		if (globals.src.fwd_string != NULL) {
		    const char *s = globals.src.fwd_string;

		    if (dvi_file_changed())
			continue;
		    
		    source_forward_search(s);
		    globals.ev.flags &= ~EV_SRC;
		    globals.src.fwd_string = NULL;

		    /* de-iconify window if needed, and raise it */
		    XMapRaised(XtDisplay(globals.widgets.top_level), XtWindow(globals.widgets.top_level));
		    raise_message_windows();
		}
		else if (source_reverse_x != -1) {
		    if (dvi_file_changed())
			continue;
		    
		    source_reverse_search(source_reverse_x, source_reverse_y, True);
		    globals.ev.flags &= ~EV_SRC;
		}
		else {
		    source_special_show(source_show_all);
		    globals.ev.flags &= ~EV_SRC;
		}
	    }
	    /* support for `-findstring' */
	    else if (globals.ev.flags & EV_FIND) {
		if (dvi_file_changed())
		    continue;

		if (resource.find_string != NULL) { /* not first call */
		    dvi_find_string(resource.find_string, False);
		    resource.find_string = NULL;
		}
		else { /* actually should never arrive here?? */
		    dvi_find_string(NULL, True);
		}
		globals.ev.flags &= ~EV_FIND;
	    }
	    else if (globals.ev.flags & EV_MAG_MOVE) {
		move_magnifier();
	    }
	    else if (globals.ev.flags & EV_EXPOSE) {
		if (magnifier.min_x < MAXDIM) {
/*  		    fprintf(stderr, "magnifier < maxdim!\n"); */
		    if (mane.min_x >= MAXDIM) {
/*  			fprintf(stderr, "mane >= maxdim!\n"); */
			globals.ev.flags &= ~EV_EXPOSE;
		    }
		    redraw(&magnifier);
		}
		else {
		    /* see comment in mag.c */
		    globals.ev.flags &= ~EV_EXPOSE;
		    if (mane.min_x < MAXDIM) {
			redraw(&mane);
		    }
		}
	    }
	    else if (globals.ev.flags & EV_CURSOR) {
		/*
		 * This code eliminates unnecessary calls to XDefineCursor,
		 * since this is a slow operation on some hardware (e.g., S3
		 * chips).
		 */
		XSync(DISP, False);
		if (!XtPending()) {
		    Cursor curr;
			
		    if (globals.cursor.flags & CURSOR_DRAG_V)
			curr = globals.cursor.drag_v;
		    else if (globals.cursor.flags & CURSOR_DRAG_H)
			curr = globals.cursor.drag_h;
		    else if (globals.cursor.flags & CURSOR_DRAG_A)
			curr = globals.cursor.drag_a;
		    else if (resource.mouse_mode == MOUSE_RULER_MODE)
			curr = globals.cursor.rule;
		    else if (resource.mouse_mode == MOUSE_TEXT_MODE && !(globals.cursor.flags & CURSOR_LINK))
			curr = globals.cursor.text;
		    else if (globals.cursor.flags & CURSOR_LINK)
			curr = globals.cursor.link;
		    else if (globals.cursor.flags & CURSOR_MAG)
			curr = globals.cursor.mag;
		    else if (globals.cursor.flags & CURSOR_CORRUPTED)
			curr = globals.cursor.corrupted;
		    else if (globals.pausing.flag)
			curr = globals.cursor.pause;
		    else
			curr = globals.cursor.ready;
		    XDefineCursor(DISP, CURSORWIN, curr);
		    globals.ev.flags &= ~EV_CURSOR;
		}
	    }
	    XFlush(DISP);
	}
    }
}

static void
Act_unpause_or_next(Widget w, XEvent *event,
		    String *params, Cardinal *num_params)
{
    if (block_this_mouse_event())
	return;
    
    if (globals.pausing.flag) {
        globals.pausing.num++;
	if (globals.pausing.num_save)
	    globals.pausing.num_save[current_page] = globals.pausing.num; 
	redraw_page();
    }
    else {
	Act_down_or_next(w, event, params, num_params);
    }
}

/*
 * timer callback for watching the DVI file.
 */
void
watch_file_cb(XtPointer client_data, XtIntervalId *id)
{
    static XtIntervalId timer = 0;
    
    UNUSED(client_data);
    UNUSED(id);

    if (resource.watch_file > 0.0) {
	unsigned long watch_time_ms;

	check_watch_dvi_file();
    
	if (timer) {
	    XtRemoveTimeOut(timer);
	    timer = (XtIntervalId)(XtIntervalId)0;
	}

	watch_time_ms = (unsigned long)(resource.watch_file * 1000);
	timer = XtAppAddTimeOut(app, watch_time_ms, watch_file_cb, (XtPointer)NULL);
    }
}

void Act_pagehistory_back(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg)) {
	arg = 1;
    }
    page_history_move(-arg);
}

void Act_pagehistory_forward(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg)) {
	arg = 1;
    }
    page_history_move(arg);
}

void Act_pagehistory_clear(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    UNUSED(w);
    UNUSED(event);
    UNUSED(params);
    UNUSED(num_params);

    if (block_this_mouse_event())
	return;
    
    page_history_clear();
}

void Act_pagehistory_delete_backward(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg)) {
	arg = 1;
    }
    page_history_delete(-arg);
}

void Act_pagehistory_delete_forward(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int arg;

    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg)) {
	arg = 1;
    }
    page_history_delete(arg);
}

#ifdef MOTIF
void Act_prefs_dialog(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    int arg;
    
    if (block_this_mouse_event())
	return;
    
    UNUSED(w);
    UNUSED(event);

    if (!get_int_arg(params, num_params, &arg)) {
	arg = -1;
    }
    
    popup_preferences_dialog(globals.widgets.top_level, arg);
}
#endif /* MOTIF */

