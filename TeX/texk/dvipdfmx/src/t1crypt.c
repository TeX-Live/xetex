/*  $Header: /home/cvsroot/dvipdfmx/src/t1crypt.c,v 1.4 2002/10/30 02:27:17 chofchof Exp $
    
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

unsigned short int r;
unsigned short int c1 = 52845;
unsigned short int c2 = 22719;

#include "t1crypt.h"

unsigned char t1_encrypt(unsigned char plain)
{
  unsigned char cipher;
  cipher = (plain ^ (r >> 8));
  r = (cipher+r)*c1 + c2;
  return cipher;
}

void t1_crypt_init (unsigned short int key)
{
  r = key;
}

unsigned char t1_decrypt(unsigned char cipher)
{
  unsigned char plain;
  plain = (cipher ^ (r>>8));
  r = (cipher+r)*c1 + c2;
  return plain;
}
