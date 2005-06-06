/* $XConsortium: objects.c,v 1.5 92/03/20 15:56:06 eswu Exp $ */
/* Copyright International Business Machines, Corp. 1991
 * All Rights Reserved
 * Copyright Lexmark International, Inc. 1991
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM or Lexmark not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM AND LEXMARK PROVIDE THIS SOFTWARE "AS IS", WITHOUT ANY WARRANTIES OF
 * ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE
 * QUALITY AND PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF THE
 * SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT IBM OR LEXMARK) ASSUMES THE
 * ENTIRE COST OF ALL SERVICING, REPAIR AND CORRECTION.  IN NO EVENT SHALL
 * IBM OR LEXMARK BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */
 /* OBJECTS  CWEB         V0025 ********                             */
/*
:h1.OBJECTS Module - TYPE1IMAGER Objects Common Routines
 
This module defines and implements the C structures that represent
objects in the TYPE1IMAGER.  All common routines for manipulating these
objects are defined in this module.  Specific routines for
specific objects are defined in the modules that deal with that
object type.
 
 
&author. Jeffrey B. Lotspiech (lotspiech@almaden.ibm.com)
 
 
:h3.Include Files
 
The included files are:
*/
#define   GLOBALS  1         /* see :hdref refid=debugvar.                   */
/*
The following two includes are C standards; we include them because we
use 'toupper' and the 'str'-type functions in this module.  Potentially
these may be defined as macros; if these ".h" files do not exist on your
system it is a pretty safe bet that these are external entry points and
you do do not need to include these header files.
*/


#include  "types.h"
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <setjmp.h>

/*
override incorrect system functions; for example you might define
a macro for "strcpy" that diverts it to "my_strcpy".
*/
 
                               /* moved these includes from above the    */
                               /*   was included first (it contains com- */
                               /*   piler defines).  dsr 081291          */
#include  "objects.h"
#include  "spaces.h"
#include  "paths.h"
#include  "regions.h"
#include  "fonts.h"
#include  "pictures.h"
#include  "strokes.h"
#include  "cluts.h"
static char *TypeFmt();
static int ObjectPostMortem();

/*
:h3.The "pointer" Macro - Define a Generic Pointer
 
Sadly, many compilers will give a warning message when a pointer to
one structure is assigned to a pointer to another.  We've even seen
some that give severe errors (when the wrong pointer type is used as
an initializer or returned from a function).  TYPE1IMAGER has routines
like Dup and Allocate that are perfectly willing to duplicate or
allocate any of a number of different types of structures.  How to
declare them in a truely portable way?
 
Well, there is no single good answer that I've found.  You can always
beg the question and "cast" everything.  I find this distracting and the
resulting code ugly.  On the other hand, we have found at least one
compiler that will accept "void *" as a generic pointer that can
assigned to any other pointer type without error or warning (apparently
this is also the ANSI standard).  So, we define "void *" to be a generic
pointer.  (You might have to change this for your compiler; the "ifndef"
allows the change to be made on the command line if you want.)
:i1/portability assumptions/
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.Functions Provided to the TYPE1IMAGER User
 
This module provides the following TYPE1IMAGER entry points:
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
Note that entry points that are intended for use external to TYPE1IMAGER
begin with the characters :q/xi/.  Macros are used to make the names
more mnemonic.
*/
 
/*
:h3.Functions Provided to Other Modules
 
This module provides the following functions for other modules:
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
Note that entry points that intended for use within TYPE1IMAGER, but
which must be global because they are used across module boundaries,
begin with the characters :q/I_/.  Macros are used to make the names
more mnemonic.
 
Entry points totally within a module use mnemonic names and are
declared :hp2/static/.  One of the compilers I used had a bug when
static functions were passed as addresses.  Thus, some functions
which are logically "static" are not so declared.
 
Note also the trick of declaring routines, like Consume(), with a
variable number of arguments.  To avoid the restrictions on variable
numbers of arguments in the macro processor, we just replace the
text 'Consume' with 'I_Consume'.
*/
/*
:h3.Macros Provided to Other Modules
 
This is the module where we define all the useful constants like
TRUE, FALSE, and NULL, and simple expressions like TYPE1_MIN(),
TYPE1_MAX(), and TYPE1_ABS().
We might as well get to it right here:
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
Notice that upper case is used for constant values and macro
definitions.  I generally follow that convention.
 
Many more global macros are defined later in this module.
*/
/*
:h2.Basic TYPE1IMAGER Object Structure
 
All TYPE1IMAGER objects which are available to the user have a common
header.  This header is defined below:
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
/*
The following define is an attempt to centralize the definition of the
common xobject data shared by structures that are derived from the
generic xobject structure. For example, the structure font, defined in
fonts.shr :
&code.
    struct font {
           char type;
           char flag;
           int references;
           ... other data types & structs ...
           }
&ecode.
would now be defined as:
&code.
    struct font {
           XOBJ_COMMON
           ... other data types & structs ...
           }
&ecode.
Thus we have a better-structured inheritance mechanism. 3-26-91 PNM
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.Object Type Definitions
 
These constants define the values which go in the 'type' field of
an TYPE1IMAGER object structure:
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.Flag Byte Definitions
 
Many programmers define flag bits as a mask (for example, 0x04), and
test, set, and reset them as follows:
 
&code.
        if ((flag & PERMANENT) != 0)
 
        flag |= PERMANENT;
        flag &= &inv.PERMANENT;
:exmp.
 
I favor a style where the 'if' statement can ask a question:
 
&code.
        if (ISPERMANENT(flag))
 
        flag |= ISPERMANENT(ON);
        flag &= &inv.ISPERMANENT(ON);
 
:exmp.
This said, we now define two bit settings of the flag byte of the
object.  "ISPERMANENT" will be set by the user, when he calls
Permanent().  "ISIMMORTAL" will be used for compiled-in objects
that we don't want the user to ever destroy.
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
Flag bit definitions that apply to all objects are assigned
starting with the least significant (0x01) bit.  Flag bit definitions
specific to a certain object type are assigned starting with the
most significant (0x80) bit.  We hope they never meet.
*/
/*
:h3 id=preserve.PRESERVE() Macro
 
Occasionally an TYPE1IMAGER operator is implemented by calling other
TYPE1IMAGER operators.  For example, Arc2() calls Conic().  When we
call more than one operator as a subroutine, we have to be careful
of temporary objects.  A temporary object will be consumed by the
subroutine operator and then is no longer available for the caller.
This can be prevented simply by bumping a temporary object's reference
count.
*/
/*SHARED LINE(S) ORIGINATED HERE*/
 
/*
:h3.RefRoll() Macro to Detect References Count Rollover
 
The following macro is designed to check for reference count rollover.
A return value of TRUE means rollover has not occurred; a return value
of FALSE means we cannot increment the reference count.  Note also that
those functions that use this macro must decrement the reference count
afterwards.  3-26-91 PNM
*/
 
#define RefRoll(obj)  (++(obj)->references > 0)
 
/*
:h2.TYPE1IMAGER Object Functions
 
:h3.LONGCOPY() - Macro to Copy "long" Aligned Data
 
Copying arbitrary bytes in C is a bit of a problem.  "strcpy" can't be
used, because 0 bytes are special-cased.  Most environments have a
routine "memcopy" or "bcopy" or "bytecopy" that copies memory containing
zero bytes.  Sadly, there is no standard on the name of such a routine,
which makes it impossible to write truely portable code to use it.
 
It turns out that TYPE1IMAGER, when it wants to copy data, frequently
knows that both the source and destination are aligned on "long"
boundaries.  This allows us to copy by using "long *" pointers.  This
is usually very efficient on almost all processors.  Frequently, it
is more efficient than using general-purpose assembly language routines.
So, we define a macro to do this in a portable way.  "dest" and "source"
must be long-aligned, and "bytes" must be a multiple of "sizeof(long)":
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.Allocate() - Allocating a Memory Block
 
Allocate returns a pointer to memory object that is a copy of
the template passed (if any).  In addition, extra bytes may be
allocated contiguously with the object.  (This may be useful for
variable size objects such as edge lists.  See :hdref refid=regions..)
 
Allocate() always returns a non-immortal object, even if the template is
immortal.  Therefore a non-NULL template must have a "flag" byte.
 
If the template is NULL, then 'size' bytes are cleared to all NULLs.
 
If the template is non-NULL, a new object is allocated in memory.
It therefore seems logical that its reference count field should be
set to 1. So, a nun-NULL template must also have a "references" field.
PNM 3-26-91
*/


/* to pacify gcc we put the externals here ... */
extern struct XYspace *USER;
extern jmp_buf stck_state;


struct xobject *t1_Allocate(size, template, extra)  /* non-ANSI; type checking was too strict */
       register int size;    /* number of bytes to allocate & initialize     */
       register struct xobject *template;  /* example structure to allocate  */
       register int extra;   /* any extra uninitialized bytes needed contiguously */
{
 
       register struct xobject *r;
 
       /*
       * round up 'size' and 'extra' to be an integer number of 'long's:
       */
       size = (size + sizeof(LONG) - 1) & -sizeof(LONG);
       extra = (extra + sizeof(LONG) - 1) & -sizeof(LONG);
       if (size + extra <= 0)
               abort("Non-positive allocate?", 15);
       r = (struct xobject *) malloc(size + extra);
 
       while (r == NULL) {
               if (!GimeSpace()) {
                       IfTrace1(TRUE, "malloc attempted %d bytes.\n",
                                           size + extra);
                       abort("We have REALLY run out of memory", 16);
               }
               r = (struct xobject *) malloc(size + extra);
       }
 
       /*
       * copy the template into the new memory:
       */
       if (template != NULL) {
       /* Added references count decrement if template is not permanent.
          This is for the case where Allocate is called by a Dupxxxx
          function, which was in turn called by Unique(). (PNM)        */
               if (!ISPERMANENT(template->flag))
                   --template->references;
               LONGCOPY(r, template, size);
               r->flag &= ~(ISPERMANENT(ON) | ISIMMORTAL(ON));
       /* added reference field 3-2-6-91 PNM */
               r->references = 1;
       }
       else {
               register char **p1;
 
               for (p1=(char **)r; size > 0; size -= sizeof(char *))
                       *p1++ = NULL;
       }
 
       if (MemoryDebug > 1) {
               register int *L;
               L = (int *) r;
               IfTrace4(TRUE, "Allocating at %p: %x %x %x\n",
                                           L, L[-1], L[0], L[1]);
       }
       return(r);
}
 
/*
:h3.Free() - Frees an Allocated Object
 
This routine makes a sanity check to make sure the "type" field of the
standard object structure has not been cleared.  If the object is
not a standard structure, then the macro "NonObjectFree" is available
that does not perform this check.
 
In either case, the object must not be the NULL pointer.  This preserves
portability, as the C system Xfree() will not always accept NULL.
*/
 
void Free(obj)              /* non-ANSI to avoid overly strict type checking */
       register struct xobject *obj;  /* structure to free                   */
{
       if (obj->type == INVALIDTYPE)
               abort("Free of already freed object?", 17);
       obj->type = INVALIDTYPE;
 
       if (MemoryDebug > 1) {
               register int *L;
               L = (int *) obj;
               IfTrace4(TRUE,"Freeing at %p: %x %x %x\n", L, L[-1], L[0], L[1]);
       }
 
       free(obj);
}
 
/*
:h3.Permanent() - Makes an Object Permanent
 
Real simple--just set a flag.  Every routine that consumes its objects
(which is almost every user entry) must check this flag, and not consume
the object if it is set.
 
If a temporary object is made permanent, and there is more than one
reference to it, we must first Copy() it, then set the ISPERMANENT
flag. Note also that the reference count must be incremented when an
object is changed from temporary to permanent (see the ISUNIQUE macro).
 
Note that the purpose of this function is to convert an object into a
permanent object:
  If it was permanent to begin with, we do nothing;
  If it was temporary and unique, we set the PERMANENT flag and increment
the reference count;
  If it was temporary and nonunique, we must make a unique Copy(), set
the PERMANENT flag, and set the reference count to 2. We must also
decrement the original object's reference count, because what we have
done is to change one of the old temporary handles to a permanent one.
3-26-91 PNM
*/
 
struct xobject *t1_Permanent(obj) /* non-ANSI to avoid overly strict type checking */
       register struct xobject *obj;  /* object to be made permanent         */
{
       IfTrace1((MustTraceCalls),"Permanent(%p)\n", obj);
 
       if ( (obj != NULL) && ( !(ISPERMANENT(obj->flag)) ) )
       {
       /* there is a non-NULL, temporary object to be made permanent.
          If there are multiple references to this object, first get
          a new COPY().
          Note also that we have to decrement the reference count if
          we do a Copy() here, because we are consuming the temporary
          argument passed, and returning a unique, permanent one.
       */
           if ( obj->references > 1)
           {
               obj = Copy(obj);
           }
           /* now set the permanent flag, and increment the reference
              count, since a temporary object has now become permanent. */
           obj->references++;
           obj->flag |= ISPERMANENT(ON);
       }
       return(obj);
}
 
/*
:h3.Temporary() - Undoes the Effect of "Permanent()"
 
This simply resets the "ISPERMANENT" flag.
 
If a permanent object is made temporary, and there is more than one reference
to it, we must first Copy() it, then reset the ISPERMANENT flag. However,
if the permanent object has obly one reference, we need only decrement the
reference count ( and reset the flag).
 
Note that this function, in the case of a PERMANENT argument, basically
converts the PERMANENT handle to a TEMPORARY one. Thus, in the case of
a nonunique, permanent argument passed, we not only make a Copy(),
we also decrement the reference count, to reflect the fact that we have
lost a permanent handle and gained a temporary one.
PNM 3-2-6-91
*/
 
struct xobject *xiTemporary(obj) /* non-ANSI to avoid overly strict type checking */
       register struct xobject *obj;  /* object to be made permanent         */
{
       IfTrace1((MustTraceCalls),"Temporary(%p)\n", obj);
 
       if (obj != NULL) {
               /* if it's already temporary, there's nothing to do. */
               if ISPERMANENT(obj->flag)
               {
               /* if there are multiple references to this object, get a
                  Copy we can safely alter. Recall that the reference count
                  is incremented for permanent objects.
                  Recall further that Copy returns an object with the
                  same flag state and a reference count of 2 (for PERMANENT
                  objects).
                  Thus, regardless of whether or not we need to copy a
                  permanent object, we still decrement its reference
                  count and reset the flag.
               */
                   if (obj->references != 2 || ISIMMORTAL(obj->flag))
                   {
               /* not unique; consume handle, get a temporary Copy! */
                       obj = Copy(obj);
                   }
               /* else decrement the reference count (since it's going from
                  permanent to temporary) and clear the flag. */
                   else {
                       obj->references--;
                       obj->flag &= ~ISPERMANENT(ON);
                   }
               }
       }
       return(obj);
}
 
/*
:h3.Dup() - Duplicate an Object
 
Dup will increment the reference count of an object, only making a
Copy() if needed.
Note that Dup() retains the state of the permanent flag.
3-26-91 PNM
*/
 
 
struct xobject *t1_Dup(obj)   /* non-ANSI avoids overly strict type checking  */
       register struct xobject *obj;  /* object to be duplicated             */
{
       register char oldflag;   /* copy of original object's flag byte */
 
       IfTrace1((MustTraceCalls),"Dup(%p)\n", obj);
 
       if (obj == NULL)
               return(NULL);
       /* An immortal object must be Copy'ed, so that we get a mortal
          copy of it, since we try not to destroy immortal objects. */
       if (ISIMMORTAL(obj->flag))
           return(Copy(obj));
 
       /* if incrementing the reference count doesn't cause the count
          to wrap, simply return the object with the count bumped. Note
          that the RefRoll macro increments the count to perform the
          rollover check, so we must decrement the count. */
       if (RefRoll(obj))
           return(obj);
 
       /* that didn't work out, so put the count back and call Copy(). */
       --obj->references;
       oldflag = obj->flag;
       obj = Copy(obj);
       if (ISPERMANENT(oldflag))
               obj = Permanent(obj);
       return(obj);
}
 
/*
:h3.Copy() - Make a New Copy of an Object
 
This is the generic Copy() where the object type is unknown.  There
are specific Copyxxx functions for known object types.
 
Copy will create a NEW temporary object, and WILL NOT simply bump the
reference count.
 
Sometimes duplicating an object is just as simple as Allocating with it
as a template.  But other objects are complicated linked lists.  So, we
let each module provide us a routine (or macro) that duplicates the
objects it knows about.
*/
 
struct xobject *t1_Copy(obj)
       register struct xobject *obj;  /* object to be  Copy'ed              */
{
       if (obj == NULL)
               return(NULL);
 
       if (ISPATHTYPE(obj->type))
               obj = (struct xobject *) CopyPath(obj);
       else
               switch (obj->type) {
                   case SPACETYPE:
                       obj = (struct xobject *) CopySpace(obj); break;
                   case FONTTYPE:
                       obj = (struct xobject *) CopyFont(obj); break;
                   case REGIONTYPE:
                       obj = (struct xobject *) CopyRegion(obj); break;
                   case PICTURETYPE:
                       obj = (struct xobject *) CopyPicture(obj); break;
                   case LINESTYLETYPE:
                       obj = (struct xobject *) CopyLineStyle(obj); break;
                   case STROKEPATHTYPE:
                       obj = (struct xobject *) CopyStrokePath(obj); break;
                   case CLUTTYPE:
                       obj = (struct xobject *) CopyCLUT(obj); break;
                   default:
                       return(ArgErr("Copy: invalid object", obj, NULL));
               }
 
        return(obj);
}
 
/*
:h3.Destroy() - Destroys an Object
 
This can get complicated.  Just like with Copy(), we let the experts
handle it.
*/
struct xobject *Destroy(obj) /* non-ANSI avoids overly strict type checking  */
       register struct xobject *obj;  /* object to be destroyed              */
{
       IfTrace1((MustTraceCalls),"Destroy(%p)\n", obj);
 
       if (obj == NULL)
               return(NULL);
       if (ISIMMORTAL(obj->flag)) {
               IfTrace1(TRUE,"Destroy of immortal object %p ignored\n", obj);
               return(NULL);
       }
       if (ISPATHTYPE(obj->type))
               KillPath(obj);
       else {
               switch (obj->type) {
                   case REGIONTYPE:
                       KillRegion(obj);
                       break;
                   case SPACETYPE:
                       KillSpace(obj);
                       break;
                   case LINESTYLETYPE:
                       KillLineStyle(obj);
                       break;
                   case FONTTYPE:
                       KillFont(obj);
                       break;
                   case PICTURETYPE:
		       /* KillPicture macro removed from sources (RMz, 2001-04-01)
                       KillPicture(obj);
		       */
                       break;
	           case STROKEPATHTYPE:
                       KillStrokePath(obj);
                       break;
                   case CLUTTYPE:
                       KillCLUT(obj);
                       break;
                   default:
                       return(ArgErr("Destroy: invalid object", obj, NULL));
               }
       }
       return(NULL);
}
/*
:h2.Generally Useful Macros
 
:h3.FOLLOWING() - Macro to Point to the Data Following a Structure
 
There are several places in TYPE1IMAGER where we will allocate variable
data that belongs to a structure immediately after that structure.
This is a performance technique, because it reduces the number of
trips we have to take through Xalloc() and Xfree().  It turns out C has
a very convenient way to point past a structure--if 'p' is a pointer
to a structure, 'p+1' is a pointer to the data after it.  This
behavior of C is somewhat startling and somewhat hard to follow, if
you are not used to it, so we define a macro to point to the data
following a structure:
*/
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.TYPECHECK() - Verify the Type of an Argument
 
This macro tests the type of an argument.  If the test fails, it consumes
any other arguments as necessary and causes the imbedding routine to
return the value 'whenBAD'.
 
Note that the consumeables list should be an argument list itself, for
example (0) or (2,A,B).  See :hdref refid=consume. below.
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.ARGCHECK() - Perform an Arbitrary Check on an Argument
 
This macro is a generalization of TYPECHECK to take an arbitrary
predicate.  If the error occurs (i.e., the predicate is true), the
arbitrary message 'msg' is returned.
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.TYPENULLCHECK() - Extension of TYPECHECK() for NULL arguments
 
Many routines allow NULLs to be passed as arguments.  'whenBAD' will
be returned in this case, too.
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
/*
:h3.MAKECONSUME() - Create a "Consume"-type Macro
 
Consuming an object means destroying it if it is not permanent.  This
logic is so common to all the routines, that it is immortalized in this
macro.  For example, ConsumePath(p) can be simply defined as
MAKECONSUME(p,KillPath(p)).  In effect, this macro operates on a
meta-level.
:i1/consuming objects/
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
 
/*
:h3.MAKEUNIQUE() - Create a "Unique"-type Macro
 
Many routines are written to modify their arguments in place.  Thus,
they want to insure that they duplicate an object if it is permanent.
This is called making an object "unique".  For example, UniquePath(p)
can be simply defined as MAKEUNIQUE(p,DupPath(p)).
:i1/unique objects/
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
 
/*
An object is unique (and directly alterable) if there is only one
reference to it, and it is not permanent (in which case we increment
the reference count, so we don't have to check the permanent bit).
3-26-91 PNM
 
Note the rules for making a unique object:
&drawing.
   IF  (obj->references = 1)    return(obj);
   ELSE (references > 1)
       IF (ISPERMANENT(obj->flag))    return(Dupxxx(obj));
       ELSE (nonunique, temporary object!)
           obj->references--; return(Dupxxx(obj));
&edrawing.
If we must make a Copy of a nonunique, temporary object, we decrement
reference count of the original object!
*/
 
/*
:h3.Unique() - Make a Unique Object
 
Here is a generic 'Unique' function if the object type is not known.
Why didn't we build it with the MAKEUNIQUE macro, you ask?  Well, we
used to, but there is at least one damn compiler in the world that
raises errors if the types of an "(a) ? b : c" expression do not match.
Also, when we changed Dup() to retain the permanent/temporary flag, we
wanted to make sure "Unique" always returned a temporary object.
 
Note that we cannot use Dup() to create a copy of the object in question,
because Dup() may simply bump the reference count, and not return a
unique copy to us. That is why we use t1_Copy().
 
The purpose of this function is to make sure we have a copy of an object
that we can safely alter:
:ol.
:li.If we have a unique, temporary object, we simply return the argument.
:li.If we have a nonunique, temporary object, we have to make a new copy
of it, and decrement the reference count of the original object, to reflect
the fact that we traded temporary handles.
:li.If we have a permanent object, we make a temporary copy of it, but
we do not decrement the reference count of the original permanent object,
because permanent objects, by definition, are persistent. 3-2-6-91 PNM
:eol.
*/
 
struct xobject *t1_Unique(obj)
       struct xobject *obj;
{
    /* if the original object is not already unique, make a unique
       copy...Note also that if the object was not permanent, we must
       consume the old handle! 3-26-91 PNM
       NOTE : consumption of the old handle moved to Allocate. 4-18-91 */
    if (!obj || obj->references == 1)
        return(obj);
 
    obj = Copy(obj);
    /* and make sure we return a temporary object ! */
    if (ISPERMANENT(obj->flag))
    {
        obj->flag &= ~ISPERMANENT(ON);
        obj->references--;
    }
    return(obj);
}
 
 
/*
:h2.Initialization, Error, and Debug Routines
 
:h3 id=debugvar.Declarations for Debug Purposes
 
We declare all the debug flags here.  Some link editors make the not
unreasonable restriction that only one module may declare and
initialize global variables; all the rest must declare the variable
'extern'.  This is logical, but is somewhat awkward to implement with
C include files.  We solve the problem by temporarily making the name
'extern' a null name if GLOBALS is defined.  (GLOBALS is only defined
in this OBJECTS module.)  Since 'externs' can't be initialized, we
have to handle that with #defines too.
:i1/GLOBALS (&#define.)/
*/
 
/*SHARED LINE(S) ORIGINATED HERE*/
static char *ErrorMessage = NULL;
 
/*
:h3.Pragmatics() - Set/Reset Debug Flags
 
We provide a controlled way for the TYPE1IMAGER user to set and reset
our debugging and tracing:
*/
void Pragmatics(username, value)
       char *username;       /* name of the flag                             */
       int value;            /* value to set it to                           */
{
       register char *p;     /* temporary loop variable                      */
#define    NAMESIZE   40
       char name[NAMESIZE];  /* buffer to store my copy of 'username'        */
 
       if (strlen(username) >= NAMESIZE)
               abort("Pragmatics name too large", 18);
       strcpy(name, username);
       for (p = name; *p != '\0'; p++)
               *p = toupper(*p);
 
       if (!strcmp(name, "ALL"))
               MustTraceCalls = InternalTrace = /* MustCrash = */
                    LineIOTrace = value;
 
       else if (!strcmp(name, "LINEIOTRACE"))
               LineIOTrace = value;
 
       else if (!strcmp(name, "TRACECALLS"))
               MustTraceCalls = value;
 
       else if (!strcmp(name, "CHECKARGS"))
               MustCheckArgs = value;
 
       else if (!strcmp(name, "PROCESSHINTS"))
               ProcessHints = value;
 
       else if (!strcmp(name, "SAVEFONTPATHS"))
               SaveFontPaths = value;
 
       else if (!strcmp(name, "CRASTERCOMPRESSIONTYPE"))
               CRASTERCompressionType = value;
 
       else if (!strcmp(name, "CRASHONUSERERROR"))
               MustCrash = value;
 
       else if (!strcmp(name, "DEBUG"))
               StrokeDebug = SpaceDebug = PathDebug = ConicDebug = LineDebug =
                          RegionDebug = MemoryDebug = FontDebug =
                          HintDebug = ImageDebug = OffPageDebug = value;
 
       else if (!strcmp(name, "CONICDEBUG"))
               ConicDebug = value;
 
       else if (!strcmp(name, "LINEDEBUG"))
               LineDebug = value;
 
       else if (!strcmp(name, "REGIONDEBUG"))
               RegionDebug = value;
 
       else if (!strcmp(name, "PATHDEBUG"))
               PathDebug = value;
 
       else if (!strcmp(name, "SPACEDEBUG"))
               SpaceDebug = value;
 
       else if (!strcmp(name, "STROKEDEBUG"))
               StrokeDebug = value;
 
       else if (!strcmp(name, "MEMORYDEBUG"))
               MemoryDebug = value;
 
       else if (!strcmp(name, "FONTDEBUG"))
               FontDebug = value;
 
       else if (!strcmp(name, "HINTDEBUG"))
               HintDebug = value;
 
       else if (!strcmp(name, "IMAGEDEBUG"))
               ImageDebug = value;
 
       else if (!strcmp(name, "OFFPAGEDEBUG"))
               OffPageDebug = value;
 
#ifdef  MC68000
/*
The following pragmatics flag turns on or off instruction histograming
for performance analysis.  It is only defined in the Delta card
environment.
*/
       else if (!strcmp(name, "PROFILE")) {
               if (value)
                       StartProfile();
               else
                       StopProfile();
       }
#endif
       /* GimeSpace() is define as false ... */
       /*
       else if (!strcmp(name, "FLUSHCACHE")) {
               while (GimeSpace()) { ; }
       }
       */
       else if (!strcmp(name, "CACHEDCHARS"))
               CachedChars = (value <= 0) ? 1 : value;
 
       else if (!strcmp(name, "CACHEDFONTS"))
               CachedFonts = (value <= 0) ? 1 : value;
 
       else if (!strcmp(name, "CACHEBLIMIT"))
               CacheBLimit = value;
 
       else if (!strcmp(name, "CONTINUITY"))
               Continuity = value;
 
 
       else {
               printf("Pragmatics flag = '%s'\n", name);
               ArgErr("Pragmatics:  flag not known", NULL, NULL);
       }
       return;
}
 
/*
:h3.Consume() - Consume a List of Arguments
 
This general purpose routine is provided in the case where the object
type(s) to be consumed are unknown or not yet verified, and/or it is
not known whether the object is permanent.
 
If the type of the argument is known, it is faster to directly consume
that type, for example, ConsumeRegion() or ConsumePath().  Furthermore,
if it is already known that the object is temporary, it is faster to
just kill it rather than consume it, for example, KillSpace().
*/
 
void Consume(n, obj1, obj2, obj3) /* non-ANSI avoids overly strict type checking */
       int n;
       struct xobject *obj1,*obj2,*obj3;
{
       switch(n) {
 
           case 0:
               return;
 
           case 1:
               if (obj1 != NULL && !ISPERMANENT(obj1->flag))
                       Destroy(obj1);
               return;
 
           case 2:
               if (obj1 != NULL && !ISPERMANENT(obj1->flag))
                       Destroy(obj1);
               if (obj2 != NULL && !ISPERMANENT(obj2->flag))
                       Destroy(obj2);
               return;
 
           case 3:
               if (obj1 != NULL && !ISPERMANENT(obj1->flag))
                       Destroy(obj1);
               if (obj2 != NULL && !ISPERMANENT(obj2->flag))
                       Destroy(obj2);
               if (obj3 != NULL && !ISPERMANENT(obj3->flag))
                       Destroy(obj3);
               return;
 
           default:
               abort("Consume:  too many objects", 19);
       }
}
/*
:h3.TypeErr() - Handles "Invalid Object Type" Errors
*/
 
struct xobject *TypeErr(name, obj, expect, ret) /* non-ANSI avoids overly strict type checking */
       char *name;           /* Name of routine (for error message)          */
       struct xobject *obj;  /* Object in error                              */
       int expect;           /* type expected                                */
       struct xobject *ret;  /* object to return to caller                   */
{
       static char typemsg[80];
 
       if (MustCrash)
               LineIOTrace = TRUE;
 
       sprintf(typemsg, "Wrong object type in %s; expected %s, found %s.\n",
                  name, TypeFmt(expect), TypeFmt(obj->type));
       IfTrace0(TRUE,typemsg);
 
       ObjectPostMortem(obj);
 
       if (MustCrash)
               abort("Terminating because of CrashOnUserError...", 20);
       else
               ErrorMessage = typemsg;
 
/* changed ISPERMANENT to ret->references > 1 3-26-91 PNM */
       if (ret != NULL && (ret->references > 1))
               ret = Dup(ret);
       return(ret);
}
 
/*
:h4.TypeFmt() - Returns Pointer to English Name of Object Type
 
This is a subroutine of TypeErr().
*/
 
static char *TypeFmt(type)
       int type;             /* type field                                   */
{
       char *r;
 
       if (ISPATHTYPE(type))
               if (type == TEXTTYPE)
                       r = "path or region (from TextPath)";
               else
                       r = "path";
       else {
               switch (type) {
                   case INVALIDTYPE:
                       r = "INVALID (previously consumed?)";
                       break;
                   case REGIONTYPE:
                       r = "region";
                       break;
                   case SPACETYPE:
                       r = "XYspace";
                       break;
                   case LINESTYLETYPE:
                       r = "linestyle";
                       break;
                   case FONTTYPE:
                       r = "font";
                       break;
                   case PICTURETYPE:
                       r = "picture";
                       break;
                   case STROKEPATHTYPE:
                       r = "path (from StrokePath)";
                       break;
                   default:
                       r = "UNKNOWN";
                       break;
               }
       }
       return(r);
}
/*
:h4.ObjectPostMortem() - Prints as Much as We Can About a Bad Object
 
This is a subroutine of TypeErr() and ArgErr().
*/
 
/*ARGSUSED*/
static int ObjectPostMortem(obj) /* non-ANSI avoids overly strict type checking  */
       register struct xobject *obj;
{
 
       Pragmatics("Debug", 10);
       IfTrace2(TRUE,"Bad object is of %s type %p\n", TypeFmt(obj->type), obj);
 
       IfTrace0((obj == (struct xobject *) USER),
                  "Suspect that InitImager() was omitted.\n");
       Pragmatics("Debug", 0);
       /* We return a value to make ANSI-compiler happy */
       return(0);
       
}
 
/*
:h3.ArgErr() - Invalid Argument Passed to a Routine
 
A common routine to report argument errors.  It is usually called
is returned to the caller in case MustCrash is FALSE and ArgErr
returns to its caller.
*/
 
struct xobject *ArgErr(string, obj, ret) /* non-ANSI avoids overly strict type checking */
       char *string;         /* description of error                         */
       struct xobject *obj;  /* object, if any, that was in error            */
       struct xobject *ret;  /* object returned to caller or NULL            */
{
       if (MustCrash)
               LineIOTrace = TRUE;
       IfTrace1(TRUE,"ARGUMENT ERROR-- %s.\n", string);
       if (obj != NULL)
               ObjectPostMortem(obj);
       if (MustCrash)
               abort("Terminating because of CrashOnUserError...", 21);
       else
               ErrorMessage = string;
       return(ret);
}
 
/*
:h3.abort() - Crash Due to Error
 
We divide by zero, and if that doesn't work, call exit(), the results of
which is system dependent (and thus is part of the Hourglass required
environment).
*/
/* RMz: We now do a longjmp in order to be able to recover from the error */ 
/*ARGSUSED*/
void abort(string, no)
       char *string;
       int no;
{
  
  LineIOTrace = TRUE;
  TraceClose();
  longjmp( stck_state, no);

}

/* By RMz: Return the abort string to t1lib! */
char *t1_get_abort_message( int number)
{
  static char *err_msgs[]={
    "DLdiv:  dividend too large", /* 1 */
    "divide algorithm error", /* 2 */
    "Beziers this big not yet supported", /* 3 */
    "ComputeHint: invalid orientation", /* 4 */
    "ComputeHint: invalid hinttype", /* 5 */
    "ComputeHint: invalid orientation", /* 6 */
    "ProcessHint: invalid label", /* 7 */
    "ProcessHint: label is not in use", /* 8  */
    "ProcessHint: invalid label", /* 9 */
    "ProcessHint: invalid adjusttype", /* 10 */
    "bad subpath chain", /* 11 */
    "ImpliedHorizontalLine:  why ask?", /* 12 */
    "disjoint subpath?", /* 13 */
    "unable to fix subpath break?", /* 14 */
    "Non-positive allocate?", /* 15 */
    "We have REALLY run out of memory", /* 16 */
    "Free of already freed object?", /* 17 */
    "Pragmatics name too large", /* 18 */
    "Consume:  too many objects", /* 19 */
    "Terminating because of CrashOnUserError...", /* 20 */
    "Terminating because of CrashOnUserError...", /* 21 */
    "Fundamental TYPE1IMAGER assumptions invalid in this port", /* 22 */
    "Reverse: bad path segment", /* 23 */
    "UnClose:  no LASTCLOSED", /* 24 */
    "PathTransform:  invalid segment", /* 25 */
    "QueryPath: unknown segment", /* 26 */
    "QueryBounds: unknown type", /* 27 */
    "KillRegion:  negative reference count", /* 28 */
    "newedge: height not positive", /* 29 */
    "Interior: path type error", /* 30 */
    "Unwind:  uneven edges", /* 31 */
    "negative sized edge?", /* 32 */
    "splitedge: above top of list", /* 33 */
    "splitedge: would be null", /* 34 */
    "null splitedge", /* 35 */
    "vertjoin not disjoint", /* 36 */
    "SwathUnion:  0 height swath?", /* 37 */
    "discard():  ran off end", /* 38 */
    "UnJumble:  unpaired edge?", /* 39 */
    "Tighten: existing edge bound was bad", /* 40  */
    "Tighten: existing region bound was bad", /* 41 */
    "EDGE ERROR: non EDGETYPE in list", /* 42 */
    "EDGE ERROR: overlapping swaths", /* 43 */
    "Context:  QueryDeviceState didn't work", /* 44 */
    "QueryDeviceState returned invalid orientation", /* 45 */
    "Context:  out of them", /* 46 */
    "MatrixInvert:  can't", /* 47 */
    "xiStub called", /* 48 */
    "Illegal access type1 abort() message" /* 49 */
  };

  /* no is valid from 1 to 48 */
  if ( (number<1)||(number>48))
    number=49;
  return( err_msgs[number-1]);
    
}


/*
:h3.REAL Miscellaneous Stuff
 
:h4.ErrorMsg() - Return the User an Error Message
*/
 
char *ErrorMsg()
{
       register char *r;
 
       r = ErrorMessage;
       ErrorMessage = NULL;
       return(r);
}
 
/*
:h4.InitImager() - Initialize TYPE1IMAGER
 
We check that a short is 16 bits and a long 32 bits; we have made
those assumptions elsewhere in the code.  (This is almost a C standard,
anyway.)  Note that TYPE1IMAGER makes no assumptions about the size of an
'int'!
:i1/portability assumptions/
*/
void InitImager()
{
 
/* Check to see if we have been using our own malloc.  If so,*/
/* Undef malloc so that we can get to the system call. */
/* All other calls to malloc are defined to Xalloc.  */
 
       if (sizeof(SHORT) != 2 || sizeof(LONG) != 4)
          abort("Fundamental TYPE1IMAGER assumptions invalid in this port", 22);
       InitSpaces();
       InitFonts();
       InitFiles();
/*
In some environments, constants and/or exception handling need to be
*/
       LibInit();
}
/*
:h4.TermImager() - Terminate TYPE1IMAGER
 
This only makes sense in a server environment; true TYPE1IMAGER needs do
nothing.
*/
void TermImager()
{
       return;
}
/*
:h4.reportusage() - A Stub to Get a Clean Link with Portable PMP
*/
void reportusage( void)
{
       return;
}
