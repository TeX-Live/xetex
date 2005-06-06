/*
 * Copyright (c) 2001 Marcin Dalecki
 * Copyright (c) 2002-2004 the xdvik development team
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

#ifndef XM_MENU_H_
#define XM_MENU_H_

#include "menu.h"

#ifdef MOTIF

#define Xdvi_FILEHIST_MENU	"filehist_menu"

extern Widget xm_create_menu(Widget parent, char *title, char mnemonic,
			     struct button_info *item);

extern Boolean pulldown_menu_active(unsigned long event_num);
extern void toggle_menubar(void);
extern void popdown_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void filehist_menu_add_entry(const char *filename);
extern void filehist_menu_refresh(void);

#endif

#endif /* XM_MENU_H_ */
