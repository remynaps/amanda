/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ammodify.h

  Description:

  modification of the parsetree for the Amanda interpreter
***************************************************************************/

#ifndef AMMODIFY_H
#define AMMODIFY_H

#include "amtypes.h"


void initmodify(void);

/*************************************************************************
  input:  c = parsetree of an expression
  output: c with list comprehensions changed to an appropriate form
          _ifs, shortfuncs, sysfuncs introduced
**************************************************************************/
Cell *modify_expression(Cell *c);


/**************************************************************************
  modify all function definitions
  list comprehensions and local definitions are changed to an appropriate form
  consts, _ifs, shortfuncs, sysfuncs are introduced
***************************************************************************/
void modify_definitions(void);

#endif
