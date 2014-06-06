/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amprint.h

  Description:

  printing of the contents of a graph for the Amanda interpreter
  the user functions are Write, WriteCell and WriteType
*************************************************************************/

#ifndef AMPRINT_H
#define AMPRINT_H

#include "amtypes.h"

void WriteString(char string[]);

void Write(char *fmt, ...);

void WriteCell(Cell *c);

void WriteType(Cell *c);

#endif
