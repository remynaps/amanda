/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amtable.c

  Description:

  Hash table with function definitions
***********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "amtypes.h"
#include "amtable.h"
#include "amerror.h"
#include "ammem.h"
#include "amparse.h"
#include "amprint.h"
#include "amstack.h"

#define hashtablesize 2999

#define anonymousprefix            '_'

/*
#define strdup(s) strcpy(malloc(strlen(s)+1), s)
*/

Cell *template_true,
     *template_false,
     *template_nil,
     *template_match;
int sysvalue_fread;

FuncDef *hashtable = NULL;

static int hash(char name[])
{
  int val = 0, factor = 1;

  for(; *name; name++)
  {
    if(factor>4) factor = 1;
    val += *name * factor++;
    while(val>=hashtablesize) val -= hashtablesize;
  }
  return val;
}

static int getfunctionnr(char name[])
{
  int pos = hash(name), count = 0;

  while(hashtable[pos].name != NULL && strcmp(hashtable[pos].name, name) != 0)
  {
    if(count++>hashtablesize) systemerror(3);
    if(++pos>=hashtablesize) pos -= hashtablesize;
  }
  if(hashtable[pos].name == NULL)
  {
    FuncDef *temp = &hashtable[pos];
    temp->name = strdup(name);
    if(temp->name == NULL) systemerror(4);
    if('A' <= name[0] && name[0] <= 'Z')
    {
      temp->tag = TYPE;
      temp->template->tag = TYPE;
    }
  }
  return pos;
}

static FuncDef *getdef(char name[])
{
  return hashtable+getfunctionnr(name);
}

Cell *gettemplate(char name[])
{
  return hashtable[getfunctionnr(name)].template;
}

bool insert(char name[], int argcount, TagType tag, Cell *def, void (*code)(void))
{
  FuncDef *fun = getdef(name);

  if(fun->code) return False;
  if(fun->def == NULL) fun->argcount = argcount;
  else if(argcount != fun->argcount) return False;
  fun->template->tag = fun->tag = tag;
  if(code)
    fun->code = code;
  else if(def)
  {
    Cell *temp  = newcell(LIST);
    temp->left  = def;
    temp->right = template_nil;
    if(fun->def == NULL)
      fun->def = temp;
    else
    {
      Cell *list = fun->def;
      while(list->right->tag == LIST) list = list->right;
      list->right = temp;
    }
  }
  return True;
}

static char parsefunctionname[stringsize] = "";

static Cell *errorcell;

void storefunctionname(char name[])
{
  strcpy(parsefunctionname, name);
  errorcell = newcell(ERROR);
  errorcell->value = getfunctionnr(parsefunctionname);
}

Cell *makeerror(void)
{
  return errorcell;
}

static int comparefuncdef(const void *x, const void *y)
{
  return strcmp(hashtable[*(int *)x].name, hashtable[*(int *)y].name);
}

void printhashtable(void)
{
  int k, index[hashtablesize], indexsize = 0;

  for(k=0; k<hashtablesize; k++)
  {
    FuncDef *fun = &(hashtable[k]);
    if(fun->name != NULL
    && fun->name[0] != anonymousprefix
    && (fun->typeexpr || fun->abstype)) index[indexsize++] = k;
  }

  qsort(index, indexsize, sizeof(int), comparefuncdef);

  Write("\nFUNCTIONS:\n");
  for(k=0; k<indexsize; k++) Write("%s ", hashtable[index[k]].name);
}

void printusertypes(void)
{
  int k, index[hashtablesize], indexsize = 0;

  for(k=0; k<hashtablesize; k++)
    if(hashtable[k].name != NULL) index[indexsize++] = k;

  qsort(index, indexsize, sizeof(int), comparefuncdef);

  Write("\nUSERDEFINED FUNCTIONS:\n");
  for(k=0; k<indexsize; k++)
  {
    FuncDef *fun = &(hashtable[index[k]]);

    if(fun->def && fun->name[0] != anonymousprefix)
    {
      if(fun->abstype)
      {
        Write("   %-15s <= ", fun->name);
        if(fun->typeexpr->tag == TYPESYNONYM)
          WriteType(fun->abstype);
        else
          Write("%s", getfunction(fun->abstype->value)->name);
        Write("\n");
      }
      if(fun->typeexpr)
      {
        Write("   %-15s :: ", fun->name);
        WriteType(fun->typeexpr);
        Write("\n");
      }
    }
  }
}

void forallhashtable(void (*function)(FuncDef *))
{
  int k;

  for(k=0; k<hashtablesize; k++) (*function)(&hashtable[k]);
}

bool constantfunction(int funnr)
{
  FuncDef *fun = &hashtable[funnr];
  return fun->def && fun->argcount == 0 && fun->def->tag < nonevaltag;
}

void restoretemplates(void)
{
  int k;
  for(k=0; k<hashtablesize; k++)
  {
    FuncDef *fun = &(hashtable[k]);
    if(fun->name && strncmp(fun->name, anonymousstring, lengthanonymousstring) == 0)
    {
      free(fun->name);
      fun->tag      = UNDEFINED;
      fun->def      = NULL;
      fun->argcount = 0;
      fun->name     = NULL;
      fun->typeexpr = NULL;
      fun->abstype  = NULL;
      fun->code     = NULL;
      fun->prio     = 1;
      fun->assoc    = Left;
      fun->sysfunc  = False;
      fun->generic  = False;
      fun->template->tag   = UNDEFINED;
      fun->template->value = k;
      fun->template->left  = NULL;
      fun->template->right = NULL;
    }
    if(fun->def && fun->argcount == 0)
    {
      if(fun->def->tag < nonevaltag) 
        *(fun->template) = *(fun->def);
      if(fun->abstype)
      {
        fun->template->tag   = FUNC;
        fun->template->value = k;
        fun->template->left  = NULL;
        fun->template->right = NULL;
      }
    }
  }
  (template_true  = gettemplate("True"))->value  = True;
  (template_false = gettemplate("False"))->value = False;
  template_nil    = gettemplate("Nil");
  template_match  = gettemplate("Match");
  sysvalue_fread  = gettemplate("_fread")->value;
  real(gettemplate("pi")) = 3.14159;
}

void insertsys(char name[])
{
  FuncDef *fun = getdef(name);
  fun->sysfunc = True;
}

void insertoperator(char name[], int prio, Assoc assoc)
{
  FuncDef *fun = getdef(name);
  if(prio < 1) prio = 1;
  if(prio > MAXPRIO) prio = MAXPRIO;
  fun->prio  = prio;
  fun->assoc = assoc;
}

void createhashtable(void)
{
  if(hashtable == NULL) hashtable = malloc(hashtablesize * sizeof(FuncDef));
  if(hashtable == NULL) systemerror(4);
}

void inithashtable(void)
{
  static bool initialised = False;
  int k;

  for(k=0; k<hashtablesize; k++)
  {
    FuncDef *fun = &(hashtable[k]);
    if(initialised && fun->name) free(fun->name);
    fun->name     = NULL;
    fun->argcount = 0;
    fun->tag      = UNDEFINED;
    fun->def      = NULL;
    if(!initialised) fun->template = newcell(UNDEFINED);
    fun->typeexpr = NULL;
    fun->abstype  = NULL;
    fun->code     = NULL;
    fun->prio     = 1;
    fun->assoc    = Left;
    fun->sysfunc  = False;
    fun->generic  = False;
    fun->template->tag   = UNDEFINED;
    fun->template->value = k;
    fun->template->left  = NULL;
    fun->template->right = NULL;
  }
  restoretemplates();
  initialised = True;
}

Cell *anonymousfunction(int argcount, Cell *def)
{
  static int count = 0;
  char funname[stringsize];
  FuncDef *fun;

  do
  {
    sprintf(funname, "%s%s%d", parsefunctionname, anonymousstring, count++);
    fun = getdef(funname);
  }
  while(fun->code || fun->def);
  fun->argcount = argcount;
  fun->def = def;
  fun->template->tag = fun->tag = FUNC;
  return fun->template;
}

bool inserttypeexpr(char name[], Cell *typeexpr)
{
  FuncDef *fun = getdef(name);
  if(fun->typeexpr) return False;
  fun->typeexpr = typeexpr;
  return True;
}

bool inserttypestring(char name[], char typestring[])
{
  parsetypeexpr(typestring);
  return inserttypeexpr(name, pop());
}

bool insertabstype(char name[], Cell *abstype)
{
  FuncDef *fun = getdef(name);
  if(fun->abstype) return False;
  fun->abstype = abstype;
  return True;
}
