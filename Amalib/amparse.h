/**********************************************************************
  Author : Dick Bruin
  Date   : 26/10/99
  Version: 2.02
  File   : amparse.h

  Description:

  the parser for the Amanda interpreter

  grammar:

  file           = { definition | [ generic ] typedefinition | structdef | typesynonym | abstype }

  definition     = function expr_cl [where_cl]

  expr_cl        = { is expression [ comma ["if"] expression | otherwise ] {sep} }

  where_cl       = where { definition }

  function       = ( identifier | operator ) { pattern }
                 | pattern
                 | lpar pattern operator { pattern } rpar

  pattern        = number | character | string
                 | identifier [ is pattern]
                 | lbrack [ pattern { comma pattern } ] rbrack
                 | lpar pattern { comma pattern } rpar
                 | lacc identifier is pattern { comma identifier is pattern } racc
                 | type { pattern }
                 | pattern : pattern

  expression     = { pattern arrow } expr(maxprio) { bar { pattern arrow } expr(maxprio) }

  expr(prio)     = application { op(opprio, assoc) expr(assoc==Left ? opprio-1:opprio) }
                 with opprio <= prio

  application    = ( operator | type | term ) { term } [ ":" application ]

  term           = number | identifier | type | character | string
                 | lpar ( operator [ expression ] | ( expression ( operator | { comma expression } ) ) ) rpar
                 | lbrack list rbrack
                 | lacc identifier is expression { comma identifier is expression } racc

  generator      = pattern { comma pattern } ( gener | assignment ) expression { comma expression }
                 | expression

  list           = [ [ expression { comma expression } ] ]
                 | [ expression { comma expression } bar [ generator { { sep } generator } ] ]
                 | [ expression points [ expression ] ]
                 | [ expression comma expression points [ expression ] ]

  typedefinition = identifier colons typeexpr { sep }

  typeexpr       = ( identifier { typeterm }
                   | typeterm
                   )
                   [ arrow typeexpr ]

  typehead       = identifier { operator }

  typeterm       = identifier
                 | operator
                 | lpar typeexpr { comma typexpr } rpar
                 | lbrack typeexpr rbrack

  structdef      = typehead def ( type { typeterm } { bar type { typeterm } } { sep }
                                | lacc typedefinition { comma typedefinition } racc )

  typesynonym    = typehead syn typeexpr { sep }

  abstype        = abstype typehead with { typedefinition } { sep }

  (* merge of function and typehead *)
  lefthandside   = application { operator | lpar expression rpar }

  prio 1     = : | . | -> | &
  prio 2     = ^ | !
  prio 3     = * | / | %
  prio 4     = + | - | ++ | --
  prio 5     = = | ~= | > | >= | < | <=
  prio 6     =  /\
  prio 7     =  \/

  NB: (- term) denotes unary minus (more a function than an operator)
      the maximum priority is 7
***************************************************************************/

#ifndef AMPARSE_H
#define AMPARSE_H

#include "amtypes.h"

/********************************************************************
  reads definitions from the inputfile and stores them in the hashtable
*********************************************************************/
void parsefile(char filename[]);


/********************************************************************
  parses the input string s
  the parsetree is pushed on the stack
  return: true if the type of the expression is requested
*********************************************************************/
bool parseinput(char s[]);


/*********************************************************************
  s contains a type expression
  the parsetree of s is pushed on the stack
**********************************************************************/
void parsetypeexpr(char s[]);

#endif
