/*
 *  ChkTeX v1.5, error searching & report routines.
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


#include "ChkTeX.h"
#include "FindErrs.h"
#include "OpSys.h"
#include "Utility.h"
#include "Resource.h"

/***************************** ERROR MESSAGES ***************************/

#undef MSG
#define MSG(num, type, inuse, ctxt, text) {num, type, inuse, ctxt, text},

struct ErrMsg LaTeXMsgs [emMaxFault + 1] =
{
    ERRMSGS
    {emMaxFault, etErr, iuOK, 0, INTERNFAULT}
};

#define istex(c)        (isalpha(c) || (AtLetter && (c == '@')))
#define CTYPE(func) \
static int my_##func(int c) \
{ \
   return(func(c)); \
}

#define INUSE(c)        (LaTeXMsgs[(enum ErrNum) c].InUse == iuOK)

#define PSERR2(pos,len,err,a,b) \
    PrintError(CurStkName(&InputStack), RealBuf, pos, len, Line, err, a, b)

#define PSERRA(pos,len,err,a) \
    PrintError(CurStkName(&InputStack), RealBuf, pos, len, Line, err, a)

#define HEREA(len, err, a)     PSERRA(BufPtr - Buf - 1, len, err, a)
#define PSERR(pos,len,err)     PSERRA(pos,len,err,"")

#define HERE(len, err)         HEREA(len, err, "")

#define SKIP_BACK(ptr, c, check) \
    while((c = *ptr--)) \
    { \
        ifn(check)  break; \
    } \
    ptr++;

#define SKIP_AHEAD(ptr, c, check) \
    while((c = *ptr++)) \
    { \
        ifn(check) \
            break; \
    } \
    ptr--;


/*  -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=- -=><=-  */

static ULONG Line;

static STRPTR RealBuf, LineCpy, BufPtr;

static int ItFlag = efNone;

NEWBUF(Buf, BUFSIZ);
NEWBUF(CmdBuffer, BUFSIZ);
NEWBUF(ArgBuffer, BUFSIZ);

static enum ErrNum PerformCommand(const STRPTR Cmd, STRPTR Arg);

#ifdef isdigit
CTYPE(isdigit)
#else
#  define my_isdigit isdigit
#endif

#ifdef isalpha
CTYPE(isalpha)
#else
#  define my_isalpha isalpha
#endif

/*
 * Reads in a TeX token from Src and puts it in Dest.
 *
 */


static STRPTR GetLTXToken(STRPTR Src, STRPTR Dest)
{
    int Char;

    if(Src && *Src)
    {
        if(*Src == '\\')
        {
            *Dest++ = *Src++;
            Char = *Dest++ = *Src++;

            if(istex(Char))
            {
                while(istex(Char))
                    Char = *Dest++ = *Src++;

                Src--;
                Dest--;
            }

        }
        else
            *Dest++ = *Src++;
        *Dest = 0;
    }
    else
        Src = NULL;

    return(Src);
}


/*
 * Scans the `SrcBuf' for a LaTeX arg, and puts that arg into `Dest'.
 * `Until' specifies what we'll copy. Assume the text is
 * "{foo}bar! qux} baz".
 *  GET_TOKEN       => "{foo}"
 *  GET_STRIP_TOKEN => "foo"
 *  '!'             => "{foo}bar!" (i.e. till the first "!")
 * Returns NULL if we can't find the argument, ptr to the first character
 * after the argument in other cases.
 *
 * If one of the tokens found is in the wl wordlist, and we're in the
 * outer most paren, and Until isn't a single character, we'll stop.
 * You may pass NULL as wl.
 *
 * We assume that you've previously skipped over leading spaces.
 *
 */

#define GET_TOKEN       256
#define GET_STRIP_TOKEN 257

static STRPTR GetLTXArg(STRPTR SrcBuf,
                        const STRPTR OrigDest,
                        const int Until,
                        struct WordList *wl)

{
    STRPTR      Retval, TmpPtr, Dest = OrigDest;
    ULONG       DeliCnt = 0;

    *Dest = 0;
    TmpPtr = SrcBuf;

    switch(Until)
    {
    case GET_STRIP_TOKEN:
    case GET_TOKEN:
        while((Retval = GetLTXToken(TmpPtr, Dest)))
        {
            switch(*Dest)
            {
            case '{':
                DeliCnt++;
                break;
            case '}':
                DeliCnt--;
            }
            Dest += Retval - TmpPtr;
            TmpPtr = Retval;

            if(!DeliCnt || ((DeliCnt == 1) && wl && HasWord(Dest, wl)))
                break;
        }

        if(Retval && (*OrigDest == '{') && (Until == GET_STRIP_TOKEN))
        {
            strcpy(OrigDest, OrigDest + 1);
            OrigDest[strlen(OrigDest) - 1] = 0;
        }
        break;
    default:
        DeliCnt = TRUE;
        while((Retval = GetLTXArg(TmpPtr, Dest, GET_TOKEN, NULL)))
        {
            if(*Dest == Until)
                DeliCnt = FALSE;

            Dest += Retval - TmpPtr;
            TmpPtr = Retval;

            if(!DeliCnt)
                break;
        }
        break;
    }
    *Dest = 0;

    return(Retval);
}


static STRPTR MakeCpy(void)
{
    if(!LineCpy)
        LineCpy = strdup(RealBuf);

    if(!LineCpy)
        PrintPrgErr(pmStrDupErr);

    return(LineCpy);
}

static STRPTR PreProcess(void)
{
    /* First, kill comments. */

    STRPTR TmpPtr;

    strcpy(Buf, RealBuf);

    TmpPtr = Buf;

    while((TmpPtr = strchr(TmpPtr, '%')))
    {
        if(TmpPtr[-1] != '\\')
        {
            PSERR(TmpPtr - Buf, 1, emComment);
            *TmpPtr = 0;
            break;
        }
        TmpPtr++;
    }
    return(Buf);
}

/*
 * Interpret environments
 */

static void PerformEnv(STRPTR Env, BOOL Begin)
{
static
    TEXT   VBStr [BUFSIZ] = "";

    if(HasWord(Env, &MathEnvir))
    {
        MathMode += Begin ? 1 : -1;
        MathMode = max(MathMode, 0);
    }

    if(Begin && HasWord(Env, &VerbEnvir))
    {
        VerbMode = TRUE;
        strcpy(VBStr, "\\end{");
        strcat(VBStr, Env);
        strcat(VBStr, "}");
        VerbStr = VBStr;
    }
}

static STRPTR SkipVerb(void)
{
    STRPTR TmpPtr = BufPtr;
    int TmpC;

    if(VerbMode && BufPtr)
    {
        ifn(TmpPtr = strstr(BufPtr, VerbStr))
            BufPtr = &BufPtr[strlen(BufPtr)];
        else
        {
            VerbMode = FALSE;
            BufPtr = &TmpPtr[strlen(VerbStr)];
            SKIP_AHEAD(BufPtr, TmpC, LATEX_SPACE(TmpC));
            if(*BufPtr)
                PSERR(BufPtr - Buf, strlen(BufPtr) - 2, emIgnoreText);
        }
    }
    return(TmpPtr);
}

#define CHECKDOTS(wordlist, dtval) \
for(i = 0; (i < wordlist.Stack.Used) && !(Back && Front);  i++) \
 { if(!strafter(PstPtr, wordlist.Stack.Data[i])) \
         Back = dtval; \
   if(!strinfront(PrePtr, wordlist.Stack.Data[i])) \
         Front = dtval; }



/*
 * Checks that the dots are correct
 */

static enum DotLevel CheckDots(STRPTR PrePtr, STRPTR PstPtr)
{
    ULONG i;
    int TmpC;
    enum DotLevel Front = dtUnknown, Back = dtUnknown;

    if(MathMode)
    {
        PrePtr--;
#define SKIP_EMPTIES(macro, ptr) macro(ptr, TmpC, \
(LATEX_SPACE(TmpC) || (TmpC == '{') || (TmpC == '}')))

        SKIP_EMPTIES(SKIP_BACK, PrePtr);
        SKIP_EMPTIES(SKIP_AHEAD, PstPtr);

        CHECKDOTS(CenterDots, dtCDots);

        ifn(Front && Back)
        {
            CHECKDOTS(LowDots, dtLDots);
        }
        return(Front & Back);
    }
    else
        return(dtLDots);

}

static STRPTR Dot2Str(enum DotLevel dl)
{
    STRPTR Retval = INTERNFAULT;
    switch(dl)
    {
    case dtUnknown:
        Retval = "\\cdots or \\ldots";
        break;
    case dtDots:
        Retval = "\\dots";
        break;
    case dtCDots:
        Retval = "\\cdots";
        break;
    case dtLDots:
        Retval = "\\ldots";
        break;
    }
    return Retval;
}

/*
 * Wipes a command, according to the definition in WIPEARG
 */

static void WipeArgument(STRPTR Cmd, STRPTR CmdPtr)
{
    ULONG CmdLen = strlen(Cmd);
    STRPTR Format, TmpPtr;
    int c, TmpC;

    if(Cmd && *Cmd)
    {
        TmpPtr = &CmdPtr[CmdLen];
        Format = &Cmd[CmdLen + 1];

        while(TmpPtr && *TmpPtr && *Format)
        {
            switch(c = *Format++)
            {
            case '*':
                SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));
                if(*TmpPtr == '*')
                    TmpPtr++;
                break;
            case '[':
                SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));
                if(*TmpPtr == '[')
                    TmpPtr = GetLTXArg(TmpPtr, ArgBuffer, ']', NULL);
                break;
            case '{':
                SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));
                TmpPtr = GetLTXArg(TmpPtr, ArgBuffer, GET_TOKEN, NULL);
            case '}':
            case ']':
                break;
            default:
                PrintPrgErr(pmWrongWipeTemp, &Cmd[strlen(Cmd) + 1]);
                break;
            }
        }

        if(TmpPtr)
            strwrite(CmdPtr, VerbClear, TmpPtr - CmdPtr);
        else
            strxrep(CmdPtr, "()[]{}", *VerbClear);
    }
}

/*
 * Checks italic.
 *
 */

static void CheckItal(const STRPTR Cmd)
{
    int TmpC;
    STRPTR TmpPtr;
    if(HasWord(Cmd, &NonItalic))
        ItState = itOff;
    elif(HasWord(Cmd, &Italic))
        ItState = itOn;
    elif(HasWord(Cmd, &ItalCmd))
    {
        TmpPtr = BufPtr;
        SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));
        if(*TmpPtr == '{')
        {
            ItFlag = ItState? efItal : efNoItal;
            ItState = itOn;
        }
    }
}

/*
 * Interpret isolated commands.
 *
 */

static void PerformBigCmd(STRPTR CmdPtr)
{
    STRPTR  TmpPtr,
            ArgEndPtr;
    ULONG   CmdLen = strlen(CmdBuffer);
    int     TmpC;
    enum ErrNum
            ErrNum;
    struct ErrInfo *ei;

    enum DotLevel dotlev, realdl = dtUnknown;

    TmpPtr = BufPtr;
    SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));

    ArgEndPtr = GetLTXArg(TmpPtr, ArgBuffer, GET_STRIP_TOKEN, NULL);

    /* Kill `\verb' commands */

    if(WipeVerb)
    {
        if(!strcmp(CmdBuffer, "\\verb"))
        {
            if(*BufPtr)
            {
                if((TmpPtr = strchr(&BufPtr[1], *BufPtr)))
                    strwrite(CmdPtr, VerbClear, (TmpPtr - CmdPtr) + 1);
                else
                    PSERR(CmdPtr - Buf, 5, emNoArgFound);
            }
        }
    }

    if(HasWord(CmdBuffer, &IJAccent))
    {
        if(ArgEndPtr)
        {
            TmpPtr = ArgBuffer;
            SKIP_AHEAD(TmpPtr, TmpC, TmpC == '{'); /* } */

            if((*TmpPtr == 'i') || (*TmpPtr == 'j'))
                PrintError(CurStkName(&InputStack),  RealBuf,
                           CmdPtr - Buf,
                           (LONG) strlen(CmdBuffer), Line,
                           emAccent, CmdBuffer,
                           *TmpPtr, MathMode? "math" : "");
        }
        else
            PSERR(CmdPtr - Buf, CmdLen, emNoArgFound);
    }

    if(HasWord(CmdBuffer, &NotPreSpaced) &&
       isspace(CmdPtr[-1]))
        PSERRA(CmdPtr - Buf - 1, 1, emRemPSSpace, CmdBuffer);

    if((TmpPtr = HasWord(CmdBuffer, &NoCharNext)))
    {
	STRPTR BPtr = BufPtr;

	TmpPtr += strlen(TmpPtr) + 1;
	SKIP_AHEAD(BPtr, TmpC, LATEX_SPACE(TmpC));

	if(strchr(TmpPtr, *BPtr))
	{
	    PSERR2(CmdPtr - Buf, CmdLen, emNoCharMean, 
		  CmdBuffer, *BPtr);
	}
    }

    if(!strcmp(CmdBuffer, "\\begin") ||
       !strcmp(CmdBuffer, "\\end"))
    {
        if(ArgEndPtr)
        {
            if(!strcmp(ArgBuffer, "document"))
                InHeader = FALSE;

            if(CmdBuffer[1] == 'b')
            {
                ifn(PushErr(ArgBuffer, Line, CmdPtr - Buf,
                            CmdLen, MakeCpy(),
                            &EnvStack))
                    PrintPrgErr(pmNoStackMem);
            }
            else
            {
                if((ei = PopErr(&EnvStack)))
                {
                    if(strcmp(ei->Data, ArgBuffer))
                        PrintError(CurStkName(&InputStack), RealBuf,
                                   CmdPtr - Buf,
                                   (LONG) strlen(CmdBuffer),
                                   Line, emExpectC, ei->Data, ArgBuffer);

                    FreeErrInfo(ei);
                }
                else
                    PrintError(CurStkName(&InputStack), RealBuf,
                               CmdPtr - Buf,
                               (LONG) strlen(CmdBuffer),
                               Line, emSoloC, ArgBuffer);
            }

            PerformEnv(ArgBuffer, (BOOL) CmdBuffer[1] == 'b');
        } else
            PSERR(CmdPtr - Buf, CmdLen, emNoArgFound);
    }

    CheckItal(CmdBuffer);

    if((ErrNum = PerformCommand(CmdBuffer, BufPtr)))
        PSERR(CmdPtr - Buf, CmdLen, ErrNum);

    if(!strcmp(CmdBuffer, "\\cdots"))
        realdl = dtCDots;

    if(!strcmp(CmdBuffer, "\\ldots"))
        realdl = dtLDots;

    if(!strcmp(CmdBuffer, "\\dots"))
        realdl = dtLDots;

    if(realdl != dtUnknown)
    {
        dotlev = CheckDots(CmdPtr, BufPtr);
        if(dotlev && (dotlev != realdl))
        {
            TmpPtr = Dot2Str(dotlev);
            PSERRA(CmdPtr - Buf, CmdLen, emEllipsis, TmpPtr);
        }
    }

    if((TmpPtr = HasWord(CmdBuffer, &WipeArg)))
        WipeArgument(TmpPtr, CmdPtr);
}

/*
 * Check user abbreviations. Pass a pointer to the `.';
 * also ensure that it's followed by spaces, etc.
 *
 * Note: We assume that all abbrevs have been transferred from
 * AbbrevCase into Abbrev.
 */

static void CheckAbbrevs(const STRPTR Buffer)
{
    LONG i;
    STRPTR TmpPtr, AbbPtr;

    if(INUSE(emInterWord))
    {
        TmpPtr = TmpBuffer + Abbrev.MaxLen + 2;
        *TmpPtr = 0;
        AbbPtr = Buffer;

        for(i = Abbrev.MaxLen;
            i >= 0;
            i--)
        {
            *--TmpPtr = *AbbPtr--;
            if(!isalpha(*AbbPtr) && HasWord(TmpPtr, &Abbrev))
                PSERR(Buffer - Buf + 1, 1, emInterWord);
            if(!*AbbPtr)
                break;
        }
    }
}


/*
 * Check misc. things which can't be included in the main loop.
 *
 */

static void CheckRest(void)
{
    ULONG Count;
    LONG CmdLen;
    STRPTR  UsrPtr;

    /* Search for user-specified warnings */

    if(INUSE(emUserWarn))
    {
        strcpy(TmpBuffer, Buf);
	FORWL(Count, UserWarn)
        {
            for(UsrPtr = TmpBuffer;
                (UsrPtr = strstr(UsrPtr, UserWarn.Stack.Data[Count]));
                UsrPtr++)
            {
                CmdLen = strlen(UserWarn.Stack.Data[Count]);
                PSERR(UsrPtr - TmpBuffer, CmdLen, emUserWarn);
            }
        }


        strlwr(TmpBuffer);

	FORWL(Count, UserWarnCase)
        {
            for(UsrPtr = TmpBuffer;
                (UsrPtr = strstr(UsrPtr, UserWarnCase.Stack.Data[Count]));
                UsrPtr++)
            {
                CmdLen = strlen(UserWarnCase.Stack.Data[Count]);
                PSERR(UsrPtr - TmpBuffer, CmdLen, emUserWarn);
            }
        }
    }
}


/*
 * Checks that the dash-len is correct.
 */

static void CheckDash(void)
{
    STRPTR TmpPtr;
    int TmpC;
    LONG TmpCount, Len;
    struct WordList *wl = NULL;
    ULONG  i;
    BOOL Errored;
    STRPTR PrePtr = &BufPtr[-2];

    TmpPtr = BufPtr;
    SKIP_AHEAD(TmpPtr, TmpC, TmpC == '-');
    TmpCount = TmpPtr - BufPtr + 1;

    if(MathMode)
    {
        if(TmpCount > 1)
            HERE(TmpCount, emWrongDash);
    }
    else
    {
        if(LATEX_SPACE(*PrePtr) && LATEX_SPACE(*TmpPtr))
            wl = &WordDash;
        if(isdigit(*PrePtr) && isdigit(*TmpPtr))
            wl = &NumDash;
        if(isalpha(*PrePtr) && isalpha(*TmpPtr))
            wl = &HyphDash;

        if(wl)
        {
            Errored = TRUE;
	    FORWL(i, *wl)
            {
                Len = strtol(wl->Stack.Data[i], NULL, 0);
                if(TmpCount == Len)
                {
                    Errored = FALSE;
                    break;
                }
            }
            if(Errored)
                HERE(TmpCount, emWrongDash);
        }
    }
}

/*
 * Pushes and pops nesting characters.
 *
 */

static void HandleBracket(int Char)
{
    ULONG BrOffset;  /* Offset into BrOrder array */
    struct ErrInfo *ei;
    int    TmpC, Match;
    TEXT   ABuf[2], BBuf[2];
    STRPTR TmpPtr;

    AddBracket(Char);

    if((BrOffset = BrackIndex(Char)) != ~0UL)
    {
        if(BrOffset & 1)      /* Closing bracket of some sort */
        {
            if((ei = PopErr(&CharStack)))
            {
                Match = MatchBracket(*(ei->Data));
                if(ei->Flags & efNoItal)
                {
                    if(ItState == itOn)
                    {
                        TmpPtr = BufPtr;
                        SKIP_AHEAD(TmpPtr, TmpC, TmpC == '}');

                        if(!strchr(LTX_SmallPunc, *TmpPtr))
                            HERE(1, emNoItFound);
                    }

                    ItState = FALSE;
                }
                elif(ei->Flags & efItal)
                    ItState = TRUE;
                FreeErrInfo(ei);
            }
            else
                Match = 0;

            if(Match != Char)
            {
                ABuf[0] = Match;
                BBuf[0] = Char;
                ABuf[1] = BBuf[1] = 0;
                if(Match)
                    PrintError(CurStkName(&InputStack), RealBuf,
                               BufPtr - Buf - 1, 1, Line,
                               emExpectC,
                               ABuf, BBuf);
                else
                    HEREA(1, emSoloC, BBuf);
            }

        }
        else         /* Opening bracket of some sort  */
        {
            if((ei = PushChar(Char, Line, BufPtr - Buf - 1,
                         &CharStack, MakeCpy())))
            {
                if(Char == '{')
                {
                    switch(ItFlag)
                    {
                    default:
                        ei->Flags = ItFlag;
                        ItFlag = efNone;
                        break;
                    case efNone:
                        ei->Flags |= ItState?
                                     efItal : efNoItal;
                    }
                }
            }

            else
                PrintPrgErr(pmNoStackMem);
        }
    }
}



/*
 * Searches the `Buf' for possible errors, and prints the errors. `Line'
 * is supplied for error printing.
 */

BOOL FindErr(const STRPTR _RealBuf, const ULONG _Line)
{
  STRPTR
    CmdPtr,   /* We'll have to copy each command out. */
    PrePtr,   /* Ptr to char in front of command, NULL if
               * the cmd appears as the first character  */
    TmpPtr,   /* Temporary pointer */
    ErrPtr;   /* Ptr to where an error started */

  int
    TmpC,     /* Just a temp var used throughout the proc.*/
    MatchC,
    Char;     /* Char. currently processed */
  ULONG CmdLen;   /* Length of misc. things */
  BOOL MixingQuotes;

#if defined(_MSC_VER) && (_MSC_VER < 1300)
  int (* pstcb)(int c);
#else
  int (CDECL * pstcb)(int c);
#endif

  enum DotLevel dotlev;

    LineCpy = NULL;

    if(_RealBuf)
    {
        RealBuf = _RealBuf;
        Line = _Line;

        BufPtr = PreProcess();

        BufPtr = SkipVerb();

        while(BufPtr && *BufPtr)
        {
            PrePtr = BufPtr - 1;
	    Char = *BufPtr++;
	    if(isspace(Char))
		Char = ' ';

            switch(Char)
            {
            case '~':
                TmpPtr = NULL;
                if(isspace(*PrePtr))
                    TmpPtr = PrePtr;
                elif(isspace(*BufPtr))
                    TmpPtr = BufPtr;

                if(TmpPtr)
                    PSERR(TmpPtr - Buf, 1, emDblSpace);
                break;

            case 'X':
            case 'x':
                TmpPtr = PrePtr;

                SKIP_BACK(TmpPtr, TmpC,
                    (LATEX_SPACE(TmpC) || strchr("{}$", TmpC)));

                if(isdigit(*TmpPtr))
                {
                    TmpPtr = BufPtr;

                    SKIP_AHEAD(TmpPtr, TmpC,
                        (LATEX_SPACE(TmpC) || strchr("{}$", TmpC)));

                    if(isdigit(*TmpPtr))
                        HERE(1, emUseTimes);
                }
                /* FALLTHRU */
                /* CTYPE: isalpha() */
            case 'a':  case 'b':  case 'c':  case 'd':
            case 'e':  case 'f':  case 'g':  case 'h':
            case 'i':  case 'j':  case 'k':  case 'l':
            case 'm':  case 'n':  case 'o':  case 'p':
            case 'q':  case 'r':  case 's':  case 't':
            case 'u':  case 'v':  case 'w':  /* case 'x': */
            case 'y':  case 'z':

            case 'A':  case 'B':  case 'C':  case 'D':
            case 'E':  case 'F':  case 'G':  case 'H':
            case 'I':  case 'J':  case 'K':  case 'L':
            case 'M':  case 'N':  case 'O':  case 'P':
            case 'Q':  case 'R':  case 'S':  case 'T':
            case 'U':  case 'V':  case 'W':  /* case 'X': */
            case 'Y':  case 'Z':
                if(!isalpha(*PrePtr) && (*PrePtr != '\\') && MathMode)
                {
                    TmpPtr = BufPtr;
                    CmdPtr = CmdBuffer;
                    do
                    {
                        *CmdPtr++ = Char;
                        Char = *TmpPtr++;
                    } while(isalpha(Char));

                    *CmdPtr = 0;

                    if(HasWord(CmdBuffer, &MathRoman))
                        HEREA(strlen(CmdBuffer), emWordCommand, CmdBuffer);
                }

                break;
	    case ' ':
                TmpPtr = BufPtr;
                SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));

                if(*TmpPtr && *PrePtr)
                {
                    if((TmpPtr - BufPtr) > 0)
                    {
                        HERE(TmpPtr - BufPtr + 1, emMultiSpace);
                        strwrite(BufPtr, VerbClear, TmpPtr - BufPtr - 1);
                    }
                }
                break;

            case '.':
                if((Char == *BufPtr) && (Char == BufPtr[1]))
                {
                    dotlev = CheckDots(&PrePtr[1], &BufPtr[2]);
                    TmpPtr = Dot2Str(dotlev);
                    HEREA(3, emEllipsis, TmpPtr);
                }

                /* Regexp: "([^A-Z@.])\.[.!?:;]*\s+[a-z]" */

                TmpPtr = BufPtr;
                SKIP_AHEAD(TmpPtr, TmpC, strchr(LTX_GenPunc, TmpC));
                if(LATEX_SPACE(*TmpPtr))
                {
                    if(!isupper(*PrePtr) && (*PrePtr != '@') &&
                       (*PrePtr != '.'))
                    {
                        SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));
                        if(islower(*TmpPtr))
                            PSERR(BufPtr - Buf, 1, emInterWord);
                        else
                            CheckAbbrevs(&BufPtr[-1]);
                    }
                }

                /* FALLTHRU */
            case ':': case '?': case '!': case ';':
                /* Regexp: "[A-Z][A-Z][.!?:;]\s+" */

                if(isspace(*BufPtr) && isupper(*PrePtr) &&
                   (isupper(PrePtr[-1]) || (Char != '.')))
                    HERE(1, emInterSent);

                /* FALLTHRU */
            case ',':
                if(isspace(*PrePtr) && 
		   !(isdigit(*BufPtr) && 
		     ((BufPtr[-1] == '.') || (BufPtr[-1] == ','))))
                    PSERR(PrePtr - Buf, 1, emSpacePunct);

                if(MathMode &&
                   (((*BufPtr == '$') && (BufPtr[1] != '$')) ||
                    (!strafter(BufPtr, "\\)"))))
                    HEREA(1, emPunctMath, "outside inner");

                if(!MathMode &&
                   (((*PrePtr == '$') && (PrePtr[-1] == '$')) ||
                    (!strinfront(PrePtr, "\\]"))))
                    HEREA(1, emPunctMath, "inside display");

                break;
            case '\'':
            case '`':
                if((Char == *BufPtr) && (Char == BufPtr[1]))
                {
                    PrintError(CurStkName(&InputStack), RealBuf,
                               BufPtr - Buf - 1, 3, Line,
                               emThreeQuotes,
                               Char, Char, Char,
                               Char, Char, Char);
                }

                if(Char == '\'')
                    MatchC = '`';
                else
                    MatchC = '\'';

                TmpPtr = BufPtr;
                SKIP_AHEAD(TmpPtr, TmpC, TmpC == Char);

                MixingQuotes = FALSE;

                if((*TmpPtr == MatchC) || (*TmpPtr == '\"') ||
                   (*TmpPtr == '�'))
                    MixingQuotes = TRUE;

                SKIP_AHEAD(TmpPtr, TmpC, strchr("`\'\"�", TmpC));

                if(MixingQuotes)
                    HERE(TmpPtr - BufPtr + 1, emQuoteMix);

                switch(Char)
                {
                case '\'':
                    if(isalpha(*TmpPtr) &&
                       (strchr(LTX_GenPunc, *PrePtr) || isspace(*PrePtr)))
                        HERE(TmpPtr - BufPtr + 1, emBeginQ);

                    /* Now check quote style */
#define ISPUNCT(ptr) (strchr(LTX_GenPunc, *ptr) && (ptr[-1] != '\\'))

		    /* We ignore all single words/abbreviations in quotes */

		    {
			STRPTR WordPtr = PrePtr;
			SKIP_BACK(WordPtr, TmpC, (isalnum(TmpC) ||
						  strchr(LTX_GenPunc, TmpC)));
			
			if(*WordPtr != '`')
			{
			    if(*PrePtr && (Quote != quTrad) && ISPUNCT(PrePtr))
				PSERRA(PrePtr - Buf, 1, emQuoteStyle, 
				       "in front of");
			    
			    if(*TmpPtr && (Quote != quLogic) && 
			       ISPUNCT(TmpPtr))
				PSERRA(TmpPtr - Buf, 1, emQuoteStyle, "after");
			}
		    }

                    break;
                case '`':
                    if(isalpha(*PrePtr) &&
                       (strchr(LTX_GenPunc, *TmpPtr) || isspace(*TmpPtr)))
                        HERE(TmpPtr - BufPtr + 1, emEndQ);
                    break;
                }
                BufPtr = TmpPtr;
                break;
            case '"':
                HERE(1, emUseQuoteLiga);
                break;

		/* One of these are unnecessary, but what the heck... */
            case 180: /* �. NOTE: '\xb4' gets converted to - something*/
	    case ~(0xff &(~180)):  /* This yields 0xff...fb4 in */
				   /* arbitrary precision. */

                HERE(1, emUseOtherQuote);
                break;

            case '_':
            case '^':
                if(*PrePtr != '\\')
                {
                    TmpPtr = PrePtr;
                    SKIP_BACK(TmpPtr, TmpC, LATEX_SPACE(TmpC));

                    CmdLen = 1;

                    switch(*TmpPtr)
                    {
                        /*{*/
                    case '}':
                        if(PrePtr[-1] != '\\')
                            break;

                        CmdLen++;
                        PrePtr--;
                        /* FALLTHRU */
                    /*[(*/
                    case ')':
                    case ']':
                        PSERR(PrePtr - Buf, CmdLen, emEnclosePar);
                    }

                    TmpPtr = BufPtr;
                    SKIP_AHEAD(TmpPtr, TmpC, LATEX_SPACE(TmpC));

                    ErrPtr = TmpPtr;

                    if(isalpha(*TmpPtr))
                        pstcb = &my_isalpha;
                    elif(isdigit(*TmpPtr))
                        pstcb = &my_isdigit;
                    else
                        break;

                    while((*pstcb)(*TmpPtr++))
                        ;
                    TmpPtr--;

                    if((TmpPtr - ErrPtr) > 1)
                        PSERR(ErrPtr - Buf, TmpPtr - ErrPtr, emEmbrace);
                }
                break;
            case '-':
                CheckDash();
                break;
            case '\\':                    /* Command encountered  */
                BufPtr = GetLTXToken(--BufPtr, CmdBuffer);

                if(LATEX_SPACE(*PrePtr))
                {
                    if(HasWord(CmdBuffer, &Linker))
                        PSERR(PrePtr - Buf, 1, emNBSpace);
                    if(HasWord(CmdBuffer, &PostLink))
                        PSERR(PrePtr - Buf, 1, emFalsePage);
                }

                if(LATEX_SPACE(*BufPtr) && !MathMode &&
                   (!HasWord(CmdBuffer, &Silent)) &&
                   (strlen(CmdBuffer) != 2))
                {
                    PSERR(BufPtr - Buf, 1, emSpaceTerm);
                }
                elif((*BufPtr == '\\') && (!isalpha(BufPtr[1])) &&
                     (!LATEX_SPACE(BufPtr[1])))
                    PSERR(BufPtr - Buf, 2, emNotIntended);

                PerformBigCmd(PrePtr + 1);
                BufPtr = SkipVerb();

                break;

            LOOP(bracket,
            case '(':
                ifn(!*PrePtr || LATEX_SPACE(*PrePtr) || isdigit(*PrePtr) || 
		    strchr("([{`~", *PrePtr))
		{
		    if(PrePtr[-1] != '\\') /* Short cmds */
		    {
			TmpPtr = PrePtr;
			SKIP_BACK(TmpPtr, TmpC, istex(TmpC));
			if(*TmpPtr != '\\') /* Long cmds */
			    PSERRA(BufPtr - Buf - 1, 1, emSpaceParen,
				   "in front of");
		    }
		}
                if(isspace(*BufPtr))
                    PSERRA(BufPtr - Buf, 1, emNoSpaceParen, "after");

                LAST(bracket);
            case ')':
                if(isspace(*PrePtr))
                    PSERRA(BufPtr - Buf - 1, 1, emNoSpaceParen,
                           "in front of");
                if(isalpha(*BufPtr))
                    PSERRA(BufPtr - Buf, 1, emSpaceParen, "after");

                LAST(bracket);
            )

            case '}':
            case '{':
            case '[':
            case ']':
                HandleBracket(Char);
                break;
            case '$':
                if(*PrePtr != '\\')
                {
                    if(*BufPtr == '$')
                        BufPtr++;
                    MathMode ^= TRUE;
               }

                break;
            }
        }

        if(!VerbMode)
        {
            CheckRest();
        }
    }

    return(TRUE);
}

/*
 * Tries to create plural forms for words. Put a '%s' where a
 * suffix should be put, e.g. "warning%s". Watch your %'s!
 */

static void Transit(FILE *fh, ULONG Cnt, STRPTR Str)
{
    switch(Cnt)
    {
    case 0:
        fputs("No ", fh);
        fprintf(fh, Str, "s");
        break;
    case 1:
        fputs("One ", fh);
        fprintf(fh, Str, "");
        break;
    default:
        fprintf(fh, "%ld ", Cnt);
        fprintf(fh, Str, "s");
        break;
    }
}

/*
 * Prints the status/conclusion after doing all the testing, including
 * bracket stack status, math mode, etc.
 */

void PrintStatus(ULONG Lines)
{
  ULONG Cnt;
  struct ErrInfo *ei;


  while((ei = PopErr(&CharStack)))
  {
      PrintError(ei->File, ei->LineBuf, ei->Column,
                 ei->ErrLen, ei->Line, emNoMatchC,
                 (STRPTR) ei->Data);
      FreeErrInfo(ei);
  }

  while((ei = PopErr(&EnvStack)))
  {
      PrintError(ei->File, ei->LineBuf, ei->Column,
                 ei->ErrLen, ei->Line, emNoMatchC,
                 (STRPTR) ei->Data);
      FreeErrInfo(ei);
  }

  if(MathMode)
  {
      PrintError(CurStkName(&InputStack), "", 0L, 0L, Lines,
                 emMathStillOn);
  }

  for(Cnt = 0L; Cnt < (NUMBRACKETS>>1); Cnt++)
  {
      if(Brackets[Cnt << 1] != Brackets[(Cnt << 1) + 1])
      {
          PrintError(CurStkName(&InputStack), "", 0L, 0L, Lines,
                     emNoMatchCC,
                     BrOrder[Cnt<<1], BrOrder[(Cnt<<1) + 1]);
      }
  }

  if(!Quiet)
  {
      Transit(stderr, ErrPrint, "error%s printed; ");
      Transit(stderr, WarnPrint, "warning%s printed; ");
      Transit(stderr, UserSupp, "user suppressed warning%s printed.\n");
  }
}



/*
 * Uses OutputFormat. Be sure that `String'
 * does not contain tabs, newlines, etc.
 * Prints a formatted string. Formatting codes understood:
 *  %b  - string to print Between fields (from -s option)
 *  %c  - Column position of error
 *  %d  - lenght of error (Digit)
 *  %f  - current Filename
 *  %i  - Turn on inverse printing mode.
 *  %I  - Turn off inverse printing mode.
 *  %k  - Kind of error (warning, error, message)
 *  %l  - Line number of error
 *  %m  - warning Message
 *  %n  - warning Number
 *  %u  - an Underlining line (like the one which appears when using -v1)
 *  %r  - part of line in front of error ('S' - 1)
 *  %s  - part of line which contains error (String) *  %t  - part of line after error ('S' + 1)
 */


void  PrintError(const  STRPTR  File,   const  STRPTR String,
                 const LONG Position,   const LONG Len,
                 const LONG LineNo, const enum ErrNum Error, ...)
{
    static      /* Just to reduce stack usage... */
        TEXT   PrintBuffer[BUFSIZ];
    va_list     MsgArgs;

    STRPTR      LastNorm = OutputFormat, of;
    int         c;

    enum Context Context;

    if(betw(emMinFault, Error, emMaxFault))
    {
        switch(LaTeXMsgs[Error].InUse)
        {
        case iuOK:
            do
            {
                Context  = LaTeXMsgs[Error].Context;

                if(!HeadErrOut)
                    Context |= ctOutHead;

#define RGTCTXT(Ctxt, Var) if((Context & Ctxt) && !(Var)) break;

                RGTCTXT(ctInMath, MathMode);
                RGTCTXT(ctOutMath, !MathMode);
                RGTCTXT(ctInHead, InHeader);
                RGTCTXT(ctOutHead, !InHeader);
                
                switch(LaTeXMsgs[Error].Type)
                {
                case etWarn:
                    WarnPrint++;
                    break;
                case etErr:
                    ErrPrint++;
                    break;
                case etMsg:
                    break;
                }

                while((of = strchr(LastNorm, '%')))
                {
                    c = *of;
                    *of = 0;

                    fputs(LastNorm, OutputFile);

                    *of++ = c;

                    switch(c = *of++)
                    {
                    case 'b':
                        fputs(Delimit, OutputFile);
                        break;
                    case 'c':
                        fprintf(OutputFile, "%ld", Position + 1);
                        break;
                    case 'd':
                        fprintf(OutputFile, "%ld", Len);
                        break;
                    case 'f':
                        fputs(File, OutputFile);
                        break;
                    case 'i':
                        fputs(ReverseOn, OutputFile);
                        break;
                    case 'I':
                        fputs(ReverseOff, OutputFile);
                        break;
                    case 'k':
                        switch(LaTeXMsgs[Error].Type)
                        {
                        case etWarn:
                            fprintf(OutputFile, "Warning");
                            break;
                        case etErr:
                            fprintf(OutputFile, "Error");
                            break;
                        case etMsg:
                            fprintf(OutputFile, "Message");
                            break;
                        }
                        break;
                    case 'l':
                        fprintf(OutputFile, "%ld", LineNo);
                        break;
                    case 'm':
                        va_start(MsgArgs, Error);
                        vfprintf(OutputFile, LaTeXMsgs[Error].Message, 
                                 MsgArgs);
                        va_end(MsgArgs);
                        break;
                    case 'n':
                        fprintf(OutputFile, "%d", Error);
                        break;
                    case 'u':
                        sfmemset(PrintBuffer, ' ', (LONG) Position);

                        sfmemset(&PrintBuffer[Position], '^', Len);
                        PrintBuffer[Position + Len] = 0;
                        fputs(PrintBuffer, OutputFile);
                        break;
                    case 'r':
                        substring(String, PrintBuffer, 0L, Position);
                        fputs(PrintBuffer, OutputFile);
                        break;
                    case 's':
                        substring(String, PrintBuffer, Position, Len);
                        fputs(PrintBuffer, OutputFile);
                        break;
                    case 't':
                        substring(String, PrintBuffer, Position + Len, LONG_MAX);
                        fputs(PrintBuffer, OutputFile);
                        break;
                    default:
                        fputc(c, OutputFile);
                        break;
                    }
                    LastNorm = of;
                }
                fputs(LastNorm, OutputFile);
            } while(FALSE);
            break;
        case iuNotUser:
            UserSupp++;
            break;
        case iuNotSys:
            break;
        }
    }
}

/*
 * All commands isolated is routed through this command, so we can
 * update global statuses like math mode and whether @ is a letter
 * or not.
 */

static enum ErrNum PerformCommand(const STRPTR Cmd, STRPTR Arg)
{
    STRPTR Argument = "";
    enum ErrNum
        en = emMinFault;
    int TmpC;

    if(!strcmp(Cmd, "\\makeatletter"))
        AtLetter = TRUE;
    elif(!strcmp(Cmd, "\\makeatother"))
        AtLetter = FALSE;
    elif(InputFiles &&
        !(strcmp(Cmd, "\\input") && strcmp(Cmd, "\\include")))
    {
        SKIP_AHEAD(Arg, TmpC, LATEX_SPACE(TmpC));
        if(*Arg == '{') /* } */
        {
            if(GetLTXArg(Arg, TmpBuffer, GET_STRIP_TOKEN, NULL))
                Argument = TmpBuffer;
        }
        else
            Argument = strip(Arg, STRP_BTH);

        ifn(Argument && PushFileName(Argument, &InputStack))
            en = emNoCmdExec;
    }
    elif(HasWord(Cmd, &Primitives))
        en = emTeXPrim;
    elif(*Cmd == '\\')
    {
        /* Quicker check of single lettered commands. */
        switch(Cmd[1])
        {
        case '(':
        case '[':
            MathMode = TRUE;
            break;
        case ']':
        case ')':
            MathMode = FALSE;
            break;
        case '/':
            switch(ItState)
            {
            case itOn:
                ItState = itCorrected;
                Argument = Arg;

                SKIP_AHEAD(Argument, TmpC, TmpC == '{' || TmpC == '}');

                if(strchr(".,", *Argument))
                    en = emItPunct;

                break;
            case itCorrected:
                en = emItDup;
                break;
            case itOff:
                en = emItInNoIt;
            }
            break;
        }
    }

    return(en);
}
