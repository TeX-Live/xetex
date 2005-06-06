/*  $Header: /home/cvsroot/dvipdfmx/src/subfont.h,v 1.5 2002/10/30 02:27:17 chofchof Exp $
    
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

#ifndef _SUBFONT_H_
#define _SUBFONT_H_

extern int load_sfd_record(char *sfd_name, char *sub_name);
extern unsigned int lookup_sfd_record(int subfont_id, unsigned char s);
extern void release_sfd_record(void);
extern char *sfd_sfd_name(int subfont_id);
extern char *sfd_sub_name(int subfont_id);

#endif /* _SUBFONT_H_ */
