/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : ammem.c

  Description:

  memory management functions for the Amanda interpreter
*************************************************************************/

#include <stdlib.h>
#include "amtypes.h"
#include "amerror.h"
#include "ammem.h"
#include "amio.h"
#include "amtable.h"
#include "amprint.h"
#include "ampatter.h"
#include "amstack.h"

#define copyspace       1000    /* space needed during copying */
#define largecopyspace  5000
#define lockmark           2

static MarkCell *memblock;
static long int memcount, freecount, lockcount, reclaimcount, newindex;
static MarkType freemark, currentmark;

void createmem(long size)
{
  if(size < 10000) size = 10000;
  if(size > 10000000) size = 10000000;
  memcount     = size;
  memblock     = calloc(memcount, sizeof(MarkCell));
  lockcount    = 0;
  freecount    = memcount - lockcount;
  freemark     = 0;
  currentmark  = 1 - freemark;
  newindex     = 0;
  reclaimcount = 0;
}

static void markcellrec(Cell *c)
{
  for(;;)
  {
    switch(c->tag)
    {
      case SYSFUNC1:
        if(c->value == sysvalue_fread) MarkIOFile(integer(c->left));
        break;
      case LAZYDIRECTOR:
        if(c->value == 1)
        {
          if(c->left->tag == LAZYDIRECTOR)
          {
            c->value = c->left->value;
            c->left = c->left->left;
            continue;
          }
        }
        else
        {
          if(c->left->tag < nonevaltag || c->left->tag > evaltag)
          {
            c->left = copydirector(c->value, c->left);
            c->value = 1;
            continue;
          }
        }
        break;
    }
    if(c->left && mark(c->left) == freemark)
    {
      mark(c->left) = currentmark;
      freecount--;
      if(c->left->tag > compositetag) markcellrec(c->left);
    }
    c = c->right;
    if(c == NULL || mark(c) != freemark) return;
    mark(c) = currentmark;
    freecount--;
    if(c->tag < compositetag) return;
  }
}

static void markcell(Cell *c)
{
  if(c && mark(c) == freemark)
  {
    mark(c) = currentmark;
    freecount--;
    if(c->tag > compositetag) markcellrec(c);
  }
}

static void markfuncdef(FuncDef *fun)
{
  if(                 mark(fun->template) == freemark) markcell(fun->template);
  if(fun->def      && mark(fun->def)      == freemark) markcell(fun->def);
  if(fun->typeexpr && mark(fun->typeexpr) == freemark) markcell(fun->typeexpr);
  if(fun->abstype  && mark(fun->abstype)  == freemark) markcell(fun->abstype);
}

void reclaim(void)
{
  long int k = newindex;
  for(; k < memcount; k++)
  {
    MarkCell *c = memblock+k;
    if(mark(c) == freemark) mark(c) = currentmark;
  }
  freecount   = memcount - lockcount;
  freemark    = currentmark;
  currentmark = 1 - freemark;
  forallhashtable(markfuncdef);
  forallstack(markcell);
  newindex    = 0;
  reclaimcount++;
  if(freecount < copyspace) systemerror(2);
}

void unlockmem(void)
{
  long int k = 0;
  for(; k < memcount; k++)
  {
    MarkCell *c = memblock+k;
    if(mark(c) == lockmark) mark(c) = currentmark;
  }
  lockcount  = 0;
  reclaim();
}

void lockmem(void)
{
  unlockmem();
  freecount   = memcount;
  freemark    = currentmark;
  currentmark = lockmark;
  forallhashtable(markfuncdef);
  forallstack(markcell);
  currentmark = freemark;
  freemark    = 1 - currentmark;
  lockcount   = memcount - freecount;
}

void checkmem(void)
{
  if(freecount < copyspace)
  {
    reclaim();
    if(freecount < copyspace) systemerror(2);
  }
}

void checkmemlarge(void)
{
  if(freecount < largecopyspace)
  {
    reclaim();
    if(freecount < largecopyspace) systemerror(2);
  }
}

void printmeminfo(void)
{
  Write("MemorySize  : %ld\nFree        : %ld\nReclamations: %ld\n", memcount, memcount-lockcount, reclaimcount);
}

Cell *newcell(TagType tag)
{
  Cell *temp;

  if(freecount == 0) reclaim();
  freecount--;
  do
  {
    temp = (Cell *)(memblock + newindex);
    newindex++;
  }
  while(mark(temp) != freemark);
  mark(temp) = currentmark;
  temp->tag = tag;
  temp->left = temp->right = NULL;
  return temp;
}
