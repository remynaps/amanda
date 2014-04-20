/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ameval.h

  Description:

  The heart of the matter: the graph reducer
  eval evaluates the top element of the stack
  toplevel fully evaluates the top element of the stack and prints it
  the top element is popped
*********************************************************************/

#ifndef AMEVAL_H
#define AMEVAL_H

#include "amtypes.h"
#include "amstack.h"

#define evaluate(c)                               \
{                                                 \
  if((c)->tag > nonevaltag && (c)->tag < evaltag) \
  {                                               \
    push(c);                                      \
    eval();                                       \
    popN(1);                                      \
  }                                               \
}

#define evaluatetop()                                         \
{                                                             \
  if(top()->tag > nonevaltag && top()->tag < evaltag) eval(); \
}

void eval(void);

void toplevel(void);

#endif
