/*  $Header: /home/cvsroot/dvipdfmx/src/system.h,v 1.6 2004/01/11 02:51:33 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef MIKTEX
#include "miktex.h"
#include "gnu-miktex.h"
#include "web2c-miktex.h"
#else
#include <kpathsea/c-auto.h>
#include <kpathsea/kpathsea.h>
#endif

#ifdef WIN32
#  undef ERROR
#  undef NO_ERROR
#  undef RGB
#  undef CMYK
#  undef SETLINECAP
#  undef SETLINEJOIN
#  undef SETMITERLIMIT
#  pragma warning(disable : 4101 4018)
#else
#  ifndef __cdecl
#  define __cdecl
#  endif
#  define CDECL
#endif /* WIN32 */

#endif /* _SYSTEM_H_ */
