
----------------------------
THE POLYGLOSSIA PACKAGE v1.0

This package provides a complete replacement to Babel for users of XeLaTeX.

Polyglossia is used to automate the following tasks:

* Loading the appropriate hyphenation patterns.
* Setting the script and language tags of the current font (if possible and
  available).
* Switching to a font assigned by the user to a particular script or language.
* Adjusting some typographical conventions in function of the current language
  (such as afterindent, frenchindent, spaces before or after punctuation marks, 
  etc.).
* redefining the document strings (like “chapter”, “figure”, “bibliography”).
* Adapting the formatting of dates (for non-gregorian calendars via external
  packages bundled with polyglossia: currently the hebrew, islamic and farsi
  calendars are supported).
* For languages that have their own numeration system, the formatting of numbers 
  is modified appropriately.
* Ensuring the proper directionality if the document contains bidirectional
  text (via the package bidi, available separately on CTAN).

Several features of Babel that do not make sense in the XeTeX world (like font 
encodings, shorthands, etc) are not supported.

Polyglossia is distributed in the traditional way with *.dtx and *.ins files,
and also comes with a TDS-conformant ready-to-unpack zip file.

To install from source (i.e. using polyglossia.dtx), run 
	xetex polyglossia.ins  
(NB: not tex!! otherwise the UTF-8 characters will come out garbled)
You will then also need to compile the *.map files using teckit_compile.
The PDF documentation can be regenerated using 
	xelatex polyglossia.dtx

----------------------------
François Charette, July 2008
<firmicus ατ gmx δοτ net>


