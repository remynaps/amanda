/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ammem.h

  Description:

  memory management functions for the Amanda interpreter
*************************************************************************/

#ifndef AMMEM_H
#define AMMEM_H

#include "amtypes.h"

Cell *newcell(TagType tag);

void createmem(long size);
/* size = 0 installs the default value */

void checkmem(void);
/* checks available memory and reclaims if the freecount is too small */
void checkmemlarge(void);
/* as checkmem but a larger piece of memory must be available */

void printmeminfo(void);

void reclaim(void);

void unlockmem(void);
void lockmem(void);
/* reclaims memory and (un)locks the memory used by the hashtabledefinitions */

#endif
