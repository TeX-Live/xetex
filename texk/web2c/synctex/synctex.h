/* synctex.h

This file is part of the SyncTeX package.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE

*/

#  ifndef __SYNCTEX_HEADER__
#    define __SYNCTEX_HEADER__

#include "synctex-common.h"

/*  Send this message when starting a new input.  */
extern void synctexstartinput(void);

/*  Recording the "{..." line.  In *tex.web, use synctex_sheet(pdf_output) at
 *  the very beginning of the ship_out procedure.
*/
extern void synctexsheet(integer mag);

/*  Recording the "}..." line.  In *tex.web, use synctex_teehs at
 *  the very end of the ship_out procedure.
*/
extern void synctexteehs(void);

/*  This message is sent when a vlist will be shipped out, more precisely at
 *  the beginning of the vlist_out procedure in *TeX.web.  It will be balanced
 *  by a synctex_tsilv, sent at the end of the vlist_out procedure.  p is the
 *  address of the vlist We assume that p is really a vlist node! */
extern void synctexvlist(halfword this_box);

/*  Recording a "f" line ending a vbox: this message is sent whenever a vlist
 *  has been shipped out. It is used to close the vlist nesting level. It is
 *  sent at the end of the vlist_out procedure in *TeX.web to balance a former
 *  synctex_vlist sent at the beginning of that procedure.    */
extern void synctextsilv(halfword this_box);

/*  This message is sent when a void vlist will be shipped out.
 *  There is no need to balance a void vlist.  */
extern void synctexvoidvlist(halfword p, halfword this_box);

/*  Send this message when an hlist will be shipped out, more precisely at
 *  the beginning of the hlist_out procedure in *TeX.web.  It must be balanced
 *  by a synctex_tsilh, sent at the end of the hlist_out procedure.  p is the
 *  address of the hlist. */
extern void synctexhlist(halfword this_box);

/*  Sent this message at the end of the hlist_out procedure in *TeX.web
 *  to balance a former synctex_hlist.    */
extern void synctextsilh(halfword this_box);

/*  This message is sent when a void hlist will be shipped out.
 *  There is no need to balance a void hlist.  */
extern void synctexvoidhlist(halfword p, halfword this_box);

/*  Send this message whenever an inline math node will ship out. */
extern void synctexmath(halfword p, halfword this_box);

/*  Send this message whenever an horizontal rule or glue node will ship out. */
extern void synctexhorizontalruleorglue(halfword p, halfword this_box);

/*  Send this message whenever a kern node will ship out. */
extern void synctexkern(halfword p, halfword this_box);

/*  this message is sent whenever a char node ships out    */
extern void synctexchar(halfword p, halfword this_box);

/*  this message should be sent to record information
         for a node of an unknown type    */
extern void synctexnode(halfword p, halfword this_box);

/*  For debugging purpose only    */
extern void synctexcurrent(void);

#  endif
