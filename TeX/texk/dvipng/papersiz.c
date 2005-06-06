/*
 * Handle pagesizes for dvipng.  Code based on dvips
 * 5.78,.... Included in dvipng by jalar@imf.au.dk.
 */

/*
 *   This function calculates approximately (whole + num/den) * sf.
 *   No need for real extreme accuracy; one twenty thousandth of an
 *   inch should be sufficient.
 *
 *   No `sf' parameter means to use an old one; inches are assumed
 *   originally.
 *
 *   Assumptions:
 *
 *      0 <= num < den <= 20000
 *      0 <= whole
 */

#include "dvipng.h"

static int32_t scale(int32_t whole, int32_t num, int32_t den, int32_t sf)
{
  int32_t v ;

   v = whole * sf + num * (sf / den) ;
   if (v / sf != whole || v < 0 || v > 0x40000000L)
      Fatal("arithmetic overflow in parameter") ;
   sf = sf % den ;
   v += (sf * num * 2 + den) / (2 * den) ;
   return(v) ;
}
/*
 *   Convert a sequence of digits into a int32_t; return -1 if no digits.
 *   Advance the passed pointer as well.
 */
static int32_t myatol(char ** s)
{
   register char *p ;
   register int32_t result ;

   result = 0 ;
   p = *s ;
   while ('0' <= *p && *p <= '9') {
      if (result > 100000000)
         Fatal("arithmetic overflow in parameter") ;
      result = 10 * result + *p++ - '0' ;
   }
   if (p == *s) {
     Warning("expected number in %s, using 10",*s) ;
     return 10 ;
   } else {
      *s = p ;
      return(result) ;
   }
}
/*
 *   Get a dimension, allowing all the various extensions, and
 *   defaults.  Returns a value in dots (was scaled points).
 */
static int32_t scalevals[] = { 1864680L, 65536L, 786432L, 186468L,
  1L, 65782L, 70124L, 841489L, 4736286L } ;
static char *scalenames = "cmptpcmmspbpddccin" ;
int32_t myatodim(char ** s)
{
   register int32_t w, num, den, sc ;
   register char *q ;
   char *p ;
   int negative = 0, i ;

   p = *s ;
   if (**s == '-') {
      p++ ;
      negative = 1 ;
   }
   w = myatol(&p) ;
   if (w < 0) {
     Warning("number too large; 1000 used") ;
     w = 1000 ;
   }
   num = 0 ;
   den = 1 ;
   if (*p == '.') {
      p++ ;
      while ('0' <= *p && *p <= '9') {
         if (den <= 1000) {
            den *= 10 ;
            num = num * 10 + *p - '0' ;
         } else if (den == 10000) {
            den *= 2 ;
            num = num * 2 + (*p - '0') / 5 ;
         }
         p++ ;
      }
   }
   while (*p == ' ')
      p++ ;
   for (i=0, q=scalenames; ; i++, q += 2)
      if (*q == 0) {
	Warning("expected units in %s, assuming inches.",*s) ;
	sc = scalevals[8] ;
	break ;
      } else if (*p == *q && p[1] == q[1]) {
         sc = scalevals[i] ;
         p += 2 ;
         break ;
      }
   /*w = scale(w, num, den, sc) ;*/
   w = (int32_t)((int64_t) scale(w, num, den, sc)*dpi*shrinkfactor
		 /shrinkfactor/4736286L);
   *s = p ;
   return(negative?-w:w) ;
}
/*
 *   The routine where we handle the paper size special.  We need to pass in
 *   the string after the `papersize=' specification.
 */
void handlepapersize(char * p, int32_t * x, int32_t * y)
{ 
   while (*p == ' ')
      p++ ;
   *x = myatodim(&p) ;
   while (*p == ' ' || *p == ',')
      p++ ;
   *y = myatodim(&p) ;
}

#if 0
const char *lengthnames[]={ "sp", "pt", "bp", 
			    "dd", "mm", "pc", 
			    "cc", "cm", "in" };
const int32_t lengthsp[]={ 1L, 65536L, 65782L,
			   70124L, 186468L, 786432L, 
			   841489L, 1864680L, 4736286L };

int32_t myatodim2(char ** p)
{ 
  double tmp;
  int i=0;

  tmp = strtod(*p,p);
  while (**p == ' ') 
    *p++;
  while (i<9 && *p[0]==lengthnames[i][0] 
	 && *p[1]==lengthnames[i][1])
    i++;
  if (i==9) {
    Warning("Unrecognized length unit \"%.2s\", assuming inches.",*p);
    i=8;
  }
  while (**p != ',' && **p !='\0')
    *p++;
  tmp*=lengthsp[i];
  return((int32_t) tmp);
}
#endif
