/**********************************************************************
  Author : Dick Bruin
  Date   : 26/12/99
  Version: 2.02
  File   : ammodify.c

  Description:

  modification of the parsetree for the Amanda interpreter
***************************************************************************/

#include <string.h>
#include "amtypes.h"
#include "ammodify.h"
#include "amtable.h"
#include "amerror.h"
#include "ammem.h"
#include "amstack.h"
#include "ampatter.h"
#include "amprint.h"

static char functionname[stringsize];

static void initmodifyfunction(char name[])
{
  char *pos;
  strcpy(functionname, name);
  pos = strstr(functionname, anonymousstring);
  if(pos) *pos = '\0';
  storefunctionname(name);
}

/***********************************************************************
  change an if with 3 arguments to a _if
  change an application of a sysfunc to a sysfunc
  change an application of a function with enough arguments to an APPLICATION
  introduce indirect functions
  perform some partial evaluation (:, \/, /\, ., ~)
  NB: this has been hard coded
************************************************************************/
static int value_if, value_not, value_cons, value_and, value_or, value_compose, value_neg;

static void modify_expr(Cell **c)
{
  FuncDef *fun;
  Cell *app = *c, *temp;
  int argcount;

  if(app == NULL)
    return;
  else if(app->tag == APPLY || app->tag == FUNC)
  {
    argcount = 0;
    while(app->tag == APPLY)
    {
      modify_expr(&app->right);
      app = app->left;
      argcount++;
    }
    if(app->tag != FUNC)
      return;
    fun = getfunction(app->value);
    if(argcount < fun->argcount)
      return;
    while(argcount > fun->argcount)
    {
      c = &((*c)->left);
      argcount--;
    }
    if(argcount == 0 && constantfunction(app->value))
      return;
    if(argcount == 1 && fun->sysfunc)
    {
      temp = newcell(SYSFUNC1);
      temp->value = app->value;
      temp->left = (*c)->right;
    }
    else if(argcount == 1 && app->value == value_not)
    {
      temp = newcell(_IF);
      temp->left         = (*c)->right;
      temp->right        = newcell(_IF);
      temp->right->left  = template_false;
      temp->right->right = template_true;
    }
    else if(argcount == 2 && fun->sysfunc)
    {
      temp = newcell(SYSFUNC2);
      temp->value = app->value;
      temp->right = (*c)->right;
      temp->left  = (*c)->left->right;
    }
    else if(argcount == 2 && app->value == value_cons)
    {
      temp = newcell(LIST);
      temp->right = (*c)->right;
      temp->left  = (*c)->left->right;
    }
    else if(argcount == 2 && app->value == value_and)
    {
      temp = newcell(_IF);
      temp->left         = (*c)->left->right;
      temp->right        = newcell(_IF);
      temp->right->left  = (*c)->right;
      temp->right->right = template_false;
    }
    else if(argcount == 2 && app->value == value_or)
    {
      temp = newcell(_IF);
      temp->left         = (*c)->left->right;
      temp->right        = newcell(_IF);
      temp->right->left  = template_true;
      temp->right->right = (*c)->right;
    }
    else if(argcount == 3 && app->value == value_if)
    {
      temp = newcell(_IF);
      temp->left = (*c)->left->left->right;
      temp->right = newcell(_IF);
      temp->right->left = (*c)->left->right;
      temp->right->right = (*c)->right;
    }
    else if(argcount == 3 && app->value == value_compose)
    {
      temp = newcell(APPLY);
      temp->left = (*c)->left->left->right;
      temp->right = newcell(APPLY);
      temp->right->left = (*c)->left->right;
      temp->right->right = (*c)->right;
      modify_expr(&temp);
    }
    else
    {
      for(app=*c; app->tag==APPLY; app=app->left) push(app->right);
      makeAPPLICATION(app->value, argcount);
      temp = pop();
    }
    *c = temp;
  }
  else if(app->tag > compositetag)
  {
    modify_expr(&app->left);
    modify_expr(&app->right);
  }
}


static bool constant(Cell *c);

static void constantify(Cell **c)
{
  if(constant(*c) && (*c)->tag > evaltag)
  {
    Cell *temp = newcell(CONST);
    temp->left = *c;
    *c = temp;
  }
}

static bool constant(Cell *c)
{
  if(c == NULL)
    return False;
  else if(c->tag < nonevaltag)
    return True;
  else if(c->tag > evaltag)
  {
    bool constleft = constant(c->left), constright = constant(c->right);
    return constleft && constright;
  }
  else if(c->tag == CONST)
    return True;
  else if(c->tag == FUNC)
    return constantfunction(c->value);
  else if(c->tag == APPLY && c->left->tag == FUNC && c->left->value == value_neg)
  {
    if(c->right->tag == INT)
    {
      c->tag = INT;
      integer(c) = -integer(c->right);
      return True;
    }
    else if(c->right->tag == REAL)
    {
      c->tag = REAL;
      real(c) = -real(c->right);
      return True;
    }
  }
  else if(c->tag == LETREC)
  {
    constantify(&c->right);
    for(c=c->left; c->tag==LIST; c=c->right) constantify(&c->left);
  }
  else if(c->tag > matchtag)
  {
    constantify(&c->left);
    constantify(&c->right);
  }
  return False;
}

static Cell *reorganise(int level, ExpressionType exprtype, Cell *c);

/********************************************************************
  replaces in c all occurences of (ARG level) by (ARG newlevel)
  returns True iff c contains (ARG level)
*********************************************************************/
static bool replacearg(int level, int newlevel, Cell *c)
{
  bool result = False;
  if(c == NULL) return result;
  if(c->tag == ARG || c->tag == MATCHARG)
  {
    if(c->value == level)
    {
      c->value = newlevel;
      result = True;
    }
  }
  if(c->tag > compositetag)
  {
    if(replacearg(level, newlevel, c->left )) result = True;
    if(replacearg(level, newlevel, c->right)) result = True;
  }
  return result;
}

static void increasearg(Cell *c, int n)
{
  if(c == NULL) return;
  if(c->tag == ARG || c->tag == MATCHARG) c->value += n;
  if(c->tag > compositetag)
  {
    increasearg(c->left, n);
    increasearg(c->right, n);
  }
}


/********************************************************************
   input:
     def         = a function definition
     neededlevel = a number of argumentforms
     argcount    = the number of arguments of the section

   intention:
     an anonymous function F is created for def
     the section: F arg[1] .. arg[neededlevel] is returned

   simplification:
     only the arguments arg[1] .. arg[neededlevel] which occur
     in def are used
*********************************************************************/
static Cell *anonymoussection(Cell *def, int neededlevel, int argcount)
{
  int k, newlevel = 1, makeapplcount;
  Cell *args = template_nil;

  for(k=1; k<=neededlevel; k++)
    if(replacearg(k, newlevel, def))
    {
      Cell *list = newcell(LIST);
      list->left = newcell(ARG);
      list->left->value = k;
      list->right = args;
      args = list;
      newlevel++;
    }
  for(; k<=neededlevel+argcount; k++)
  {
    replacearg(k, newlevel, def);
    newlevel++;
  }
  newlevel--;
  makeapplcount = 0;
  for(;args->tag == LIST; args = args->right)
  {
    push(args->left);
    makeapplcount++;
  }
  push(anonymousfunction(newlevel, def));
  while(makeapplcount-- > 0) make(APPLY);
  return pop();
}


/*******************************************************************
  input : list with generators, level = number of environment variables
  output: a set expression

  GENERATOR ---- GENERATOR ---- GENERATOR  ----     ... template_nil
     |              |              |
                 GENERATOR --
                    |

  c points to the first cell, next points to the second cell
  the second GENERATOR is a list generator
  the variable(list) is found at the left of the left
  the expression(list) is found at the right of the left
*********************************************************************/

static Cell *replace_listcomprehension(int level, Cell *c)
{
  Cell *next = c->right, *generator, *expr, *filter = NULL, *arg, *directors = template_nil;
  int filtercount = 0, generatorcount = 0;
  bool singlegenerator, set2;

  generator = next->left->left;
  expr = next->left->right;
  while(generator->tag == LIST && expr->tag == LIST)
  {
    generatorcount++;
    arg = newcell(ARG);
    arg->value = level + generatorcount;
    expr->left = reorganise(level, EXP, expr->left);
    singlegenerator = generator->left->tag == VARIABLE;
    generator->left = reorganise(level, PAT, generator->left);
    if(!singlegenerator)
    {
      push(newcell(MATCHARG));
      top()->value = arg->value;
      top()->left  = matchpattern(generator->left);
      filtercount++;
    }
    directors = appenddirectors(directors, arg, generator->left, STRICTDIRECTOR);
    generator = generator->right;
    expr = expr->right;
  }
  c = replacedirectors(directors, c);

  if(filtercount > 0)
  {
    Cell *temp = pop();
    while(--filtercount > 0)
    {
      top()->right = temp;
      temp = pop();
    }
    push(temp);
    filtercount = 1;
  }

  while(next->right->tag == GENERATOR && next->right->left->right == NULL)
  {
    push(reorganise(level+generatorcount, EXP, next->right->left->left));
    filtercount++;
    next->right = next->right->right;
  }
  if(filtercount>0)
  {
    while(--filtercount>0)
    {
      push(template_false);
      make_IF();
    }
    filter = anonymoussection(pop(), level, generatorcount);
  }

  set2 = next->right->tag == GENERATOR;
  if(set2)
  {
    c->right = next->right;
    c = replace_listcomprehension(level+generatorcount, c);
  }
  else if(c->left->right->tag == LIST)
  {
    push(NULL);
    push(NULL);
    push(reorganise(level+generatorcount, EXP, c->left));
    makeset(SET1, 1);
    c = pop();
    set2 = True;
  }
  else
  {
    c = reorganise(level+generatorcount, EXP, c->left->left);
    if(generatorcount == 1 && singlegenerator && c->tag == ARG && c->value == level+1) c = NULL;
  }

  push(c ? anonymoussection(c, level, generatorcount) : NULL);
  push(filter);
  for(expr=next->left->right; expr->tag==LIST; expr=expr->right)
    push(expr->left);
  makeset(set2 ? SET2 : SET1, generatorcount);
  return pop();
}

static Cell *extenddefinition(Cell *definition, Cell *c)
{
  if(definition == NULL || definition->tag == ERROR) return c;
  if(definition->tag == _IF)
  {
    definition->right->right = extenddefinition(definition->right->right, c);
    return definition;
  }
  modifyerror(1, functionname);
  return definition;
}

/**********************************************************************
  input : c = the right hand side of a where clause
  return: True iff c should be implemented with an auxillary  function
**********************************************************************/
static bool lazylocal(Cell *c)
{
  switch(c->tag)
  {
    case _IF: case LETREC: case MATCH:
      return True;
    case APPLY:
      {
        int argcount = 0;
        for(; c->tag==APPLY; c=c->left) argcount++;
        return c->tag == FUNC
            && (   (argcount == 1 && c->value == value_not)
                || (argcount == 2 && (c->value == value_or || c->value == value_and))
                || (argcount == 3 && c->value == value_if));
      }
  }
  return False;
}

/**********************************************************************
  input : c = partial record
  return: a full record without field names and fields properly aligned
**********************************************************************/
static Cell *modifyrecord(Cell *c)
{
  int funnr = getfunction(c->left->left->value)->typeexpr->left->left->value;
  Cell *type = getfunction(funnr)->typeexpr->right;
  Cell *temp = newcell(RECORD);
  Cell *t1 = type, *t2 = temp;
  temp->value = funnr;
  for(;;)
  {
    t2->left = newcell(UNDEFINED);
    t1 = t1->right;
    if(t1->tag != RECORD) break;
    t2 = t2->right = newcell(RECORD);
  }
  t2->right = template_match;
  while(c->tag == RECORD)
  {
    t1 = type; t2 = temp;
    while(t1->left->left->value != c->left->left->value)
    {
      t1 = t1->right;
      t2 = t2->right;
    }
    if(t2->left->tag != UNDEFINED)
    {
      Write("\n%s", getfunction(c->left->left->value)->name);
      modifyerror(4, functionname);
    }
    t2->left = c->left->right;
    c = c->right;
  }
  return temp;
}

/**********************************************************************
  level = the largest number of an argument of an enclosing function
  c     = parseroutput with all name clashes removed
  return c with listcomprehensions and local definitions properly reorganised

  for (partial) functions matches on the argument patterns are added
  the argument patterns are replaced by arg cells
  the arguments are numbered from level+1 to level+argcount

  for letrecs the local definitions names are replaced by arg cells
  the arg cells are numbered from 1-whereargcount to 0
  multiple local function definitions are grouped together (extended)
  the body of a local function is replaced by an anonymoussection
  in order for this to work well on deeper levels the argument numbers
  are temporarily increased by the whereargcount

  for every local definition the pair head + def is replaced by its def
  the value of the letrec cell is filled in
***********************************************************************/
static Cell *reorganise(int level, ExpressionType exprtype, Cell *c)
{
  Cell *directors = template_nil, *temp, *head, *dup, *duphead, *arg, *cmp;
  int k, argcount, dupargcount, whereargcount;

  if(c == NULL) return NULL;
  switch(exprtype)
  {
    case PAT:
      if(c->tag == UNDEFINED)
        c = template_match;
      else if(c->tag == RECORD)
      {
        c = modifyrecord(c);
        for(temp=c; temp->tag==RECORD; temp=temp->right)
          temp->left = reorganise(level, PAT, temp->left);
      }
      else if(c->tag == PAIR || c->tag == STRUCT)
      {
        for(temp=c, k=0; temp->tag==c->tag; temp=temp->right, k++)
          temp->left = reorganise(level, PAT, temp->left);
        c->value = k;
      }
      else if(c->tag > compositetag)
      {
        c->left  = reorganise(level, PAT, c->left);
        c->right = reorganise(level, PAT, c->right);
      }
      break;
    case FUN:
      {
        cmp = NULL;
        argcount = 0;
        for(head=c->left; head->tag==APPLY; head=head->left) argcount++;
        k = level + argcount;
        for(head=c->left; head->tag==APPLY; head=head->left)
        {
          arg = newcell(ARG);
          arg->value = k--;
          head->right = reorganise(level, PAT, head->right);
          if(head->right->tag != VARIABLE)
          {
            temp = newcell(MATCHARG);
            temp->value = arg->value;
            temp->left  = matchpattern(head->right);
            temp->right = cmp;
            cmp = temp;
          }
          directors = appenddirectors(directors, arg, head->right, STRICTDIRECTOR);
        }
        if(cmp)
        {
          push(cmp);
          push(c->right);
          push(makeerror());
          make_IF();
          c->right = pop();
        }
        c->right = reorganise(level+argcount, EXP, replacedirectors(directors, c->right));
      }
      break;
    case EXP:
      if(c->tag == GENERATOR)
      {
        if(c->right->tag != GENERATOR)
          c = reorganise(level, EXP, c->left);
        else if(c->right->left->right == NULL)
        {
          push(reorganise(level, EXP, c->right->left->left));
          c->right = c->right->right;
          push(reorganise(level, EXP, c));
          push(template_nil);
          make_IF();
          c = pop();
        }
        else
          c = replace_listcomprehension(level, c);
      }
      else if(c->tag == LETREC)
      {
        whereargcount = 0;
        for(temp=c->left; temp->tag==LIST; temp=temp->right)
        {
          if(temp->left->left->tag != APPLY && temp->left->left->tag != VARIABLE)
          {
            temp->left->left = reorganise(level, PAT, temp->left->left);
            head = newcell(MATCH);
            head->value = makeerror()->value;
            head->left  = matchpattern(temp->left->left);
            head->right = temp->left->right;
            temp->left->right = head;
          }
          head = temp->left->left;
          while(head->tag == APPLY) head = head->left;
          if(finddirector(directors, head) == NULL)
          {
            arg = newcell(ARG);
            arg->value = -(whereargcount++);
            directors = appenddirectors(directors, arg, head, LAZYDIRECTOR);
          }
        }
        c->value = whereargcount;
        increasearg(replacedirectors(directors, c), whereargcount);
        c->right = reorganise(level+whereargcount, EXP, c->right);
        for(temp=c->left; temp->tag==LIST; temp=temp->right)
        {
          if(temp->left->left->tag != APPLY)
          {
            temp->left = reorganise(level+whereargcount, EXP, temp->left->right);
            if(temp->left->tag == ARG)
              temp->left = emptydirector(temp->left);
            else if(lazylocal(temp->left))
              temp->left = anonymoussection(temp->left, level+whereargcount, 0);
          }
          else
          {
            argcount = 0;
            for(head=temp->left->left; head->tag==APPLY; head=head->left) argcount++;
            temp->left = reorganise(level+whereargcount, FUN, temp->left);
            dup=temp;
            while(dup->right->tag==LIST)
            {
              dupargcount = 0;
              for(duphead=dup->right->left->left; duphead->tag==APPLY; duphead=duphead->left) dupargcount++;
              if(duphead->tag == ARG && duphead->value == head->value)
              {
                if(dupargcount != argcount) modifyerror(2, functionname);
                dup->right->left = reorganise(level+whereargcount, FUN, dup->right->left);
                temp->left->right = extenddefinition(temp->left->right, dup->right->left->right);
                dup->right = dup->right->right;
              }
              else
                dup = dup->right;
            }
            temp->left = anonymoussection(temp->left->right, level+whereargcount, argcount);
          }
        }
        increasearg(c, -whereargcount);
      }
      else if(c->tag == LAMBDAS)
      {
        argcount = 0;
        push(template_match);
        temp = c->left;
        while(temp->tag == LAMBDA)
        {
          push(temp->left);
          makeinverse(APPLY);
          argcount++;
          temp = temp->right;
        }
        push(temp);
        makeinverse(LIST);
        c->left = reorganise(level, FUN, pop())->right;
        for(dup=c->right; dup->tag==LAMBDAS; dup=dup->right)
        {
          dupargcount = 0;
          push(template_match);
          temp = dup->left;
          while(temp->tag == LAMBDA)
          {
            push(temp->left);
            makeinverse(APPLY);
            dupargcount++;
            temp = temp->right;
          }
          push(temp);
          makeinverse(LIST);
          if(dupargcount != argcount) modifyerror(2, functionname);
          c->left = extenddefinition(c->left, reorganise(level, FUN, pop())->right);
        }
        c = anonymoussection(c->left, level, argcount);
      }
      else if(c->tag == MATCHTYPE)
        modifyerror(5, functionname);
      else if(c->tag == RECORD)
      {
        c = modifyrecord(c);
        for(temp=c; temp->tag==RECORD; temp=temp->right)
          temp->left = reorganise(level, EXP, temp->left);
      }
      else if(c->tag == PAIR || c->tag == STRUCT)
      {
        for(temp=c, k=0; temp->tag==c->tag; temp=temp->right, k++)
          temp->left = reorganise(level, EXP, temp->left);
        c->value = k;
      }
      else if(c->tag > compositetag)
      {
        if(c->tag > matchtag) c->left  = reorganise(level, EXP, c->left);
        c->right = reorganise(level, EXP, c->right);
      }
      break;
  }
  return c;
}


static void reorganisefuncdef(FuncDef *fun)
{
  if(fun->def && strstr(fun->name, anonymousstring) == NULL)
  {
    Cell *def = NULL, *temp;
    checkmemlarge();
    initmodifyfunction(fun->name);
    for(temp=fun->def; temp->tag==LIST; temp=temp->right)
    {
      temp->left = reorganise(0, FUN, temp->left);
      def = extenddefinition(def, temp->left->right);
    }
    fun->def = def;
  }
}

static bool newconstants;

static void constantifyfuncdef(FuncDef *fun)
{
  if(fun->def == NULL)
    ;
  else if(fun->argcount > 0)
  {
    checkmemlarge();
    constantify(&fun->def);
  }
  else if(fun->def->tag == FUNC && constantfunction(fun->def->value))
  {
    fun->def = getfunction(fun->def->value)->def;
    newconstants = True;
  }
  else if(!(fun->def->tag == CONST || fun->def->tag < nonevaltag))
  {
    checkmemlarge();
    constantify(&fun->def);
    if(fun->def->tag == CONST || fun->def->tag < nonevaltag) newconstants = True;
  }
}

static void modifyfuncdef(FuncDef *fun)
{
  if(fun->def)
  {
    checkmemlarge();
    modify_expr(&fun->def);
  }
}

static void constantifyanonymous(FuncDef *fun)
{
  if(fun->def && strncmp(fun->name, anonymousstring, lengthanonymousstring) == 0)
    constantify(&fun->def);
}

static void modifyanonymous(FuncDef *fun)
{
  if(fun->def && strncmp(fun->name, anonymousstring, lengthanonymousstring) == 0)
    modify_expr(&fun->def);
}

/***************************** access functions *************************/

void initmodify(void)
{
  value_if      = gettemplate("if")->value;
  value_not     = gettemplate("~")->value;
  value_cons    = gettemplate(":")->value;
  value_and     = gettemplate("/\\")->value;
  value_or      = gettemplate("\\/")->value;
  value_compose = gettemplate(".")->value;
  value_neg     = gettemplate("neg")->value;
}

Cell *modify_expression(Cell *c)
{
  initmodifyfunction("");
  c = reorganise(0, EXP, c);
  forallhashtable(constantifyanonymous);
  forallhashtable(modifyanonymous);
  modify_expr(&c);
  return c;
}


void modify_definitions(void)
{
  forallhashtable(reorganisefuncdef);
  do
  {
    newconstants = False;
    forallhashtable(constantifyfuncdef);
  }
  while(newconstants);
  forallhashtable(modifyfuncdef);
}

