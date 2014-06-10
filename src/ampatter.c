/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ampatter.c

  Description:

  some functions to handle patterns for the Amanda interpreter
***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "amtypes.h"
#include "ampatter.h"
#include "ameval.h"
#include "amstack.h"
#include "amtable.h"
#include "amerror.h"
#include "amprint.h"
#include "ammem.h"

/*******************************************************************
  a pattern may only consist of unused variables, constants, lists,
  types ,structs and pairs
  or an application of the form (identifier = pattern)
********************************************************************/
void checkpattern(Cell *c)
{
  if(c != NULL)
    switch(c->tag)
    {
      case UNDEFINED: case TYPE: case FUNC: case INT: case REAL: case BOOLEAN: case CHAR: case NIL: case NULLTUPLE: case MATCHTYPE:
        break;
      case LIST: case STRUCT: case PAIR:
        checkpattern(c->left);
        checkpattern(c->right);
        break;
      case RECORD:
        checkpattern(c->left->right);
        checkpattern(c->right);
        break;
      case APPLY:
        checkpattern(c->right);
        if(c->left->tag != APPLY
        || (c->right->tag == UNDEFINED || c->right->tag == FUNC)
        || !(c->left->right->tag == UNDEFINED || c->left->right->tag == FUNC)
        || c->left->left != gettemplate("="))
          parseerror(30);
        c->tag = ALIAS;
        c->left = c->left->right;
        break;
      default:
        parseerror(30);
    }
}

static Cell *fastmatchpattern(Cell *c)
{
  if(c != NULL)
    switch(c->tag)
    {
      case VARIABLE:
        return template_match;
      case ALIAS:
        return matchpattern(c->right);
      case LIST:
      {
        Cell *temp  = newcell(c->tag);
        temp->left  = matchpattern(c->left);
        temp->right = matchpattern(c->right);
        return temp;
      }
      case STRUCT: case PAIR: case RECORD:
      {
        Cell *temp  = newcell(c->tag);
        temp->left  = matchpattern(c->left);
        temp->right = fastmatchpattern(c->right);
        if(temp->left == template_match && temp->right == template_match)
          return template_match;
        else
          return temp;
      }
    }
  return c;
}

Cell *matchpattern(Cell *c)
{
  if(c != NULL)
    switch(c->tag)
    {
      case VARIABLE:
        return template_match;
      case ALIAS:
        return matchpattern(c->right);
      case LIST:
      {
        Cell *temp  = newcell(c->tag);
        temp->left  = matchpattern(c->left);
        temp->right = matchpattern(c->right);
        return temp;
      }
      case STRUCT: case PAIR: case RECORD:
      {
        Cell *temp  = newcell(c->tag);
        temp->value = c->value;
        temp->left  = matchpattern(c->left);
        temp->right = fastmatchpattern(c->right);
        return temp;
      }
    }
  return c;
}


/************************************************************************
  Input : pattern = a nontrivial pattern, c = a possibly unevaluated expression
  Return: whether the expression (after evaluation) matches the pattern
*************************************************************************/
bool match(Cell *pattern, Cell *c)
{
  evaluate(c);
  if(pattern->tag != c->tag)
    return pattern->tag == MATCHTYPE
        && (pattern->value == c->tag || (pattern->value == INT && c->tag == REAL));
  switch(pattern->tag)
  {
    case INT:
      return integer(pattern) == integer(c);
    case REAL:
      return real(pattern) == real(c);
    case NIL:
      return True;
    case LIST:
      for(;;)
      {
        if(pattern->left != template_match && !match(pattern->left, c->left)) return False;
        if(pattern->right == template_match) return True;
        pattern = pattern->right;
        c = c->right;
        evaluate(c);
        if(pattern->tag != c->tag) return False;
        if(pattern->tag != LIST) return True;
      }
    case STRUCT: case PAIR: case RECORD:
      if(pattern->value != c->value) return False;
      for(;;)
      {
        if(pattern->left != template_match && !match(pattern->left, c->left)) return False;
        if(pattern->right == template_match) return True;
        pattern = pattern->right;
        c = c->right;
      }
    default:
      return pattern->value == c->value;
  }
}


/************************* directorlist datastructure *********************/

/*****************************************************************
  fast evaluation of a strictdirector
******************************************************************/
Cell *copydirector(int bitstring, Cell *c)
{
  while(bitstring>=16)
  {
    switch(bitstring&15)
    {
      case 0:
        c = c->right->right->right->right;
        break;
      case 1:
        c = c->left->right->right->right;
        break;
      case 2:
        c = c->right->left->right->right;
        break;
      case 3:
        c = c->left->left->right->right;
        break;
      case 4:
        c = c->right->right->left->right;
        break;
      case 5:
        c = c->left->right->left->right;
        break;
      case 6:
        c = c->right->left->left->right;
        break;
      case 7:
        c = c->left->left->left->right;
        break;
      case 8:
        c = c->right->right->right->left;
        break;
      case 9:
        c = c->left->right->right->left;
        break;
      case 10:
        c = c->right->left->right->left;
        break;
      case 11:
        c = c->left->left->right->left;
        break;
      case 12:
        c = c->right->right->left->left;
        break;
      case 13:
        c = c->left->right->left->left;
        break;
      case 14:
        c = c->right->left->left->left;
        break;
      case 15:
        c = c->left->left->left->left;
        break;
    }
    bitstring >>= 4;
  }
  switch(bitstring)
  {
    case 2:
      c = c->right;
      break;
    case 3:
      c = c->left;
      break;
    case 4:
      c = c->right->right;
      break;
    case 5:
      c = c->left->right;
      break;
    case 6:
      c = c->right->left;
      break;
    case 7:
      c = c->left->left;
      break;
    case 8:
      c = c->right->right->right;
      break;
    case 9:
      c = c->left->right->right;
      break;
    case 10:
      c = c->right->left->right;
      break;
    case 11:
      c = c->left->left->right;
      break;
    case 12:
      c = c->right->right->left;
      break;
    case 13:
      c = c->left->right->left;
      break;
    case 14:
      c = c->right->left->left;
      break;
    case 15:
      c = c->left->left->left;
      break;
  }
  return c;
}

/*******************************************************************
  return: an empty lazydirector to c (a kind of indirection)
********************************************************************/
Cell *emptydirector(Cell *c)
{
  Cell *director = newcell(LAZYDIRECTOR);
  director->value = 1;
  director->left = c;
  return director;
}

/********************************************************************
  directorlist contains a list of directors to patternvariables
  c is a variable
  return: the corresponding director or else NULL
*********************************************************************/
Cell *finddirector(Cell *directorlist, Cell *c)
{
  if(c == NULL) return NULL;
  if(c->tag == VARIABLE)
    for(; directorlist->tag == LIST; directorlist = directorlist->right)
      if(directorlist->left->left->value == c->value) return directorlist->left->right;
  return NULL;
}

static Cell *recappend(Cell *dlist,
                       Cell *expression,
                       int bitstring,
                       int power,
                       Cell *pattern,
                       TagType tag)
{
  Cell *temp, *director;

  if(pattern->tag == VARIABLE)
  {
    if(power == 0)
      systemerror(17);
    else if(power == 1)
      director = expression;
    else
    {
      director = newcell(tag);
      director->value = bitstring|power;
      director->left = expression;
    }
    temp = newcell(LIST);
    temp->right = dlist;
    temp->left = newcell(LIST);
    temp->left->left = pattern;
    temp->left->right = director;
    dlist = temp;
  }
  else if(pattern->tag == ALIAS)
  {
    dlist = recappend(dlist, expression, bitstring, power, pattern->left , tag);
    dlist = recappend(dlist, expression, bitstring, power, pattern->right, tag);
  }
  else if(pattern->tag > evaltag)
  {
    dlist = recappend(dlist, expression, bitstring|power, 2*power, pattern->left,  tag);
    dlist = recappend(dlist, expression, bitstring,       2*power, pattern->right, tag);
  }
  return dlist;
}

/**************************************************************************
  input: dlist = list of patternvariables and corresponding directors
         director = cell to which directors must be made
         pattern = current (part of) pattern
         tag = STRICTDIRECTOR | LAZYDIRECTOR
  return: dlist with variables found in pattern appended
***************************************************************************/
Cell *appenddirectors(Cell *dlist, Cell *director, Cell *pattern, TagType tag)
{
  return recappend(dlist, director, 0, 1, pattern, tag);
}

static Cell *copy(Cell *c)
{
  if(c == NULL)
    return NULL;
  else if(c->tag == ARG)
  {
    Cell *temp = newcell(ARG);
    temp->value = c->value;
    return temp;
  }
  else if(c->tag > compositetag)
  {
    Cell *temp = newcell(c->tag);
    temp->value = c->value;
    temp->left  = copy(c->left);
    temp->right = copy(c->right);
    return temp;
  }
  else
    return c;
}

/********************************************************************
  directorlist contains a list of directors to patternvariables
  return: c with all occurrences of patternvariables replaced
          by a copy of the corresponding director
*********************************************************************/
Cell *replacedirectors(Cell *directorlist, Cell *c)
{
  if(c == NULL);
  else if(c->tag == VARIABLE)
  {
    Cell *replacement = finddirector(directorlist, c);
    if(replacement) return copy(replacement);
  }
  else if(c->tag > compositetag)
  {
    c->left  = replacedirectors(directorlist, c->left);
    c->right = replacedirectors(directorlist, c->right);
  }
  return c;
}


/************************* renamelist datastructure *********************/

static int renamecount;
static char *functionname;

/* initialises renaming */
void initrename(char *funname)
{
  renamecount = 0;
  functionname = funname;
}

static Cell *appendrenamelist(Cell *renamelist, bool checkduplicate, Cell *c)
{
  Cell *temp;
  for(temp=renamelist; temp->tag==LIST; temp=temp->right)
    if(temp->left->left->value != c->value);
    else if(checkduplicate)
    {
      Write("\n%s", getfunction(c->value)->name);
      modifyerror(3, functionname);
    }
    else
      return renamelist;
  temp = newcell(LIST);
  temp->left = newcell(LIST);
  temp->left->left = c;
  temp->left->right = newcell(VARIABLE);
  temp->left->right->value = ++renamecount;
  temp->left->right->left  = c;
  temp->right = renamelist;
  return temp;
}

/************************************************************************
  renamelist contains a list of variables and their corresponding renaming
  c is a pattern containing variables
  checkduplicate indicates whether duplicates should be avoided
  return: renamelist with variables in c and their renamings appended
*************************************************************************/
Cell *appendrenamelistrec(Cell *renamelist, bool checkduplicate, Cell *c)
{
  if(c == NULL);
  else if(c->tag == UNDEFINED || c->tag == FUNC)
    renamelist = appendrenamelist(renamelist, checkduplicate, c);
  else if(c->tag == RECORD)
  {
    renamelist = appendrenamelistrec(renamelist, checkduplicate, c->left->right);
    renamelist = appendrenamelistrec(renamelist, checkduplicate, c->right);
  }
  else if(c->tag > compositetag)
  {
    renamelist = appendrenamelistrec(renamelist, checkduplicate, c->left);
    renamelist = appendrenamelistrec(renamelist, checkduplicate, c->right);
  }
  return renamelist;
}

/*************************************************************************
  renamelist contains a list of variables and their corresponding renaming
  c is an expression
  return: c with variables replaced by their renaming
**************************************************************************/
Cell *replacerenamelist(Cell *renamelist, Cell *c)
{
  if(c == NULL);
  else if(c->tag == UNDEFINED || c->tag == FUNC)
  {
    for(; renamelist->tag==LIST; renamelist=renamelist->right)
      if(renamelist->left->left->value == c->value)
      {
        c = newcell(VARIABLE);
        c->value = renamelist->left->right->value;
        c->left  = renamelist->left->right->left;
        return c;
      }
  }
  else if(c->tag == RECORD)
  {
    c->left->right = replacerenamelist(renamelist, c->left->right);
    c->right       = replacerenamelist(renamelist, c->right);
  }
  else if(c->tag > compositetag)
  {
    c->left  = replacerenamelist(renamelist, c->left);
    c->right = replacerenamelist(renamelist, c->right);
  }
  return c;
}

