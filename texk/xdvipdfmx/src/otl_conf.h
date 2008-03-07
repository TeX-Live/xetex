/*  $Header: /home/cvsroot/dvipdfmx/src/otl_conf.h,v 1.1 2004/08/22 10:26:24 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _OTL_CONF_H_
#define _OTL_CONF_H_

extern void     otl_conf_set_verbose (void);

extern pdf_obj *otl_find_conf  (const char *conf_name);
extern void     otl_init_conf  (void);
extern void     otl_close_conf (void);

extern pdf_obj *otl_conf_get_class (pdf_obj *conf, const char *ident);

extern char *otl_conf_get_script   (pdf_obj *conf);
extern char *otl_conf_get_language (pdf_obj *conf);
extern pdf_obj *otl_conf_get_rule  (pdf_obj *conf);
extern pdf_obj *otl_conf_find_opt  (pdf_obj *conf, const char *opt_tag);

#endif /* _OTL_CONF_H_ */
