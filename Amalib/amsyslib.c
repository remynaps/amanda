/**********************************************************************
  Author : Dick Bruin
  Date   : 04/05/2000
  Version: 2.03
  File   : amsyslib.c

  Description:

  library with definitions of system functions
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include "amio.h"
#include "amtypes.h"
#include "ameval.h"
#include "amcheck.h"
#include "amerror.h"
#include "ammem.h"
#include "amstack.h"
#include "amtable.h"
#include "amparse.h"
#include "amprint.h"
#include "ammodify.h"
#include "amlib.h"
#include "amlex.h"
#include "amsyslib.h"

/****************** FindWords ********************************************/

static int FindWords(char line[], char *words[], int maxwords)
{
  enum { fwStart, fwWord, fwString } state = fwStart;
  int k, count = 0;
  for(k=0; line[k] && count<=maxwords; k++)
    if(isspace(line[k]) && state != fwString)
    {
      line[k] = '\0';
      state = fwStart;
    }
    else if(isspace(line[k]) && state == fwString)
      ;
    else if(line[k] == '\"' && state == fwString)
    {
      line[k] = '\0';
      state = fwStart;
    }
    else if(line[k] == '\"' && state != fwString)
    {
      line[k] = '\0';
      if(count < maxwords) words[count++] = &line[k+1];
      state = fwString;
    }
    else if(state == fwStart)
    {
      if(count < maxwords) words[count++] = &line[k];
      state = fwWord;
    }
  return count;
}

/****************** options **********************************************/

#define BANNER "Amanda V2.05\n\n"
#define AMAINI "amanda.ini"

#define optionsize 40
#define maxoption   4

static char inipath[stringsize];

static struct
{
  char option[optionsize];
  char value[optionsize];
} OptionTable[maxoption] =
{
  { "MemorySize"  , "100000" },
  { "WinFontName" , "Courier New" },
  { "WinFontSize" , "10" },
  { "ConPrompt"   , "> " }
};

char *GetOption(char option[])
{
  int k = 0;
  while(k < maxoption && strcmp(option, OptionTable[k].option) != 0) k++;
  return k < maxoption ? OptionTable[k].value : "";
}

static void SetOption(char option[], char value[])
{
  int k = 0;
  while(k < maxoption && strcmp(option, OptionTable[k].option) != 0) k++;
  if(k < maxoption) strncat(strcpy(OptionTable[k].value, ""), value, optionsize-1);
}

void InitOptions(bool console, char *path)
{
  FILE *fp;
  int k = 0;
  if(path)
  {
    strcpy(inipath, path);
    k = strlen(inipath);
    while(k > 0 && inipath[k-1] != '\\' && inipath[k-1] != '/') k--;
  }
  strcpy(inipath+k, AMAINI);
  fp = fopen(inipath, "r");
  if(fp)
  {
    char line[stringsize], *words[4];
    while(fgets(line, sizeof(line), fp))
      if(FindWords(line, words, 4) >= 4 && strcmp(words[0], "||") == 0 && strcmp(words[2], "=") == 0)
        SetOption(words[1], words[3]);
    fclose(fp);
  }
  if(!console) SetOption("ConPrompt", "> ");
}

/****************** interpreter installation *****************************/

static void initsyslib(void);

static Cell *template_divide,
            *template_div,
            *template_mod,
            *template_power,
            *template_update;

void CreateInterpreter(void)
{
  if(seterror() == 0)
  {
    Write(BANNER);
    createstack();
    createhashtable();
    createIO();
    createmem(atol(GetOption("MemorySize")));
    initstack();
    inithashtable();
    lockmem();
    initlex();
    initlib();
    initsyslib();
    initmodify();
    parsefile(inipath);
    checkdefinitions();
    modify_definitions();
    lockmem();
  }
  else
    exit(1);
}

bool Load(char *filename)
{
  static char fileName[stringsize] = "";
  if(filename) strncat(strcpy(fileName, ""), filename, stringsize-1);
  if(strlen(fileName) == 0)
    return False;
  else if(seterror()==0)
  {
    initstack();
    unlockmem();
    inithashtable();
    lockmem();
    initlex();
    initlib();
    initsyslib();
    initmodify();
    parsefile(inipath);
    parsefile(fileName);
    checkdefinitions();
    modify_definitions();
    lockmem();
    return True;
  }
  else
  {
    initstack();
    unlockmem();
    inithashtable();
    lockmem();
    initlex();
    initlib();
    initsyslib();
    initmodify();
    parsefile(inipath);
    checkdefinitions();
    modify_definitions();
    lockmem();
    return False;
  }
}

static bool commandline(char expr[])
{
  int wc;
  char line[stringsize], *words[2];
  strncat(strcpy(line, ""), expr, stringsize-1);
  wc = FindWords(line, words, 2);
  if(wc == 1 && strcmp(words[0], "exit") == 0)
    exit(0);
  if(wc == 1 && strcmp(words[0], "info") == 0)
  {
    Write(BANNER);
    printmeminfo();
    printhashtable();
    return True;
  }
  if(wc == 1 && strcmp(words[0], "time") == 0)
  {
    timing = !timing;
    Write(timing ? "True" : "False");
    return True;
  }
  if(wc == 1 && strcmp(words[0], "reload") == 0)
  {
    Write(Load(NULL) ? "True" : "");
    return True;
  }
  if(wc == 2 && strcmp(words[0], "load") == 0)
  {
    Write(Load(words[1]) ? "True" : "");
    return True;
  }
  return False;
}

void Interpret(char expr[])
{
  SetIntSignal(True);
  if(commandline(expr))
    ;
  else if(seterror()==0)
  {
    initstack();
    restoretemplates();
    CloseAllIOFiles();
    interrupted = False;
    if(parseinput(expr))
      checkexpression(top(), True);
    else
    {
      checkexpression(top(), False);
      settop(modify_expression(top()));
      starttiming();
      toplevel();
      stoptiming();
    }
  }
  SetIntSignal(False);
  initstack();
  restoretemplates();
  CloseAllIOFiles();
  interrupted = False;
  Write("\n");
}


/************************************************************************
  pushes a list of characters (based on s)
*************************************************************************/
static void buildstring(char *s)
{
  Cell *temp;

  if(*s == '\0')
  {
    push(template_nil);
    return;
  }
  push(temp = newcell(LIST));
  for(;;)
  {
    temp->left = newcell(CHAR);
    temp->left->value = *s++;
    if(*s == '\0')
    {
      temp->right = template_nil;
      return;
    }
    temp = temp->right = newcell(LIST);
  }
}

/************************************************************************
  c must evaluate to a list of characters
  the characters are put in s
*************************************************************************/
static void fillstring(Cell *c, char *s, int size)
{
  char *t = s;

  size--;
  evaluate(c);
  while(c->tag == LIST)
  {
    evaluate(c->left);
    *t++ = c->left->value;
    if(t-s >= size) break;
    c = c->right;
    evaluate(c);
  }
  *t = '\0';
}

/************************************************************
  Remote objects
*************************************************************/

static int ObjectCount = 0;
static bool RemoteOk = False;

bool InitRemote(void)
{
  initstack();
  restoretemplates();
  CloseAllIOFiles();
  interrupted = False;
  SetIntSignal(True);
  ObjectCount = 0;
  RemoteOk = True;
  return True;
}

int CreateRemote(char s[])
{
  if(!RemoteOk) return -1;
  if(seterror() == 0)
  {
    Cell *pair;
    int k;
    buildstring(s);
    push(gettemplate("object"));
    make(APPLY);
    eval();
    pair = pop();
    push(template_nil);
    push(template_nil);
    push(pair->left);
    push(pair->right->left);
    push(template_nil);
    makeinverse(LIST);
    makeinverse(LIST);
    makeinverse(LIST);
    makeinverse(LIST);
    for(k=0; k < ObjectCount; k++)
      if(stack[k]->tag != LIST)
      {
        stack[k]= pop();
        return k; 
      }
    return ObjectCount++;
  }
  else
  {
    setstackheight(ObjectCount);
    interrupted = False;
    return -1;
  }
}

void DropRemote(int handle)
{
  if(handle < 0 || handle >= ObjectCount) return;
  stack[handle] = template_nil;
}

void PutRemote(int handle, char s[])
{
  Cell *list;
  if(handle < 0 || handle >= ObjectCount || stack[handle]->tag != LIST) return;
  list = stack[handle]->right;
  push(list->left);
  buildstring(s);
  make(LIST);
  list->left = pop();
}

bool CallRemote(int handle, char s[])
{
  if(handle < 0 || handle >= ObjectCount || stack[handle]->tag != LIST) return False;
  if(seterror() == 0)
  {
    Cell *res, *params, *list = stack[handle];
    list->left = template_nil;
    push(list->right->right->left);
    push(template_nil);
    for(params=list->right; params->left->tag==LIST; params->left=params->left->right)
    {
      push(params->left->left);
      make(LIST);
    }
    buildstring(s);
    push(list->right->right->right->left);
    make(APPLY);
    make(APPLY);
    make(APPLY);
    eval();
    res = pop();
    list->right->right->left = res->left;
    list->left = res->right->left;
    return True;
  }
  else
  {
    setstackheight(ObjectCount);
    stack[handle] = template_nil;
    interrupted = False;
    return False;
  }
}

bool GetRemote(int handle, char s[], int size)
{
  if(handle < 0 || handle >= ObjectCount || stack[handle]->tag != LIST) return False;
  if(seterror() == 0)
  {
    Cell *list = stack[handle];
    evaluate(list->left);
    if(list->left->tag != LIST) return False;
    fillstring(list->left->left, s, size);
    list->left = list->left->right;
    return True;
  }
  else
  {
    setstackheight(ObjectCount);
    stack[handle] = template_nil;
    interrupted = False;
    return False;
  }
}


/************************************************************
  Evaluates its argument to an integer and returns its value
*************************************************************/
Integer evalint(Cell *c, int hashtablenr)
{
  evaluate(c);
  if(c->tag == REAL)
  {
    Real r = real(c);
    Integer i;
    if(r >= LONG_MIN && r <= LONG_MAX && r == (i = r)) return i;
    runtimeerror(INT, hashtablenr);
  }
  return integer(c);
}

/************************************************************
  Evaluates its argument to a real and returns its value
*************************************************************/
Real evalreal(Cell *c)
{
  evaluate(c);
  if(c->tag == INT) return integer(c);
  return real(c);
}

/************************************************************
  comparison of cells
  returned values: -, 0, + indicating smaller, equal, bigger
*************************************************************/
int comparecell(Cell *c1, Cell *c2)
{
  Integer int_x;
  Real real_x;
  int cmp;

  evaluate(c1);
  evaluate(c2);
  switch(c1->tag)
  {
    case INT:
      if(c2->tag == INT)
      {
        int_x = integer(c1) - integer(c2);
        if(int_x < 0) return -1;
        if(int_x > 0) return 1;
        return 0;
      }
      else
      {
        real_x = integer(c1) - real(c2);
        if(real_x < 0) return -1;
        if(real_x > 0) return 1;
        return 0;
      }
    case REAL:
      if(c2->tag == INT)
        real_x = real(c1) - integer(c2);
      else
        real_x = real(c1) - real(c2);
      if(real_x < 0) return -1;
      if(real_x > 0) return 1;
      return 0;
    case LIST: case NIL:
      while(c1->tag == LIST && c2->tag == LIST)
      {
        cmp = comparecell(c1->left, c2->left);
        if(cmp != 0) return cmp;
        c1 = c1->right;
        c2 = c2->right;
        evaluate(c1);
        evaluate(c2);
      }
      if(c2->tag == LIST) return -1;
      if(c1->tag == LIST) return 1;
      return 0;
    case STRUCT:
      do
      {
        cmp = comparecell(c1->left, c2->left);
        if(cmp != 0) return cmp;
        c1 = c1->right;
        c2 = c2->right;
      }
      while(c1->tag == STRUCT);
      return 0;
    case PAIR:
      do
      {
        cmp = comparecell(c1->left, c2->left);
        if(cmp != 0) return cmp;
        c1 = c1->right;
        c2 = c2->right;
      }
      while(c1->tag == PAIR);
      return 0;
    default:
      return c1->value - c2->value;
  }
}


void applyIF(void)
{
  evaluatetop();
  push(top()->value ? getN(2) : getN(3));
  evaluatetop();
}

static void apply_SECTION(void)
{
  Cell *temp;
  push(temp = newcell(APPLY));
  temp->right = getN(3);
  temp = temp->left = newcell(APPLY);
  temp->right = getN(4);
  temp->left = getN(2);
}

static void applySTRICT(void)
{
  push(getN(2));
  eval();
  popN(1);
  push(newcell(APPLY));
  top()->left  = getN(2);
  top()->right = getN(3);
}

static void applyEQ(void)
{
  push(comparecell(getN(1), getN(2)) == 0 ? template_true:template_false);
}

static void applyNE(void)
{
  push(comparecell(getN(1), getN(2)) != 0 ? template_true:template_false);
}

static void applyLT(void)
{
  push(comparecell(getN(1), getN(2)) < 0 ? template_true:template_false);
}

static void applyLE(void)
{
  push(comparecell(getN(1), getN(2)) <= 0 ? template_true:template_false);
}

static void applyGT(void)
{
  push(comparecell(getN(1), getN(2)) > 0 ? template_true:template_false);
}

static void applyGE(void)
{
  push(comparecell(getN(1), getN(2)) >= 0 ? template_true:template_false);
}

static void applyPLUS(void)
{
  Cell *c1 = getN(1), *c2 = getN(2);
  evaluate(c1);
  evaluate(c2);
  if(c1->tag == INT)
  {
    if(c2->tag == INT)
      makeINT(integer(c1) + integer(c2));
    else
      makeREAL(integer(c1) + real(c2));
  }
  else
  {
    if(c2->tag == INT)
      makeREAL(real(c1) + integer(c2));
    else
      makeREAL(real(c1) + real(c2));
  }
}

static void applyNEG(void)
{
  Cell *c = getN(1);
  evaluate(c);
  if(c->tag == INT)
    makeINT(-integer(c));
  else
    makeREAL(-real(c));
}

static void applyMINUS(void)
{
  Cell *c1 = getN(1), *c2 = getN(2);
  evaluate(c1);
  evaluate(c2);
  if(c1->tag == INT)
  {
    if(c2->tag == INT)
      makeINT(integer(c1) - integer(c2));
    else
      makeREAL(integer(c1) - real(c2));
  }
  else
  {
    if(c2->tag == INT)
      makeREAL(real(c1) - integer(c2));
    else
      makeREAL(real(c1) - real(c2));
  }
}

static void applyTIMES(void)
{
  Cell *c1 = getN(1), *c2 = getN(2);
  evaluate(c1);
  evaluate(c2);
  if(c1->tag == INT)
  {
    if(c2->tag == INT)
      makeINT(integer(c1) * integer(c2));
    else
      makeREAL(integer(c1) * real(c2));
  }
  else
  {
    if(c2->tag == INT)
      makeREAL(real(c1) * integer(c2));
    else
      makeREAL(real(c1) * real(c2));
  }
}

static void applyDIVIDE(void)
{
  Real x = evalreal(getN(1)), y = evalreal(getN(2));
  if(y==0)
    makeconstant(ERROR, template_divide->value);
  else
    makeREAL(x/y);
}

static void applyDIV(void)
{
  Integer x = evalint(getN(1), template_div->value);
  Integer y = evalint(getN(2), template_div->value);
  if(y==0)
    makeconstant(ERROR, template_div->value);
  else
    makeINT(x/y);
}

static void applyMOD(void)
{
  Integer x = evalint(getN(1), template_mod->value);
  Integer y = evalint(getN(2), template_mod->value);
  if(y==0)
    makeconstant(ERROR, template_mod->value);
  else
    makeINT(x%y);
}

static void applyPOWER(void)
{
  Real r, x = evalreal(getN(1)), y = evalreal(getN(2));
  Integer i = y;
  if(x < 0 && y != i) runtimeerror(ERROR, template_power->value);
  if(x == 0)
  {
    if(y < 0)
      runtimeerror(ERROR, template_power->value);
    else if(y == 0)
      makeINT(1);
    else
      makeINT(0);
  }
  else
  {
    errno = 0;
    r = pow(x, y);
    if(errno != 0) runtimeerror(ERROR, template_power->value);
    if(r >= LONG_MIN && r <= LONG_MAX && r == (i = r))
      makeINT(i);
    else
      makeREAL(r);
  }
}

static void applyUPDATE(void)
{
  Cell *r1 = getN(1), *r2 = getN(2), *temp;
  evaluate(r1);
  evaluate(r2);
  if(r1->tag != RECORD || r2->tag != RECORD || r1->value != r2->value) runtimeerror(ERROR, template_update->value);
  push(temp = newcell(RECORD));
  temp->value = r1->value;
  for(;;)
  {
    temp->left = r2->left->tag == UNDEFINED ? r1->left : r2->left;
    r1 = r1->right;
    r2 = r2->right;
    if(r1->tag != RECORD || r2->tag != RECORD) break;
    temp = temp->right = newcell(RECORD);
  }
  temp->right = template_match;
  if(r1->tag == RECORD || r2->tag == RECORD) runtimeerror(ERROR, template_update->value);
}


/********************************************************************
  initialisation of hashtable with system functions
*********************************************************************/
static void initsyslib(void)
{
  Cell *obj = gettemplate("objecttype");
  inserttypestring("object", "[char] -> objecttype");
  insertabstype("object", obj);
  parsetypeexpr("(*, [char] -> [[char]] -> * -> (*, [[char]]))");
  makeconstant(FUNC, obj->value);
  makecompound(STRUCT, 1);
  make(TYPESYNONYM);
  inserttypeexpr("objecttype", pop());
  insertabstype("objecttype", obj);

  insert("_section", 3, FUNC      , NULL, apply_SECTION);
  insert("if"      , 3, FUNC      , NULL, applyIF);
  insert("^"       , 2, FUNC      , NULL, applyPOWER);
  insert("neg"     , 1, FUNC      , NULL, applyNEG);
  insert("*"       , 2, FUNC      , NULL, applyTIMES);
  insert("/"       , 2, FUNC      , NULL, applyDIV);
  insert("//"      , 2, FUNC      , NULL, applyDIVIDE);
  insert("%"       , 2, FUNC      , NULL, applyMOD);
  insert("+"       , 2, FUNC      , NULL, applyPLUS);
  insert("-"       , 2, FUNC      , NULL, applyMINUS);
  insert("="       , 2, FUNC      , NULL, applyEQ);
  insert("~="      , 2, FUNC      , NULL, applyNE);
  insert("<"       , 2, FUNC      , NULL, applyLT);
  insert("<="      , 2, FUNC      , NULL, applyLE);
  insert(">"       , 2, FUNC      , NULL, applyGT);
  insert(">="      , 2, FUNC      , NULL, applyGE);
  insert("&"       , 2, FUNC      , NULL, applyUPDATE);
  insert("True"    , 0, BOOLEAN   , NULL, NULL);
  insert("False"   , 0, BOOLEAN   , NULL, NULL);
  insert("pi"      , 0, REAL      , NULL, NULL);
  insert("Nil"     , 0, NIL       , NULL, NULL);
  insert(""        , 1, FUNC      , NULL, NULL);
  insert("strict"  , 2, FUNC      , NULL, applySTRICT);

  inserttypestring("_section"  , "(* -> ** -> ***) -> ** -> * -> ***");
  inserttypestring("if"        , "bool -> * -> * -> *");
  inserttypestring("^"         , "num -> num -> num");
  inserttypestring("neg"       , "num -> num");
  inserttypestring("*"         , "num -> num -> num");
  inserttypestring("/"         , "num -> num -> num");
  inserttypestring("//"        , "num -> num -> num");
  inserttypestring("%"         , "num -> num -> num");
  inserttypestring("+"         , "num -> num -> num");
  inserttypestring("-"         , "num -> num -> num");
  inserttypestring("="         , "* -> * -> bool");
  inserttypestring("~="        , "* -> * -> bool");
  inserttypestring("<"         , "* -> * -> bool");
  inserttypestring("<="        , "* -> * -> bool");
  inserttypestring(">"         , "* -> * -> bool");
  inserttypestring(">="        , "* -> * -> bool");
  inserttypestring("&"         , "* -> * -> *");
  inserttypestring("True"      , "bool");
  inserttypestring("False"     , "bool");
  inserttypestring("pi"        , "num");
  inserttypestring("Nil"       , "[*]");
  inserttypestring("strict"    , "(* -> **) -> * -> **");

  insertsys("strict");
  insertsys("^");
  insertsys("neg");
  insertsys("*");
  insertsys("/");
  insertsys("//");
  insertsys("%");
  insertsys("+");
  insertsys("-");
  insertsys("=");
  insertsys("~=");
  insertsys("<");
  insertsys("<=");
  insertsys(">");
  insertsys(">=");

  insertoperator("."   , 1, Right);
  insertoperator(":"   , 1, Right);
  insertoperator("&"   , 1, Left);
  insertoperator("!"   , 2, Left);
  insertoperator("^"   , 2, Right);
  insertoperator("*"   , 3, Left);
  insertoperator("/"   , 3, Left);
  insertoperator("//"  , 3, Left);
  insertoperator("%"   , 3, Left);
  insertoperator("++"  , 4, Right);
  insertoperator("--"  , 4, Left);
  insertoperator("+"   , 4, Left);
  insertoperator("-"   , 4, Left);
  insertoperator("="   , 5, Right);
  insertoperator("~="  , 5, Left);
  insertoperator("<"   , 5, Left);
  insertoperator("<="  , 5, Left);
  insertoperator(">"   , 5, Left);
  insertoperator(">="  , 5, Left);
  insertoperator("/\\" , 6, Right);
  insertoperator("\\/" , 7, Right);

  template_divide = gettemplate("//");
  template_div    = gettemplate("/");
  template_mod    = gettemplate("%");
  template_power  = gettemplate("^");
  template_update = gettemplate("&");
}


