/**********************************************************************
  Author : Dick Bruin
  Date   : 15/11/98
  Version: 2.01
  File   : amerror.c

  Description:

  Error handler for the Amanda interpreter
  NB: don't use another jmp_buf than mainenv
*******************************************************************/

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "amtypes.h"
#include "amio.h"
#include "amerror.h"
#include "amlex.h"
#include "amprint.h"
#include "amstack.h"
#include "amtable.h"

/**************** interrupts ****************************************/

#define interruptfreq 500
#define ESC            27


bool interrupted  = False;

int interruptcount = interruptfreq;

void checkinterrupt(void)
{
  CheckIO();
  interruptcount = interruptfreq;
  if(interrupted) systemerror(1);
}

/**************** error handling ************************************/

jmp_buf mainenv;

static void error(char message[])
{
  Write("\nERROR: %s", message);
  interrupted = False;
  longjmp(mainenv, 1);
}

#define maxparseerrormessage 34

static char *parseerrormessage[maxparseerrormessage+1] =
{
/*  0 */  "parse error",
/*  1 */  "] expected",
/*  2 */  ") expected",
/*  3 */  "term expected",
/*  4 */  "cannot extend definition with another clause",
/*  5 */  "= expected",
/*  6 */  "illegal type variable",
/*  7 */  "unbound typevariable",
/*  8 */  "typename expected",
/*  9 */  "illegal typevariable",
/* 10 */  "illegal typestructure definition",
/* 11 */  "type identifier expected",
/* 12 */  "duplicate type definition",
/* 13 */  "illegal typedefinition",
/* 14 */  "with expected",
/* 15 */  ":: expected",
/* 16 */  "useless cycle in local definitions",
/* 17 */  "illegal definition",
/* 18 */  "incompatible definition",
/* 19 */  "end of line expected",
/* 20 */  "illegal attempt to open file",
/* 21 */  "import level too deep",
/* 22 */  "#import \"<filename>\" expected",
/* 23 */  "lexical scanner limitation",
/* 24 */  "0..9 expected",
/* 25 */  "identifier expected",
/* 26 */  "missing character constant",
/* 27 */  "\' expected",
/* 28 */  "\" expected",
/* 29 */  "comment not finished",
/* 30 */  "illegal pattern",
/* 31 */  "<- expected",
/* 32 */  "wrong number of patterns",
/* 33 */  "} expected",
/* 34 */  "#operator (l|r) prio operator expected"
};

void parseerror(int messagenr)
{
  lexerror();
  if(messagenr < 1 || messagenr > maxparseerrormessage) messagenr = 0;
  error(parseerrormessage[messagenr]);
}

static char *runtimeerrormessage(TagType tag)
{
  switch(tag)
  {
    case LIST:
      return "list expected";
    case INT:
      return "int expected";
    case REAL:
      return "number expected";
    case UNDEFINED:
      return "infinite loop or record field undefined or object undefined";
    default:
      return "run time error";
  }
}

void runtimeerror(TagType tag, int hashtablenr)
{
  char string[stringsize];
  if(hashtablenr >= 0)
    sprintf(string, "%s in function %s", runtimeerrormessage(tag), getfunction(hashtablenr)->name);
  else
    sprintf(string, "%s", runtimeerrormessage(tag));
  error(string);
}

#define maxmodifyerrormessage 5

static char *modifyerrormessage[maxmodifyerrormessage+1] =
{
/*  0 */  "modify error",
/*  1 */  "cannot extend definition with another clause",
/*  2 */  "incompatible local function definitions",
/*  3 */  "duplicate patternvariable",
/*  4 */  "duplicate recordfield",
/*  5 */  "num, bool or char found in expression"
};

void modifyerror(int messagenr, char functionname[])
{
  char string[stringsize];
  if(messagenr < 1 || messagenr > maxmodifyerrormessage) messagenr = 0;
  if(strlen(functionname) > 0)
    sprintf(string, "%s in function: %s", modifyerrormessage[messagenr], functionname);
  else
    strcpy(string, modifyerrormessage[messagenr]);
  error(string);
}

#define maxsystemerrormessage 19

static char *systemerrormessage[maxsystemerrormessage+1] =
{
/*  0 */  "internal error",
/*  1 */  "user interrupt" ,
/*  2 */  "out of memory",
/*  3 */  "too many definitions",
/*  4 */  "workspace full",
/*  5 */  "too many sysfuncdefs",
/*  6 */  "too many operators",
/*  7 */  "internal error at WriteCell",
/*  8 */  "internal error at WriteType",
/*  9 */  "internal error: no input stream opened",
/* 10 */  "illegal use of commandline function",
/* 11 */  "internal error at eval",
/* 12 */  "cannot open file for output",
/* 13 */  "illegal use of MATCH",
/* 14 */  "illegal use of MATCHARG",
/* 15 */  "illegal use of where",
/* 16 */  "no compilation known",
/* 17 */  "implementation restriction: pattern too deeply nested",
/* 18 */  "stack overflow",
/* 19 */  "type error"
};

void systemerror(int messagenr)
{
  if(messagenr < 1 || messagenr > maxsystemerrormessage) messagenr = 0;
  error(systemerrormessage[messagenr]);
}

/**************** timing datastructure ******************************/
/*
bool timing = False;

static time_t starttime, stoptime;

void starttiming(void)
{
  time(&starttime);
}

void stoptiming(void)
{
  if(!timing) return;
  time(&stoptime);
  Write("\nElapsed time: %d secs", (int)difftime(stoptime, starttime));
}
*/
bool timing = False;

static clock_t starttime, stoptime;

void starttiming(void)
{
  starttime = clock();
}

void stoptiming(void)
{
  if(!timing) return;
  stoptime = clock();
  Write("\nElapsed time: %3.1f secs", (float)(stoptime-starttime) / CLOCKS_PER_SEC);
}
