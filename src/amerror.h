/**********************************************************************
  Author : Dick Bruin
  Date   : 02/10/98
  Version: 2.01
  File   : amerror.h

  Description:

  Error handler for the Amanda interpreter
  Only use seterror() to perform a long jump back
  Timing functions
*******************************************************************/

#ifndef AMERROR_H
#define AMERROR_H

#include <setjmp.h>
#include "amtypes.h"


extern jmp_buf mainenv;

#define seterror() setjmp(mainenv)

extern bool interrupted;

void CheckIO(void);

void parseerror(int messagenr);

void runtimeerror(TagType tag, int hashtablenr);

void modifyerror(int messagenr, char functionname[]);

void systemerror(int messagenr);

extern int interruptcount;

void checkinterrupt(void);

extern bool timing;

void starttiming(void);

void stoptiming(void);

#endif
