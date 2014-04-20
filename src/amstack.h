/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amstack.h

  Description:

  evaluation stack for the Amanda interpreter
  if faststack is not defined all stack manipulations are functions
  else all stack manipulations are inline expanded
*******************************************************************/

#ifndef AMSTACK_H
#define AMSTACK_H

#include "amtypes.h"

#define stacksize  20000
#define stacklimit 19750

extern Cell **stack;
extern int stackpointer, basepointer;


#define top()               stack[stackpointer-1]
#define settop(c)           stack[stackpointer-1] = (c)
#define push(c)             ( stack[stackpointer] = (c), stackpointer++ )
#define pop()               stack[--stackpointer]
#define popN(n)             stackpointer -= (n)
#define getN(n)             stack[stackpointer-(n)]
#define setN(n, c)          stack[stackpointer-(n)] = (c)
#define initstack()         stackpointer = 0
#define stackheight()       stackpointer
#define setstackheight(n)   stackpointer = (n)
#define getbasepointer()    basepointer
#define setbasepointer(n)   basepointer = (n)
#define getBaseN(n)         stack[basepointer-(n)]
#define stackcheck()        if(stackpointer > stacklimit) systemerror(18)

#define makeconstant(t, v)  ( push(newcell(t))   , top()->value = (v) )
#define makeINT(n)          ( push(newcell(INT)) , integer(top()) = (n) )
#define makeREAL(r)         ( push(newcell(REAL)), real(top()) = (r) )

void createstack(void);
void forallstack(void (*fun)(Cell *));
void squeeze(int between, int count);
void slide(int n);
void make(TagType tag);
void makecompound(TagType tag, int n);
void makeset(TagType tag, int n);
void makeAPPLICATION(int value, int n);
void make_IF(void);
void makeinverse(TagType tag);
void rotatestack(void);

#endif
