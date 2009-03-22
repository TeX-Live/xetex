%% This file is xepersian-moderncv.cls (modified version of moderncv.cls)
%%
%% Copyright 2009 by Vafa Khalighi
%%
%%%% It may be distributed and/or modified under the
%% conditions of the LaTeX Project Public License, either version 1.3c
%% of this license or (at your option) any later version.
%%
%%
%%
%%
%% start of file `moderncv.cls'.
%% Copyright 2006-2008 Xavier Danaux (xdanaux@gmail.com).
%
% This work may be distributed and/or modified under the
% conditions of the LaTeX Project Public License version 1.3c,
% available at http://www.latex-project.org/lppl/.
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{xepersian-moderncv}[2009/04/01 v0.1 modern Persian curriculum vitae document class in XeLaTeX (Author: Vafa Khalighi)]
\newif\if@DEBUG\@DEBUGfalse
\DeclareOption{a4paper}{
  \setlength\paperheight{297mm}
  \setlength\paperwidth{210mm}}
\DeclareOption{a5paper}{
  \setlength\paperheight{210mm}
  \setlength\paperwidth{148mm}}
\DeclareOption{b5paper}{
  \setlength\paperheight{250mm}
  \setlength\paperwidth{176mm}}
\DeclareOption{letterpaper}{
  \setlength\paperheight{11in}
  \setlength\paperwidth{8.5in}}
\DeclareOption{legalpaper}{
  \setlength\paperheight{14in}
  \setlength\paperwidth{8.5in}}
\DeclareOption{executivepaper}{
  \setlength\paperheight{10.5in}
  \setlength\paperwidth{7.25in}}
\DeclareOption{landscape}{
  \setlength\@tempdima{\paperheight}
  \setlength\paperheight{\paperwidth}
  \setlength\paperwidth{\@tempdima}}
\newcommand\@ptsize{}
\DeclareOption{10pt}{\renewcommand\@ptsize{0}}
\DeclareOption{11pt}{\renewcommand\@ptsize{1}}
\DeclareOption{12pt}{\renewcommand\@ptsize{2}}
\DeclareOption{draft}{\setlength\overfullrule{5pt}}
\DeclareOption{final}{\setlength\overfullrule{0pt}}
\newif\if@colour\@colourtrue
\DeclareOption{nocolour}{\@colourfalse}
\ExecuteOptions{a4paper,11pt,colour,final}
\ProcessOptions\relax
\input{size1\@ptsize.clo}
\RequirePackage{graphicx}
\RequirePackage{pstricks}
\RequirePackage{ifthen}
\RequirePackage{marvosym}
\addtolength{\oddsidemargin}{-54pt}
\addtolength{\textwidth}{109pt}
\addtolength{\topmargin}{-70pt}
\addtolength{\textheight}{122pt}
\addtolength{\marginparsep}{-5pt}
\addtolength{\marginparwidth}{-10pt}
\RequirePackage{url}
\RequirePackage{hyperref}
\hypersetup{%
  breaklinks,
  baseurl       = http://,%
  pdfborder     = 0 0 0,%
  pdfpagemode   = UseNone,%
  pdfstartpage  = 1,%
  pdfcreator    = XePersian v1.1,%
  pdfproducer   = XePersian v1.1}
\AtEndOfClass{%
  \AtBeginDocument{%
    \hypersetup{%
      pdfauthor     = \@firstname~\@familyname,%
      pdftitle      = \@title,%
      pdfsubject    = \@firstname~\@familyname,%
      pdfkeywords   = \@firstname~\@familyname{} curriculum vit\ae{}}}}
\urlstyle{tt}
\RequirePackage{fancyhdr}
\fancypagestyle{plain}{%
  \renewcommand{\headrulewidth}{0pt}
  \renewcommand{\footrulewidth}{0pt}
  \fancyhf{}}
\pagestyle{plain}
\setlength\lineskip{1\p@}
\setlength\normallineskip{1\p@}
\renewcommand\baselinestretch{}
\setlength{\parindent}{0pt}
\setlength{\parskip}{0pt}
\setlength\columnsep{10\p@}
\setlength\columnseprule{0\p@}
\pagestyle{empty}
\pagenumbering{arabic}
\raggedbottom
\onecolumn
\providecommand*{\listitemsymbol}{\textbullet}
\providecommand*{\addresssymbol}{}
\providecommand*{\mobilesymbol}{\Mobilefone}
\providecommand*{\phonesymbol}{\Telefon}
\providecommand*{\faxsymbol}{\FAX}
\providecommand*{\emailsymbol}{\Letter}
\def\firstnamecolour#1{\gdef\@firstnamecolour{\addfontfeature{Color=#1}}}
\def\familynamecolour#1{\gdef\@familynamecolour{\addfontfeature{Color=#1}}}
\def\titlecolour#1{\gdef\@titlecolour{\addfontfeature{Color=#1}}}
\def\addresscolour#1{\gdef\@addresscolour{\addfontfeature{Color=#1}}}
\def\quotecolour#1{\gdef\@quotecolour{\addfontfeature{Color=#1}}}
\def\sectiontitlecolour#1{\gdef\@sectiontitlecolour{\addfontfeature{Color=#1}}}
\def\subsectioncolour#1{\gdef\@subsectioncolour{\addfontfeature{Color=#1}}}
\def\hintcolour#1{\gdef\@hintcolour{\addfontfeature{Color=#1}}}
\providecommand*{\firstnamefont}{}
\providecommand*{\familynamefont}{}
\providecommand*{\titlefont}{}
\providecommand*{\addressfont}{}
\providecommand*{\quotefont}{}
\providecommand*{\sectionfont}{}
\providecommand*{\subsectionfont}{}
\providecommand*{\hintfont}{\small}
\providecommand*{\firstnamestyle}[1]{{\firstnamefont\@firstnamecolour#1}}
\providecommand*{\familynamestyle}[1]{{\familynamefont\@familynamecolour#1}}
\providecommand*{\titlestyle}[1]{{\titlefont\@titlecolour#1}}
\providecommand*{\addresstyle}[1]{{\addressfont\@addresscolour#1}}
\providecommand*{\quotestyle}[1]{{\quotefont\@quotecolour#1}}
\providecommand*{\sectionstyle}[1]{{\sectionfont\@sectiontitlecolour#1}}
\providecommand*{\subsectionstyle}[1]{{\subsectionfont\@subsectioncolour#1}}
\providecommand*{\hintstyle}[1]{{\hintfont\@hintcolour#1}}
\newcommand*{\xepersiantheme}[2][]{
  \def\@xepersiantheme{#2}
  \def\@xepersianthemeoptions{#1}}
\xepersiantheme{xepersian-casual}   
\AtBeginDocument{\RequirePackage[\@xepersianthemeoptions]{xepersiantheme\@xepersiantheme}}
\newcommand*{\firstname}[1]{\def\@firstname{#1}}
\newcommand*{\familyname}[1]{\def\@familyname{#1}}
\renewcommand*{\title}[1]{\def\@title{#1}}
\newcommand*{\address}[1]{\def\@address{#1}}
\newcommand*{\mobile}[1]{\def\@mobile{#1}}
\newcommand*{\phone}[1]{\def\@phone{#1}}
\renewcommand*{\fax}[1]{\def\@fax{#1}}
\newcommand*{\email}[1]{\def\@email{#1}}
\newcommand*{\extrainfo}[1]{\def\@extrainfo{#1}}
\def\@photowidth{0pt}
\newcommand*{\photo}[2][64pt]{\def\@photowidth{#1}\def\@photo{#2}}
\newcommand*{\quote}[1]{\def\@quote{#1}}
\newlength{\quotewidth}
\newlength{\hintscolumnwidth}
\newlength{\separatorcolumnwidth}
\setlength{\separatorcolumnwidth}{0.025\textwidth}
\newlength{\maincolumnwidth}
\newlength{\doubleitemmaincolumnwidth}
\newlength{\listitemsymbolwidth}
\settowidth{\listitemsymbolwidth}{\listitemsymbol{}~}
\newlength{\listitemmaincolumnwidth}
\newlength{\listdoubleitemmaincolumnwidth}
\newcommand*{\recomputethemelengths}{}
\newcommand*{\recomputelengths}{%
\setlength{\quotewidth}{0.65\textwidth}%
\setlength{\maincolumnwidth}{\textwidth}%
\addtolength{\maincolumnwidth}{-\separatorcolumnwidth}%
\addtolength{\maincolumnwidth}{-\hintscolumnwidth}%
\setlength{\listitemmaincolumnwidth}{\maincolumnwidth}%
\addtolength{\listitemmaincolumnwidth}{-\listitemsymbolwidth}%
\setlength{\doubleitemmaincolumnwidth}{\maincolumnwidth}%
\addtolength{\doubleitemmaincolumnwidth}{-\hintscolumnwidth}%
\addtolength{\doubleitemmaincolumnwidth}{-\separatorcolumnwidth}%
\setlength{\doubleitemmaincolumnwidth}{0.5\doubleitemmaincolumnwidth}%
\setlength{\listdoubleitemmaincolumnwidth}{\maincolumnwidth}%
\addtolength{\listdoubleitemmaincolumnwidth}{-\listitemsymbolwidth}%
\setlength{\listdoubleitemmaincolumnwidth}{0.475\listdoubleitemmaincolumnwidth}%
\renewcommand{\headwidth}{\textwidth}%
\recomputethemelengths}
\setlength{\hintscolumnwidth}{0.15\textwidth}
\recomputelengths
\title{}
\renewcommand*{\maketitle}{}
\newcommand*{\section}[1]{%
	\vspace*{2.5ex \@plus 1ex \@minus .2ex}%
  \phantomsection{}% 
  \addcontentsline{toc}{part}{#1}%
  \parbox[m]{\hintscolumnwidth}{\raggedleft\hintfont{\beginL\psline[linecolor=sectionrectanglecolor,linewidth=4pt](0,0)(-2,0)\endL}}%
  \hspace{\separatorcolumnwidth}%
  \parbox[m]{\maincolumnwidth}{\sectionstyle{#1}}\\[1ex]}
\newcommand*{\subsection}[1]{%
  \cvline[0.45em]{}{\subsectionstyle{#1}}}
\newcommand*{\cvline}[3][.25em]{%
  \begin{tabular}{@{}p{\hintscolumnwidth}@{\hspace{\separatorcolumnwidth}}p{\maincolumnwidth}@{}}%
 	  \raggedleft\hintfont{#2} &{#3}%
  \end{tabular}\\[#1]}
\newcommand*{\cvdoubleitem}[4]{%
 \cvline{#1}{\begin{minipage}[t]{\doubleitemmaincolumnwidth}#2\end{minipage}%
 \hfill%
 \begin{minipage}[t]{\hintscolumnwidth}\raggedleft\hintfont{#3}\end{minipage}\hspace*{\separatorcolumnwidth}\begin{minipage}[t]{\doubleitemmaincolumnwidth}#4\end{minipage}}}
\newcommand*{\cvlistitem}[2][\listitemsymbol{}]{%
  \cvline[0pt]{}{#1~\begin{minipage}[t]{\listitemmaincolumnwidth}#2\end{minipage}}}
\newcommand*{\cvlistdoubleitem}[3][\listitemsymbol{}]{%
  \cvline[0pt]{}{#1~\begin{minipage}[t]{\listdoubleitemmaincolumnwidth}#2\end{minipage}%
  \hfill%
  \ifthenelse{\equal{#3}{}}%
    {}%
    {#1~\begin{minipage}[t]{\listdoubleitemmaincolumnwidth}#3\end{minipage}}}}
\newcommand*{\cventry}[6]{%
  \cvline{#1}{%
    {\bfseries#2}%
    \ifx#3\else{, {\slshape#3}}\fi%
    \ifx#4\else{, #4}\fi%
    \ifx#5\else{, #5}\fi%
    .%
    \ifx#6\else{\newline{}\begin{minipage}[t]{\linewidth}\small#6\end{minipage}}\fi
    }}%
\newcommand*{\cvlanguage}[3]{%
  \cvline{#1}{\begin{minipage}[t]{.225\maincolumnwidth}\textbf{#2}\end{minipage}\hfill\begin{minipage}[t]{0.725\maincolumnwidth}\raggedleft\footnotesize\itshape #3\end{minipage}}}
\newcommand*{\cvcomputer}[4]{%
  \cvdoubleitem{#1}{\small#2}{#3}{\small#4}}
\newcommand*{\link}[2][]{%
  \ifthenelse{\equal{#1}{}}%
    {\beginL\href{#2}{\beginR#2\endR}\endL}%
    {\beginL\href{#2}{\beginR#1\endR}\endL}}
\newcommand*{\httplink}[2][]{%
  \ifthenelse{\equal{#1}{}}%
    {\beginL\href{http://#2}{\beginR#2\endR}\endL}%
    {\beginL\href{http://#2}{\beginR#1\endR}\endL}}
\newcommand*{\emaillink}[2][]{%
  \ifthenelse{\equal{#1}{}}%
    {\beginL\href{mailto:#2}{\beginR#2\endR}\endL}%
    {\beginL\href{mailto:#2}{\beginR#1\endR}\endL}}
\newif\if@displaypagenumbers\@displaypagenumberstrue
\newcommand*{\nopagenumbers}{\@displaypagenumbersfalse}
\AtBeginDocument{%
  \if@displaypagenumbers%
    \@ifundefined{r@lastpage}{}{%
      \ifthenelse{\pageref{lastpage} > 1}{%
      \fancypagestyle{plain}{%
        \fancyfoot[r]{\addressfont\@quotecolour\footnotesize\beginL\thepage/\pageref{lastpage}\endL}}%
      \pagestyle{plain}}{}}%
  \AtEndDocument{\label{lastpage}}%
  \fi}
\newcommand{\refname}{انتشارات}
\setlength{\bibindent}{1.5em}
\newcommand*{\bibliographyitemlabel}{}
\newenvironment{thebibliography}[1]%
  {%
    \section{\refname}%
%    
    \small%
    \begin{list}{\bibliographyitemlabel}%
      {%
        \setlength{\topsep}{0pt}%
        \setlength{\labelwidth}{\hintscolumnwidth}%
        \setlength{\labelsep}{\separatorcolumnwidth}%
        \leftmargin\labelwidth%
        \advance\leftmargin\labelsep%
        \@openbib@code%
        \usecounter{enumiv}%
        \let\p@enumiv\@empty%
        \renewcommand\theenumiv{\@arabic\c@enumiv}}%
        \sloppy\clubpenalty4000\widowpenalty4000%
%        
%        
  }%
  {%
    \def\@noitemerr{\@latex@warning{Empty `thebibliography' environment}}%
    \end{list}%
  }
\newcommand\newblock{\hskip .11em\@plus.33em\@minus.07em}
\let\@openbib@code\@empty
\endinput
