/*  $Header: /home/cvsroot/dvipdfmx/src/numbers.c,v 1.6 2003/10/06 06:48:35 chofchof Exp $

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

#include "system.h"	
#include "error.h"
#include "mfileio.h"
#include "numbers.h"

UNSIGNED_BYTE get_unsigned_byte (FILE *file)
{
  int ch;
  if ((ch = fgetc (file)) < 0)
    ERROR ("File ended prematurely\n");
  return (UNSIGNED_BYTE) ch;
}

UNSIGNED_BYTE sget_unsigned_byte (char *s)
{
  return *((unsigned char *) s);
}

SIGNED_BYTE get_signed_byte (FILE *file)
{
  int byte;
  byte = get_unsigned_byte(file);
  if (byte >= 0x80) 
    byte -= 0x100;
  return (SIGNED_BYTE) byte;
}

UNSIGNED_PAIR get_unsigned_pair (FILE *file)
{
  int i;
  UNSIGNED_BYTE byte;
  UNSIGNED_PAIR pair = 0;
  for (i=0; i<2; i++) {
    byte = get_unsigned_byte(file);
    pair = pair*0x100u + byte;
  }
  return pair;
}

UNSIGNED_PAIR sget_unsigned_pair (unsigned char *s)
{
  int i;
  UNSIGNED_BYTE byte;
  UNSIGNED_PAIR pair = 0;
  for (i=0; i<2; i++) {
    byte = *(s++);
    pair = pair*0x100u + byte;
  }
  return pair;
}

SIGNED_PAIR get_signed_pair (FILE *file)
{
  int i;
  long pair = 0;
  for (i=0; i<2; i++) {
    pair = pair*0x100 + get_unsigned_byte(file);
  }
  if (pair >= 0x8000) {
    pair -= 0x10000l;
  }
  return (SIGNED_PAIR) pair;
}


UNSIGNED_TRIPLE get_unsigned_triple(FILE *file)
{
  int i;
  long triple = 0;
  for (i=0; i<3; i++) {
    triple = triple*0x100u + get_unsigned_byte(file);
  }
  return (UNSIGNED_TRIPLE) triple;
}

SIGNED_TRIPLE get_signed_triple(FILE *file)
{
  int i;
  long triple = 0;
  for (i=0; i<3; i++) {
    triple = triple*0x100 + get_unsigned_byte(file);
  }
  if (triple >= 0x800000l) 
    triple -= 0x1000000l;
  return (SIGNED_TRIPLE) triple;
}

SIGNED_QUAD get_signed_quad(FILE *file)
{
  int byte, i;
  long quad = 0;

  /* Check sign on first byte before reading others */
  byte = get_unsigned_byte(file);
  quad = byte;
  if (quad >= 0x80) 
    quad = byte - 0x100;
  for (i=0; i<3; i++) {
    quad = quad*0x100 + get_unsigned_byte(file);
  }
  return (SIGNED_QUAD) quad;
}

UNSIGNED_QUAD get_unsigned_quad(FILE *file)
{
  int i;
  unsigned long quad = 0;
  for (i=0; i<4; i++) {
    quad = quad*0x100u + get_unsigned_byte(file);
  }
  return (UNSIGNED_QUAD) quad;
}

SIGNED_QUAD sqxfw (SIGNED_QUAD sq, fixword fw)
{
  int sign = 1;
  unsigned long a, b, c, d, ad, bd, bc, ac;
  unsigned long e, f, g, h, i, j, k;
  unsigned long result;
  /* Make positive. */
  if (sq < 0) {
    sign = -sign;
    sq = -sq;
  }
  if (fw < 0) {
    sign = -sign;
    fw = -fw;
  }
  a = ((unsigned long) sq) >> 16u;
  b = ((unsigned long) sq) & 0xffffu;
  c = ((unsigned long) fw) >> 16u;
  d = ((unsigned long) fw) & 0xffffu;
  ad = a*d; bd = b*d; bc = b*c; ac = a*c;
  e = bd >> 16u;
  f = ad >> 16u;
  g = ad & 0xffffu;
  h = bc >> 16u;
  i = bc & 0xffffu;
  j = ac >> 16u;
  k = ac & 0xffffu;
  result = (e+g+i + (1<<3)) >> 4u;  /* 1<<3 is for rounding */
  result += (f+h+k) << 12u;
  result += j << 28u;
  return (sign > 0) ? result : result * -1L;
}

SIGNED_QUAD axboverc (SIGNED_QUAD n1, SIGNED_QUAD n2, SIGNED_QUAD div)
{
  int sign = 1;
  unsigned long a, b, c, d, ad, bd, bc, ac, e, f, g, h, i, j, o;
  unsigned long high, low;
  SIGNED_QUAD result = 0;
  /*  Make positive. */
  if (n1 < 0) {
    sign = -sign;
    n1 = -n1;
  }
  if (n2 < 0) {
    sign = -sign;
    n2 = -n2;
  }
  if (div < 0) {
    sign = -sign;
    div = -div;
  }
  a = ((unsigned long) n1) >> 16u;
  b = ((unsigned long) n1) & 0xffffu;
  c = ((unsigned long) n2) >> 16u;
  d = ((unsigned long) n2) & 0xffffu;
  ad = a*d; bd = b*d; bc = b*c; ac = a*c;
  e = bd >> 16u; f = bd & 0xffffu;
  g = ad >> 16u; h = ad & 0xffffu;
  i = bc >> 16u; j = bc & 0xffffu;
  o = e+h+j;
  high = g+i+(o>>16u)+ac; o &= 0xffffu;
  low = (o << 16) + f;
  if (high >= div)
    ERROR ("Overflow in axboc");
  {
    int i;
    for (i=0; i<32; i++) {
      high *= 2;
      result *= 2;
      if (low >= 0x80000000) {
	low -= 0x80000000;
	high += 1;
      }
      low *= 2;
      if (high > div) {
	high -= div;
	result += 1;
      }
    }
  }
  high *= 2;
  if (high >= div)
    result += 1;
  return (sign>0)?result:-result;
}

long one_bp;		/* scaled value corresponds to 1bp */
long one_hundred_bp;	/* scaled value corresponds to 100bp */
long min_bp_val, centi_min_bp_val;
long scaled_out;	/* amount of |scaled| that was taken out in |divide_scaled| */
int fixed_decimal_digits;

static long ten_pow[10];		/* $10^0..10^9$ */

/* Create a private_itoa to be used internally in the
   hopes that a good optimizing compiler will use it inline */
static int private_itoa (char *s, long n, int mindigits)
{
   int j, nwhole;
   char *p = s;
   if (n<0) {
      *(p++) = '-';
      n = -n;
   }
   /* Generate at least one digit in reverse order */
   nwhole = 0;
   do {
      p[nwhole++] = n%10 + '0';
      n /= 10;
   } while (n != 0 || nwhole < mindigits);
   /* Reverse the digits */
   for (j=0; j<nwhole/2; j++) {
      char tmp = p[j];
      p[j] = p[nwhole-j-1];
      p[nwhole-j-1]=tmp;
   }
   p += nwhole;
   *p = 0;
   return (p-s);
}

static long divide_scaled (long s, long m, int dd)
{
  long q, r;
  int sign, i;

  sign = 1;
  if (s < 0) {
    sign = -sign;
    s = -s;
  }
  if (m < 0) {
    sign = -sign;
    m = -m;
  }
  if (m == 0) {
    ERROR ("[arithmetic] divided by zero");
  } else if (m >= (0x7fffffff / 10)) {
    ERROR ("[arithmetic] number too big");
  }
  q = s / m;
  r = s % m;
  for (i = 1; i <= dd; i++) {
    q = 10 * q + (10 * r) / m;
    r = (10 * r) % m;
  }
  if (2 * r >= m) {
    q++;
    r = r - m;
  }
  scaled_out = sign * (s - (r / ten_pow[dd]));
  return(sign * q);
}

/* print $m/10^d$ as real */
static int pdf_print_real (char *buf, long m, int d)
{
  long n;
  char *p = buf;

  if (m < 0) {
    *p++ = '-';
    m = -m;
  }
  n = ten_pow[d];
  p += private_itoa(p, m / n, 0);
  m = m % n;
  if (m > 0) {
    *p++ = '.';
    n = n / 10;
    while (m < n) {
      *p++ = '0';
      n = n / 10;
    }
    while (m % 10 == 0)
      m = m / 10;
    p += private_itoa(p, m, 0);
  }
  return(p - buf);
}

/* print scaled as |bp| */
int pdf_print_bp (char *buf, long s)
{
  return pdf_print_real(buf, divide_scaled(s, one_hundred_bp, fixed_decimal_digits + 2), fixed_decimal_digits);
}

int pdf_print_pt (char *buf, double s)
{
  return pdf_print_real(buf, (long)(s * ten_pow[fixed_decimal_digits]), fixed_decimal_digits);
}

int pdf_print_int (char *buf, long s)
{
  return private_itoa(buf, s, 0);
}

void init_pdf_numbers (int pdfdecimaldigits)
{
  int i;

  one_bp = 65782;
  one_hundred_bp = 6578176;

  ten_pow[0] = 1;
  for (i = 1; i < 10; i++)
    ten_pow[i] = 10 * ten_pow[i - 1];

  min_bp_val = divide_scaled(one_hundred_bp, ten_pow[fixed_decimal_digits + 2], 0);
  centi_min_bp_val = min_bp_val / 100;

  if (pdfdecimaldigits < 0)
    fixed_decimal_digits = 0;
  else if (pdfdecimaldigits > 4)
    fixed_decimal_digits = 4;
  else 
    fixed_decimal_digits = pdfdecimaldigits;
}
