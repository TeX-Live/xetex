%% This file is xepersian-user.cls 
%%
%% Copyright 2009 by Vafa Khalighi
%%
%%%% It may be distributed and/or modified under the
%% conditions of the LaTeX Project Public License, either version 1.3c
%% of this license or (at your option) any later version.
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{xepersian-user}[2009/01/05 Class for XePersian User's Guide (Author: Vafa Khalighi)]
\TeXXeTstate=1
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions\relax
\LoadClass{article}
\RequirePackage{pstcol}
\RequirePackage{pst-coil}
\RequirePackage{pst-plot}
\RequirePackage{pst-tree}
\RequirePackage{fancybox}       
\RequirePackage{fancyvrb}
\RequirePackage{geometry}
\RequirePackage{fontspec}
\RequirePackage{xltxtra}
\RequirePackage{url}
\RequirePackage[hyperfootnotes=false,colorlinks=true,bookmarks=true]
               {hyperref}
\RequirePackage{pifont}
\def\thetitle{\XePersian : User's Guide}
\newcommand*\setPersianFont[2][]{%
\newfontfamily\PersianFont[Script=Arabic,#1]{#2}
}
\def\PersianTxt#1{\begingroup\beginR\PersianFont#1\endR\endgroup}
\let\theheadertitle\thetitle  
\let\fileversion\Version
\let\filedate\Date
\parindent 0pt
\parskip 7pt plus 1pt minus 1pt
\oddsidemargin 5cm
\evensidemargin 5cm
\textwidth 11.5cm
\topmargin\z@
\textheight 8.8in
\footskip .625in
\headheight 0pt
\headsep 0pt
\marginparwidth 3.5cm
\marginparsep .5cm
\reversemarginpar
\ifdim\paperwidth=210mm \papersizeadjust\fi
\def\arraystretch{1.2}
\hfuzz 1pt
\def\twosideclearpage{%
  \clearpage
  \if@twoside
    \ifodd\c@page\else
      \thispagestyle{empty}%
      \null\leavevmode
      \clearpage
    \fi
  \fi}
\setlength{\parindent}{0pt}
\setlength{\parskip}{7pt plus 1pt minus 1pt}
\def\@listI{%
  \leftmargin\leftmargini
  \parsep 5\p@ plus2.5\p@ minus\p@
  \topsep 5\p@ plus2\p@ minus2\p@
  \itemsep 5\p@ plus2.5\p@ minus\p@}
\let\@listi\@listI
\@listi
\def\@listii{%
  \leftmargin\leftmarginii
  \labelwidth\leftmarginii
  \advance\labelwidth-\labelsep
  \topsep 2\p@ plus.5\p@ minus\p@
  \parsep 2.5\p@ plus\p@ minus\p@
  \itemsep \parsep}
\def\@listiii{%
  \leftmargin\leftmarginiii
  \labelwidth\leftmarginiii
  \advance\labelwidth-\labelsep
  \topsep 1\p@ plus.2\p@ minus.2\p@
  \parsep \z@
  \partopsep \p@ plus\z@ minus\p@
  \itemsep \topsep}
\skip\footins 20pt plus 4pt minus 2pt
\newcounter{faq}[section]
\def\thefaq{\arabic{faq}}
\let\l@faq\@gobbletwo
\def\faq{\@startsection{faq}{2}{\z@}%
  {3.25ex plus 1ex minus .2ex}{1.5ex plus .2ex}{\faqfont}}
\def\faqfont{\normalsize\sffamily}
\def\faqmark#1{}
\def\error{\vskip 2ex\noindent}
\def\section{\@startsection{section}{1}{\sechang}%
  {-3ex plus -1ex minus -.2ex}{1.5ex plus .2ex}{\secfont}}
\def\subsection{\@startsection{subsection}{2}{\subsechang}%
  {-2.75ex plus -1ex minus -.2ex}{1.25ex plus .2ex}{\subsecfont}}
\newdimen\sechang
\newdimen\subsechang
\sechang=-.75in
\subsechang=-.375in
\def\secfont{\sffamily\LARGE\bfseries}       
\def\subsecfont{\sffamily\Large\bfseries}    
\def\ps@docheadings{%
  \let\@mkboth\markboth
  \def\@oddhead{}%
  \let\@evenhead\@oddhead%
  \def\@oddfoot{%
    \small\sffamily
    \rightmark
    \hfil
    \thepage}%
  \def\@evenfoot{%
    \small\sffamily
    \thepage
    \hfil
    \theheadertitle}%
  \def\sectionmark##1{\markright {##1}}}%
\pagestyle{docheadings}
\newskip\tempskip
\def\TOCshrink{.5}
\def\TableOfContents{%
  \begin{quote}
    \begin{center} \Large\bf Contents \end{center}
    \def\numberline##1{\hbox to 0pt{\hss##1\hskip 1em}}%
    \let\oldaddvspace\addvspace
    \def\addvspace##1{%
      \tempskip##1\relax
      \oldaddvspace{\TOCshrink\tempskip}}
    \@starttoc{toc}%
  \end{quote}}
\def\tableofcontents{%
  \clearpage
  \markright{CONTENTS}%
  \TableOfContents
  \clearpage}
\def\expanddate#1/#2/#3/{%
  \number#3{}
  \month=#2
  \ifcase\month\or
    January \or February \or March \or April \or May \or June \or
    July \or August \or September \or October \or November \or December
  \fi
  20#1}
\def\thefiledate{\expandafter\expanddate\Date/}
\date{\thefiledate}
\def\NotNested{\def\begin##1{%
  \@latexerr{Bad nesting of environments.
    Perhaps missing \end{\@currenvir}}\@eha
  \end{\@currenvir}%
  \begin{##1}}}
\def\Ex{%
  \NotNested
  \list{}{}\item[]%
  \if@minipage\else\vskip\parskip\fi
  \leftskip\@totalleftmargin
  \rightskip\z@
  \parindent\z@
  \parskip\z@
  \parfillskip\@flushglue
  \hfuzz 1pt
  \@@par
  \global\@inlabelfalse}
\def\endEx{\endlist}
\begingroup
  \catcode`\{=12 \catcode`\}=12
  \catcode`\(=1 \catcode`\)=2
  \catcode`\+=0 \catcode`\\=12
  +gdef+|(\)          % \| = character \
  +gdef+{({)          % \{ = character {
  +gdef+}(})          % \} = character }
+endgroup
\def\Backslash{\protect\|}
\def\times@scale{.96}
\def\helv@scale{.82}
\def\courier@scale{.985}
\def\AmS{AMS}
\def\symchar#1{%
  \mbox{%
    \kern .3pt
    \Pisymbol{psy}{#1}
    \kern .3pt}}

\def\lt{\symchar{'074}}
\def\gt{\symchar{'076}}
\def\leq{\symchar{'243}}
\def\geq{\symchar{'263}}
\def\bullet{\symchar{'267}}
\def\ldots{\symchar{'274}}
\mathcode`\-="20B1
\renewcommand{\ttdefault}{\sfdefault}
\def\MainFont{\ttfamily\bfseries\large} 
\def\UsageFont{\ttfamily\bfseries}      
\def\InlineFont{\ttfamily}              
\def\MetaFont{\sffamily\slshape}        
\def\OpArgFont{\normalfont}             
\begingroup
  \catcode`\<=\active
  \gdef\ShortMeta{%
    \catcode`\<=\active
    \def<##1>{{\MetaFont ##1\/}}}
\endgroup
\def\DeleteShortMeta{\catcode`\<=12\relax}
\ShortMeta
\def\x#1{{\MetaFont x#1\/}}
\def\y#1{{\MetaFont y#1\/}}
\def\c#1{{\InlineFont(\x{#1},\kern 1pt\y{#1})}}
\def\cn{\c{n}}
\def\FV@SetupFont{%
  \FV@BaseLineStretch
  \FV@FontSize
  \FV@FontFamily
  \FV@FontSeries
  \FV@FontShape
  \selectfont
}
\def\FV@GetLine{\expandafter\FV@CheckScan\FancyVerbGetLine}
\AtBeginDocument{\DefineShortVerb{\"}}

\VerbatimFootnotes

\fvset{gobble=2,commentchar=\",codes={\catcode`\<=12},fontfamily=helvetica,
       frame=single,numbers=left,numbersep=1mm}

\DefineVerbatimEnvironment{LVerb}{LVerbatim}{fontsize=\small}
\DefineVerbatimEnvironment{LVerb*}{LVerbatim}%
  {fontsize=\small,codes={\catcode`\<=13}}

\def\FV@Space{\ \ }

% Verbatim items:

\newcommand{\vitem}{\SaveVerb[codes={\catcode`\<=13},
                              aftersave={\item[\UseVerb{item}]}]{item}}


\newcommand{\BeginExample}{%
  \VerbatimEnvironment
  \catcode`\<=12\relax
  \catcode`\"=9\relax
  \begin{VerbatimOut}[commentchar=none,codes={\catcode`\"=12}]{\jobname.tmp}}

\newcommand{\EndExample}{%
  \end{VerbatimOut}
  \fvset{gobble=0,fontsize=\small}}

\newenvironment{example}{\BeginExample}{%
  \EndExample
  \LVerbatimInput{\jobname.tmp}%
  \begin{center}\leavevmode\input{\jobname.tmp}\end{center}}

\newenvironment{example*}{\BeginExample}{%
  \EndExample\LVerbatimInput{\jobname.tmp}\input{\jobname.tmp}}

\newenvironment{example**}{\BeginExample}{%
  \EndExample\SideBySideEx{\input{\jobname.tmp}}}

\newif\ifgrid
\newpsobject{usergrid}{psgrid}{subgriddiv=1,griddots=10,gridlabels=7pt}

\def\MEx{\@ifnextchar[{\@MEx}{\@MEx[0,0]}}
\def\@MEx[#1,#2](#3,#4){%
  \def\PictureSize{(#1,#2)(#3,#4)}%
  \BeginExample}
\def\endMEx{%
  \EndExample
  \SideBySideEx{%
    \expandafter\pspicture\PictureSize
    \ifgrid\usergrid\fi\relax
    \input{\jobname.tmp}%
    \endpspicture}}

\def\SideBySideEx#1{%
  \trivlist\item[]%
  \leavevmode\kern-\@totalleftmargin
  \hbox{\valign{%
    \vfil##\vfil\cr
    \hbox to \z@{\hss#1}\cr
    \hbox to \leftmargini{\hss}\cr
    \@tempdimc=\textwidth
    \advance\@tempdimc-\leftmargini
    \advance\@tempdimc-\psframesep
    \advance\@tempdimc-\psframesep
    \advance\@tempdimc-\pslinewidth
    \advance\@tempdimc-\pslinewidth
    \hbox{\psframebox{\BVerbatimInput[frame=none,numbersep=2mm,
                                      boxwidth=\@tempdimc]{\jobname.tmp}}}\cr}}%
  \endtrivlist}

\@namedef{MEx*}{\gridtrue\MEx}
\@namedef{endMEx*}{\endMEx}


\def\FV@BProcessLine#1{%
\hbox\ifx\FV@boxwidth\relax\else to\FV@boxwidth\fi{%
\FV@LeftListNumber
\FV@LeftListFrame
\FancyVerbFormatLine{#1}\hss
\FV@RightListFrame
\FV@RightListNumber}}

\def\BUseVerbatim{\FV@Command{}{BNumbersUseVerbatim}}

\def\FVC@BNumbersUseVerbatim#1{\FV@CheckIfSaved{#1}{\FV@BNumbersUseVerbatim}}

\def\FV@BNumbersUseVerbatim#1{\FV@BNumbersVerbatimBegin#1\FV@BVerbatimEnd}

\def\FV@BNumbersVerbatimBegin{%
  \begingroup
    \FV@UseKeyValues
    \FV@BeginVBox
    \let\FV@ProcessLine\FV@BNumbersProcessLine
    \FV@FormattingPrep
    \FV@ObeyTabsInit}%

\def\FV@BNumbersProcessLine#1{%
\global\advance\FV@CodeLineNo\@ne
\FV@StepLineNo
\FV@BProcessLine{#1}}


%%%%%%%%%%%%%%%%%%%% Logos %%%%%%%%%%%%%%%%%%%%

\def\b@LaTeX{%
  \begingroup
    \csname reset@font\endcsname
    \rmfamily\bfseries
    L\kern-.32em
    \raise.36ex\hbox{%
      \ifx\@currsize\Huge\LARGE\else
      \ifx\@currsize\huge\Large\else
      \ifx\@currsize\LARGE\large\else
      \ifx\@currsize\Large\normalsize\else
      \ifx\@currsize\large\small\else
      \ifx\@currsize\normalsize\footnotesize\else
      \ifx\@currsize\small\scriptsize\else
      \ifx\@currsize\footnotesize\scriptscriptsize
      \fi\fi\fi\fi\fi\fi\fi\fi
      \textbf{A}}%
    \kern-.15em
    T\kern-.1667em
    \lower.7ex\hbox{E}\kern-.125em
    X%
  \endgroup}
\def\bLaTeX{\b@LaTeX}


\@ifundefined{BibTeX}
   {\def\BibTeX{{\rmfamily B\kern-.05em{\sc i\kern-.025em b}\kern-.08em%
    T\kern-.1667em\lower.7ex\hbox{E}\kern-.125emX}}}{}
\@ifundefined{ConTeXt}
   {\def\ConTeXt{{\rmfamily Con\TeX t}}}{}
%DG \@ifundefined{SliTeX}
%DG    {\def\SliTeX{{\rmfamily S\kern-.06emL\kern-.18em\raise.32ex\hbox
%DG                 \textsc{i}\kern -.03em\TeX}}}{}
\@ifundefined{PlainTeX}{\def\PlainTeX{\textsc{Plain}\kern2pt\TeX}}{}

\def\reflect#1{{\setbox0=\hbox{#1}\rlap{\kern0.5\wd0
  \special{x:gsave}\special{x:scale -1 1}}\box0 \special{x:grestore}}}
\def\XePersian{\leavevmode$\smash{\hbox{X\lower.5ex
  \hbox{\kern-.125em\reflect{E}}Persian}}$}

%%%%%%%%%%%%%%%%%%%% Quick reference %%%%%%%%%%%%%%%%%%%%



% An entry in \quickref is of the form: \qentry{type}{entry}
\def\quickref{}
\long\def\addtoquickref#1#2{%
  \expandafter\gdef\expandafter\quickref\expandafter{%
  \quickref\qentry{#1}{#2}}}
\def\qentry#1{\@nameuse{qr@#1}}

\long\def\addtoquickref#1#2{}

\def\MakeQuickRef{%
  \long\def\addtoquickref##1##2{%
    \wlog{}%
    \toks@{\qentry{##1}{##2}}%
    \message{\the\toks@}%
    \wlog{}%
    \let\MakeQuickRef\relax}}

%% For switching into landscape mode for the Quick Reference:
\let\NormalShipout\shipout
\newbox\OutputBox
\def\LandscapeMode{%
  \long\def\shipout##1##2{%
    \setbox\OutputBox=\hbox{\rput[tl]{90}{\vbox{##2}}}%
    \NormalShipout\box\OutputBox}%
  \voffset 9in}

% This makes the Quick Reference:
\def\QuickReference{%
  \begin{Landscape}{\paperwidth}{\paperheight}{\rotateleft}
    \oddsidemargin 0pt
    \evensidemargin 0pt
    \topmargin -.5in
    \textheight 6.5in
    \textwidth 9in
    \columnsep 60pt
    \footskip 0pt
    \footheight 0pt
    \headheight 15pt
    \headsep 40pt
    \parskip 4pt plus 1pt minus 1pt
    \UsePageParameters
    \twocolumn
    \def\label##1{}
    \subsechang=-.25in
    \pagenumbering{Roman}
    \pagestyle{quickref}
    \MacroDefCodes
    \let\Main\string
    \let\MainFont\UsageFont
    \MainFont
    \quickref
  \end{Landscape}}
\let\QuickReference\relax

\def\ps@quickref{%
  \def\@oddhead{%
    \large\bfseries\sffamily
    \hbox to .5\textwidth{\XePersian\ v\Version\hfill}%
    \hbox to \z@{\hss Quick Reference\hss}%
    \hbox to .5\textwidth{\hfill\thepage}}
  \let\@evenhead\@oddhead
  \def\@oddfoot{}\def\@evenfoot{}}

% For page numbering in the QR:
\setbox\pst@boxg=\hbox{\UsageFont 99}
\newdimen\refnumdim
\refnumdim=\wd\pst@boxg
\def\refnum#1{\hbox to \refnumdim{\hss\rmfamily\bfseries\pageref{#1}}}

% Section commands that add an entry to the QR:
\def\Section#1{\addtoquickref{section}{#1}\@checksec\section{#1}}
\def\Subsection#1{\addtoquickref{section}{#1}\subsection{#1}}

% This is just an error checker:
\def\@checksec{%
  \def\@tempa{document}%
  \ifx\@tempa\@currenvir\else
    \@latexerr{\string\Section\space occurred inside
      \@currenvir\space environment}\@ehc
    \expandafter\end\expandafter{\@currenvir}%
    \expandafter\@checksec
  \fi}

% The qr@ definitions:
\def\qr@@object#1{\mbox{\begingroup\escapechar`\+%
  \refnum{\string#1}\endgroup\hskip 1em\string#1}}
\def\qr@object#1{\qr@@object#1\par}
\def\qr@@param#1=#2\@nil{\refnum{\string#1}}
\def\qr@param#1{\leavevmode\hbox to\hsize{\qr@@param#1\@nil\hskip 1em#1}\par}
\def\qr@section#1{\subsection*{#1}}
\long\def\qr@center#1{\begin{center}#1\end{center}}

%%%%%%%%%%%%%%%%%%%% Parts %%%%%%%%%%%%%%%%%%%%

\font\bighelv=phvb at 88bp


\def\part{\clearpage\leavevmode\@ifstar{\part@ii}{\part@i}}
\def\part@i#1{%
  \refstepcounter{part}%
  \addcontentsline{toc}{part}{\protect\pnumberline{\thepart}#1}%
  \lower 20pt\hbox to \z@{\hss{\bighelv\Roman{part}}\hskip 2em}%
  \part@iii{#1}}
\def\part@ii#1{\addcontentsline{toc}{part}{#1}\part@iii{#1}}
\def\part@iii#1{%
\markright{#1}{\sffamily\Huge\bfseries#1}\par\vskip 2cm\@afterheading}
\def\thepart{\Roman{part}}
\def\pnumberline#1{Part #1\hspace{1cm}}

\def\l@part#1#2{%
  \addpenalty{\@secpenalty}%
  \addvspace{\lpartskip}%
  \begingroup
    \@tempdima 1em
    \parindent \z@ \rightskip \@pnumwidth
    \parfillskip -\@pnumwidth
    {\large\bfseries\leavevmode#1\hfil \hbox to\@pnumwidth{\hss #2}}\par
    \nobreak
  \endgroup}
\newskip\lpartskip
\lpartskip 1.25em plus 1pt minus 1pt

%%%%%%%%%%%%%%%%%%%% Parameter definitions and indexing %%%%%%%%%%%%%%%%%%%%

% Like \MainEnvIndex, etc., but for parameters:
\def\MainParIndex{\SpecialIndex{parameter}{main}}
\def\UsageParIndex{\SpecialIndex{parameter}{usage}}

% Like \MainPar, but for parameters. Can optionally include =foo.
\def\MainPar#1{\@MainPar#1==\@nil}
\def\@MainPar#1=#2=#3\@nil{%
  \MainParIndex{#1}%
  \label{#1}%
  {\MainFont #1\ifx#3=\relax=\MetaFont#2\fi}}

% Analogous to \n, but optionally argument can include =foo.
\def\p#1{\@p#1==\@nil}
\def\@p#1=#2=#3\@nil{%
  \UsageParIndex{#1}%
  {\UsageFont #1\ifx#3=\relax=#2\fi}}

% For listing parameter definitions in a {description} environment:
\def\pitem[#1]{\@ifnextchar({\@pitem#1}{\@@pitem#1\@nil}}
\def\@pitem#1=#2(#3){%
  \item[\MainFont#1=\MetaFont#2]%    % Set the definition in \MainFont
  \MainParIndex{#1}%
  \label{#1}%
  \addtoquickref{param}{#1={\MetaFont #2}\hfill {\rmfamily Default:} #3}%
  \hfill {\MainFont {\rmfamily Default:} #3}\par}
\def\@@pitem#1=#2\@nil{%
  \@ifundefined{default@#1}%
    {\@warning{Parameter `#1' default not defined.}\@eha\@pitem#1=#2()}
    {\@pitem#1=#2(\csname default@#1\endcsname)}}


\begingroup
  \catcode`\@=12
  \catcode`\{=12
  \catcode`\}=12
  \catcode`\&=1
  \catcode`\^=2
  \def\psset@&%
    \begingroup
    \catcode`\\=12
    \pssetxx^
  \def\pssetxx#1{#2}&%
    \expandafter\gdef\csname default@#1\endcsname&#2^%
    \endgroup^
  \csname @input\endcsname&defaults.pst^
\endgroup

% For putting parameter definitions in an Ex environment:
\def\Par#1{\@ifnextchar({\@Par#1}{\@@Par#1\@nil}}
\def\@Par#1=#2(#3){%
  \leavevmode
  \MainParIndex{#1}%
  \label{#1}%
  \addtoquickref{param}{#1={\MetaFont #2}\hfill {\rmfamily Default:} #3}%
  \hbox to\linewidth{%
    \MainFont #1={\MetaFont#2}\hfill {\rmfamily Default:} #3}\par}
\def\@@Par#1=#2\@nil{%
  \@ifundefined{default@#1}%
    {\@warning{Parameter `#1' default not defined.}\@Par#1=#2()}
    {\@Par#1=#2(\csname default@#1\endcsname)}}




\begingroup
  \catcode`\(=13
  \catcode`\,=13
  \catcode`\==13
  \gdef\MacroDefCodes{%
    \@MD
    \catcode`\(13
    \catcode`\^^M12
    \catcode`\`13
    \catcode`\,13
    \catcode`\!1
    \catcode`\@2
    \def~{}%
    \def={{\fontshape{n}\selectfont\string=}}%
    \def,{{\fontshape{n}\selectfont\string,}}%
    \def(##1){\string({\MetaFont ##1})}}
\endgroup

\begingroup
  \catcode`\`=13 \gdef`#1'{{\OpArgFont #1}}
\endgroup

\begingroup
  \catcode`\^^M=12\relax%
  \gdef\GetMacroDef#1{\begingroup\MacroDefCodes\@GetMacroDef{#1}}%
  \gdef\@GetMacroDef#1#2^^M{#1{#2}\endgroup}%
\endgroup

% For listing macro definitions in a "description" environment:
\def\mitem{\GetMacroDef\@mitem}
\def\@mitem#1{%
  \item[{\MainFont\Main#1}]%
  \addtoquickref{object}{#1}%
  \leavevmode\par}

% For listing graphics object definitions in a {description} environment:
% The optional *[par] is inserted automatically.
\def\oitem{\GetMacroDef\@oitem}
\def\@oitem#1{\@@oitem#1\@nil}
\def\@@oitem#1#2\@nil{\@mitem{#1{\OpArgFont*[{\MetaFont par}]}#2}}

% For generic macro definition, usually to be put in a displayed env.
\def\object{\GetMacroDef\@object}
\def\@object#1{\addtoquickref{object}{#1}{\MainFont\Main#1\@@par}}

% For a single displayed macro definition:
\def\Mac{\GetMacroDef\@Mac}
\def\@Mac#1{\begin{Ex}\@object{#1}\end{Ex}}


%% We must hack the generation of indexes modified by `hyperref'
\def\@@wrindex#1|#2|#3\\{%
  \ifx\\#2\\%
    \protected@write\@indexfile{}{%
%      \string\indexentry{#1|hyperpage}{\thepage}%
      \string\indexentry{#1}{\thepage}%
    }%
  \else
    \def\Hy@temp@A{#2}%
    \ifcase0\ifx\Hy@temp@A\HyInd@ParenLeft 1\fi
            \ifx\Hy@temp@A\HyInd@ParenRight 1\fi
            \relax
      \protected@write\@indexfile{}{%
        \string\indexentry{#1|#2}{\thepage}%
      }%
    \else
      \protected@write\@indexfile{}{%
         \string\indexentry{#1|#2hyperpage}{\thepage}%
      }%
    \fi
  \fi
  \endgroup
  \@esphack
}

%%%%%%%%%%%%%%%%%%%% Indexes %%%%%%%%%%%%%%%%%%%%

\def\PrintUserIndex{%
  \clearpage
  \columnsep=20pt
  \advance\oddsidemargin\sechang
  \advance\evensidemargin\sechang
  \advance\textwidth-\sechang
  \sechang=0pt
  \onecolumn
  \@input{\jobname.ind}}

%%%%%%%%%%%%%%%%%%%% Special indexing for User's Guides %%%%%%%%%%%%%%%%%%%%

\@ifundefined{actualchar}{\def\actualchar{@}}{}
\@ifundefined{quotechar}{\def\quotechar{"}}{}
\@ifundefined{levelchar}{\def\levelchar{!}}{}
\@ifundefined{encapchar}{\def\encapchar{|}}{}
\@ifundefined{verbatimchar}{\def\verbatimchar{+}}{}

%% These are the indexing commands:

%% Commands:       \MainIndex        \UsageIndex
%% Environments:   \MainEnvIndex     \UsageEnvIndex
%% Style options:  \MainOptionIndex  \UsageOptionIndex
%% Counters:       \MainCntIndex     \UsageCntIndex

\def\@Index#1{\@bsphack\index{#1}\@esphack}
\def\Index#1#2{%
  \@bsphack
  \index{\expandafter\@gobble\string#2\actualchar\string
  \verb\quotechar*\verbatimchar\string#2\verbatimchar
  \encapchar #1}}
{\catcode`\!=0 !catcode`!\=12!gdef!special@gobble#1{!if\#1!else#1!fi}}
\def\MainIndex{\Index{main}}
\def\UsageIndex{\Index{usage}}
\def\SpecialIndex#1#2#3{%
  \@Index{#3\actualchar{\ttfamily #3} (#1)\encapchar #2}%
  \@Index{#1s:\levelchar{\ttfamily #3}\encapchar #2}}
\def\MainEnvIndex{\SpecialIndex{environment}{main}}
\def\UsageEnvIndex{\SpecialIndex{environment}{usage}}
\def\MainOptionIndex{\SpecialIndex{option}{main}}
\def\UsageOptionIndex{\SpecialIndex{option}{usage}}
\def\MainCntIndex{\SpecialIndex{counter}{main}}
\def\UsageCntIndex{\SpecialIndex{counter}{usage}}

%% Font for index page numbers:
\def\main#1{\textbf{\hyperpage{#1}}}
\def\usage#1{\hyperpage{#1}}

%% These combine various functions:

%% Commands:       \Main        \Usage
%% Environments:   \MainEnv     \UsageEnv
%% Style options:  \MainOption  \UsageOption
%% Counters:       \MainCnt     \UsageCnt

%% The "Main" ones
%%   * Index argument.
%%   * Print argument with \MainFont
%%   * Create label +foo, e+foo or o+foo.

%% The "Usage" ones
%%   * Index argument.
%%   * Print argument with \UsageFont.

\def\SpecialLabel#1#2+{\label{+#2}}
\def\LabelIndex#1{\MainIndex{#1}\expandafter\SpecialLabel\string#1+}
\def\Main#1{%
  \MainIndex{#1}%
  \expandafter\SpecialLabel\string#1+%
  {\MainFont\string#1}}
\def\Usage#1{\UsageIndex{#1}{\UsageFont\string#1}}
\def\MainEnv#1{%
  \MainEnvIndex{#1}%
  \label{e+#1}%
  {\MainFont #1}}
\def\UsageEnv#1{\UsageEnvIndex{#1}{\UsageFont #1}}
\def\MainOption#1{%
  \MainOptionIndex{#1}%
  \label{o+#1}%
  {\MainFont #1}}
\def\UsageOption#1{\UsageOptionIndex{#1}{\UsageFont #1}}
\def\MainCnt#1{%
  \MainCntIndex{#1}%
  \label{c+#1}%
  {\MainFont #1}}
\def\UsageCnt#1{\UsageCntIndex{#1}{\UsageFont #1}}

%% These are abbreviations.
\def\N{\Main}
\def\n{\Usage}
\def\E{\MainEnv}
\def\e{\UsageEnv}
\def\O{\MainOption}
\def\o{\UsageOption}
\def\C{\MainCnt}
% The \c macro was already defined for coordinates
%\def\c{\UsageCnt}


%% User's guide and documented code have different index prologue.
\def\PrintUserIndex{{\let\ps@plain\@empty\@input{\jobname.ind}}}

%% This is for listing commands in a description environment.
\def\xitem#1{\item[\Main{#1}]}

\begingroup
  \catcode`\<=1 \catcode`\>=2
  \catcode`\{=13 \catcode`\}=12
  \catcode`\[=13
  \gdef\@MD<%
    \catcode`\{=13 \catcode`\}=12 \catcode`\[=13
    \def{##1}<\{<\MetaFont ##1\/>\}>%
    \def[##1]<\string[<\MetaFont ##1\/>]>>
\endgroup
\def\MD{%
  \NotNested
  \Ex
    \begingroup
      \@MD
      \def\end{\par\endgroup\end}%
      \def\\{\@centercr\Main}%
      \MainFont
      \Main}
\def\endMD{\endEx}

%%%%%%%%%%%%%%%%%%%% Framed warnings %%%%%%%%%%%%%%%%%%%%

\newbox\bombbox
\setbox\bombbox=\hbox to\z@{%
  \hss
  \rput[r](-1.5cm,0){%
    \psset{unit=7pt}
    \begin{pspicture}(-4,-4)(4,4)
      \pscircle[fillcolor=gray,fillstyle=solid]{4}
      \rput{45}{\psscaleboxto(6,0){\textsf{XE}\kern-1pt}}
      \psellipse*(0,3.8)(1,.2)
      \psellipse*(0,5.5)(1,.2)
      \psframe*(-1,3.8)(1,5.5)
      \psbezier(0,5.5)(0,7)(1,8)(3.5,8)
      \multido{\r=-0+40}{9}{\rput{\r}(3.5,8){\psline(0,0)(.4,0)}}
    \end{pspicture}}}

\def\Warning{%
  \trivlist\item[]
  \leavevmode\kern\@totalleftmargin
  \valign\bgroup%
    \vfil##\vfil\cr
    \copy\bombbox\cr
    \minipage{\textwidth}
    \quote
    \emph{Warning:}\hskip 1em
    \ignorespaces}
\def\endWarning{\endquote\endminipage\cr\egroup\endtrivlist}

\newbox\filebox
\setbox\filebox=\hbox{%
  \pspicture(-1,-.5)(1,.7)
  \pspolygon[linearc=2pt,shadow=true,shadowangle=45,xunit=1.1]%
    (-1,-.55)(-1,.5)(-.8,.5)(-.8,.65)(-.2,.65)(-.2,.5)(1,.5)(1,-.55)
   \endpspicture}
\def\File#1{%
  \leavevmode
  \marginpar{%
    \leavevmode\lower 20pt\hbox{%
      \hbox to0pt{\hbox to \linewidth{\hss\copy\filebox\hss}\hss}%
      \raise 10pt\hbox to \linewidth{\hss\large\bfseries\sffamily #1\hss}}}%
   \ignorespaces}
\def\drivers{\noindent\small Driver notes:\hskip 1em\ignorespaces}
\endinput
