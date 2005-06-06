/*
 *  ChkTeX v1.5, finds typographic errors in (La)TeX files.
 *  Copyright (C) 1995-96 Jens T. Berger Thielemann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact the author at:
 *              Jens Berger
 *              Spektrumvn. 4
 *              N-0666 Oslo
 *              Norway
 *              E-mail: <jensthi@ifi.uio.no>
 *
 *
 */


#ifdef AMIGA
#  include <exec/execbase.h>
#  include "WB2Argv.h"
#endif

#ifdef WIN32
#include <getopt.h>
#else
#include "getopt.h"
#endif
#include "ChkTeX.h"
#include "OpSys.h"
#include "Utility.h"
#include "FindErrs.h"
#include "Resource.h"
#include <string.h>

#ifdef KPATHSEA
#include <kpathsea/config.h>
#include <kpathsea/progname.h>
#endif

#undef MSG
#define MSG(num, type, inuse, ctxt, text) {num, type, inuse, ctxt, text},

struct ErrMsg PrgMsgs [pmMaxFault + 1] =
{
    PRGMSGS
    {pmMaxFault, etErr, TRUE, 0, INTERNFAULT}
};

struct Stack
        CharStack       = {0L},
        InputStack      = {0L},
        EnvStack        = {0L};

/************************************************************************/

const TEXT BrOrder     [NUMBRACKETS + 1] = "()[]{}";

ULONG Brackets  [NUMBRACKETS];

/************************************************************************/


/*
 * Have to do things this way, to ease some checking throughout the
 * program.
 */


NEWBUF(TmpBuffer,       BUFSIZ);
NEWBUF(ReadBuffer,      BUFSIZ);

static const STRPTR
        Banner  =
"ChkTeX v1.5 - Copyright 1995-96 Jens T. Berger Thielemann.\n"
#ifdef __OS2__
"OS/2 port generated with emx compiler, by Wolfgang Fritsch, <fritsch@hmi.de>"
#elif defined(__MSDOS__)
"MS-DOS port by Bj\\o rn Ove Thue, <bjort@ifi.uio.no>"
#elif defined(KPATHSEA)
"Kpathsea port by Fabrice Popineau, <Fabrice.Popineau@supelec.fr>"
#endif
"\n",
        BigBanner  =
"ChkTeX comes with ABSOLUTELY NO WARRANTY; details on this and\n"
"distribution conditions in the GNU General Public License file.\n"
"Type \"ChkTeX -h\" for help, \"ChkTeX -i\" for distribution info.\n"
"Author: Jens Berger, Spektrumvn. 4, N-0666 Oslo, Norway.\n"
"E-mail: <jensthi@ifi.uio.no>\n"
"Press " STDIN_BREAK " to abort stdin input.\n",
        GiftBanner =
"\n"
"     If you like this program and use it frequently the author\n"
"      would like you to send him any gift that you feel would\n"
"      be an appropriate `payment' for `ChkTeX' --- thank you!\n"
"\n",
Distrib  =
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
,
OnText  =
"On",
OffText  =
"Off",
HowHelp =
"-h or --help gives usage information. See also ChkTeX.{ps,dvi}.\n",
HelpText  =
"\n"
"\n"
"                         Usage of ChkTeX v1.5\n"
"                         ~~~~~~~~~~~~~~~~~~~~\n"
"\n"
"                               Template\n"
"                               ~~~~~~~~\n"
"chktex [-hiqrW] [-v[0-...]] [-l <rcfile>] [-[wemn] <[1-42]|all>]\n"
"       [-d[0-...]] [-p <name>] [-o <outfile>] [-[btxgI][0|1]]\n"
"       file1 file2 ...\n"
"\n"
"----------------------------------------------------------------------\n"
"                       Description of options:\n"
"                       ~~~~~~~~~~~~~~~~~~~~~~~\n"
"Misc. options\n"
"~~~~~~~~~~~~~\n"
"    -h  --help      : This text.\n"
"    -i  --license   : Show distribution information\n"
"    -l  --localrc   : Read local .chktexrc formatted  file.\n"
"    -d  --debug     : Debug information. Give it a number.\n"
"    -r  --reset     : Reset settings to default.\n"
"\n"
"Muting warning messages:\n"
"~~~~~~~~~~~~~~~~~~~~~~~~\n"
"    -w  --warnon    : Makes msg # given a warning and turns it on.\n"
"    -e  --erroron   : Makes msg # given an error and turns it on.\n"
"    -m  --msgon     : Makes msg # given a message and turns it on.\n"
"    -n  --nowarn    : Mutes msg # given.\n"
"\n"
"Output control flags:\n"
"~~~~~~~~~~~~~~~~~~~~~\n"
"    -v  --verbosity : How errors are displayed.\n"
"                      Default 1, 0=Less, 2=Fancy, 3=lacheck.\n"
"    -V  --pipeverb  : How errors are displayed when stdout != tty.\n"
"                      Defaults to the same as -v.\n"
"    -s  --splitchar : String used to split fields when doing -v0\n"
"    -o  --output    : Redirect error report to a file.\n"
"    -q  --quiet     : Shuts up about version information.\n"
"    -p  --pseudoname: Input file-name when reporting.\n"
"    -f  --format    : Format to use for output\n"
"\n"
"Boolean switches (1 -> enables / 0 -> disables):\n"
"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
"    -b  --backup    : Backup output file.\n"
"    -x  --wipeverb  : Ignore contents of `\\verb' commands.\n"
"    -g  --globalrc  : Read global .chktexrc file.\n"
"    -I  --inputfiles: Execute \\input statements.\n"
"    -H  --headererr : Show errors found in front of \\begin{document}\n"
"\n"
"Miscellaneous switches:\n"
"~~~~~~~~~~~~~~~~~~~~~~~\n"
"    -W  --version   : Version information\n"
"\n"
"----------------------------------------------------------------------\n"
"If no LaTeX files are specified on the command line, we will read from\n"
"stdin.   For explanation of warning/error messages, please consult the\n"
"main document ChkTeX.dvi or ChkTeX.ps.\n";




#define CHAR(a)  a,

TEXT LTX_EosPunc[] = {LATEX_EOSPUNC 0};
TEXT LTX_GenPunc[] = {LATEX_GENPUNC 0};
TEXT LTX_SmallPunc[] = {LATEX_SMALLPUNC 0};

/*
 * Options we will set.
 *
 */

enum Quote Quote;

TEXT VerbNormal [] =
  "%k %n in %f line %l: %m\n"
  "%r%s%t\n"
  "%u\n";

#define DEF(type, name, value)  type name = value;
OPTION_DEFAULTS
STATE_VARS
#undef DEF

FILE *OutputFile = NULL;

STRPTR  PrgName;

BOOL   StdInTTY, StdOutTTY;

/*
 * End of config params.
 */

static int ParseArgs      __PROTO((int argc, char **argv));
static void ShowIntStatus __PROTO((void));
static BOOL OpenOut       __PROTO((void));
static int ShiftArg       __PROTO((STRPTR *Argument));


/*
 * Duplicates all arguments, and appends an asterix to each of them.
 */

static void AddStars(struct WordList *wl)
{
    ULONG Count, CmdLen;
    STRPTR  Data;

    FORWL(Count, *wl)
    {
        Data = wl->Stack.Data[Count];
        CmdLen = strlen(Data);
        if(Data[CmdLen - 1] != '*')
        {
            strcpy(TmpBuffer, Data);
            strcat(TmpBuffer, "*");
            InsertWord(TmpBuffer, wl);
        }
    }
}

/*
 * Sets up all the lists.
 *
 */

static void SetupLists(void)
{
    ULONG i;

    AddStars(&VerbEnvir);
    AddStars(&MathEnvir);

    MakeLower(&UserWarnCase);

    ListRep(&WipeArg, ':', 0);
    ListRep(&NoCharNext, ':', 0);

#define ThisItem ((STRPTR) AbbrevCase.Stack.Data[i])

    FORWL(i, AbbrevCase)
    {
        if(isalpha(ThisItem[0]))
        {
            ThisItem[0] = toupper(ThisItem[0]);
            InsertWord(ThisItem, &Abbrev);
            ThisItem[0] = tolower(ThisItem[0]);
        }
        InsertWord(ThisItem, &Abbrev);
    }
}

#define NOCOMMON(a,b) NoCommon(&a,#a,&b,#b)

/*
 * Checks that two lists don't have any common element.
 */

static void NoCommon(struct WordList *a, const STRPTR aName,
                     struct WordList *b, const STRPTR bName)
{
    ULONG i;
    
    FORWL(i, *a)
    {
        if(HasWord((STRPTR) a->Stack.Data[i], b))
            PrintPrgErr(pmNoCommon, a->Stack.Data[i], aName, bName);
    }
}

/*
 * Expands the tabs in a string to regular intervals sized
 * TSize.
 */

static void ExpandTabs(STRPTR From, STRPTR To, LONG TSize)
{
    STRPTR Next, Orig;
    ULONG Diff;

    Next = From;
    Orig = To;

    while((Next = strchr(From, '\t')))
    {
        if((Diff = Next - From))
        {
            strncpy(To, From, Diff);
            To += Diff;
            Diff = TSize - ((To - Orig) % TSize);
        }
        else
            Diff = TSize;

        memset(To, ' ', Diff);
        To += Diff;

        From = ++Next;
    }
    strcpy(To, From);
}


int main(int argc, char **argv)
{
  int    retval = EXIT_FAILURE, CurArg;
  ULONG  Count;
  BOOL   StdInUse = FALSE;
  STRPTR NameMatch = "";
  LONG  Tab = 8;

#ifdef KPATHSEA
   extern KPSEDLL char *kpse_bug_address;

   kpse_set_program_name (argv[0], "ChkTeX");
#endif

#ifdef __LOCALIZED
  InitStrings();
#endif

#ifdef AMIGA
  if(_WBenchMsg)
  {
      if(argv = WB2Argv(_WBenchMsg, W2A_LOWER))
          argc = CountArgv(argv);
      else
          exit(EXIT_FAILURE);
  }
#endif

  OutputFile = stdout;
  PrgName = argv[0];

#undef KEY
#undef LCASE
#undef LIST
#undef LNEMPTY
#define KEY(a, def)
#define LCASE(a)
#define LIST(a)
#define LNEMPTY(a) InsertWord("", &a);

  RESOURCE_INFO;
  
  while(SetupVars())
    ReadRC(ConfigFile);

  if(CmdLine.Stack.Used)
  {
      ParseArgs(CmdLine.Stack.Used, (STRPTR *) CmdLine.Stack.Data);
      CmdLine.Stack.Used = 1L;
  }
  
  if((CurArg = ParseArgs((ULONG) argc, argv)))
  {
      if(CmdLine.Stack.Used)
      {
          ParseArgs(CmdLine.Stack.Used, (STRPTR *) CmdLine.Stack.Data);
          CmdLine.Stack.Used = 1L;
      }
      
      if(!Quiet || LicenseOnly)
          fprintf(stderr, Banner);

      if(CurArg == argc)
	  UsingStdIn = TRUE;

#if defined(HAVE_FILENO) && defined(HAVE_ISATTY)
      StdInTTY = isatty(fileno(stdin));
      StdOutTTY = isatty(fileno(stdout));
#else
      StdInTTY =  StdOutTTY = TRUE;
#endif
      SetupTerm();

      if((UsingStdIn && StdInTTY && !Quiet) || LicenseOnly)
      {
          fprintf(stderr, BigBanner);
          if(!LicenseOnly)
              fprintf(stderr, GiftBanner);
      }

      if(!StdOutTTY && PipeOutputFormat)
	  OutputFormat = PipeOutputFormat;

      if(LicenseOnly)
      {
          fprintf(stderr, Distrib);
          fprintf(stderr, GiftBanner);
      }
      else
      {
          SetupLists();
          if(QuoteStyle)
          {
      	      if(!strcasecmp(QuoteStyle, "LOGICAL"))
	          Quote = quLogic;
	      elif(!strcasecmp(QuoteStyle, "TRADITIONAL"))
	          Quote = quTrad;
	      else
	      {
	          PrintPrgErr(pmQuoteStyle, QuoteStyle);
	          Quote = quTrad;
	      }
          }

          if(DebugLevel)
              ShowIntStatus();

          NOCOMMON(Italic, NonItalic);
          NOCOMMON(Italic, ItalCmd);
          NOCOMMON(LowDots, CenterDots);

          if(TabSize && isdigit(*TabSize))
              Tab = strtol(TabSize, NULL, 10);

          if(OpenOut())
          {
              for(;;)
              {
                  for(Count = 0; Count < NUMBRACKETS; Count++)
                      Brackets[Count] = 0L;

#define DEF(type, name, value) name = value;
                  STATE_VARS
#undef DEF

                  if(UsingStdIn) {
                      if(StdInUse)
                          break;
                      else {
                          StdInUse = TRUE;
                          PushFile("stdin", stdin, &InputStack);
                      }
                  } else {
                      if((CurArg <= argc) || NameMatch) {
                          ifn(NameMatch = MatchFileName(NULL)) {
                              if(CurArg < argc)
                                  NameMatch = MatchFileName(argv[CurArg++]);
                          }

                          if(!PushFileName(NameMatch, &InputStack))
                              break;
                      }
                  }

                  if(StkTop(&InputStack) && OutputFile)
                  {
                      while(!ferror(OutputFile) && StkTop(&InputStack) &&
                            !ferror(CurStkFile(&InputStack)) &&
                            FGetsStk(ReadBuffer, BUFSIZ-1, &InputStack))
                      {
                          
                          /* Make all spaces ordinary spaces */

                          strrep(ReadBuffer, '\n', ' ');
                          strrep(ReadBuffer, '\r', ' ');
                          ExpandTabs(ReadBuffer, TmpBuffer, Tab);
                          strcpy(ReadBuffer, TmpBuffer);

                          strcat(ReadBuffer, " ");
                          FindErr(ReadBuffer, CurStkLine(&InputStack));
                      }
                      
                      PrintStatus(CurStkLine(&InputStack));
                      retval = EXIT_SUCCESS;
                  }
              }
          }
      }
  }
  exit(retval);
  return retval;
}

/*
 * Opens the output file handle & possibly renames
 */

static BOOL OpenOut(void)
{
#ifdef __MSDOS__
    char *p;
#endif
    BOOL Success = TRUE;

    if(*OutputName)
    {
        if(BackupOut && fexists(OutputName))
        {
	    strcpy(TmpBuffer, OutputName);
            AddAppendix(TmpBuffer, BAKAPPENDIX);

            if(fexists(TmpBuffer))
                remove(TmpBuffer);

            if(!rename(OutputName, TmpBuffer))
                PrintPrgErr(pmRename,
                            OutputName, TmpBuffer);
            else
            {
                PrintPrgErr(pmRenameErr,
                            OutputName, TmpBuffer);
                Success = FALSE;
            }

        }
        
        if(Success)
        {
            ifn(OutputFile = fopen(OutputName, "w"))
            {
                PrintPrgErr(pmOutOpen);
                Success = FALSE;
            }
        }
    }
    else
        OutputFile = stdout;

    return(Success);
}

#ifndef STRIP_DEBUG
static void ShowWL(const STRPTR Name, const struct WordList *wl)
{
    ULONG i, j, percent;

    fprintf(stderr,
            "Name: %12s", Name);

    if(DebugLevel & FLG_DbgListInfo)
    {
        fprintf(stderr, ", MaxLen: %3ld, Entries: %3ld, ",
                wl->MaxLen, wl->Stack.Used);

        if(wl->Hash.Index && wl->Stack.Used)
        {
            for(i = j = 0;
                i < HASH_SIZE;
                i++)
            {
                if(wl->Hash.Index[i])
                    j++;
            }
            percent = (j * 10000)/wl->Stack.Used;


            fprintf(stderr, "Hash usage: %3ld.%02ld%%",
                    percent/100, percent%100);
        }
        else
            fprintf(stderr, "No hash table.");
    }

    fputc('\n', stderr);

    if(DebugLevel & FLG_DbgListCont)
    {
	FORWL(i, *wl)
            fprintf(stderr, "\t%s\n", (STRPTR) wl->Stack.Data[i]);
    }
}
#endif

#define BOOLDISP(var)           ((var)? OnText : OffText)
#define SHOWSTAT(text, arg)     fprintf(stderr, "\t" text ": %s\n", arg)
#define BOOLSTAT(name, var)     SHOWSTAT(name, BOOLDISP(var))
#define SHOWSTR(text, arg)      fprintf(stderr, "%s:\n\t%s\n", text, arg);


/*
 * Prints some of the internal flags; mainly for debugging purposes 
 */

static void ShowIntStatus(void)
{
#ifndef STRIP_DEBUG
    ULONG       Cnt;
    STRPTR      String, iuStr;

    if(DebugLevel & FLG_DbgMsgs)
    {
        fprintf(stderr, "There are %d warnings/error messages available:\n",
                emMaxFault - emMinFault - 1);

        for(Cnt = emMinFault + 1; Cnt < emMaxFault; Cnt++)
        {
            switch(LaTeXMsgs[Cnt].Type)
            {
            case etWarn: String = "Warning"; break;
            case etErr:  String = "Error"; break;
            case etMsg:  String = "Message"; break;
	    default:     String = ""; break;
            }

	    switch(LaTeXMsgs[Cnt].InUse)
            {
	    case iuOK:      iuStr = "In use"; break;
	    case iuNotUser: iuStr = "User muted"; break;
	    case iuNotSys:  iuStr = "System muted"; break;
	    }

            fprintf(stderr, "Number: %2ld, Type: %s, Status: %s\n"
                    "\tText: %s\n\n",
                    Cnt, String,
		    iuStr, LaTeXMsgs[Cnt].Message);
        }
    }

#undef KEY
#undef LCASE
#undef LNEMPTY
#undef LIST

#define LNEMPTY    LIST
#define LIST(a)    ShowWL(#a, &a);
#define LCASE(a)   LIST(a); LIST(a ## Case);
#define KEY(a,def) SHOWSTR(#a, a);


    if(DebugLevel & (FLG_DbgListInfo | FLG_DbgListCont))
    {
        RESOURCE_INFO;
    }

    if(DebugLevel & FLG_DbgOtherInfo)
    {
        SHOWSTR("Outputformat", OutputFormat);

        fprintf(stderr, "Current flags include:\n");

        BOOLSTAT("Read global resource", GlobalRC);
        BOOLSTAT("Wipe verbose commands", WipeVerb);
        BOOLSTAT("Backup outfile", BackupOut);
        BOOLSTAT("Quiet mode", Quiet);
        BOOLSTAT("Show license", LicenseOnly);
        BOOLSTAT("Use stdin", UsingStdIn);
        BOOLSTAT("\\input files", InputFiles);
        BOOLSTAT("Output header errors", HeadErrOut);
    }
#endif
}

/*
 * Resets all stacks.
 *
 */

#undef KEY
#undef LCASE
#undef LNEMPTY
#undef LIST

#define LNEMPTY     LIST
#define LIST(a)     ClearWord(&a);
#define LCASE(a)    LIST(a); LIST(a ## Case);
#define KEY(a,def)  a = def;

static void ResetStacks(void)
{
    RESOURCE_INFO;

}

/*
 * Resets all flags (not wordlists) to their default values. Sets
 * Outputfile to stdout.
 *
 */

static void ResetSettings(void)
{

#define DEF(type, name, value)  name = value;
OPTION_DEFAULTS
#undef DEF

    if(OutputFile != stdout)
    {
        fclose(OutputFile);
        OutputFile = stdout;
    }
}

/*
 * Reads a numerical argument from the argument. Supports concatenation
 * of arguments (main purpose)
 */

static int ParseNumArg(LONG *Dest,             /* Where to put the value */
		       LONG Default,           /* Value to put in if no in argue */
		       STRPTR *Argument)       /* optarg or similar */
{
    if(Argument && *Argument && isdigit(**Argument))
        *Dest = strtol(*Argument, Argument, 10);
    else
        *Dest = Default;

    return(ShiftArg(Argument));
}

/*
 * Same as above; however, will toggle the boolean if user doesn't
 * supply value
 */

static int ParseBoolArg(BOOL *Dest,            /* Boolean value */
			STRPTR *Argument)      /* optarg or similar */
{
    LONG        D = *Dest? 1L : 0L;
    int         Retval;
    
    Retval = ParseNumArg(&D, *Dest? 0L : 1L, Argument);
    
    *Dest = D ? TRUE : FALSE;

    return(Retval);
}

/*
 * Returns the first character in the string passed, updates the
 * string pointer, if the string is non-empty.
 *
 * 0 if the string is empty.
 */

static int ShiftArg(STRPTR *Argument)                  /* optarg or similar */
{
    if(Argument && *Argument && **Argument)
        return(*((STRPTR) (*Argument)++));
    else
        return 0;
}

/*
 * Parses an argv similar array.
 */

static int ParseArgs(int argc, char **argv)
{
    /* Needed for option parsing. */

static const
  struct option long_options[] =
  {
    {"help",       no_argument,       0L, 'h'},
    {"localrc",    required_argument, 0L, 'l'},
    {"output",     required_argument, 0L, 'o'},
    {"warnon",     required_argument, 0L, 'w'},
    {"erroron",    required_argument, 0L, 'e'},
    {"msgon",      required_argument, 0L, 'm'},
    {"nowarn",     required_argument, 0L, 'n'},
    {"verbosity",  optional_argument, 0L, 'v'},
    {"pipeverb",   optional_argument, 0L, 'V'},
    {"debug",      required_argument, 0L, 'd'},
    {"reset",      no_argument,       0L, 'r'},
    {"quiet",      no_argument,       0L, 'q'},
    {"license",    no_argument,       0L, 'i'},
    {"splitchar",  required_argument, 0L, 's'},
    {"format",     required_argument, 0L, 'f'},
    {"pseudoname", required_argument, 0L, 'p'},

    {"inputfiles", optional_argument, 0L, 'I'},
    {"backup",     optional_argument, 0L, 'b'},
    {"globalrc",   optional_argument, 0L, 'g'},
    {"wipeverb",   optional_argument, 0L, 'x'},
    {"tictoc",     optional_argument, 0L, 't'},
    {"headererr",  optional_argument, 0L, 'H'},
    {"version",    no_argument,       0L, 'W'},

    {0L,                0L,                     0L,      0L}
  };

  int   option_index = 0L, c, i, nextc, ErrType;

  int   Retval = FALSE, InUse;
  BOOL  Success, Foo;
  LONG  Err, Verb = 1, PipeVerb = 1;

  enum
  {
      aeNoErr = 0,
      aeArg,              /* missing/bad required argument */
      aeOpt,              /* unknown option returned */
      aeHelp,             /* just a call for help */
      aeMem               /* no memory */
  } ArgErr = aeNoErr;

  optind = 0;

  while(!ArgErr &&
        ((c = getopt_long((int) argc, argv,
                          "b::d:e:f:g::hH::I::il:m:n:o:p:qrs:t::v::V::w:Wx::",
                          long_options,
                          &option_index)) != EOF))
  {
      while(c) {
          nextc = 0;
          switch(c)
          {
          case 's':
              ifn(Delimit = strdup(optarg)) {
                  PrintPrgErr(pmStrDupErr);
                  ArgErr = aeMem;
              }

              break;
          case 'p':
              ifn(PseudoInName = strdup(optarg)) {
                  PrintPrgErr(pmStrDupErr);
                  ArgErr = aeMem;
              }

              break;

          case 'd':
#ifdef STRIP_DEBUG
	      PrintPrgErr(pmNoDebugFlag);
#else
              nextc = ParseNumArg(&DebugLevel, 0xffff, &optarg);
#endif
              break;
          case 'i':
              LicenseOnly = TRUE;
              
              nextc = ShiftArg(&optarg);
              break;
          case 'q':
              Quiet = TRUE;

              nextc = ShiftArg(&optarg);
              break;

          LOOP(warntype,
	    case 'w':
	        ErrType = etWarn;
	        InUse = iuOK;
                LAST(warntype);
	    case 'e':
	        ErrType = etErr;
	        InUse = iuOK;
                LAST(warntype);
	    case 'm':
	        ErrType = etMsg;
	        InUse = iuOK;
                LAST(warntype);
	    case 'n':
	        ErrType = etMsg;
	        InUse = iuNotUser;
                LAST(warntype);
              )

	      if(isdigit(*optarg))
	      {
		  nextc = ParseNumArg(&Err, -1, &optarg);
		  if(betw(emMinFault, Err, emMaxFault)) 
		  {
		      LaTeXMsgs[Err].Type  = ErrType;
		      LaTeXMsgs[Err].InUse = InUse;
		  } 
		  else 
		  {
		      ArgErr = aeOpt;
		      PrintPrgErr(pmWarnNumErr);
		  }
	      }
	      else if(!strcasecmp(optarg, "all"))
	      {
		  for(i = emMinFault + 1; i < emMaxFault; i++)
		  {
		      LaTeXMsgs[i].Type  = ErrType;
		      LaTeXMsgs[i].InUse = InUse;
		  }
	      }
	      else 
	      {
		  ArgErr = aeOpt;
		  PrintPrgErr(pmWarnNumErr);
	      }

              break;

          case 'g':
              nextc = ParseBoolArg(&GlobalRC, &optarg);
              if(!GlobalRC) {
		  ResetStacks();
              }
              break;
              
          case 'r':
              ResetSettings();
              nextc = ShiftArg(&optarg);
              break;
              
          case 'l':
              if(optarg)
                  ReadRC(optarg);
              break;
              
          case 'f':
              ifn(OutputFormat = strdup(optarg))
              {
                  PrintPrgErr(pmStrDupErr);
                  ArgErr = aeMem;
              }
              break;

          case 'v':
              nextc = ParseNumArg(&Verb, 2, &optarg);

	      if(Verb < (LONG) OutFormat.Stack.Used)
		  OutputFormat = strdup(OutFormat.Stack.Data[Verb]);
	      else
	      {
                  PrintPrgErr(pmVerbLevErr);
                  ArgErr = aeArg;
	      }
              break;
          case 'V':
              nextc = ParseNumArg(&PipeVerb, 1, &optarg);

	      if(PipeVerb < (LONG) OutFormat.Stack.Used)
		  PipeOutputFormat = strdup(OutFormat.Stack.Data[PipeVerb]);
	      else
	      {
                  PrintPrgErr(pmVerbLevErr);
                  ArgErr = aeArg;
	      }
              break;

          case 'o':
              if(optarg)
              {
                  if(*OutputName)
                  {
                      PrintPrgErr(pmOutTwice);
                      ArgErr = aeOpt;
                  }
                  else
                  {
                      ifn(OutputName = strdup(optarg))
                      {
                          PrintPrgErr(pmStrDupErr);
                          ArgErr = aeMem;
                      }
                  }
              }

              break;

          case 't':
              nextc = ParseBoolArg(&Foo, &optarg);
              break;
          case 'x':
              nextc = ParseBoolArg(&WipeVerb, &optarg);
              break;
          case 'b':
              nextc = ParseBoolArg(&BackupOut, &optarg);
              break;
          case 'I':
              nextc = ParseBoolArg(&InputFiles, &optarg);
              break;
          case 'H':
              nextc = ParseBoolArg(&HeadErrOut, &optarg);
              break;
	  case 'W':
	      printf(Banner);
	      exit(EXIT_SUCCESS);
          case '?':
          default:
              fputs(Banner, stderr);
              fputs(HowHelp, stderr);
              exit(EXIT_FAILURE);
              break;
          case 'h':
              ArgErr = aeHelp;
              break;
          }
          c = nextc;
      }
  }

  if((argc > optind) && !strcmp(argv[optind], "?"))
      ArgErr = aeHelp;

  if(ArgErr)
  {
      fputs(Banner, stderr);
      fputs(BigBanner, stderr);
      fputs(HelpText, stderr);
      Success = FALSE;
  }
  else
      Success = TRUE;

  if(Success)
      Retval = optind;

  return(Retval);
}

/*
 * Outputs a program error.
 */


void PrintPrgErr(enum PrgErrNum Error, ...)
{
    STRPTR      Type;
    va_list     MsgArgs;

    if(betw(pmMinFault, Error, pmMaxFault)) {
        switch(PrgMsgs[Error].Type) {
        case etWarn:
            Type = "WARNING";
            break;
        case etMsg:
            Type = "NOTE";
            break;
	default:
        case etErr:
            Type = "ERROR";
            break;
        }
        fprintf(stderr, "%s: %s -- ", PrgName, Type);
        
        va_start(MsgArgs, Error);
        vfprintf(stderr, PrgMsgs[Error].Message, MsgArgs);
        va_end(MsgArgs);
        fputc('\n', stderr);

	if(PrgMsgs[Error].Type == etErr)
	    exit(EXIT_FAILURE);
    }
}

void ErrPrintf(const char *fmt, ...)
{
    va_list     MsgArgs;

    va_start(MsgArgs, fmt);
    vfprintf(stderr, fmt, MsgArgs);
    va_end(MsgArgs);
}



