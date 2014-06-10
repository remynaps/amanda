/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ampatter.h

  Description:

  some functions to handle patterns for the Amanda interpreter
***************************************************************************/

#ifndef AMPATTER_H
#define AMPATTER_H

#include "amtypes.h"


/*******************************************************************
  a pattern may only consist of unused variables, constants, lists,
  types and structs
********************************************************************/
void checkpattern(Cell *c);


/*************************************************************************
  input:  c = valid pattern
  output: c with all occurrences of variables replaced by Match
**************************************************************************/
Cell *matchpattern(Cell *c);


/************************************************************************
  Input : pattern = a nontrivial pattern, c = a possibly unevaluated expression
  Return: whether the expression (after evaluation) matches the pattern
*************************************************************************/
bool match(Cell *pattern, Cell *c);


/*****************************************************************
  fast evaluation of a strictdirector
******************************************************************/
Cell *copydirector(int bitstring, Cell *c);


/*******************************************************************
  return: an empty lazydirector to c (a kind of indirection)
********************************************************************/
Cell *emptydirector(Cell *c);


/**************************************************************************
  input: dlist = list of patternvariables and corresponding directors
         director = cell to which directors must be made
         pattern = current (part of) pattern
         tag = STRICTDIRECTOR | LAZYDIRECTOR
  return: dlist with variables found in pattern appended
  NB: always start with dlist = template_nil
***************************************************************************/
Cell *appenddirectors(Cell *dlist, Cell *director, Cell *pattern, TagType tag);


/********************************************************************
  directorlist contains a list of directors to patternvariables
  return: c with all occurrences of patternvariables replaced
          by a copy of the corresponding director
*********************************************************************/
Cell *replacedirectors(Cell *directorlist, Cell *c);


/********************************************************************
  directorlist contains a list of directors to patternvariables
  c is a variable
  return: the corresponding director or else NULL
*********************************************************************/
Cell *finddirector(Cell *directorlist, Cell *c);


/* initialises renaming */
void initrename(char *funname);


/************************************************************************
  renamelist contains a list of variables and their corresponding renaming
  c is a pattern containing variables
  checkduplicate indicates whether duplicates should be avoided
  return: renamelist with variables in c and their renamings appended
*************************************************************************/
Cell *appendrenamelistrec(Cell *renamelist, bool checkduplicate, Cell *c);


/*************************************************************************
  renamelist contains a list of variables and their corresponding renaming
  c is an expression
  return: c with variables replaced by their renaming
**************************************************************************/
Cell *replacerenamelist(Cell *renamelist, Cell *c);

#endif
