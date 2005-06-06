/*
 *  dialog.h -- common declarations for all dialog modules
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <dialogconfig.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NCURSES
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <ncurses/curses.h>
#endif
#else

#ifdef ultrix
#include <cursesX.h>
#else
#include <curses.h>
#endif

#endif

#if defined(LOCALE)
#include <locale.h>
#endif

/*
 * Change these if you want
 */
#ifndef USE_SHADOW
#define USE_SHADOW FALSE
#endif
#ifndef USE_COLORS
#define USE_COLORS TRUE
#endif



#define VERSION "0.4"
#define ESC 27
#define TAB 9
#define MAX_LEN 2048
#define BUF_SIZE (10*1024)
#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

#ifndef HAVE_NCURSES
#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE '-'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE '|'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE '+'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE '+'
#endif
#ifndef ACS_UARROW
#define ACS_UARROW '^'
#endif
#ifndef ACS_DARROW
#define ACS_DARROW 'v'
#endif
#endif    /* HAVE_NCURSES */


/* 
 * Attribute names
 */
#define screen_attr                   attributes[0]
#define shadow_attr                   attributes[1]
#define dialog_attr                   attributes[2]
#define title_attr                    attributes[3]
#define border_attr                   attributes[4]
#define button_active_attr            attributes[5]
#define button_inactive_attr          attributes[6]
#define button_key_active_attr        attributes[7]
#define button_key_inactive_attr      attributes[8]
#define button_label_active_attr      attributes[9]
#define button_label_inactive_attr    attributes[10]
#define inputbox_attr                 attributes[11]
#define inputbox_border_attr          attributes[12]
#define searchbox_attr                attributes[13]
#define searchbox_title_attr          attributes[14]
#define searchbox_border_attr         attributes[15]
#define position_indicator_attr       attributes[16]
#define menubox_attr                  attributes[17]
#define menubox_border_attr           attributes[18]
#define item_attr                     attributes[19]
#define item_selected_attr            attributes[20]
#define tag_attr                      attributes[21]
#define tag_selected_attr             attributes[22]
#define tag_key_attr                  attributes[23]
#define tag_key_selected_attr         attributes[24]
#define check_attr                    attributes[25]
#define check_selected_attr           attributes[26]
#define uarrow_attr                   attributes[27]
#define darrow_attr                   attributes[28]

/* number of attributes */
#define ATTRIBUTE_COUNT               29


/*
 * Global variables
 */
#ifdef __DIALOG_MAIN__

#ifdef HAVE_NCURSES

/* use colors by default? */
bool use_colors = USE_COLORS;

/* shadow dialog boxes by default?
   Note that 'use_shadow' implies 'use_colors' */
bool use_shadow = USE_SHADOW;

#endif

/* 
 * Attribute values, default is for mono display
 */
chtype attributes[] = {
  A_NORMAL,       /* screen_attr */
  A_NORMAL,       /* shadow_attr */
  A_REVERSE,      /* dialog_attr */
  A_REVERSE,      /* title_attr */
  A_REVERSE,      /* border_attr */
  A_BOLD,         /* button_active_attr */
  A_DIM,          /* button_inactive_attr */
  A_UNDERLINE,    /* button_key_active_attr */
  A_UNDERLINE,    /* button_key_inactive_attr */
  A_NORMAL,       /* button_label_active_attr */
  A_NORMAL,       /* button_label_inactive_attr */
  A_REVERSE,      /* inputbox_attr */
  A_REVERSE,      /* inputbox_border_attr */
  A_REVERSE,      /* searchbox_attr */
  A_REVERSE,      /* searchbox_title_attr */
  A_REVERSE,      /* searchbox_border_attr */
  A_REVERSE,      /* position_indicator_attr */
  A_REVERSE,      /* menubox_attr */
  A_REVERSE,      /* menubox_border_attr */
  A_REVERSE,      /* item_attr */
  A_NORMAL,       /* item_selected_attr */
  A_REVERSE,      /* tag_attr */
  A_REVERSE,      /* tag_selected_attr */
  A_NORMAL,       /* tag_key_attr */
  A_BOLD,         /* tag_key_selected_attr */
  A_REVERSE,      /* check_attr */
  A_REVERSE,      /* check_selected_attr */
  A_REVERSE,      /* uarrow_attr */
  A_REVERSE       /* darrow_attr */
};

#else

#ifdef HAVE_NCURSES
extern bool use_colors;
extern bool use_shadow;
#endif

extern chtype attributes[];

#endif    /* __DIALOG_MAIN__ */

/* Governs printing of checklist and radiobox output */
extern int separate_output;


#ifdef HAVE_NCURSES

/*
 * Function prototypes
 */
#ifdef __DIALOG_MAIN__

extern void create_rc(char *filename);
extern int parse_rc(void);

#endif    /* __DIALOG_MAIN__ */

#endif


void Usage(char *name);
void init_dialog(void);
#ifdef HAVE_NCURSES
void color_setup(void);
#endif
void attr_clear(WINDOW *win, int height, int width, chtype attr);
void print_autowrap(WINDOW *win, char *prompt, int width, int y, int x);
void print_button(WINDOW *win, char *label, int y, int x, int selected);
void draw_box(WINDOW *win, int y, int x, int height, int width, chtype box, chtype border);
#ifdef HAVE_NCURSES
void draw_shadow(WINDOW *win, int y, int x, int height, int width);
#endif

#ifndef HAVE_STRCASECMP
extern int strcasecmp();
#endif

int dialog_yesno(char *title, char *prompt, int height, int width);
int dialog_msgbox(char *title, char *prompt, int height, int width, int pause);
int dialog_textbox(char *title, char *file, int height, int width);
int dialog_menu(char *title, char *prompt, int height, int width, int menu_height, int item_no, char **items);
int dialog_checklist(char *title, char *prompt, int height, int width, int list_height, int item_no, char **items);
int dialog_radiolist(char *title, char *prompt, int height, int width, int list_height, int item_no, char **items);
int dialog_inputbox(char *title, char *prompt, int height, int width, char *init);
int dialog_guage(char *title, char *prompt, int height, int width, int percent);

