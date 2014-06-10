/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amcheck.h

  Description:

  Type check of function definitions and expressions.
***********************************************************************/

#ifndef AMCHECK_H
#define AMCHECK_H

#include "amtypes.h"

/* print indicates whether the type of c should be printed */
void checkexpression(Cell *c, bool print);

void checkdefinitions(void);

#endif
