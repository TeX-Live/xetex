%% This file is xepersian-manual.cls 
%%
%% Copyright 2009 by Vafa Khalighi
%%
%%%% It may be distributed and/or modified under the
%% conditions of the LaTeX Project Public License, either version 1.3c
%% of this license or (at your option) any later version.
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{xepersian-manual}[2009/03/01 v0.1 Typesetting XePersian Manual in Persian in XeLaTeX (Author: Vafa Khalighi)]
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions
\LoadClass[a4paper,12pt,twoside]{article}
\PassOptionsToPackage{dvipsnames}{xcolor}
\RequirePackage[top=2.5cm,bottom=2.5cm,left=2.25cm,right=2.25cm]{geometry}
\RequirePackage{pstricks}
\RequirePackage{fmultico,soul,fancybox,colortab,framed,eso-pic}
\RequirePackage{pifont,calc,bbding}
\RequirePackage[version=latest]{pgf}
\RequirePackage{tikz}
\usetikzlibrary{%
  arrows,%
  calc,%
  fit,%
  patterns,%
  plotmarks,%
  shapes.geometric,%
  shapes.misc,%
  shapes.symbols,%
  shapes.arrows,%
  shapes.callouts,%
  shapes.multipart,%
  shapes.gates.logic.US,%
  shapes.gates.logic.IEC,%
  circuits.logic.US,%
  circuits.logic.IEC,%
  circuits.logic.CDH,%
  circuits.ee.IEC,%
  datavisualization,%
  datavisualization.formats.functions,%
  er,%
  fpu,%
  automata,%
  backgrounds,%
  chains,%
  topaths,%
  trees,%
  petri,%
  mindmap,%
  matrix,%
  calendar,%
  folding,%
  fadings,%
  shadings,%
  through,%
  turtle,%
  positioning,%
  scopes,%
  decorations.fractals,%
  decorations.shapes,%
  decorations.text,%
  decorations.pathmorphing,%
  decorations.pathreplacing,%
  decorations.footprints,%
  decorations.markings,%
  shadows,%
  lindenmayersystems,%
  intersections,%
  fixedpointarithmetic,%
  fpu,%
  svg.path}
\usetikzlibrary{snakes}
\RequirePackage{fancyhdr}
\RequirePackage{caption}
\captionsetup{
   margin = 10pt,
   font = {small,rm},
   labelfont = {small,bf},
   format = plain, % 
   indention = 0em,  % 
   labelsep = colon, %
   justification = RaggedRight, % 
   singlelinecheck = true, % 
   position = bottom %
}
\renewcommand\labelenumi{\theenumi}
\renewcommand\labelitemi{\HandLeft}
\renewcommand\labelitemii{\HandLeftUp}
\renewcommand\labelitemiii{\HandCuffLeft}
\renewcommand\labelitemiv{\HandPencilLeft}
\setlength\columnseprule{1pt}
\def\thanktext{%
\begin{persian}
متن قدردانی در این قسمت قرار می‌گیرد.
\end{persian}}
\newcommand\thank{
\@RTLfalse%
  \noindent
  \begin{tikzpicture}
     \draw (0,0)  node[circle,
                       shade,
                       ball color=myred,minimum size=2cm]{};
     \draw[snake      = expanding waves,%
           color      = myred,%
           line width = 3pt](1.2,0)--(4,0);
    \end{tikzpicture}\par\vspace{1 cm}
    \noindent
  \newlength{\@lx}\settowidth{\@lx}{\huge قدردانی}%
  \advance\@lx3em\quad
    \rotatebox{90}{\hspace*{1.5em}{\huge\bfseries\color{lightgray} قدردانی}}
  \textcolor{myred}{\rule{2pt}{\@lx}}\quad%
  \begin{minipage}[b]{.90\textwidth}
    {%
    \noindent\hfill
     \raggedleft\Large
     \textbf{\textcolor{myred}{قدردانی}}\par
    \bigskip\bigskip\par%
     }%
     \emph{\color{darkgray}\thanktext}\par
    \par\medskip
  \end{minipage}
}
\renewcommand\tableofcontents{%
  \begin{multicols}{2}[{\noindent\normalfont\Large\bfseries\color{darkgray} عناوین}]%
    \small
    \@starttoc{toc}%
\end{multicols}
\vfill
\thank\vfill
  \newpage\@RTLtrue}
\def\versionnumber#1{\gdef\@versionnumber{#1}}
\def\qedsymbol{\CheckmarkBold}
\def\myMathBox{\@ifnextchar[{\my@MBoxi}{\my@MBoxi[black]}}
 \def\my@MBoxi[#1]{\@ifnextchar[{\my@MBoxii[#1]}{\my@MBoxii[#1][white]}}
\def\my@MBoxii[#1][#2]#3#4{%
\par\noindent%
\fcolorbox{#1}{#2}{%
\parbox{\linewidth-\labelwidth-2\fboxrule-2\fboxsep}{#3}%
}%
 \parbox{\labelwidth}{%
 \begin{eqnarray}\label{#4}\end{eqnarray}%
}%
 \par%
 }
\def\MyBox{\pst@object{MyBox}}
\def\MyBox@i#1#2#3{\beginL{%
	\use@par                        % Local parameters
	\if@star\solid@star\fi          % Stared version
	\pssetlength{\pst@dimc}{-\psframesep}
	\psaddtolength{\pst@dimc}{-\psframesep}
	\psaddtolength{\pst@dimc}{-\pslinewidth}
	\psaddtolength{\pst@dimc}{-\pslinewidth}
	\pssetlength{\pst@dima}{#1}
	\psaddtolength{\pst@dima}{\pst@dimc}
	\pssetlength{\pst@dimb}{#2}
	\psaddtolength{\pst@dimb}{\pst@dimc}
	\setbox\z@\vbox to\pst@dimb{\hsize\pst@dima\sloppy\vfil#3\vfil}%
	\psframebox{\box\z@}%
}\endL}
\definecolor{Orange}{rgb}{1,.4,.2}
\definecolor{myblue}{rgb}{0.02,0.04,0.48}
\definecolor{lightblue}{rgb}{0.61,.8,.8}
\definecolor{myred}{rgb}{0.65,0.04,0.07}
\definecolor{mygreen}{rgb}{0,.43,0}
\definecolor{blendedred}{rgb}{0.9,0.1,0.1} 
\definecolor{blendedgreen}{rgb}{0.2,0.7,0.2}
\definecolor{darkgray}{gray}{0.3}
\definecolor{mygray}{gray}{0.6}
\definecolor{skugga}{gray}{0.7} 
\newlength{\skuggsep} 
\setlength{\skuggsep}{3pt} 
\newcommand{\skuggadtext}[3]{% 
\makebox[0pt][l]{% 
\rule{\skuggsep}{0pt}% 
\raisebox{-\skuggsep}{\textcolor{#2}{#3}}% 
}% 
\textcolor{#1}{#3}% 
}
\renewcommand\maketitle{\@RTLfalse%
\begin{titlepage}
\null
\AddToShipoutPicture*{%
\setlength\unitlength{1mm}
\put(0,0){\makebox(0,0)[lb]{\color{Peach!10}\rule{\paperwidth}{\paperheight}}}
\put(0,0){\makebox(0,0)[lb]{\color{Mahogany}\rule{40mm}{\paperheight}}}
\put(18,200){\rotatebox{90}{% 
\makebox(0,0)[r]{\fontsize{60}{60}\color{orange!50!white}%
\bfseries \selectfont{\rl{بستهٔ \lr{\XePersian}}}}}}%
\put(8,230){%
        \begin{tikzpicture}
         \draw (0,0)  node[circle,
                           shade,%
                           ball color=orange!50!white,
                           minimum size = 2 cm]{};
         \draw[snake      = expanding waves,%
               color      = orange!50!white,%
               line width = 3pt](0,-1.2)--(0,-4);
        \end{tikzpicture}}
    \put(60,80){%
    \begin{pgfpicture} 
\pgfmathdeclarerandomlist{color}{{Peach}{Orange}{Red}{Brown}} 
\foreach \a in {1,...,5}{% 
  \pgfmathrandominteger{\x}{1}{300} 
  \pgfmathrandominteger{\y}{1}{200} 
  \pgfmathrandominteger{\r}{20}{100} 
  \pgfmathrandominteger{\s}{5}{95} 
  \pgfmathrandominteger{\o}{50}{80}
  \pgfmathrandomitem{\c}{color} 
  \pgfdeclareradialshading{ballshading}{\pgfpoint{-10bp}{10bp}} 
  {color(0bp)=(\c!15!white); color(9bp)=(\c!75!white); 
   color(18bp)=(\c!70!black); color(25bp)=(\c!50!black); color(50bp)=(black)}
  \pgfsetfillopacity{\o/100} 
  \pgfpathcircle{\pgfpoint{+\x pt}{+\y pt}}{+\r pt} 
  \pgfshadepath{ballshading}{\s} 
  \pgfusepath{}
}%
\end{pgfpicture}}
\put(80,250){\makebox(0,0)[l]{%
\skuggadtext{Mahogany}{mygray}{\fontsize{50}{60}\selectfont{\rl{راهنمای فارسی}}}}}%
\put(130,40){\makebox(0,0)[r]{%
\fontsize{17}{21}
\textcolor{Mahogany}{\selectfont{\rl{\textbf{مؤلف:}~وفا خلیقی}}}}}
\put(130,30){\makebox(0,0)[r]{%
\textcolor{Mahogany}{\rl{\textbf{نسخه:}~\@versionnumber}}}}}
\end{titlepage}
\setcounter{footnote}{0}%
}
\AtBeginDocument{\pagestyle{fancy} 
\fancyhf{}
\def\headrulewidth{0pt}
\fancyfoot[RO]{\psline[linewidth=1pt,linecolor=myred](-0.4,0.5)(-0.4,-2)\textcolor{darkgray}{\bfseries \thepage}} 
\fancyfoot[LE]{\textcolor{darkgray}{\bfseries \thepage}\psline[linewidth=1pt,linecolor=myred](0.4,0.5)(0.4,-2)}}


\endinput


