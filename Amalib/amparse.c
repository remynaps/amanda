/**********************************************************************
  Author : Dick Bruin
  Date   : 26/12/99
  Version: 2.02
  File   : amparse.c

  Description:

  the parser for the Amanda interpreter
***************************************************************************/


#include <string.h>
#include <stdlib.h>
#include "amtypes.h"
#include "amparse.h"
#include "amtable.h"
#include "amerror.h"
#include "amlex.h"
#include "ammem.h"
#include "amstack.h"
#include "ampatter.h"

static void parsename(void)
{
  if(strcmp(tokenval, "-") == 0)
    push(gettemplate("neg"));
  else if(strcmp(tokenval, "num") == 0)
    makeconstant(MATCHTYPE, INT);
  else if(strcmp(tokenval, "bool") == 0)
    makeconstant(MATCHTYPE, BOOLEAN);
  else if(strcmp(tokenval, "char") == 0)
    makeconstant(MATCHTYPE, CHAR);
  else
    push(gettemplate(tokenval));
  gettoken();
}

static void parseapplication(void);
static void parseexpression(int prio);

static void parsesection(int prio)
{
  while(tokentype == OPERATOR)
  {
    Cell *temp  = gettemplate(tokenval);
    FuncDef *fun = getfunction(temp->value);

    if(fun->prio > prio) break;
    push(temp);
    make(APPLY);
    gettoken();
    if(tokentype == RPAR) break;
    parseexpression(fun->assoc==Left ? fun->prio-1 : fun->prio);
    makeinverse(APPLY);
  }
}

static void parseexpression(int prio)
{
  int lambdacount;
  parseapplication();
  if(tokentype != ARROW || prio < MAXPRIO)
  {
    parsesection(prio);
    return;
  }
  lambdacount = 1;
  for(;;)
  {
    int count = 0;
    while(tokentype == ARROW)
    {
      checkpattern(top());
      count++;
      gettoken();
      parseapplication();
    }
    parsesection(MAXPRIO);
    while(count-- > 0) makeinverse(LAMBDA);
    if(tokentype != BAR) break;
    lambdacount++;
    gettoken();
    parseapplication();
  }
  push(template_match);
  while(lambdacount-- > 0) makeinverse(LAMBDAS);
}

static void buildstring(char *s)
{
  int count = 0;
  for(;*s != '\0'; s++)
  {
    makeconstant(CHAR, *s);
    count++;
  }
  push(template_nil);
  while(count-->0) makeinverse(LIST);
}

/*********************************************************************
  everything between a list bar and a rbrack is transformed in a
  generator structure:
  GENERATOR ---- GENERATOR  ----     ... NULL
     |               |
  GENERATOR --
     |
  in case of an ordinary expression the right part is NULL
  in case of a generator the variable(list) is left and the list-expr(list) right
**********************************************************************/
static void parsegenerators(int *count)
{
  Cell *temp;

  for(;;)
  {
    int varcount = 0, exprcount = 0;
    while(tokentype == SEP) gettoken();
    if(tokentype == RBRACK) break;
    push(temp = newcell(GENERATOR));
    temp->value = getPositionCode();
    (*count)++;
    for(;;)
    {
      parseexpression(MAXPRIO);
      varcount++;
      if(tokentype != COMMA) break;
      gettoken();
    }
    if(tokentype == GENER || tokentype == ASSIGNMENT)
    {
      bool assignment = tokentype == ASSIGNMENT;
      do
      {
        gettoken();
        parseexpression(MAXPRIO);
        exprcount++;
        if(assignment)
        {
          push(template_nil);
          makeinverse(LIST);
        }
      }
      while(tokentype == COMMA);
      if(exprcount != varcount) parseerror(32);
      push(template_nil);
      while(exprcount-- > 0) makeinverse(LIST);
      temp->right = pop();
      push(template_nil);
      while(varcount-- > 0) makeinverse(LIST);
      checkpattern(temp->left = pop());
    }
    else if(varcount > 1)
      parseerror(31);
    else
      temp->left = pop();
  }
}

static void parselist(void)
{
  int count = 0;

  gettoken();
  if(tokentype != RBRACK)
  {
    parseexpression(MAXPRIO);
    count++;
  }
  while(tokentype == COMMA)
  {
    gettoken();
    parseexpression(MAXPRIO);
    count++;
  }
  if(tokentype == RBRACK)
  {
    push(template_nil);
    while(count-->0) makeinverse(LIST);
  }
  else if(tokentype == BAR && count >= 1)
  {
    push(template_nil);
    while(count-->0) makeinverse(LIST);
    count = 1;
    gettoken();
    parsegenerators(&count);
    push(template_nil);
    while(count-->0) makeinverse(GENERATOR);
  }
  else if(tokentype == POINTS && count == 1)
  {
    gettoken();
    if(tokentype == RBRACK)
    {
      push(gettemplate("nats"));
      make(APPLY);
    }
    else
    {
      push(gettemplate("nat"));
      make(APPLY);
      parseexpression(MAXPRIO);
      makeinverse(APPLY);
    }
  }
  else if(tokentype == POINTS && count == 2)
  {
    gettoken();
    if(tokentype == RBRACK)
    {
      rotatestack();
      push(gettemplate("gennats"));
      make(APPLY);
      make(APPLY);
    }
    else
    {
      rotatestack();
      push(gettemplate("gennat"));
      make(APPLY);
      make(APPLY);
      parseexpression(MAXPRIO);
      makeinverse(APPLY);
    }
  }
  if(tokentype != RBRACK) parseerror(1);
  gettoken();
}

static void parseterm(void)
{
  int count;
  switch(tokentype)
  {
    case NUMBER:
      if(strchr(tokenval, '.') == NULL)
        makeINT(atol(tokenval));
      else
        makeREAL(atof(tokenval));
      gettoken();
      break;
    case IDENTIFIER:
      parsename();
      break;
    case TYPEID:
      push(gettemplate(tokenval));
      makecompound(STRUCT, 1);
      gettoken();
      break;
    case CHARACTER:
      makeconstant(CHAR, tokenval[0]);
      gettoken();
      break;
    case STRING:
      buildstring(tokenval);
      gettoken();
      break;
    case LPAR:
      gettoken();
      if(tokentype == OPERATOR && strcmp(tokenval, "-") != 0)
      {
        parsename();
        if(tokentype != RPAR)
        {
          parseexpression(MAXPRIO);
          rotatestack();
          push(gettemplate("_section"));
          make(APPLY);
          make(APPLY);
        }
      }
      else if(tokentype == RPAR)
        makeconstant(NULLTUPLE, 0);
      else
      {
        parseexpression(MAXPRIO);
        if(tokentype == COMMA)
        {
          count = 1;
          while(tokentype == COMMA)
          {
            gettoken();
            parseexpression(MAXPRIO);
            count++;
          }
          makecompound(PAIR, count);
        }
      }
      if(tokentype != RPAR) parseerror(2);
      gettoken();
      break;
    case LBRACK:
      parselist();
      break;
    case LACC:
      count = 0;
      do
      {
        gettoken();
        if(tokentype != IDENTIFIER) parseerror(25);
        push(gettemplate(tokenval));
        gettoken();
        if(strcmp(tokenval, "=") != 0) parseerror(5);
        gettoken();
        parseexpression(MAXPRIO);
        makeinverse(ALIAS);
        count++;
      }
      while(tokentype == COMMA);
      makecompound(RECORD, count);
      if(tokentype != RACC) parseerror(33);
      gettoken();
      break;
    default:
      parseerror(3);
  }
}

static void parseapplication(void)
{
  if(tokentype == TYPEID)
  {
    int count = 1;
    push(gettemplate(tokenval));
    gettoken();
    while(tokentype == NUMBER
       || tokentype == IDENTIFIER
       || tokentype == TYPEID
       || tokentype == CHARACTER
       || tokentype == STRING
       || tokentype == LPAR
       || tokentype == LBRACK
       || tokentype == LACC)
    {
      parseterm();
      count++;
    }
    makecompound(STRUCT, count);
  }
  else if(tokentype == OPERATOR)
    parsename();
  else
    parseterm();
  while(tokentype == NUMBER
     || tokentype == IDENTIFIER
     || tokentype == TYPEID
     || tokentype == CHARACTER
     || tokentype == STRING
     || tokentype == LPAR
     || tokentype == LBRACK
     || tokentype == LACC)
  {
    parseterm();
    makeinverse(APPLY);
  }
  if(tokentype == OPERATOR && strcmp(tokenval, ":") == 0)
  {
    gettoken();
    if(tokentype == RPAR)
    {
      push(gettemplate(":"));
      make(APPLY);
    }
    else
    {
      parseapplication();
      makeinverse(LIST);
    }
  }
}

static void parselefthandside(void)
{
  parseapplication();
  for(;;)
    if(tokentype == OPERATOR && strcmp(tokenval, "=") != 0)
    {
      parsename();
      makeinverse(APPLY);
    }
    else if(tokentype == LPAR)
    {
      gettoken();
      parseexpression(MAXPRIO);
      makeinverse(APPLY);
      if(tokentype != RPAR) parseerror(2);
      gettoken();
    }
    else
      break;
}

static Cell *extenddefinition(Cell *definition, Cell *c)
{
  if(definition == NULL) parseerror(4);
  if(definition->tag == ERROR) return c;
  if(definition->tag == _IF)
  {
    definition->right->right = extenddefinition(definition->right->right, c);
    return definition;
  }
  parseerror(4);
  return definition;
}

static void parseexpressionclause(void)
{
  Cell *definition = makeerror();

  if(strcmp(tokenval, "=") != 0) parseerror(5);
  do
  {
    gettoken();
    parseexpression(MAXPRIO);
    if(tokentype == COMMA)
    {
      gettoken();
      if(strcmp(tokenval, "if") == 0)
        gettoken();
      if(tokentype == OTHERWISE)
        gettoken();
      else
      {
        push(makeerror());
        makeinverse(_IF);
        parseexpression(MAXPRIO);
        make(_IF);
      }
    }
    definition = extenddefinition(definition, pop());
    while(tokentype == SEP) gettoken();
    if(tokentype == offside)
    {
      tokenoffside--;
      gettoken();
      tokenoffside++;
    }
  }
  while(strcmp(tokenval, "=") == 0);
  push(definition);
}

static void parsedefinition(bool globallevel);

static void parsewhereclause(void)
{
  int globaltokenoffside = tokenoffside, count = 0;

  while(tokentype == IDENTIFIER
     || tokentype == OPERATOR
     || tokentype == TYPEID
     || tokentype == NUMBER
     || tokentype == CHARACTER
     || tokentype == STRING
     || tokentype == LBRACK
     || tokentype == LACC
     || tokentype == LPAR)
  {
    parsedefinition(False);
    count++;
    tokenoffside = globaltokenoffside;
    if(tokentype == offside) gettoken();
  }
  if(count > 0)
  {
    push(template_nil);
    while(count-->0) makeinverse(LIST);
    make(LETREC);
  }
}

typedef enum { NOCHECK, COLLECT, CHECK } CheckTypeVariable;
static CheckTypeVariable checktypevariable;
static Cell *typevariables;

static void setchecktypevariables(CheckTypeVariable check)
{
  if(check == COLLECT) typevariables = template_nil;
  checktypevariable = check;
}

static Cell *maketypevariable(char name[])
{
  Cell *temp;
  int k = strlen(name), l;
  for(l=0; l<k; l++)
    if(name[l] != '*') parseerror(6);
  switch(checktypevariable)
  {
    case COLLECT:
      push(typevariables);
      push(temp = newcell(TYPEVAR));
      temp->value = k;
      make(LIST);
      typevariables = pop();
      return temp;
    case CHECK:
      temp = typevariables;
      while(temp->tag==LIST && temp->left->value != k) temp = temp->right;
      if(temp->tag != LIST) parseerror(7);
      return temp->left;
    default:
      temp = newcell(TYPEVAR);
      temp->value = k;
      return temp;
  }
}

typedef enum { TYPEEXPR, TYPETERM } TypeType;

static void parsetype(TypeType typetype)
{
  switch(tokentype)
  {
    case IDENTIFIER:
      if(strcmp(tokenval, "num") == 0)
      {
        push(newcell(INT));
        gettoken();
      }
      else if(strcmp(tokenval, "char") == 0)
      {
        push(newcell(CHAR));
        gettoken();
      }
      else if(strcmp(tokenval, "bool") == 0)
      {
        push(newcell(BOOLEAN));
        gettoken();
      }
      else
      {
        int count = 1;
        push(gettemplate(tokenval));
        gettoken();
        if(typetype == TYPEEXPR)
          while(tokentype == IDENTIFIER
             || tokentype == OPERATOR
             || tokentype == LBRACK
             || tokentype == LPAR)
          {
            parsetype(TYPETERM);
            count++;
          }
        makecompound(STRUCT, count);
      }
      break;
    case OPERATOR:
      push(maketypevariable(tokenval));
      gettoken();
      break;
    case LPAR:
      gettoken();
      if(tokentype == RPAR)
        push(newcell(NULLTUPLE));
      else
      {
        parsetype(TYPEEXPR);
        if(tokentype == COMMA)
        {
          int count = 1;
          while(tokentype == COMMA)
          {
            gettoken();
            parsetype(TYPEEXPR);
            count++;
          }
          makecompound(PAIR, count);
        }
      }
      if(tokentype != RPAR) parseerror(2);
      gettoken();
      break;
    case LBRACK:
      gettoken();
      parsetype(TYPEEXPR);
      push(template_nil);
      makeinverse(LIST);
      if(tokentype != RBRACK) parseerror(1);
      gettoken();
      break;
    default:
      parseerror(8);
  }
  if(typetype == TYPEEXPR && tokentype == ARROW)
  {
    gettoken();
    parsetype(TYPEEXPR);
    makeinverse(APPLY);
  }
}

static void makerecordfield(Cell *recordtype, Cell *field, Cell *fieldtype)
{
  char *fieldname = getfunction(field->value)->name;
  Cell *var = newcell(VARIABLE);
  var->value = 1;
  var->left = field;
  push(fieldtype);
  push(recordtype);
  make(APPLY);
  if(!inserttypeexpr(fieldname, pop())) parseerror(12);
  push(var);
  push(var);
  push(field);
  make(ALIAS);
  makecompound(RECORD, 1);
  push(field);
  make(APPLY);
  make(LIST);
  if(!insert(fieldname, 1, FUNC, pop(), NULL)) parseerror(18);
}

static void parsestructdef(void)
{
  char structname[stringsize];
  char *headname;
  int count;
  Cell *head = pop();

  setchecktypevariables(COLLECT);
  push(template_match);
  for(; head->tag==APPLY; head=head->left)
  {
    if(head->right->tag != UNDEFINED && head->right->tag != FUNC) parseerror(9);
    push(maketypevariable(getfunction(head->right->value)->name));
    make(STRUCT);
  }
  if(head->tag != UNDEFINED && head->tag != FUNC) parseerror(10);
  headname = getfunction(head->value)->name;
  makeconstant(FUNC, head->value);
  make(STRUCT);
  setchecktypevariables(CHECK);
  gettoken();
  head = top();
  if(tokentype == LACC)
  {
    count = 0;
    do
    {
      gettoken();
      if(tokentype != IDENTIFIER) parseerror(25);
      push(gettemplate(tokenval));
      gettoken();
      if(tokentype != COLONS) parseerror(15);
      gettoken();
      parsetype(TYPEEXPR);
      makerecordfield(head, getN(2), getN(1));
      makeinverse(TYPEDEF);
      count++;
    }
    while(tokentype == COMMA);
    makecompound(RECORD, count);
    makeinverse(TYPEDEF);
    if(tokentype != RACC) parseerror(33);
    gettoken();
  }
  else
  {
    for(;;)
    {
      if(tokentype != TYPEID) parseerror(11);
      strcpy(structname, tokenval);
      count = 0;
      gettoken();
      while(tokentype == IDENTIFIER
         || tokentype == OPERATOR
         || tokentype == LBRACK
         || tokentype == LPAR)
      {
        parsetype(TYPETERM);
        count++;
      }
      push(head);
      while(count-- > 0) makeinverse(APPLY);
      if(!inserttypeexpr(structname, pop())) parseerror(12);
      if(tokentype != BAR) break;
      gettoken();
    }
  }
  if(!inserttypeexpr(headname, pop())) parseerror(12);
  setchecktypevariables(NOCHECK);
}

static void parsetypesynonym(void)
{
  Cell *head = pop();

  setchecktypevariables(COLLECT);
  push(template_match);
  for(; head->tag==APPLY; head=head->left)
  {
    if(head->right->tag != UNDEFINED && head->right->tag != FUNC) parseerror(9);
    push(maketypevariable(getfunction(head->right->value)->name));
    make(STRUCT);
  }
  if(head->tag != UNDEFINED && head->tag != FUNC) parseerror(10);
  makeconstant(FUNC, head->value);
  make(STRUCT);
  setchecktypevariables(CHECK);
  gettoken();
  parsetype(TYPEEXPR);
  makeinverse(TYPESYNONYM);
  if(!inserttypeexpr(getfunction(head->value)->name, pop())) parseerror(12);
  setchecktypevariables(NOCHECK);
}

static void parseabstype(void)
{
  Cell *head, *abstype;
  int globaltokenoffside;

  gettoken();
  parselefthandside();
  abstype = pop();
  while(abstype->tag == APPLY) abstype = abstype->left;
  if(abstype->tag != UNDEFINED && abstype->tag != FUNC) parseerror(13);
  if(!insertabstype(getfunction(abstype->value)->name, abstype)) parseerror(12);
  if(tokentype != WITH) parseerror(14);
  globaltokenoffside = tokenoffside;
  tokenoffside = tokenindent + 1;
  gettoken();
  while(tokentype == IDENTIFIER || tokentype == OPERATOR || tokentype == LPAR)
  {
    int temptokenoffside = tokenoffside;
    parselefthandside();
    tokenoffside = tokenindent + 1;
    if(tokentype != COLONS) parseerror(15);
    head = pop();
    if(head->tag != UNDEFINED && head->tag != FUNC) parseerror(13);
    gettoken();
    parsetype(TYPEEXPR);
    if(!inserttypeexpr(getfunction(head->value)->name, pop()))
      parseerror(12);
    if(!insertabstype(getfunction(head->value)->name, abstype))
      parseerror(12);
    while(tokentype == SEP) gettoken();
    tokenoffside = temptokenoffside;
    if(tokentype == offside) gettoken();
  }
  tokenoffside = globaltokenoffside;
  if(tokentype == offside) gettoken();
}

/***********************************************************************
  renamerec renames variables (function argument patterns,
  patterns of simple local definitions, names of local functions,
  patterns of generators of listcomprehensions)
  every variable is replaced by a cell of type VARIABLE with a
  unique value
  moreover local definitions of simple where clauses are lifted upward
  local definitions of the form (x = y) are removed
************************************************************************/
static Cell *renamerec(ExpressionType exprtype, Cell *c)
{
  Cell *renamelist = template_nil, *temp, *head;

  if(c == NULL) return NULL;
  switch(exprtype)
  {
    case FUN:
      c->right = renamerec(EXP, c->right);
      for(head=c->left; head->tag==APPLY; head=head->left)
        renamelist = appendrenamelistrec(renamelist, True, head->right);
      c = replacerenamelist(renamelist, c);
      break;
    case EXP:
      if(c->tag == GENERATOR)
      {
        for(head=c->right; head->tag==GENERATOR && head->left->right==NULL; head=head->right)
          head->left->left = renamerec(EXP, head->left->left);
        if(head->tag == GENERATOR)
        {
          temp = c->right;
          c->right = head->right;
          c = renamerec(EXP, c);
          c->right = temp;
          head->left->right = renamerec(EXP, head->left->right);
          renamelist = appendrenamelistrec(renamelist, True, head->left->left);
          c->left = replacerenamelist(renamelist, c->left);
          head->right = replacerenamelist(renamelist, head->right);
          head->left->left = replacerenamelist(renamelist, head->left->left);
        }
        else
          c->left = renamerec(EXP, c->left);
      }
      else if(c->tag == LETREC)
      {
        c->right = renamerec(EXP, c->right);
        for(temp=c->left; temp->tag==LIST; temp=temp->right)
        {
          head = temp->left = renamerec(FUN, temp->left);
          if(head->left->tag == APPLY)
          {
            while(head->left->tag==APPLY) head = head->left;
            renamelist = appendrenamelistrec(renamelist, False, head->left);
          }
          else
            renamelist = appendrenamelistrec(renamelist, True, head->left);
        }
        c = replacerenamelist(renamelist, c);
        for(temp=c->left; temp->tag==LIST; temp=temp->right)
        {
          head = temp->left;
          if(head->left->tag != APPLY
          && head->right->tag == LETREC)
          {
            Cell *last = temp;
            while(last->right->tag == LIST) last = last->right;
            last->right = head->right->left;
            head->right = head->right->right;
          }
          if(head->left->tag == VARIABLE && head->right->tag == VARIABLE)
          {
            Cell *directors = appenddirectors(template_nil, head->right, head->left, LAZYDIRECTOR);
            if(head->left->value == head->right->value)
              parseerror(16);
            if(temp == c->left)
              c->left = temp->right;
            else
            {
              Cell *before = c->left;
              while(before->right != temp) before = before->right;
              before->right = temp->right;
            }
            c = replacedirectors(directors, c);
          }
        }
      }
      else if(c->tag == LAMBDA)
      {
        c->right = renamerec(EXP, c->right);
        renamelist = appendrenamelistrec(renamelist, True, c->left);
        c = replacerenamelist(renamelist, c);
      }
      else if(c->tag > compositetag)
      {
        c->left  = renamerec(EXP, c->left);
        c->right = renamerec(EXP, c->right);
      }
      break;
  }
  return c;
}

static void parsedefinition(bool globallevel)
{
  Cell *head;
  int globaltokenoffside = tokenindent, posCode;
  bool generic = False;

  if(tokentype == ABSTYPE && globallevel)
  {
    parseabstype();
    while(tokentype == SEP) gettoken();
    return;
  }
  else if(tokentype == GENERIC && globallevel)
  {
    generic = True;
    gettoken();
  }
  parselefthandside();
  posCode = getPositionCode();
  tokenoffside = tokenindent + 1;
  if(tokentype == COLONS && globallevel)
  {
    head = pop();
    if(head->tag != UNDEFINED && head->tag != FUNC) parseerror(13);
    gettoken();
    parsetype(TYPEEXPR);
    if(!inserttypeexpr(getfunction(head->value)->name, pop())) parseerror(12);
    getfunction(head->value)->generic = generic;
    while(tokentype == SEP) gettoken();
  }
  else if(tokentype == DEF && globallevel)
  {
    parsestructdef();
    while(tokentype == SEP) gettoken();
  }
  else if(tokentype == SYN && globallevel)
  {
    parsetypesynonym();
    while(tokentype == SEP) gettoken();
  }
  else
  {
    head = top();
    if(head->tag == APPLY || globallevel)
    {
      for(; head->tag==APPLY; head=head->left) checkpattern(head->right);
      if(head->tag != UNDEFINED && head->tag != FUNC) parseerror(17);
      if(globallevel) storefunctionname(getfunction(head->value)->name);
    }
    else
      checkpattern(head);
    parseexpressionclause();
    if(tokentype == WHERE)
    {
      gettoken();
      parsewhereclause();
    }
    else if(tokentype == offside)
    {
      tokenoffside = globaltokenoffside;
      gettoken();
      if(tokentype == WHERE)
      {
        tokenoffside = tokenindent + 1;
        gettoken();
        parsewhereclause();
      }
    }
    makeinverse(LIST);
    top()->value = posCode;
    if(globallevel)
    {
      Cell *def = pop();
      int argcount = 0;
      char *funname;
      head = def;
      for(head=head->left; head->tag==APPLY; head=head->left) argcount++;
      funname = getfunction(head->value)->name;
      initrename(funname);
      def = renamerec(FUN, def);
      if(!insert(funname, argcount, FUNC, def, NULL)) parseerror(18);
    }
  }
}


void parsefile(char filename[])
{
  setchecktypevariables(NOCHECK);
  openfileinput(filename);
  tokenoffside = 0;
  gettoken();
  while(tokentype != empty)
  {
    checkmemlarge();
    parsedefinition(True);
    tokenoffside = 0;
    if(tokentype == offside) gettoken();
  }
  closeinput();
}

bool parseinput(char s[])
{
  bool result = False;

  setchecktypevariables(NOCHECK);
  openinput(s);
  checkmemlarge();
  storefunctionname("");
  tokenoffside = 0;
  gettoken();
  if(tokentype == empty)
  {
    push(template_nil);
    return result;
  }
  parseexpression(MAXPRIO);
  if(tokentype == WHERE)
  {
    gettoken();
    parsewhereclause();
  }
  if(tokentype == COLONS) result = True;
  else if(tokentype != empty) parseerror(19);
  closeinput();
  initrename("");
  settop(renamerec(EXP, top()));
  return result;
}

void parsetypeexpr(char s[])
{
  setchecktypevariables(NOCHECK);
  openinput(s);
  checkmemlarge();
  tokenoffside = 0;
  gettoken();
  parsetype(TYPEEXPR);
  closeinput();
}

