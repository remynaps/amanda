/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amtable.h

  Description:

  Hash table with function definitions
  functions are accessible by a functionnr which refers to a funcdef
***********************************************************************/

#ifndef AMTABLE_H
#define AMTABLE_H

#include "amtypes.h"

#define anonymousstring         "_nn_"
#define lengthanonymousstring        4

/* do not mess around with these */
extern FuncDef    *hashtable;

/* initialisation must be called ! */
void createhashtable(void);
void inithashtable(void);

/* prints all definitions sorted by name */
void printhashtable(void);

/* prints the types of all user defined functions sorted by name */
void printusertypes(void);

/***********************************************************************
  for all entries in the hash table the function is called
  with arguments: corresponding funcdef
************************************************************************/
void forallhashtable(void (*function)(FuncDef *));

/***********************************************************************
  access functions:
  getfunction  : returns funcdef corresponding to a functionnr
  gettemplate  : returns the template of the function
************************************************************************/
#define getfunction(pos) (hashtable+(pos))

Cell *gettemplate(char name[]);

/************************************************************************
  inserts a function in the hash table
  possibly existing definitions can be extended with a new clause
  input: name     = name of the function
         argcount = number of arguments
         tag      = normally FUNC
         def      = the parsetree of its definition
         code     = the compiled function
*************************************************************************/
bool insert(char name[], int argcount, TagType tag, Cell *def, void (*code)(void));

/**************************************************************************
  access functions used for typing
  input: name = name of the function
***************************************************************************/
bool inserttypeexpr(char name[], Cell *typeexpr);

bool inserttypestring(char name[], char typestring[]);

bool insertabstype(char name[], Cell *abstype);

/************************************************************************
  inserts an anonymous function in the hashtable
  returns its template
*************************************************************************/
Cell *anonymousfunction(int argcount, Cell *def);

/* stores the name of the currently parsed function for use in anonymusfunction */
void storefunctionname(char name[]);

/* returns an error cell for the currently parsed function */
Cell *makeerror(void);

extern Cell *template_true,
            *template_false,
            *template_nil,
            *template_match;
extern int sysvalue_fread;

/* returns true if the entry corresponding to funnr is a constant function */
bool constantfunction(int funnr);

/* restores templates which are evaluated */
void restoretemplates(void);

/************************************************************************
  inserts a sysfunction in the hashtable
  input: name = name of function
*************************************************************************/
void insertsys(char name[]);

/************************************************************************
  inserts an operator in the hashtable
  input: name     = name of the operator
         prio     = parsing priority
         assoc    = the direction of associativity
*************************************************************************/
void insertoperator(char name[], int prio, Assoc assoc);

#endif
