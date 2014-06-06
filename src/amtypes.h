/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amtypes.h

  Description:

  The basic types for the Amanda interpreter
  The graph consists of cells classified by a tag
  The definition of functions is stored in a (hash) table
************************************************************************/

#ifndef AMTYPES_H
#define AMTYPES_H

#include "bool.h"

#define stringsize 256

#define MAXPRIO 7

typedef long int       Integer;
typedef double         Real;
typedef unsigned char  TagType;
typedef unsigned char  MarkType;

enum TAGTYPE
{
  firsttag,
  TYPE, BOOLEAN, INT, REAL, CHAR, NIL, NULLTUPLE,
  nonevaltag,
  VARIABLE, TYPEVAR, MATCHTYPE,
  UNDEFINED, ERROR,
  FUNC,
  copytag,
  ARG,
  compositetag,
  MATCH, MATCHARG, ALIAS,
  matchtag,
  APPLY, _IF, APPLICATION,
  SYSFUNC1, SYSFUNC2,
  STRICTDIRECTOR, LAZYDIRECTOR,
  SET1, SET2,
  CONST, LETREC,
  TYPESYNONYM, TYPEDEF, GENERATOR, LAMBDA, LAMBDAS,
  evaltag,
  LIST, STRUCT, PAIR, RECORD,
  lasttag
};

typedef enum { FUN, EXP, PAT } ExpressionType;

typedef struct CELL
{
  struct CELL *left, *right;
  int value;
  TagType tag;
} Cell;

typedef struct
{
  Cell cell;
  MarkType mark;
} MarkCell;

typedef struct
{
  Integer value;
} IntCell;

typedef struct
{
  Real value;
} RealCell;

#define mark(c)    (((MarkCell *)(c))->mark)
#define integer(c) (((IntCell  *)(c))->value)
#define real(c)    (((RealCell *)(c))->value)

typedef enum { Left, Right } Assoc;

typedef struct
{
  void  (*code)(void);
  Cell   *def;
  char   *name;
  int     argcount;
  bool    sysfunc;
  bool    typechanged;
  bool    generic;
  Cell   *typeexpr;
  Cell   *abstype;
  int     prio;
  Assoc   assoc;
  Cell   *template;
  TagType tag;
} FuncDef;

#endif

/***************************************************************************
  tag classifiers:
    firsttag
    nonevaltag  : these cells need not be evaluated
    copytag     : these cells need not be copied by copystructure
    compositetag: these cells use their left and right pointers
    matchtag    : the matching cells
    evaltag     : already evaluated cells
    lasttag
  NB: the tag classifiers are used by the system

  during garbage collection cells are marked using the mark field

  the value of a cell is used for multiple purposes:
  - as value for bool, char cells
  - as offset in the hashtable for function cells, error cells and match cells
  - as offset in the sysfunctable for sysfunc cells
  - as count of the number of local definitions for letrec cells
  - as argumentnumber for matcharg cells
  - as length of pairs and structs at run time
  - as offset in the hashtable for records at run time

  the function definitions are used by the parser to classify a name and to
  find the priority of an operator
  the parser fills the def with a parse tree

  standard built-in functions are stored in infix form as sysfunc's

  an application is a special representation of a function applied to
  enough arguments
  the arguments are stored in "right-reverse" mode i.e.
  the last argument is stored on the left, the others on the right

  a letrec cell denotes a function body in the context of local definitions
  its value is the number of local definitions
  left is a list of local definitions
  right is the function body

  a strictdirector contains a director bitstring to an evaluated arg cell
  its value is the length of the bitstring
  its left pointer points to an INT cell containing the bitstring
  its right pointer points to the evaluated arg cell

  a lazydirector contains a director bitstring to a possibly unevaluated expression
  its value is the length of the bitstring
  its left pointer points to an INT cell containing the bitstring
  its right pointer points to the expression

  set1 is the stored form of a list comprehension with one generator
  left is a pair (fun, filter, list)
  set2 represents a list comprehension with more generators
  left is a pair (fun, filter, list)
  where fun instantiates a subset
  in both cases right is the suspended part of the list comprehension
  in both cases a fun and filter can be NULL indicating the identity function
  and the empty filter

  in the functable the following holds:
  at run time the template is used to create references to compiled functions
  the def is used for interpretation
  the code points to a compiled function
**************************************************************************/
