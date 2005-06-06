/*
 * Copyright (c) 2001-2004 the xdvik development team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAUL VOJTA OR ANY OTHER AUTHOR OF THIS SOFTWARE BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef XAW_MENU_H_
#define XAW_MENU_H_

#ifdef NEW_MENU_CREATION
#include "menu.h"
#endif /* NEW_MENU_CREATION */

#ifndef MOTIF

#ifdef NEW_MENU_CREATION
extern void xaw_set_button_state(struct button_elems *elems, Boolean on);
extern void xaw_initialize_menu_bitmaps(void);
extern Widget xaw_create_menu_widgets(Widget parent);
extern void xaw_create_menu(struct button_info *items, Widget parent, int *ret_width);
extern void xaw_set_button_state(struct button_elems *elems, Boolean on);

extern Pixmap menu_check_bitmap;
extern Pixmap menu_uncheck_bitmap;
#else
extern void create_menu_buttons(Widget menu_bar, int *ret_panel_width);
extern void toggle_menu(int val, XtActionProc proc);
#endif /* NEW_MENU_CREATION */

extern void SubMenuHandleEvent(XtAppContext app, XEvent *event);
extern void filehist_menu_add_entry(const char *filename);
extern int get_panel_width(void);

extern void toggle_tick(Boolean val, Widget w);
extern void set_button_panel_height(XtArgVal);
extern void filehist_menu_refresh(void);

extern void xaw_create_pagelist(void);
extern void toggle_scrollbars(void);
extern void toggle_buttons(void);
extern void scroll_y_panner(int y);
extern void scroll_x_panner(int x);
#endif /* not MOTIF */

#endif /* XAW_MENU_H_ */
