/**********************************************************************
  Author : Dick Bruin
  Date   : 26/12/99, 20/03/2012
  Version: 2.02, 2.05
  File   : amcheck.c

  Description:

  type check of function definitions and expressions
***********************************************************************/


#include <stdio.h>
#include <string.h>
#include "amtable.h"
#include "ammem.h"
#include "amerror.h"
#include "amlex.h"
#include "amprint.h"

#define MAXTYPINGCYCLES 25
#define MAXTYPESIZE 1000

static Cell *make(TagType tag, Cell *left, Cell *right)
{
  Cell *temp = newcell(tag);
  temp->left  = left;
  temp->right = right;
  return temp;
}

/********************* type datastructures ***********************
  structure of the bindings list: [[variable|typeexpr]]
  a typevariable is associated with a typeexpression
  findbinding: finds for a typevariable the typeexpression which
               is associated with it in one or more steps
               non-present variables are associated with themselves
  addbinding : adds an association of a variable and typeexpression
               and checks if the bindings stay acyclic

  structure of the names list: [[name|typevariable]]
  a name (renamed pattern variable) is associated with a typevariable
  getvariable: finds the typevariable associated with a name
               non-present names are associated with a new typevariable

  stucture of a variablelist: [variable]
  a list of variables occuring in a typeexpression
  the place (index) of a variable is used for renaming purposes
  addvariable  : non-present variables are appended at the tail of the list
                 the (new) index of the variable is returned

  a hashing technique is used to speed up access to the bindings- and
  names datastructures
***********************************************************************/

#define BINDINGSSIZE 10
#define NAMESSIZE    10

static int  variablenumber, positionCode;
static Cell *bindings[BINDINGSSIZE];
static Cell *names[NAMESSIZE];
static Cell *abstypedef;
static char *functionname;
static Cell *clausehead, *typecontext;

static void initfunction(char *funname, Cell *abstype, Cell *clause)
{
  int k;
  functionname   = funname;
  abstypedef     = abstype;
  clausehead     = clause;
  typecontext    = NULL;
  variablenumber = 0;
  positionCode   = 0;
  for(k=0; k<BINDINGSSIZE; k++) bindings[k] = template_nil;
  for(k=0; k<NAMESSIZE   ; k++) names[k]    = template_nil;
}

static void swapcontext(Cell **context)
{
  Cell *temp = *context;
  *context = typecontext;
  typecontext = temp;
}

static void swapPosCode(int *n)
{
  int temp = *n;
  *n = positionCode;
  positionCode = temp;
}

static void typeerror(void)
{
  if(typecontext) 
  { 
    Write("\ncontext  : "); 
    WriteCell(typecontext); 
  }
  if(positionCode > 0)
  {
    Write("\nfile     : %s", posCodeFileName(positionCode)); 
    Write("\nnear line: %d", posCodeLinenr(positionCode)); 
  }
  else if(clausehead) 
  { 
    Write("\nclause   : "); 
    WriteCell(clausehead); 
  }
  else if(functionname) 
    Write("\nfunction : %s", functionname);
  systemerror(19);
}

static void globaltypeerror(char message[])
{
  Write("\n%s", message);
  functionname = NULL;
  clausehead = NULL;
  typecontext = NULL;
  positionCode = 0;
  typeerror();
}

/************************* variablelist datastructure ******************/

static int addvariable(Cell *var, Cell **variablelist)
{
  Cell **temp = variablelist;
  int number = 1;
  while((*temp)->tag == LIST)
  {
    if((*temp)->left->value == var->value) return number;
    temp = &((*temp)->right);
    number++;
  }
  *temp = make(LIST, var, template_nil);
  return number;
}

/*********************************************************************
  check whether a typeexpression has been refined
  an expression is refined if a typevariable is bound to an expression
  or if the number of free typevariables has been reduced
**********************************************************************/
static Cell *findbinding(Cell *);

static Cell *oldvariablelist, *newvariablelist;

static bool refinedrec(Cell *type)
{
  if(type->tag == TYPEVAR)
  {
    addvariable(type, &oldvariablelist);
    type = findbinding(type);
    if(type->tag != TYPEVAR) return True;
    addvariable(type, &newvariablelist);
    return False;
  }
  else if(type->tag > compositetag)
    return refinedrec(type->left) || refinedrec(type->right);
  return False;
}

static bool checkrefined(Cell *type)
{
  Cell *temp;
  int lengthold = 0, lengthnew = 0;
  newvariablelist = oldvariablelist = template_nil;
  if(refinedrec(type)) return True;
  for(temp=oldvariablelist; temp->tag==LIST; temp=temp->right) lengthold++;
  for(temp=newvariablelist; temp->tag==LIST; temp=temp->right) lengthnew++;
  return lengthnew < lengthold;
}

/**********************************************************************
  expansion of a typeexpression
  recursively the bounded typevariables are replaced by the
  corresponding typeexpressions
  the numbering of the typevariables is adapted
***********************************************************************/
static Cell *expandvariablelist;

static Cell *expandtyperec(Cell *type)
{
  if(type->tag == TYPEVAR) type = findbinding(type);
  if(type->tag == TYPEVAR)
  {
    Cell *temp = newcell(TYPEVAR);
    temp->value = addvariable(type, &expandvariablelist);
    return temp;
  }
  else if(type->tag > compositetag)
    return make(type->tag, expandtyperec(type->left), expandtyperec(type->right));
  return type;
}

static Cell *expandtype(Cell *type)
{
  expandvariablelist = template_nil;
  return expandtyperec(type);
}

/************************ bindings datastructure **************************/

static Cell *findbinding(Cell *var)
{
  for(;;)
  {
    Cell *temp = bindings[var->value % BINDINGSSIZE];
    while(temp->tag==LIST && temp->left->left->value!=var->value) temp = temp->right;
    if(temp->tag != LIST) return var;
    var = temp->left->right;
    if(var->tag != TYPEVAR) return var;
  }
}

static bool occurs(Cell *var, Cell *type)
{
  if(type->tag == TYPEVAR) type = findbinding(type);
  if(type->tag == TYPEVAR) return type->value == var->value;
  if(type->tag > compositetag)
    return occurs(var, type->left) || occurs(var, type->right);
  return False;
}

/* if typeexpr is a typevariable it is not bound to anything */
static bool addbinding(Cell *var, Cell *typeexpr)
{
  if(typeexpr->tag == TYPEVAR && var->value == typeexpr->value)
    return True;
  else if(typeexpr->tag != TYPEVAR && occurs(var, typeexpr))
    return False;
  else
  {
    Cell **bds = &(bindings[var->value % BINDINGSSIZE]);
    *bds = make(LIST, make(LIST, var, typeexpr), *bds);
    return True;
  }
}

/********************* names datastructure ***************************/

static Cell *newvariable(void)
{
  Cell *temp = newcell(TYPEVAR);
  temp->value = ++variablenumber;
  return temp;
}

static Cell *getvariable(Cell *name)
{
  Cell **nms = &(names[name->value % NAMESSIZE]);
  Cell *temp = *nms;
  while(temp->tag==LIST && temp->left->left->value!=name->value) temp = temp->right;
  if(temp->tag != LIST)
    temp = *nms = make(LIST, make(LIST, name, newvariable()), *nms);
  return temp->left->right;
}

/*********************************************************************
  recursive copy of a typeexpression
  typevariables are replaced by a new typevariable
  variablenumber is adapted accordingly
**********************************************************************/
static int variableoffset;

static Cell *copytypeexprrec(Cell *type)
{
  if(type->tag == TYPEVAR)
  {
    Cell *temp  = newcell(TYPEVAR);
    temp->value = variableoffset + type->value;
    if(temp->value > variablenumber) variablenumber = temp->value;
    return temp;
  }
  else if(type->tag > compositetag)
    return make(type->tag, copytypeexprrec(type->left), copytypeexprrec(type->right));
  return type;
}

static Cell *copytypeexpr(Cell *type)
{
  variableoffset = variablenumber;
  return copytypeexprrec(type);
}

static int typesize(Cell *type)
{
  if(type->tag > compositetag)
    return 1 + typesize(type->left) + typesize(type->right);
  return 1;
}

/***********************************************************************
  input: type1 = typeexpression
         type2 = typeexpression
  the typeexpressions are unified and matching variables are put
  in the bindings list
************************************************************************/
static bool unifyabstype(Cell *abstype, Cell *type);

static bool unify(Cell *type1, Cell *type2)
{
  if(type1->tag == TYPEVAR) type1 = findbinding(type1);
  if(type2->tag == TYPEVAR) type2 = findbinding(type2);

  if(type1->tag == TYPEVAR)
    return addbinding(type1, type2);
  else if(type2->tag == TYPEVAR)
    return addbinding(type2, type1);
  else if(abstypedef && type1->tag == STRUCT && type1->left->value == abstypedef->value)
    return unifyabstype(type1, type2);
  else if(abstypedef && type2->tag == STRUCT && type2->left->value == abstypedef->value)
    return unifyabstype(type2, type1);
  else if(type1->tag != type2->tag)
    return False;
  else if(type1->tag == LIST)
    return unify(type1->left, type2->left);
  else if(type1->tag == PAIR)
  {
    Cell *t1 = type1, *t2 = type2;
    while(t1->tag==PAIR && t2->tag==PAIR)
    {
      if(!unify(t1->left, t2->left)) return False;
      t1 = t1->right;
      t2 = t2->right;
    }
    return t1->tag == t2->tag;
  }
  else if(type1->tag == STRUCT)
  {
    if(type1->left->value != type2->left->value)
      return False;
    else
    {
      Cell *t1 = type1->right, *t2 = type2->right;
      while(t1->tag==STRUCT && t2->tag==STRUCT)
      {
        if(!unify(t1->left, t2->left)) return False;
        t1 = t1->right;
        t2 = t2->right;
      }
      return t1->tag == t2->tag;
    }
  }
  else if(type1->tag == APPLY)
    return unify(type1->left, type2->left) && unify(type1->right, type2->right);
  else
    return True;
}

static bool unifyabstype(Cell *abstype, Cell *type)
{
  FuncDef *fun = getfunction(abstype->left->value);
  if(fun->typeexpr && fun->typeexpr->tag == TYPESYNONYM)
  {
    Cell *synonym = copytypeexpr(fun->typeexpr),
         *t1 = synonym->left->right,
         *t2 = abstype->right;
    while(t1->tag==STRUCT && t2->tag==STRUCT)
    {
      if(!unify(t1->left, t2->left)) return False;
      t1 = t1->right;
      t2 = t2->right;
    }
    if(t1->tag != t2->tag) return False;
    return unify(synonym->right, type);
  }
  else
  {
    Write("\nmissing declaration for type: ");
    WriteType(abstype);
    typeerror();
    return False;
  }
}

static void tryunify(Cell *type1, Cell *type2)
{
  if(!unify(type1, type2))
  {
    Cell *list = make(LIST, type1, type2);
    list = expandtype(list);
    Write("\ncannot unify: ");
    WriteType(list->left);
    Write(" with: ");
    WriteType(list->right);
    typeerror();
  }
}

static void checkunify(Cell *type1, Cell *type2, Cell *c, Cell *context)
{
  swapcontext(&context);
  if(!unify(type1, type2))
  {
    Cell *list = make(LIST, type1, type2);
    list = expandtype(list);
    Write("\nincorrect: ");
    WriteCell(c);
    Write("\ntype1    : ");
    WriteType(list->left);
    Write("\ntype2    : ");
    WriteType(list->right);
    typeerror();
  }
  swapcontext(&context);
}

/************************************************************************
  input : c = parsetree of a function or an expression
  return: corresponding typeexpression
          the bindingslist and the nameslist are used
*************************************************************************/

static Cell *findtypeapply(Cell *c, Cell *org);

static Cell *findtype(Cell *c)
{
  Cell *temp, *new, *t1, *t2, *org = c;
  int posCode;
  if(c)
    switch(c->tag)
    {
      case UNDEFINED:
        Write("\nundefined function: ");
        Write("%s", getfunction(c->value)->name);
        typeerror();
        break;
      case VARIABLE:
        return getvariable(c);
      case INT: case REAL:
        return newcell(INT);
      case CHAR:
        return newcell(CHAR);
      case BOOLEAN:
        return newcell(BOOLEAN);
      case NULLTUPLE:
        return newcell(NULLTUPLE);
      case NIL:
        return make(LIST, newvariable(), template_nil);
      case MATCHTYPE:
        return newcell(c->value);
      case LIST:
        org = c;
        temp = findtype(c->left);
        new = make(LIST, temp, template_nil);
        for(c=c->right; c->tag==LIST; c=c->right)
          checkunify(temp, findtype(c->left), c->left, org);
        checkunify(new, findtype(c), c, org);
        return new;
      case PAIR:
        new = temp = newcell(PAIR);
        temp->left = findtype(c->left);
        for(c=c->right; c->tag==PAIR; c=c->right)
        {
          temp = temp->right = newcell(PAIR);
          temp->left = findtype(c->left);
        }
        temp->right = template_match;
        return new;
      case STRUCT:
        temp = getfunction(c->left->value)->typeexpr;
        if(temp)
        {
          temp = copytypeexpr(temp);
          t1 = c->right;
          while(temp->tag == APPLY && t1->tag == STRUCT)
          {
            checkunify(temp->left, findtype(t1->left), t1->left, org);
            temp = temp->right;
            t1 = t1->right;
          }
          if(temp->tag != APPLY && t1->tag != STRUCT) return temp;
          Write("\nnonconforming constructor: ");
          WriteCell(c);
          typeerror();
          break;
        }
        Write("\nunknown constructor: ");
        Write("%s", getfunction(c->left->value)->name);
        typeerror();
        break;
      case RECORD:
        new = newvariable();
        for(;;)
        {
          temp = getfunction(c->left->left->value)->typeexpr;
          if(temp == NULL || temp->tag != APPLY || temp->left->tag != STRUCT) break;
          temp = getfunction(temp->left->left->value)->typeexpr;
          if(temp == NULL || temp->tag != TYPEDEF) break;
          do
            temp = temp->right;
          while(temp->tag == RECORD && temp->left->left->value != c->left->left->value);
          if(temp->tag != RECORD) break;
          temp = findtype(c->left->left);
          checkunify(new, temp->left, c->left, org);
          checkunify(findtype(c->left->right), temp->right, c->left, org);
          c = c->right;
          if(c->tag != RECORD) return new;
        }
        Write("\nunknown recordfield: ");
        Write("%s", getfunction(c->left->left->value)->name);
        typeerror();
        break;
      case GENERATOR:
        temp = findtype(c->left);
        for(c=c->right; c->tag==GENERATOR; c=c->right)
        {
          posCode = c->left->value;
          swapPosCode(&posCode);
          if(c->left->right)
          {
            t1 = c->left->left;
            t2 = c->left->right;
            while(t1->tag == LIST && t2->tag == LIST)
            {
              checkunify(make(LIST, findtype(t1->left), template_nil), findtype(t2->left), t2->left, org);
              t1 = t1->right;
              t2 = t2->right;
            }
          }
          else
            checkunify(newcell(BOOLEAN), findtype(c->left->left), c->left->left, org);
          swapPosCode(&posCode);
        }
        return temp;
      case LETREC:
        for(temp=c->left; temp->tag==LIST; temp=temp->right)
        {
          posCode = temp->left->value;
          swapPosCode(&posCode);
          checkunify(findtype(temp->left->left), findtype(temp->left->right), temp->left->right, temp->left->left);
          swapPosCode(&posCode);
        }
        return findtype(c->right);
      case FUNC:
        temp = getfunction(c->value)->typeexpr;
        if(temp) return copytypeexpr(temp);
        Write("\nuntyped function: ");
        Write("%s", getfunction(c->value)->name);
        typeerror();
        break;
      case LAMBDA:
        return make(APPLY, findtype(c->left), findtype(c->right));
      case LAMBDAS:
        new = findtype(c->left);
        for(temp=c->right; temp->tag==LAMBDAS; temp=temp->right)
          checkunify(new, findtype(temp->left), temp->left, org);
        return new;
      case ALIAS:
        temp = findtype(c->right);
        checkunify(temp, findtype(c->left), c->left, org);
        return temp;
      case _IF:
        checkunify(newcell(BOOLEAN), findtype(c->left), c->left, org);
        temp = findtype(c->right->left);
        checkunify(temp, findtype(c->right->right), c->right->right, org);
        return temp;
      case APPLY:
        return findtypeapply(c, c);
    }
  return newvariable();
}

static Cell *findtypeapply(Cell *c, Cell *org)
{
  Cell *temp, *new;
  if(c->tag != APPLY) return findtype(c);
  temp = findtypeapply(c->left, org);
  if(temp->tag == TYPEVAR) temp = findbinding(temp);
  if(temp->tag == APPLY)
  {
    checkunify(temp->left, findtype(c->right), c->right, org);
    return temp->right;
  }
  new = newvariable();
  checkunify(make(APPLY, findtype(c->right), new), temp, c->left, org);
  return new;
}


/********************* type check access functions *********************/

static bool newtypes;
static int synonymcount, abstypecount;

void checkexpression(Cell *c, bool print)
{
  initfunction(NULL, NULL, NULL);
  c = findtype(c);
  if(print) WriteType(expandtype(c));
}

static Cell *substituteabstypes(Cell *c)
{
  if(c == NULL) return c;
  if(c->tag > compositetag)
  {
    c->left  = substituteabstypes(c->left);
    c->right = substituteabstypes(c->right);
  }
  if(c->tag == STRUCT)
  {
    FuncDef *fun = getfunction(c->left->value);
    Cell *synonym = fun->abstype;
    if(synonym && synonym->tag == TYPESYNONYM)
    {
      newtypes = True;
      synonym = copytypeexpr(synonym);
      tryunify(synonym->left, c);
      return synonym->right;
    }
  }
  return c;
}

static void countabstype(FuncDef *fun)
{
  if(fun->typeexpr && fun->typeexpr->tag == TYPESYNONYM && fun->abstype)
  {
    fun->abstype = fun->typeexpr;
    abstypecount++;
  }
}

static void expandabstype(FuncDef *fun)
{
  Cell *typeexpr = fun->abstype;
  if(typeexpr && typeexpr->tag == TYPESYNONYM)
  {
    checkmemlarge();
    initfunction(fun->name, NULL, NULL);
    typeexpr = copytypeexpr(typeexpr);
    typeexpr->right = substituteabstypes(typeexpr->right);
    fun->abstype = expandtype(typeexpr);
  }
}

static Cell *substitutesynonyms(Cell *c)
{
  if(c == NULL) return c;
  if(c->tag > compositetag)
  {
    c->left  = substitutesynonyms(c->left);
    c->right = substitutesynonyms(c->right);
  }
  if(c->tag == STRUCT)
  {
    FuncDef *fun = getfunction(c->left->value);
    Cell *synonym = fun->typeexpr;
    if(synonym && synonym->tag == TYPESYNONYM && fun->abstype == NULL)
    {
      newtypes = True;
      synonym = copytypeexpr(synonym);
      tryunify(synonym->left, c);
      return synonym->right;
    }
  }
  return c;
}

static void countsynonym(FuncDef *fun)
{
  if(fun->typeexpr && fun->typeexpr->tag == TYPESYNONYM && fun->abstype == NULL) synonymcount++;
}

static void expandsynonym(FuncDef *fun)
{
  Cell *typeexpr = fun->typeexpr;
  if(typeexpr && typeexpr->tag == TYPESYNONYM && fun->abstype == NULL)
  {
    checkmemlarge();
    initfunction(fun->name, NULL, NULL);
    typeexpr = copytypeexpr(typeexpr);
    typeexpr->right = substitutesynonyms(typeexpr->right);
    fun->typeexpr = expandtype(typeexpr);
  }
}

static void modifytypeexpr(FuncDef *fun)
{
  if(fun->def && fun->typeexpr == NULL)
  {
    fun->typeexpr = newcell(TYPEVAR);
    fun->typeexpr->value = 1;
  }
  else if(fun->typeexpr && (fun->typeexpr->tag != TYPESYNONYM || fun->abstype))
  {
    checkmemlarge();
    initfunction(fun->name, NULL, NULL);
    fun->typeexpr = expandtype(substitutesynonyms(copytypeexpr(fun->typeexpr)));
  }
}

static void settypechanged(FuncDef *fun)
{
  if(fun->def && !fun->generic)
    fun->typechanged = True;
  else
    fun->typechanged = False;
}

static bool typemaychange(Cell *c)
{
  if(c == NULL)
    ;
  else if((c->tag == FUNC || c->tag == APPLICATION)
          && getfunction(c->value)->typechanged)
    return True;
  else if(c->tag > compositetag)
    return typemaychange(c->left) || typemaychange(c->right);
  return False;
}

static void checkfuncdef(FuncDef *fun)
{
  Cell *temp, *typetemplate, *spine, *functiontype;
  int posCode;
  if(fun->def && !fun->generic && (fun->typechanged || typemaychange(fun->def)))
  {
    fun->typechanged = False;
    for(temp=fun->def; temp->tag==LIST; temp=temp->right)
    {
      checkmemlarge();
      initfunction(fun->name, fun->abstype, temp->left->left);
      posCode = temp->left->value;
      swapPosCode(&posCode);
      typetemplate = findtype(fun->template);
      functiontype = findtype(temp->left->right);
      for(spine=temp->left->left; spine->tag==APPLY; spine=spine->left)
        functiontype = make(APPLY, findtype(spine->right), functiontype);
      checkunify(typetemplate, functiontype, fun->template, NULL);
      if(!checkrefined(typetemplate))
        ;
      else if(fun->abstype)
      {
        Write("\ntypedefinition too general");
        Write("\nfunction : %s", fun->name);
        typeerror();
      }
      else
      {
        newtypes = fun->typechanged = True;
        fun->typeexpr = expandtype(typetemplate);
        if(typesize(fun->typeexpr) > MAXTYPESIZE)
        {
          Write("\ntype too large\n");
          WriteType(fun->typeexpr);
          Write("\nfunction : %s", fun->name);
          typeerror();
        }
      }
      swapPosCode(&posCode);
    }
  }
}

static void checkgeneric(FuncDef *fun)
{
  Cell *temp, *spine, *functiontype, *new, *new1, *new2;
  int posCode;
  if(fun->def && fun->generic)
  {
    for(temp=fun->def; temp->tag==LIST; temp=temp->right)
    {
      checkmemlarge();
      initfunction(fun->name, fun->abstype, temp->left->left);
      posCode = temp->left->value;
      swapPosCode(&posCode);
      functiontype = new = newvariable();
      for(spine=temp->left->left; spine->tag==APPLY; spine=spine->left)
        functiontype = make(APPLY, findtype(spine->right), functiontype);
      checkunify(functiontype, findtype(fun->template), fun->template, NULL);
      new1 = copytypeexpr(expandtype(functiontype));
      checkunify(new, findtype(temp->left->right), fun->template, NULL);
      new2 = copytypeexpr(expandtype(functiontype));
      if(!unify(new1, new2) || checkrefined(new1))
      {
        Write("\nnonconforming generic definition: ");
        WriteCell(temp->left->left);
        Write("\nusing patterns  : ");
        WriteType(new1);
        Write("\nusing definition: ");
        WriteType(new2);
        typeerror();
      }
      swapPosCode(&posCode);
    }
  }
}

void checkdefinitions(void)
{
  int cycles;

  synonymcount = 0;
  forallhashtable(countsynonym);
  cycles = 0;
  do
  {
    newtypes = False;
    forallhashtable(expandsynonym);
  }
  while(newtypes && cycles++<synonymcount);
  if(newtypes) globaltypeerror("cyclic type synonyms");

  forallhashtable(modifytypeexpr);

  abstypecount = 0;
  forallhashtable(countabstype);
  cycles = 0;
  do
  {
    newtypes = False;
    forallhashtable(expandabstype);
  }
  while(newtypes && cycles++<abstypecount);
  if(newtypes) globaltypeerror("cyclic abstypes");

  forallhashtable(settypechanged);
  cycles = 0;
  do
  {
    newtypes = False;
    forallhashtable(checkfuncdef);
  }
  while(newtypes && cycles++<MAXTYPINGCYCLES);
  if(newtypes)
  {
    Write("\nrestriction: type too complicated or cyclic types");
    Write("\n");
    printusertypes();
    globaltypeerror("");
  }
  forallhashtable(checkgeneric);
}

