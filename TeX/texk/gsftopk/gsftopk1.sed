.\" Copyright (c) 1993-1998  Paul Vojta
.\"
.\" Permission is hereby granted, free of charge, to any person obtaining a copy
.\" of this software and associated documentation files (the "Software"), to
.\" deal in the Software without restriction, including without limitation the
.\" rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
.\" sell copies of the Software, and to permit persons to whom the Software is
.\" furnished to do so, subject to the following conditions:
.\"
.\" The above copyright notice and this permission notice shall be included in
.\" all copies or substantial portions of the Software.
.\"
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
.\" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
.\" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
.\" PAUL VOJTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
.\" IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
.\" CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
.\"
.TH GSFTOPK 1 "28 November 1998"
.SH NAME
gsftopk \- render a ghostscript font in TeX pk format
'	# small and boldface (not all -man's provide it)
.de SB
\&\fB\s-1\&\\$1 \\$2\s0\fR
..
.SH SYNOPSIS
.B gsftopk
[\-i \fIpath\fP]
[\-q]
[\-t]
#ifkpathsea
[\-\-debug=\fIn\fP]
#endif
[\-\-dosnames]
[\-\-interpreter=\fIpath\fP]
[\-\-mapline=\fIline\fP]
[\-\-mapfile=\fIfile\fP]
[\-\-quiet]
[\-\-test]
[\-\-help]
[\-\-version]
.I font
.I dpi
.SH ARGUMENTS
.IP \fIfont\fP \w'\fIfont\fP'u+2m
Name of the font to be created.
.IP \fIdpi\fP
Desired resolution of the font to be created, in dots per inch.  This may
be a real number.
.SH DESCRIPTION
.B gsftopk
is a program which calls up the ghostscript program
.BR gs (1)
to render a given font at a given resolution.  It packs the resulting
characters into the
.B pk
file format and writes them to a file whose name is formed from the font
name and the resolution (rounded to the nearest integer).  The font may
be in any format acceptable to Ghostscript, including
.RI . pfa ,
.RI . pfb ,
.RI . gsf ,
and
.RI . ttf
files.
.PP
This program should normally be called by a script, such as
.BR mktexpk ,
to create fonts on demand.
.PP
.B gsftopk
obtains the character widths from the
.RI . tfm
file, which must exist in the standard search path.  It also must be
able to find the font in a map file (such as
.BR psfonts.map ),
formatted as in
.BR dvips (1)),
unless the
.B \-\-mapline
option is used.  The set of map files is given by the
.B \-\-mapfile
option, or in the files
.BR config.ps ,
.BR $HOME/.dvipsrc ,
and
.B config.gsftopk
(as would be used by
.BR "dvips -Pgsftopk" ).
.PP
The following
.B pk
"specials" are added at the end of the output file, to provide an internal
check on the contents of the file:
"\fBjobname=\fP\fIfont\fP",
"\fBmag=1\fP",
"\fBmode=modeless\fP", and
"\fBpixels_per_inch=\fP\fIdpi\fP".
This is in accordance with the TeX Directory Standard (TDS).
.SH OPTIONS
#ifkpathsea
.TP
.B \-\-debug=\fIn\fP
Set the
.B Kpathsea
debug flags according to the integer
.IR n .
#endif
.TP
.B \-\-dosnames
Use a name of the form
.IB font .pk
instead of
.IB font . dpi pk\fR.\fP
.TP
.B \-h\fR,\fP \-\-help
Print a brief help synopsis and exit.
.TP
.B \-i \fIpath\fP\fR,\fP \-\-interpreter=\fIpath\fP
Use
.I path
as the Ghostscript interpreter.
.TP
.B \-\-mapfile=\fIfile\fP
Use
.I file
to look for the map information for
.IR font .
This should be the full name of the file (in other words, no path searching
algorithms are applied).
.TP
.B \-\-mapline=\fIline\fP
Use
.I line
instead of looking for an entry in a map file.
The first word of
.I line
must match
.IR font .
.TP
.B \-q\fR,\fP \-\-quiet
Operate quietly; i.e., without writing any messages to the standard output.
.TP
.B \-t\fR,\fP \-\-test
Test run:  return zero status if the font can be found in the map file(s),
and nonzero status if it cannot.  If this option is specified, then the
.I dpi
argument is optional (since the font will not be generated).
.TP
.B \-v\fR,\fP \-\-version
Print the version number and exit.
.SH ENVIRONMENT VARIABLES
.IP \fBDVIPSRC\fP \w'\fBGSFTOPKHEADERS\fP'u+2m
Name of file to read instead of
.BR $HOME/.dvipsrc .
This should be the full name of the file (in other words, no path searching
algorithms are applied).
.IP \fBGSFTOPKFONTS\fP
See
.SB TEXFONTS.
.IP \fBGSFTOPKHEADERS\fP
See
.SB TEXPSHEADERS.
.IP \fBPSHEADERS\fP
See
.SB TEXPSHEADERS.
.IP \fBTEXCONFIG\fP
Colon-separated list of paths to search for map files.
An extra colon in the list will include the
compiled-in default paths at that point.  A double slash will enable recursive
subdirectory searching at that point in the path.
.IP \fBTEXFONTS\fP
Colon-separated list of paths to search for the
.RI . tfm
file associated with the font.  Double slashes and extra colons behave as with
.SB TEXCONFIG.
This information may also be supplied by using the environment variables
.SB TFMFONTS
or
.SB GSFTOPKFONTS.
These environment variables are checked in the order
.SB GSFTOPKFONTS,
.SB TFMFONTS,
.SB TEXFONTS;
the first one (if any) having a value is used.
.IP \fBTEXPSHEADERS\fP
Colon-separated list of paths to search for the Ghostscript driver file
.B render.ps
and for any PostScript header or font files
.RI (. enc ,
.RI . pfa ,
.RI . pfb ,
.RI . gsf ,
or
.RI . ttf
files).  Double slashes and extra colons behave as with
.SB TEXCONFIG.
This information may also be supplied by using the environment variables
.SB PSHEADERS
or
.SB GSFTOPKHEADERS.
These environment variables are checked in the order
.SB GSFTOPKHEADERS,
.SB TEXPSHEADERS,
.SB PSHEADERS;
the first one (if any) having a value is used.
.IP \fBTFMFONTS\fP
See
.SB TEXFONTS.
.SH CONFIGURATION
In order to determine the set of map files to be used and the path for
finding PostScript files,
.B gsftopk
reads, in order, the files
.BR config.ps ,
.BR .dvipsrc ,
and
.BR config.gsftopk .
The files
.B config.ps
and
.B config.gsftopk
are searched for using the environment variable
#ifnokpathsea
.SB TEXCONFIG
#endif
#ifkpathsea
.SB TEXCONFIG,
the
.B Kpathsea
configuration file,
#endif
or the compiled-in default paths.  The file
.B .dvipsrc
is searched for in the user's home directory.
.PP
These files are in the same format as for
.B dvips
(as well as being in the same locations).  The entries used by
.B gsftopk
are as follows.
.TP
.RI "H " path
Indicates that the Ghostscript driver file
.B render.ps
and the PostScript header and font files are to be searched for using
.IR path .
.TP
.RI "p " file
Indicates that the list of map files is to be erased and replaced by
.IR file .
.TP
.RI "p +" file
Indicates that
.I file
is to be added to the list of map files.
.PP
All other entries are ignored.
.PP
This is similar to the handling of these options when running
.BR "dvips -Pgsftopk" .
#ifkpathsea
For more details, see the
.B Kpathsea
manual.
#endif
.SH BUGS
.B gsftopk
sometimes has trouble with fonts with very complicated characters
(such as the Seal of the University of California).  This is because
.B gsftopk
uses the
.B charpath
operator to determine the bounding box of each character.  If the character
is too complicated, then old versions of Ghostscript fail, causing
.B gsftopk
to terminate with an error message
.IP
.B "Call to gs stopped by signal 10"
.LP
(The number may vary from system to system; it corresponds to a bus error
or a segmentation fault.)  The best way to fix this bug is to install a
current version of ghostscript.  As an alternative,
.B gsftopk
can be instructed to use the bounding box provided with the font (if one
exists) instead of finding a bounding box for each character.  To do this,
include the string
.IP
.B /usefontbbox true def
.LP
in the font map file;
.IR e.g. ,
.IP
.B ucseal """/usefontbbox true def"""
.LP
This will not affect use of the font by
.BR dvips .
.SH SEE ALSO
.BR gs (1),
.BR gftopk (1),
.BR tex (1),
.BR xdvi (1),
.BR dvips (1)
.SH AUTHOR
Written by Paul Vojta.  This program was inspired by Karl Berry's
.BR gsrenderfont .
#ifkpathsea
.SH MODIFICATIONS
Modified by Yves Arrouye to use Karl Berry's
.B Kpathsea
library.
#endif
