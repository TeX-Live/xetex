
\input csmac % Makra pro �e�tinu
\pageheight=9.5in \fullpageheight=9.8in  \setpage
%\nocon % omit table of contents
\datethis % print date on listing

\def\begitems{\medskip\bgroup\catcode`\*=13 \narrower\narrower}
\def\enditems{\par\egroup\medskip}
{\catcode`\*=13 \gdef*{\par\noindent\llap{$\bullet$\ }\ignorespaces}}


@* PROGRAM VLNA.
Program �te vstupn� textov� soubor a nahrazuje za specifikovan�mi
jednop�smenn�mi slovy (nap�.~v, k, u) mezery symbolem \uv{\.{\char126}}. To
zabr�n� p�i n�sledn�m zpracov�n� \TeX{}em zlomit ��dek na nevhodn�ch
m�stech, kter� jsou v rozporu s typografickou normou.

Program sest�v� z t�chto hlavn�ch celk�:
@c
@<Hlavi�kov� soubory k na�ten�@>@/
@<Glob�ln� deklarace@>@/
@<Pomocn� funkce@>@/
@<Vlnkovac� funkce |tie|@>@/
@<Hlavn� program@>

@ Definujeme |BANNER|, co� je text, kter� se objevi p�i startu
programu a obsahuje ��slo verze programu. 
Zde je n�zorn� vid�t, �e m�ch�n� dvou jazyk� se nevyhneme. P�i tisku
text� na termin�l nesm�me p�edpokl�dat, �e tam budou �esk� fonty.
V~t�to dokumentaci se setk�me se t�emi jazyky: angli�tinou (v�t�inou
v~k�du programu, cestinou v~/* koment���ch */ a �e�tinou jinde.
Tu cestinu si vynutil fakt, �e DOS-ovsk� varianta \.{tangle} a
\.{weave} se nesn�� s~akcentovan�mi p�smeny v~/* koment���ch */.
A~nyn� u� sl�ben� (v�cejazy�n�) |BANNER|.
@d BANNER "This is program vlna, version 1.2, (c) 1995, 2002 Petr Olsak\n"

@ V programu jsou pou�ity knihovn� funkce, jej�ch� prototypy jsou
definov�ny ve t�ech standardn�ch hlavi�kov�ch souborech.
@<Hlavi�kov� ...@>=
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

@ Definujeme konstanty pro n�vratov� k�d. |OK| pro �sp�n� b�h,
|WARNING| p�i v�skytu aspo� jedn� varovn� zpr�vy, |IO_ERR| pro chybu
v~p��stupu ke vtupn�m nebo v�stupn�m soubor�m, |BAD_OPTIONS| pro
syntaktickou chybu na p��kazov� ��dce a |BAD_PROGRAM| pro p��pad
hav�rie programu. Ta by nem�la nikdy nastat. Prom�nn� |status| bude
obsahovat n�vratov� k�d a prom�nn� |prog_name| bude ukazovat na text
nult�ho parametru p��kazov� ��dky.
@d OK 0
@d WARNING 1
@d IO_ERR 2
@d BAD_OPTIONS 3
@d BAD_PROGRAM 4
@<Glob�ln� deklarace@>=
char *prog_name;
int status;

@ Z�kladn� rozvr�en� funkce |main|.
@<Hlavn� program@>=
int main (argc,argv)
    int argc;
    char **argv;
{
  @<Lok�ln� prom�nn� funkce |main|@>;
  prog_name=argv[0]; status = OK;
  @<Na�ten� parametr� p��kazov�ho ��dku@>;
  if (!silent) fprintf (stderr, BANNER);
  @<Inicializace datov�ch struktur@>;
  @<Zpracov�n� soubor�@>;
  return status;
}

@* Parametry p��kazov�ho ��dku.
Program �te z~p��kazov�ho ��dku postupn� (nepovinn�) parametry,
kter� za��naj� znakem \uv{\.{-}}. Pak n�sleduj� jm�na vstupn�ch a v�stupn�ch
soubor�.
\begitems
* \.{-f} \dots\ program pracuje jako filtr (viz sekce |@<Zpracov�n�
  soubor�@>|). Nen�-li tento parametr pou�it, program pracuje v tzv.
  standardn�m re�imu, kdy jednotliv� soubory jsou vstupn� i v�stupn�.
* \.{-s} \dots\ program nevyp�e |BANNER|, ani sumarizaci, ani varov�n�,
  p�i nich� nen� program p�ed�asn� ukon�en.  V�echny tyto v�pisy
  sm��uj� do |stderr|, tak�e pokud program pracuje v re�imu \uv{filtr},
  nen� nutn� tento parametr pou��t.
* \.{-r} \dots\ program ma�e pracovn� soubor (soubory), kter� vytv���
  ve standardn�m re�imu (tj. nen� pou�it \.{-f}). V re�imu filter nem�
  tento parametr vliv.
* \.{-v} \dots\ parametr definuje skupinu p�smen, kter� budou
  interpretov�ny jako neslabi�n� p�edlo�ky.
  Nap�. \.{-v KkSsVvZzOoUuAI}. Pokud nen� parametr uveden, je pou�ita
  skupina uveden� v tomto p��klad�.
* \.{-m} \dots\ program neprov�d� kontrolu math/text m�d�, tj. vlnkuje i
  uvnit� matematick�ho m�du \TeX{}u. (Implicite tam nevlnkuje).
* \.{-n} \dots\ prorgram neprov�d� kontrolu verbatim m�du, tj. vlnkuje i
  uvnit� verbatim m�du definovan�m b�n�mi prost�ed�mi. Imlicite ve
  verbatim prost�ed� nevlnkuje.
* \.{-l} \dots\ La\TeX{} re�im. P�i kontrole text-math-verbatim m�d� jsou
  br�ny v �vahu dal�� sekvence, obvykl� v La\TeX{}ov�ch dokumentech.
* \.{-w} \dots\ WEB re�im. Ohrani�en� verbatim m�du je dopln�no znaky
  pou��van�mi v dokumentech WEB (nap�. tento dokument). D�sledek: program
  vlnkuje dokumenta�n� ��st ka�d� sekce, ale nikoli k�d.
\enditems

Definujeme funkci |printusage|, kter� tiskne (p�i chyb�) stru�n� p�ehled
mo�n�ch parametr�. Nepoda�ilo se mi zjistit, jak se ve WEBu nap�e
kulturn� dlouh� string obsahuj�c� \.{\char92n} s form�tovac�mi
po�adavky. Byl jsem nucen to takto nehezky zapsat.
@<Pomocn� funkce@>=
void printusage ()
{
  fprintf(stderr,
    "usage: vlna [opt] [filenames]\n"
    "  opt -f :  filter mode: file1 file2 ... file1->file2\n"
    "                         file1       ... file1->stdout\n"
    "                                     ... stdin->stdout\n"
    "            nofilter: file1 [file2 file3 ...] all are in/out\n"
    "      -s :  silent: no messages to stderr\n"
    "      -r :  rmbackup: if nofilter, removes temporary files\n"
    "      -v charset :  set of lettres to add tie, default: KkSsVvZzOoUuAI\n" 
    "      -m :  nomath: ignores math modes\n"
    "      -n :  noverb: ignores verbatim modes\n"
    "      -l :  LaTeX mode\n"
    "      -w :  web mode\n");
}

@ Prom�nn� |isfilter|, |silent|, |rmbackup|, |nomath|, |noverb|,
|latex|, resp. |web| ��kaj�, �e je nastaven parametr \.{-f}, \.{-s},
\.{-r}, \.{-m}, \.{-n}, \.{-l}, resp. \.{-w}.  Prom�nn� |charset|
ukazuje bu� na implicitn� skupinu znak� |charsetdefault|, nebo (p�i
pou�it� parametru \.{-v}) na text uveden� v p��kazov�m ��dku. 
@<Glob�ln� deklarace@>=
int isfilter=0, silent=0, rmbackup=0, nomath=0, noverb=0, web=0, latex=0;
char charsetdefault[]="KkSsVvZzOoUuAI";
char *charset=charsetdefault;

@ @<Na�ten� parametr� ...@>=
while (argc>1 && argv[1][0] == '-') {
  if (argv[1][2] != 0) printusage (), exit (BAD_OPTIONS);
  switch(argv[1][1]) {
  case 'f': isfilter = 1; break;
  case 's': silent = 1; break;
  case 'r': rmbackup = 1; break;
  case 'v': argv++; argc--; charset = argv[1]; break;
  case 'm': nomath = 1; break;
  case 'n': noverb = 1; break;
  case 'l': latex = 1; break;
  case 'w': web = 1; break;
  default: printusage (), exit (BAD_OPTIONS);
          /* nezn\'am\'y parametr */
  }
  argc--; argv++;
}

@* Zpracov�n� soubor�.  Parametr |MAXLEN| definuje maxim�ln� mo�nou
d�lku jm�na souboru, kter� vytvo��me jako p�echodn�, nebo z�lohov�.
D�le deklarujeme prom�nn� typu \uv{stream}.
@d MAXLEN 120
@<Lok�ln� prom�nn� funkce...@>=
FILE *infile, *outfile;
char backup[MAXLEN];
int j;

@ Definujeme funkci pro v�pis chybov�ho hl�en� p�i ne�sp�n�m otev�en�
souboru.
@<Pomocn� funkce@>=
void ioerr (f)
  char *f;
{
   fprintf(stderr, "%s: cannot open file %s\n", prog_name, f);
}

@ Zp�sob zpracov�n� soubor� rozli��me podle re�imu dan�m p�ep�na�em \.{-f}.
@<Zpracov�n� soubor�@>=
if (isfilter)  @<Zpracov�n� v re�imu filter@> @/
else   @<Zpracov�n� v�ech soubor� p��kazov� ��dky@>

@ V re�imu |isfilter==1| je dal�� zpracov�n� z�visl� na po�tu soubor� v
p��kazov� ��dce:
\begitems
* nula soubor� -- vstup je |stdin| a v�stup je |stdout|,
* jeden soubor -- je vstupn�, v�stup je |stdout|,
* dva soubory -- prvn� je vstupn�, druh� v�stupn�,
* v�ce soubor� -- program skon�� s chybou.
\enditems
@<Zpracov�n� v re�imu filter@>=
{
  if (argc > 3) printusage (), exit (BAD_OPTIONS) ;
  infile = stdin; outfile = stdout;
  if (argc >= 2) infile = fopen (argv[1], "r");
  if (infile == NULL)  ioerr (argv[1]), exit (IO_ERR);
  if (argc == 3) outfile = fopen(argv[2], "w");
  if (outfile == NULL) ioerr (argv[2]), exit (IO_ERR);
  if (argc >= 2) filename = argv[1];
  else filename = NULL;
  tie (infile, outfile);
  if (outfile != stdout) fclose (outfile);
  if (infile != stdin) fclose (infile);
}

@ V~re�imu |isfilter==0| jsou jednotliv� soubory v~p��kazov�m ��dku
interpretov�ny jako vstupn� i v�stupn�. V�ce soubor� v~p��kazov�m ��dku m�
stejn� efekt, jako opakovan� vol�n� programu na jednotliv� soubory.
V~\UNIX/u lze tedy nap�. napsat \.{\jobname\ *.tex} a program dopln� vlnky do
v�ech soubor� s~p��ponou~\.{tex}. Toto neplat� v~DOSu, proto�e interpretace
masky je v~\UNIX/u starost� shellu a nikoli programu samotn�ho. N� program
masku nebude interpretovat. Je-li v~tomto re�imu nulov� po�et soubor�,
program se ukon�� s~chybou. 
@<Zpracov�n� v�ech soubor� p��kazov� ��dky@>=
{
  if (argc==1) printusage (), exit(BAD_OPTIONS);
  while (argc>1) {
     argc--; argv++;
     @<P�ejmenuj vstup |argv[0]| na |backup| a otev�i jej jako |infile|@>;
     if (infile == NULL) {
       ioerr (argv[0]); continue;
     }
     outfile = fopen (argv[0], "w");
     if (outfile == NULL) {
       ioerr (argv[0]);
       rename (backup, argv[0]); 
       status = WARNING; 
       continue;
     }
     filename = argv[0];
     tie (infile, outfile);
     fclose (outfile), fclose (infile);
     if (rmbackup) remove (backup);
   }
}

@ P�i |isfilter==0| program p�ejmenuje ka�d�  zpracov�van� soubor tak, �e
zm�n� posledn� p�smeno n�zvu souboru na znak \.{\char126}. Tento
p�ejmenovan� soubor bude otev�en jako vstupn� a v�stupem bude p�vodn�
soubor. Vstupn� soubor p�i |rmbackup==0| z�stane zachov�n jako z�loha.

Pro� vlnku nep�id�v�me na konec n�zvu souboru, ale m�n�me ji za posledn�
znak souboru? Proto�e chceme, aby program fungoval i v tak nemo�n�ch
syst�mech, jako je DOS.
@<P�ejmenuj vstup...@>=
infile = NULL;
j = strlen (argv[0]) - 1;
if (j >= MAXLEN || argv[0][j] == '~') {
   if (!silent) fprintf (stderr, "%s: the conflict of file name %s\n",
      prog_name, argv[0]);
}
else {
  strcpy (backup, argv[0]);
  backup[j] = '~';
  remove (backup);
  j = rename (argv[0], backup);
  if (j == 0) infile = fopen (backup, "r");
}

@* Patterny.  Abychom mohli ��eln� definovat chov�n� programu
v~r�zn�ch situac�ch, zavedeme datovou strukturu |PATTERN|. Zhruba
�e�eno, budeme sledovat vstup znak po znaku a pokud bude ��st vstupu
souhlasit s~definovan�m patternem, provedeme n�mi po�adovanou
akci. Nap��klad nej�ast�j�� aktivitu, p�id�n� vlnky uvnit� ��dku,
spust�me v~okam�iku, kdy vstupn� text odpov�d� patternu \uv{\.{\ (v\
p}}, kde \uv{\.{\ }} znamen� jedna nebo v�ce mezer a tabel�tor�,
\uv{\.{(}} je nula nebo v�ce otev�rac�ch z�vorek v�eho druhu,
\uv{\.{v}} znamen� jedno p�smeno z~mno�iny p�edlo�ek (viz |charset|) a
\uv{\.{p}} zde znamen� libovoln� p�smeno. P��klad zde nen� zcela p�esn�.
P�esn� jsou v�echny patterny pro n� program definov�ny v~z�v�re�n�ch
sekc�ch tohoto pov�d�n�.

Pattern bude znamenat kone�nou sekvenci tzv. pozic patternu (|PATITEM|).
Cykly uvnit� pozic pro jednoduchost nep�ipust�me. Ka�d� pozice obsahuje
�et�zec znak�, uva�ovan� pro danou pozici (v~p��kladu pozice~\uv{\.{\ }} by
obsahovala mezeru a tabel�tor, zat�mco pozice \.{v} odpov�d� |charset|).
Ka�d� pozice m� sv�j p�ep�na� (|flag|), kter� obsahuje informaci o~tom,
zda shodu testovan�ho znaku s~n�kter�m prvkem v~mno�in� znak�
budeme pova�ovat za �sp�ch �i ne�sp�ch a zda pozice se ve zkouman�m
�et�zci m��e vyskytovat pr�v� jednou nebo opakovan�. Jako druh� p��pad
sta�� implementovat \uv{nula nebo v�ce} proto�e \uv{jedna nebo v�ce} lze
popsat pomoc� dvou pozic, prvn� \uv{pr�v� jednou} a n�sleduj�c� \uv{nula
nebo v�ce}. Jednotliv� pozice jsou z�et�zeny ukazatelem |next|, posledn�
pozice m� |next==NULL|. Stejn� tak jednotliv� patterny budeme
sestavovat do seznam� a budou rovn� z�et�zeny ukazatelem |next|.

Pattern krom� �et�zu pozic obsahuje ukazatel na funkci (proceduru) |proc|,
kter� se m� vykonat v~p��pad�, �e testovan� �et�zec vyhovuje patternu. 

@d ONE      1        /* flag: prave jeden vyskyt */
@d ANY      2        /* flag: nula nebo vice */
@d ONE_NOT -1        /* flag: prave jednou, znak nesmi byt v mnozine */
@d ANY_NOT -2        /* flag: nula nebo vice, znak nesmi byt v mnozine */

@<Glob�ln� deklarace@>=
typedef struct PATITEM {     /* jedna pozice patternu */
   char *str;                /* seznam znaku na teto pozici */
   int flag;                 /* vyznam seznamu znaku */
   struct PATITEM *next ;    /* nasledujici pozice patternu */
} PATITEM;
typedef struct PATTERN {     /* jeden pattern */
   PATITEM *patt;            /* ukazatel na prvni pozici */
   void (*proc)();           /* procedura spustena pri souhlasu patternu */
   struct PATTERN *next ;    /* nasledujici v seznamu vsech patternu */
} PATTERN;

@ Deklarujeme n�kter� glob�ln� prom�nn� pro pr�ci s~patterny. |lapi| je pole
obsahuj�c� ukazatele na aktu�ln� pozice v~otev�en�ch patternech. ��k�me,
�e \uv{pattern je otev�en}, pokud zkouman� �et�zec s~n�m {\it za��n�\/}
souhlasit. Pattern se uzav�e, pokud nastane jedna ze dvou mo�nost�:
zkouman� �et�zec s~m�m souhlas� a� do konce (v~takov�m p��pad� se provede
procedura |proc|), nebo p�i vy�et�ov�n� dal��ch znak� ze zkouman�ho
�et�zce p�estane �et�zec s~patternem souhlasit.

V~dan� chv�li m��e b�t pattern otev�en n�kolikr�t. Nap�. pattern \.{abac}
je p�i stringu \.{aba} p�i v�skytu druh�ho \.{a} otev�en podruh�. Proto
pole obsahuje ukazatele na pr�v� aktu�ln� pozici patternu a nikoli na
pattern jako takov�.

V~poli |lapi| budou na po��tku sam� |NULL| (to se p�i p�ekladu inicializuje
samo) a p�emaz�n� ukazatele na pozici konstantou |NULL| budeme pova�ovat
za zav�en� patternu. Vedle pole |lapi| soum�rn� udr�ujeme pole |lapt|,
do n�ho� budeme ukl�dat ukazatele na odpov�daj�c� otev�en� pattern. Tuto
informaci pou�ijeme v~p��pad�, �e pot�ebujeme nap�, zn�t |proc|
patternu.

|listpatt| bude ukazovat na za��tek aktu�ln�ho seznamu pattern�. Seznamy
budeme m�t dva. Jeden se pou�ije, nach�z�me-li se mimo koment�� a druh�
v~p��pad�, �e se nach�z�me v~prostoru \TeX{}ovsk�ho koment��e (tj. za
procentem). Starty t�chto seznam� pattern� jsou |normallist| a
|commentlist| a aktivn� |listpatt| m� v�dy jednu z~t�chto dvou hodnot.

Prom�nn� |lastpt| a |lastpi| pou�ijeme pro budov�n� �et�zov� struktury
pattern�.

Prom�nn� |c| obsahuje pr�v� testovan� znak ze vstupu (kter� se rovn�
p�ep�e do bufferu |buff|). Z~bufferu ob�as ukl�d�me data do v�stupn�ho
proudu. D�l�me to ale v�dy jen v~okam�iku, kdy nen� otev�en ��dn�
pattern. Tehdy toti� \uv{nehroz�} situace, �e by n�jak� procedura vyvolan�
souhlasem patternu po�adovala v~tomto bufferu n�jak� zm�ny se zp�tnou
platnost�. O~vypr�zdn�n� bufferu se za�neme zaj�mat a� v~okam�iku, kdy je
zapln�n aspo� na hodnotu |BUFI|, abychom proceduru p�episu bufferu do
v�stupn�ho proudu neaktivovali zbyte�n� �asto.
@d MAXPATT 200       /* maximalni pocet patternu */
@d MAXBUFF 500      /* velikost bufferu pro operace */
@d BUFI 300         /* velikost stredniho zaplneni */
@<Glob�ln� deklarace@>=
PATITEM *lapi[MAXPATT];      /* pole ukazatelu na aktualni pozice */
PATTERN *lapt[MAXPATT];      /* pole odpovidajicich ukazatelu na patterny */
PATTERN *listpatt, *normallist, *commentlist, *pt, *lastpt=NULL;
PATITEM *pi, *lastpi=NULL;
char c;             /* zrovna nacetny znak */
char buff[MAXBUFF]; /* prechodny buffer */
int ind;            /* aktualni pozice prechodneho bufferu */

@ Nyn� definujeme pomocn� funkce |setpattern|, |setpi| a |normalpattern|.
Tyto funkce alokuj� pam� pomoc� standardn� funkce |malloc|. Abychom mohli
ohl�dat p��padnou chybu p�i alokaci, budeme allokovat pam� zprost�edkovan�
pomoc� funkce |myalloc|.
@<Pomocn� funkce@>=
void *myalloc (size)
  int size;
{
  void *p;
  p = malloc (size);
  if (p == NULL)
  {
    fprintf (stderr, "%s, no memory, malloc failed\n", prog_name);
    exit (BAD_PROGRAM) ;
  }
  return p;
}

@ Funkce |setpattern| alokuje pam�ov� m�sto struktury |PATTERN| a napoj�
ji pomoc� prom�nn� |lastpt| na u� alokovan� �et�z pattern�. 
Vr�t� ukazatel na nov� alokovan� m�sto. Jednotliv� pozice patternu se mus�
n�sledovn� alokovat pomoc� |setpi|.
@<Pomocn� funkce@>=
PATTERN *setpattern (proc) @/
  void (*proc)();
{
  PATTERN *pp;
  pp = myalloc (sizeof (PATTERN));
  pp->proc = proc;  
  pp->next = NULL;
  pp->patt = NULL;
  if (lastpt != NULL) lastpt->next = pp;
  lastpt = pp;
  lastpi = NULL;
  return pp;
}

@ Funkce |setpi| alokuje pam�ov� m�sto pro jednu pozici patternu. Provede
z�et�zen� tak, aby prvn� pozice �et�zu pozic byla zaznamen�na v polo�ce
|patt| ve struktu�e |PATTERN| a dal�� byly prov�z�ny polo�kou |next| ve
struktu�e |PATITEM|. Posledn� pozice m� |next==NULL|.
@<Pomocn� funkce@>=
void setpi (str, flag)
  char *str;
  int flag;
{
  PATITEM* p;
  p = myalloc (sizeof (PATITEM));
  p->str = str; p->flag = flag;
  p->next = NULL;
  if (lastpi == NULL) lastpt->patt = p;
  else lastpi->next = p;
  lastpi = p;
}

@ P�ipravme si p�du pro funkci |normalpattern|. Tato funkce alokuje
strukturu pro jeden pattern v�etn� pozic patternu na z�klad� vstupn�ho
stringu. Ka�d� pozice patternu obsahuje v~mno�in� znak� jedin� znak a m�
|flag=ONE|. Znaky ve vstupn�m stringu odpov�daj� po �ad� jednotliv�m
pozic�m. Vytvo�� se vlastn� jak�si absolutn� pattern, tj. testovan� �et�zec
se mus� p�esn� shodovat s~uveden�m stringem. V�jimku tvo�� znak |"."|,
kter� se interpretuje jako nula nebo v�ce mezer. Chceme-li te�ku
vnutit do patternu, nap�eme dv� te�ky za sebou.

Nejd��ve deklarujeme pole v�ech mo�n�ch jednop�smenn�ch string�.
@<Glob�ln� deklarace@>=
char strings[512];
int i;

@ Inicializujeme toto pole (znak, nula, znak, nula, atd...).
@<Inicializace datov�ch struktur@>=
for (i=0; i<256; i++) {
  strings[2*i] = (char) i; strings[2*i+1] = 0;
}

@ Definujme funkci |normalpattern|.
@<Pomocn� funkce@>=
PATTERN *normalpattern (proc, str) @/
  void (*proc)();
  char *str;
{
  PATTERN *pp;
  int j=0;
  pp = setpattern (proc);
  while (str[j]) {
    if (str[j]=='.') {
      j++;
      if (str[j]!='.') {
        setpi (blankscr, ANY); 
        continue;
      }
    }  
    setpi (&strings[(unsigned char)str[j]*2], ONE);
    j++;
  }
  return pp;
}

@ Funkce |match|. Definujeme funkci, kter� na z�klad� hodnoty znaku |c|
(prom�nn� |c| je definov�na jako glob�ln�), a pozice patternu |p| (parametr
funkce) vr�t� informaci o tom, zda znak souhlas� s patternem. Z�porn� ��sla
|FOUND|, resp. |NOFOUND| znamenaj�, �e je t�eba uzav��t pattern s t�m, �e
vzor odpov�d�, resp. neodpov�d� patternu. Nez�porn� ��slo vr�t� v p��pad�,
�e zkouman� vstup st�le souhlas� s patternem, ale nen� je�t�
rozhodnuto. Velikost n�vratov� hodnoty v takov�m p��pad� ud�v�, o kolik
pozic je t�eba se posunout v patternu, abychom m�li ukazatel na pozici
patternu v souhlase s novou situac�, zp�sobenou znakem |c|.

Pokud je |c| v mno�in� znak� pro danou pozici |p->str|, bude |m==1|, jinak
je |m==-1|. Pokud t�mto ��slem pron�sob�me hodnotu |p->flag|, nemus�me
v�tven� podle |p->flag| programovat dvakr�t. Hodnoty |flag| jsou toti�
symetrick� podle nuly, nap�. |ANY==-ANY_NOT|.
@d FOUND   -1
@d NOFOUND -2
@<Pomocn� funkce@>=
int match (p)
  PATITEM *p;
{
  int m;
  if (strchr (p->str, c) != NULL) m = 1;  /* Znak nalezen */
  else m = -1;                            /* Znak nenalezen */
  switch (m * p->flag) {
  case ANY: return 0;                  /* Souhas, neni nutny posun */
  case ONE: if (p->next == NULL) return FOUND;
            return 1;                    /* Souhas, nutny posun o 1 */
  case ONE_NOT: return NOFOUND;          /* Nesouhlas */
  case ANY_NOT: @<Vra� hodnotu podle n�sleduj�c�...@>;
  }
  return 0; /* Tady bychom nikdy nemeli byt, return pro potlaceni varovani */
}

@ O kolik pozic je t�eba se posunout a s jak�m v�sledkem zjist�me
rekurzivn�m vol�n�m funkce |match|.
@<Vra� hodnotu podle n�sleduj�c� pozice patternu@>=
switch (m = match (p->next)) {
case NOFOUND: return NOFOUND;
case FOUND: return FOUND;
default: return 1 + m;
}

@* Vlnkovac� funkce.
Nejprve p�iprav�me glob�ln� deklarace pro \uv{vlnkovac�} funkci |tie|.
Funkce |tie| \uv{ovlnkuje} vstupn� soubor |infile| a vytvo�� soubor
|outfile|.  P�i |silent=0| tiskne z�v�re�nou zpr�vu o zpracov�n�. V t�to
zpr�v� se objev� jm�no souboru, kter� se funkce \uv{dozv�} prost�ednictv�m
glob�ln� prom�nn� |filename|. Prom�nn� |numline| po��t� ��dky, prom�nn�
|numchanges| s��t� zm�ny, tj. po�et dopln�n�ch vlnek. 
Prom�nn� |mode| nab�v� n�kter� z hodnot |TEXTMODE|, |MATHMODE|,
|DISPLAYMODE| a |VERBMODE| podle stavu ve �ten�m textu.
@d TEXTMODE 0
@d MATHMODE 1
@d DISPLAYMODE 2
@d VERBMODE 3
@<Glob�ln� deklarace@>=
char *filename;     /* jmeno zpracovavaneho souboru */
long int numline, numchanges;   /* pro zaverecnou statistiku */
int mode;   

@ Nyn� definujeme vlnkovac� funkci |tie|. Ve�ker� �innost se op�r� o
strukturu pattern�. V�hodn� je (z d�vodu rychlosti) \uv{natvrdo} zde
implementovat jen p�ep�n�n� mezi stavem �ten� z oblasti koment��e
(|listpatt==commentlist|) a mimo koment�� (|listpatt==normallist|);
@<Vlnkovac� funkce |tie|@>=
void tie (input, output)
  FILE *input, *output;
{
  int ap;  /* ap je pocet otevrenych patternu */
  register int k, m, n;
  int ic;
  PATTERN *pp;
  PATITEM *pi;

  @<Inicializace prom�nn�ch p�i startu funkce |tie|@>;

  while (!feof(input)) {
    @<Otev�i nov� patterny@>;
    if (ap == 0  && ind > BUFI && c !='\\') @<Vypr�zdni buffer@>;
    if (ind >= MAXBUFF) {
      fprintf (stderr, "Operating buffer overflow, is anything wrong?\n");
      exit (BAD_PROGRAM);
    }
    if ((ic = getc(input)) == EOF)  /* opravil Cejka Rudolf */
      break;
    buff[ind++] = c = ic;
    if (c == '\n') numline++, listpatt = normallist;
    if (c == '%' && mode!=VERBMODE && buff[ind-2] != '\\') listpatt = commentlist;
    @<Projdi otev�en� patterny@>;
  }
  @<Vypr�zdni buffer@>;
  if (!web) checkmode ();   /* zaverecna kontrola modu */
  if (!silent) @<Tiskni z�v�re�nou zpr�vu@>;
}

@ @<Inicializace prom�nn�ch p�i ...@>=
for (k=0; k<MAXPATT; k++) lapi[k] = NULL;
c = '\n';
buff[0] = mode = ap = 0;  ind = 1;
numline = 1; numchanges = 0;
mode = TEXTMODE;

@ P�i manipulaci s bufferem byl pou�it jeden trik. Ve�ker� na�ten� znaky
za��naj� a� od |buff[1]|, zat�mco |buff[0]| je rovno nule. Je to proto, �e
n�kter� algoritmy se vrac� o jeden znak zp�t za sv�j pattern, aby zjistily,
zda tam nen� symbol \uv{\.{\char92}} (nap��klad na v�skyt sekvence
\.{\char92\char37} je t�eba reagovat jinak, ne� na v�skyt oby�ejn�ho
procenta). Kdybychm zaza�ali od |buff[0]|, v n�kter�ch situac�ch
bychom se ptali, zda |buff[-1]=='\\'|, tj. sahali bychom na neo�et�en�
m�sto v pam�ti.
@<Vypr�zdni buffer@>=
{
  buff[ind] = 0;
  fputs (&buff[1], output);
  ind = 1;
}

@ P�i proch�zen� otev�en�mi patterny posunujeme v poli |lapi| pozice
jednotliv�ch pattern� podle pokyn� funkce |match|, p��padn� pattern zav�eme
a p��padn� vyvol�me proceduru patternu. 

N�kter� patterny v poli |lapi| u� mohou b�t zav�eny, tak�e je nutno s t�mto
polem pracovat jako s jak�msi d�rav�m s�rem.
@<Projdi otev�en� patterny@>=
n = ap; k = 0;
while (n) {
  while (lapi[k]==NULL) k++;  /* zastav se na prvnim ukazateli na pattern */
  switch (m = match (lapi[k])) {
  case FOUND:   (*lapt[k]->proc)();  /* Pattern nalezen, spustit proceduru */
  case NOFOUND: lapi[k] = NULL;  /* Deaktivace patternu */
                ap--; break;  
  default:  while (m--) lapi[k] = lapi[k]->next;  /* dalsi pozice patternu */
  }
  k++; n--;
}

@ P�i otev�r�n� nov�ch pattern�, kter� nejsou v tuto chv�li zablokov�ny,
se hned vypo��d�me s takov�mi patterny, kter� n�m d�vaj� rovnou odpov��
typu |FOUND| nebo |NOFOUND|. V takov�ch p��padech ani nezan��me ukazatel
na pozici do pole |lapi|.
@<Otev�i nov� patterny@>=
pp = listpatt;
while (pp != NULL) {
  switch (m = match (pp->patt)) {
  case FOUND:    (*pp->proc)();   /* spustit proceduru */
  case NOFOUND: break;
  default: @<Vytvo� ukazatel na nov� pattern a |break|@>;
  }
  pp=pp->next;
}

@ Nen�-li hned zn�ma odpov��, zda pattern vyhovuje �i nikoli,
p�ekontrolujeme nejd��ve, zda u� nen� pattern ve stejn� pozici otev�en�.
Pak najdeme prvn� \uv{d�ru} v tabulce |lapi| a tam uhn�zd�me nov� ukazatel
na pozici v patternu.
@<Vytvo� ukazatel na nov� pattern...@>=
pi = pp->patt;
while (m--) pi = pi->next;
n = ap;  k = 0;
while (n) {
  if (lapi[k]==pi) break;
  if (lapi[k++] != NULL) n--;
}
if (!n) {
  k = 0;
  while (lapi[k] != NULL) k++;
  if (k >= MAXPATT) {
    fprintf (stderr, "I cannot allocate pp, is anything wrong?\n");
    exit (BAD_PROGRAM);
  }
  lapt[k] = pp;  lapi[k] = pi; ap++;
}

@ Posledn� v�c� ve funci |tie| je tisk z�v�re�n� statistiky zpracov�n�.
@<Tiskni z�v�re�nou zpr�vu@>=
fprintf (stderr, "~~~ file: %s\t  lines: %ld, changes: %ld\n", 
   filename, numline,  numchanges);

@* Inicializace pattern�.
Po vytvo�en� p�edchoz�ho k�du op�raj�c�ho se o~patterny m�me nyn� v~ruce
pom�rn� siln� n�stroj na definov�n� r�zn�ch �innost� programu prost�m
vytvo�en�m patternu a p��slu�n� jeho procedury. Pokud budeme cht�t
v~budoucnu n�jak� rys programu p�idat, pravd�podobn� to bude snadn�.

Nejprve deklarujeme n�kter� �asto pou��van� skupiny znak� v~patternech.

@<Glob�ln� deklarace@>=
char tblanks[] = " ~\t";
char blanks[] =  " \t";
char blankscr[] = " \t\n";
char tblankscr[] = " ~\t\n";
char nochar[] = "%~\n";
char cr[] = "\n";
char prefixes[] = "[({~";
char dolar[] = "$";
char backslash[] = "\\";
char openbrace[] = "{";
char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
PATTERN *vlnkalist, *mathlist, *parcheck, *verblist ;

@ Za�neme definic� nej�ast�ji pou��van�ho patternu na vlnkov�n� uvnit�
��dku. P�ipome�me, �e opakovan� vol�n� funkce |setpattern| vytv��� intern�
seznam pattern�, p�i�em� o~jejich propojen� se nemus�me starat. Vyzvedneme
si z~n�vratov�ho k�du funkce pouze ukazatel na prvn� polo�ku seznamu
|normallist|. Stejn� tak opakovan� vol�n� funkce |setpi| vytv��� seznam
pozic pro naposledy deklarovan� pattern.
@<Inicializace datov�ch struktur@>=
vlnkalist = setpattern (vlnkain);
setpi (tblankscr, ONE);
setpi (tblanks,   ANY);
setpi (prefixes,  ANY);
setpi (charset,   ONE);
setpi (blanks,    ONE);
setpi (blanks,    ANY);
setpi (nochar,    ONE_NOT);

@ @<Inicializace prom�nn�ch p�i ...@>=
listpatt = normallist = vlnkalist;

@ @<Pomocn� funkce@>=
void vlnkain()
{
  char p;
  ind--;
  p = buff[ind--];
  while (strchr(blanks, buff[ind]) !=NULL) ind--;
  ind++;
  buff[ind++] = '~';
  buff[ind++] = p;
  numchanges++;
}

@ Podobn� pro tvorbu vlnky \uv{p�es ��dek} vytvo��me pattern a k�d
procedury.
@<Inicializace dat...@>=
setpattern (vlnkacr);
setpi (tblankscr, ONE);
setpi (tblanks,   ANY);
setpi (prefixes,  ANY);
setpi (charset,   ONE);
setpi (blanks,    ANY);
setpi (cr,        ONE);
setpi (blanks,    ANY);
setpi (nochar,    ONE_NOT);

@ V procedu�e k tomuto patternu mus�me o�et�it p��pad typu 
\uv{\.{a\char126v\char92np}},
kdy nelze prost� p�ehodit \uv{\.{\char92n}} za \uv{\.{v}}, proto�e 
bychom roztrhli
mezeru sv�zanou vlnkou u� d��ve. Proto mus�me vyhledat vhodn� m�sto pro
roztr�en� ��dku, kter� bude a� {\it p�ed\/} znakem \uv{\.{a}}. P�i d�sledn�m
o�et�en� tohoto fenom�nu m��eme dokonce narazit na situaci
\uv{\.{\char92n\ v\char126v\char126v\char92np}},  kde nem��eme vlo�it 
\uv{\.{\char92n}} p�ed prvn� v�skyt \uv{\.{v}}, proto�e bychom dostali 
\uv{\.{\char92n\char92n}}, tedy pr�zdn�
��dek. Ten je v \TeX{}u interperetov�n odli�n�. V t�to v�jime�n�
situaci pouze zru��me st�vaj�c� (v po�ad� druh�) \uv{\.{\char92n}} a
nebudeme vytv��et nov�. Na v�stupu bude soubor o jeden ��dek krat��.
@<Pomocn� funkce@>=
void vlnkacr()
{
  char p;
  int i, j;
  ind--;
  p = buff[ind--];
  while (strchr(blankscr, buff[ind]) !=NULL) ind--;
  i = ind;  /* misto predlozky, kterou chceme vazat */
  while (i >= 0 && (strchr(blankscr, buff[i]) == NULL)) i--;
  j = i;
  while (i >= 0 && (strchr(blanks, buff[i]) != NULL)) i--;
  if (i >= 0 && buff[i] == '\n') j = -1;
  if (j >= 0)  buff[j] = '\n';
  else numline--;
  ind++;
  buff[ind++] = '~';
  buff[ind++] = p;
  numchanges++;
}

@ Nyn� vytvo��me patterny pro p��pady typu \.{\char92uv\char`\{v lese\char`\}}.
@<Inicializace dat...@>=
setpattern (vlnkain);    /* na radku */
setpi (tblankscr, ONE);
setpi (backslash, ONE);
setpi (letters,   ONE);
setpi (letters,   ANY);
setpi (openbrace, ONE);
setpi (prefixes,  ANY);
setpi (charset,   ONE);
setpi (blanks,    ONE);
setpi (blanks,    ANY);
setpi (nochar,    ONE_NOT);

setpattern (vlnkacr);    /* pres radek */
setpi (tblankscr, ONE);
setpi (backslash, ONE);
setpi (letters,   ONE);
setpi (letters,   ANY);
setpi (openbrace, ONE);
setpi (prefixes,  ANY);
setpi (charset,   ONE);
setpi (blanks,    ANY);
setpi (cr,        ONE);
setpi (blanks,    ANY);
setpi (nochar,    ONE_NOT);



@ Vytvo��me patterny a proceduru pro potlat�en� tvorby vlnky u p�smen t�sn�
n�sleduj�c�ch sekvence \.{\char92TeX} a \.{\char92LaTeX}. Tj. nechceme, aby
nap� z textu \uv{\.{Vlastnosti~\char92TeX~u~jsou...}} jsme dostali text
s nespr�vn� v�zan�m p�smenem
\uv{\.{Vlastnosti~\char92TeX~u\char126jsou...}}.
@<Inicializace dat...@>=
normalpattern (tielock, "\\TeX");
setpi (blankscr, ONE);
normalpattern (tielock, "\\LaTeX");
setpi (blankscr, ONE);

@ Procedura |tielock| obsahuje ne�ist� trik. P�i prov�d�n� procedury je
pr�v� na�ten znak z |blankscr| a je ulo�en do buff. Testy na otev�r�n�
nov�ch pattern� pro tento znak teprve budou n�sledovat a testuj� se na
hodnotu prom�nn� |c|. Sta�� tedy zm�nit hodnotu |c| a vlnkovac� patterny se
neotev�ou.
@<Pomocn� funkce@>=
void tielock ()
{
  c = 1;
}

@ O�et��me nyn� p�echod do/z matematick�ho re�imu \TeX{}u. Uvnit� math
m�du vlnky ned�l�me. P�i zji�t�n�m nesouladu v p�echodech mezi
math-m�dy spust�me n�sleduj�c� proceduru.
@<Pomocn� funkce@>=
void printwarning ()
{
  if (!silent)
    fprintf (stderr, 
      "~!~ warning: text/math/verb mode mismatch,  file: %s,  line: %ld\n", 
      filename, numline - (c=='\n'?1:0));
  status = WARNING;
}

@ Za�neme patterny pro p�echod do/z matematick�ho re�imu, ohrani�en�ho
jedn�m dolarem, nebo v La\TeX{}u p��slu�n�mi sekvencemi.  Sekvence
La\TeX{}u \.{\char92(} a \.{\char92)} nejsou zahrnuty, proto�e b�vaj�
�asto p�edefinov�ny k jin�m u�ite�n�j��m v�cem.
@<Inicializace datov�ch ...@>=
if (!nomath) {
  mathlist = setpattern (onedollar);
  setpi (dolar, ONE);
  setpi (dolar, ONE_NOT);
  if (latex) {
    normalpattern (mathin, "\\begin.{math}");
    normalpattern (mathout, "\\end.{math}");
  }
}

@ @<Pomocn� funkce@>=
void mathin ()
{
  if (mode!=TEXTMODE) printwarning ();
  mode = MATHMODE;
  normallist = listpatt = mathlist;
}
void mathout ()
{
  if (mode!=MATHMODE) printwarning ();
  mode = TEXTMODE;
  normallist = listpatt = vlnkalist;
}

@ P�i programov�n� procedury |onedollar| nesm�me zapomenout na v�skyt
sekvence \.{\char92\$}. V tom p��pad� akci ignorujeme. Podobn� u sekvence
\.{\$\$} souhlas� ten druh� dolar s na��m patternem, ale to u� jsme uvnit�
display m�du. V takov�m p��pad� tak� nic ned�l�me.
@<Pomocn� funkce@>=
void onedollar ()
{
  if (buff[ind-3]=='\\' || (buff[ind-3]=='$' && buff[ind-4]!='\\')) return;
  if (mode==DISPLAYMODE) printwarning ();
  else {
    if (mode==TEXTMODE) mathin();
    else mathout();
  }
}

@ Pokud najdeme pr�zdn� ��dek, p�ekontrolujeme, zda n�hodou nejsme v
math-m�du. Pokud ano, vyp�eme varov�n� a p�ejdeme do textov�ho m�du.
@<Inicializace dat...@>=
parcheck = setpattern (checkmode);
setpi (cr, ONE);
setpi (blanks, ANY);
setpi (cr, ONE);

@ @<Pomocn� funkce@>=
void checkmode ()
{
  if (mode!=TEXTMODE) {
    printwarning ();
    mode = TEXTMODE;
    normallist = listpatt = vlnkalist;
  }
}

@ Nyn� o�et��me v�skyt dvou dolar�, tj. vstup do/z display m�du.
Rovn� mysleme na La\TeX{}isty a jejich prost�ed� pro display-m�d. Proto�e
je mo�n� alternativa s hv�zdi�kou na konci n�zvu prost�ed�, rad�ji u�
uzav�rac� z�vorku do patternu nezahrnujeme.

@<Inicializace dat...@>=
if (!nomath) {
  normalpattern (twodollars, "$$");
  if (latex) {
    normalpattern (displayin, "\\begin.{displaymath");
    normalpattern (displayin, "\\begin.{equation");
    normalpattern (displayout, "\\end.{displaymath");
    normalpattern (displayout, "\\end.{equation");
  }
}

@ @<Pomocn� funkce@>=
void displayin ()
{
  if (mode!=TEXTMODE) printwarning ();
  mode = DISPLAYMODE; normallist = listpatt = parcheck;
}
void displayout ()
{
  if (mode!=DISPLAYMODE) printwarning();
  mode = TEXTMODE; normallist =  listpatt = vlnkalist;
}
void twodollars ()
{
  if (buff[ind-3]=='\\') return;
  if (mode==DISPLAYMODE) displayout ();
  else displayin ();
}

@ N�sleduje o�et�en� tzv. verbatim m�du. Pro plain i La\TeX{} jsou nej�ast�j��
z�vorky pro verbatim mod tyto (variantu s \.{\char92begtt} pou��v�m
s oblibou j�).
@<Inicializace dat...@>=
if (!noverb) {
  verblist = normalpattern (verbinchar, "\\verb"); 
  setpi (blankscr, ANY);
  setpi (blankscr, ONE_NOT); 
  normalpattern (verbin, "\\begtt"); 
  if (latex) normalpattern (verbin, "\\begin.{verbatim");
}
if (web) {
  normalpattern (verbin, "@@<");
  normalpattern (verbin, "@@d");
}
if (!noverb) {
  verboutlist[0] = setpattern (verbout);
  setpi (verbchar, ONE);
  verboutlist[1] = normalpattern (verbout, "\\endtt");
  if (latex) verboutlist[2] = normalpattern (verbout, "\\end{verbatim");
}
if (web) {
  verboutlist[3] = normalpattern (verbout, "@@ ");
  normalpattern (verbout, "@@*");
  normalpattern (verbout, "@@>|");
}


@ Procedura |verbinchar| se od \uv{spole�n�} procedury |verbin| li�� v
tom, �e zavede do stringu |verbchar| moment�ln� hodnotu prom�nn� |c|.
Proto druh� v�skyt t�to hodnoty verbatim re�im ukon��.
@<Pomocn� funkce@>=
int prevmode;
PATTERN *prevlist, *verboutlist[4];
char verbchar[2];
void verbinchar ()
{
  prevmode = mode;
  verbchar[0] = c;
  c = 1;
  listpatt = normallist = verboutlist[0];
  prevlist = listpatt->next;
  listpatt->next = NULL;
  mode = VERBMODE;
}

@ P�i programov�n� \uv{obecn�} funkce |verbin| mus�me db�t na to, aby
z�stal aktivn� pouze odpov�daj�c� \uv{v�stupn�} pattern k dan�mu
vstupn�mu. Tak� si zapamatujeme m�d, ze kter�ho jsme do verbatim
oblasti vstoupili, abychom se k n�mu mohli vr�tit (nap�. uvnit�
math. m�du m��e b�t
\.{\char92hbox} a v n�m lok�ln� verbatim konstrukce).
@<Pomocn� funkce@>=
void verbin ()
{ 
  int i;
  i = 0;
  prevmode = mode; 
  switch (c) {
  case 't': i = 1; break;
  case 'm': i = 2; break;
  case '<': ;
  case 'd': i = 3; 
       if (buff[ind-3]=='@@') return;  /* dvojity @@ ignorovat */ 
       break;
  }
  listpatt = normallist = verboutlist[i]; 
  prevlist = listpatt->next;
  if (c != '<' && c != 'd')  listpatt->next = NULL;
  mode = VERBMODE;
}

@ @<Pomocn� funkce@>=
void verbout ()
{
  if (mode!=VERBMODE) return;
  if (web && buff[ind-2] == '@@' && buff[ind-3] == '@@') return;
  mode = prevmode;
  normallist->next = prevlist;
  switch (mode) {
  case DISPLAYMODE: normallist = listpatt = parcheck; break;
  case MATHMODE: normallist = listpatt = mathlist; break ;
  case TEXTMODE:  normallist = listpatt = vlnkalist; break;
  }
}

@ Nyn� implementujeme vlastnost d��ve pou��van�ho programu vlnka, tj. �e
lze jeho �innost vypnout a op�t zapnout v koment���ch. Vytv���me druh�
nez�visl� seznam pattern� a proto nejprve pronulujeme |lastpt|.
@<Inicializace dat...@>=
lastpt = 0;
commentlist = normalpattern (tieoff, "%.~.-");
normalpattern (tieon, "%.~.+");

@ @<Pomocn� funkce@>=
void tieoff ()
{
  normallist = NULL;
}
void tieon ()
{
  normallist = vlnkalist;
}

@ Dal�� pl�novan� vylep�en�. Program by mohl ��st definici sv�ho chov�n�
nejen z~p��kazov� ��dky, ale v~mnohem kompletn�j�� podob�, v�etn�
u�ivatelsky definovan�ch pattern�, z koment��ov� oblasti ve �ten�m souboru.
Parametry zde uveden� by mohly m�t vy��� prioritu, ne� parametry 
z~p��kazov� ��dky a mohl by se t�eba roz�i�ovat seznam sekvenc�, za nimi�
p�smena nemaj� b�t v�zana vlnkou (zat�m je implemenov�no na pevno jen
\.{\char92TeX} a \.{\char92LaTeX}). 

@* Rejst��k.


