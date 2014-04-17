/**********************************************************************
  Author : Dick Bruin
  Date   : 25/09/2000
  Version: 2.03
  File   : amlib.c

  Description:

  library with definitions of standard functions
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include "amio.h"
#include "amtypes.h"
#include "amlib.h"
#include "amsyslib.h"
#include "ameval.h"
#include "amerror.h"
#include "ammem.h"
#include "amstack.h"
#include "amtable.h"
#include "amprint.h"

static Cell *template_hd,
            *template_tl,
            *template_min,
            *template_max,
            *template_drop,
            *template_split,
            *template_index,
            *template_itoa,
            *template_random,
            *template_decode,
            *template_log,
            *template_sqrt,
            *template_append,
            *template_concat,
            *template_filter,
            *template_foldl,
            *template_foldl1,
            *template_foldr,
            *template_foldr1,
            *template_iterate,
            *template_map,
            *template_nat,
            *template_nats,
            *template_gennat,
            *template_gennats,
            *template_remove,
            *template_scan,
            *template_take,
            *template_takewhile,
            *template_zip2,
            *template_zip3;

/**********************************************************************
  Evaluation of standard functions
  PRE:  arguments on top of stack accessible by getN(1), getN(2) ...
  POST: function result on top of arguments on stack
***********************************************************************/
static void applyERROR(void)
{
  Cell *temp = getN(1);
  evaluate(temp);
  while(temp->tag == LIST)
  {
    evaluate(temp->left);
    Write("%c", (char)temp->left->value);
    temp = temp->right;
    evaluate(temp);
  }
  runtimeerror(ERROR, -1);
}

static void applyDEBUG(void)
{
  Write("\nDEBUG: ");
  WriteCell(getN(1));
  Write("\n");
  push(getN(2));
  eval();
}

static void applyTIMEDATE(void)
{
  time_t t;
  struct tm *dt;
  Cell *temp;
  time(&t);
  dt = localtime(&t);
  push(temp = newcell(PAIR));
  temp->value = 6;
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_sec;
  temp = temp->right = newcell(PAIR);
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_min;
  temp = temp->right = newcell(PAIR);
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_hour;
  temp = temp->right = newcell(PAIR);
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_mday;
  temp = temp->right = newcell(PAIR);
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_mon;
  temp = temp->right = newcell(PAIR);
  temp->left = newcell(INT);
  integer(temp->left) = dt->tm_year+1900;
  temp->right = template_match;
}

static void applyEMPTY(void)
{
  evaluatetop();
  push(top()->tag == NIL ? template_true:template_false);
}

static void applyCONS(void)
{
  Cell *temp = newcell(LIST);
  temp->left  = getN(1);
  temp->right = getN(2);
  push(temp);
}

static void applyHD(void)
{
  Cell *temp = top();
  evaluatetop();
  if(temp->tag != LIST) runtimeerror(LIST, template_hd->value);
  push(temp->left);
  evaluatetop();
}

static void applyTL(void)
{
  Cell *temp = top();
  evaluatetop();
  if(temp->tag != LIST) runtimeerror(LIST, template_tl->value);
  push(temp->right);
  evaluatetop();
}

static void applyFST(void)
{
  Cell *temp = top();
  evaluatetop();
  push(temp->left);
  evaluatetop();
}

static void applySND(void)
{
  Cell *temp = top();
  evaluatetop();
  push(temp->right->left);
  evaluatetop();
}

static void applyCOMPOSE(void)
{
  Cell *temp;
  push(temp = newcell(APPLY));
  temp->left = getN(2);
  temp = temp->right = newcell(APPLY);
  temp->left = getN(3);
  temp->right = getN(4);
}

static void apply_OR(void)
{
  evaluatetop();
  if(top()->value)
    push(template_true);
  else
  {
    push(getN(2));
    evaluatetop();
  }
}

static void apply_AND(void)
{
  evaluatetop();
  if(top()->value)
  {
    push(getN(2));
    evaluatetop();
  }
  else
    push(template_false);
}

static void apply_NOT(void)
{
  evaluatetop();
  push(top()->value ? template_false : template_true);
}

static void applyLENGTH(void)
{
  Integer count = 0;
  Cell *l = getN(1);

  evaluate(l);
  while(l->tag == LIST)
  {
    count++;
    setN(1, l = l->right);
    evaluate(l);
  }
  makeINT(count);
}

static void applySUM(void)
{
  Integer val_int = 0;
  Real    val_real;
  Cell    *l = getN(1), *temp;

  for(;;)
  {
    evaluate(l);
    if(l->tag != LIST)
    {
      makeINT(val_int);
      return;
    }
    temp = l->left;
    evaluate(temp);
    setN(1, l = l->right);
    if(temp->tag == INT)
      val_int += integer(temp);
    else
    {
      val_real = val_int + real(temp);
      break;
    }
  }
  for(;;)
  {
    evaluate(l);
    if(l->tag != LIST)
    {
      makeREAL(val_real);
      return;
    }
    temp = l->left;
    evaluate(temp);
    setN(1, l = l->right);
    if(temp->tag == INT)
      val_real += integer(temp);
    else
      val_real += real(temp);
  }
}

static void applyPROD(void)
{
  Integer val_int = 1;
  Real    val_real;
  Cell    *l = getN(1), *temp;

  for(;;)
  {
    evaluate(l);
    if(l->tag != LIST)
    {
      makeINT(val_int);
      return;
    }
    temp = l->left;
    evaluate(temp);
    setN(1, l = l->right);
    if(temp->tag == INT)
      val_int *= integer(temp);
    else
    {
      val_real = val_int * real(temp);
      break;
    }
  }
  for(;;)
  {
    evaluate(l);
    if(l->tag != LIST)
    {
      makeREAL(val_real);
      return;
    }
    temp = l->left;
    evaluate(temp);
    setN(1, l = l->right);
    if(temp->tag == INT)
      val_real *= integer(temp);
    else
      val_real *= real(temp);
  }
}

static void applyMIN(void)
{
  Cell *l = getN(1), *res;

  evaluate(l);
  if(l->tag != LIST) runtimeerror(LIST, template_min->value);
  push(res = l->left);
  setN(2, l = l->right);
  evaluate(l);
  while(l->tag == LIST)
  {
    if(comparecell(l->left, res) < 0) settop(res = l->left);
    setN(2, l = l->right);
    evaluate(l);
  }
  eval();
}

static void applyMAX(void)
{
  Cell *l = getN(1), *res;

  evaluate(l);
  if(l->tag != LIST) runtimeerror(LIST, template_max->value);
  push(res = l->left);
  setN(2, l = l->right);
  evaluate(l);
  while(l->tag == LIST)
  {
    if(comparecell(l->left, res) > 0) settop(res = l->left);
    setN(2, l = l->right);
    evaluate(l);
  }
  eval();
}

static void applyAND(void)
{
  Cell *l = getN(1), *temp;

  evaluate(l);
  while(l->tag == LIST)
  {
    temp = l->left;
    evaluate(temp);
    if(!temp->value)
    {
      push(template_false);
      return;
    }
    setN(1, l = l->right);
    evaluate(l);
  }
  push(template_true);
}

static void applyOR(void)
{
  Cell *l = getN(1), *temp;

  evaluate(l);
  while(l->tag == LIST)
  {
    temp = l->left;
    evaluate(temp);
    if(temp->value)
    {
      push(template_true);
      return;
    }
    setN(1, l = l->right);
    evaluate(l);
  }
  push(template_false);
}

static void applyDROP(void)
{
  Integer count = evalint(getN(1), template_drop->value);
  Cell *l = getN(2);

  evaluate(l);
  while(count-- > 0 && l->tag == LIST)
  {
    setN(2, l = l->right);
    evaluate(l);
  }
  push(l);
}

static void applySPLIT(void)
{
  Integer count = evalint(getN(1), template_split->value);
  Cell *l = getN(2), *pair, **copy;

  push(pair = newcell(PAIR));
  pair->value = 2;
  pair->right = newcell(PAIR);
  pair->right->right = template_match;
  copy = &pair->left;
  evaluate(l);
  while(count-- > 0 && l->tag == LIST)
  {
    *copy = newcell(LIST);
    (*copy)->left = l->left;
    copy = &((*copy)->right);
    setN(3, l = l->right);
    evaluate(l);
  }
  *copy = template_nil;
  pair->right->left = l;
}

static void applyINDEX(void)
{
  Cell *l = getN(1);
  Integer count = evalint(getN(2), template_index->value);

  if(count < 0) runtimeerror(ERROR, template_index->value);
  evaluate(l);
  while(count-- > 0 && l->tag == LIST)
  {
    setN(1, l = l->right);
    evaluate(l);
  }
  if(l->tag != LIST) runtimeerror(LIST, template_index->value);
  push(l->left);
  eval();
}

static void applyDROPWHILE(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp;

  evaluate(l);
  while(l->tag == LIST)
  {
    push(temp = newcell(APPLY));
    temp->left  = fun;
    temp->right = l->left;
    eval();
    if(!pop()->value) break;
    setN(2, l = l->right);
    evaluate(l);
  }
  push(l);
}

static void applySPLITWHILE(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp, *pair, **copy;

  push(pair = newcell(PAIR));
  pair->value = 2;
  pair->right = newcell(PAIR);
  pair->right->right = template_match;
  copy = &pair->left;
  evaluate(l);
  while(l->tag == LIST)
  {
    push(temp = newcell(APPLY));
    temp->left  = fun;
    temp->right = l->left;
    eval();
    if(!pop()->value) break;
    *copy = newcell(LIST);
    (*copy)->left = l->left;
    copy = &((*copy)->right);
    setN(3, l = l->right);
    evaluate(l);
  }
  *copy = template_nil;
  pair->right->left = l;
}

static void applyMEMBER(void)
{
  Cell *l = getN(1), *x = getN(2);

  evaluate(l);
  while(l->tag == LIST)
  {
    if(comparecell(l->left, x) == 0)
    {
      push(template_true);
      return;
    }
    setN(1, l = l->right);
    evaluate(l);
  }
  push(template_false);
}

static void applyREVERSE(void)
{
  Cell *l = getN(1), *rev, *temp;

  push(rev = template_nil);
  evaluate(l);
  while(l->tag == LIST)
  {
    temp = newcell(LIST);
    temp->left = l->left;
    temp->right = rev;
    settop(rev = temp);
    setN(2, l = l->right);
    evaluate(l);
  }
}

static void applyREMOVE(void)
{
  Cell *l1 = getN(1), *l2 = getN(2), *temp, *remove, *copy, *lastcopy = NULL;

  for(;;)
  {
    evaluate(l1);
    if(l1->tag != LIST)
    {
      push(l1);
      return;
    }
    evaluate(l2);
    if(l2->tag != LIST)
    {
      push(l1);
      return;
    }
    remove = l2;
    while(remove->tag == LIST && comparecell(remove->left, l1->left) != 0)
    {
      remove = remove->right;
      evaluate(remove);
    }
    if(remove->tag != LIST)
    {
      push(temp = newcell(LIST));
      temp->left = l1->left;
      temp = temp->right = newcell(APPLICATION);
      temp->value = template_remove->value;
      temp->left  = l2;
      temp->right = l1->right;
      return;
    }
    else if(remove == l2)
    {
      setN(1, l1 = l1->right);
      setN(2, l2 = l2->right);
      if(lastcopy == remove) lastcopy = NULL;
    }
    else if(lastcopy)
    {
      setN(1, l1 = l1->right);
      copy = l2;
      while(copy->right != remove && copy != lastcopy) copy = copy->right;
      while(copy->right != remove)
      {
        lastcopy = newcell(LIST);
        *lastcopy = *(copy->right);
        copy = copy->right = lastcopy;
      }
      if(lastcopy == remove) lastcopy = copy;
      copy->right = remove->right;
    }
    else
    {
      lastcopy = newcell(LIST);
      *lastcopy = *l2;
      setN(1, l1 = l1->right);
      setN(2, l2 = lastcopy);
      copy = l2;
      while(copy->right != remove)
      {
        lastcopy = newcell(LIST);
        *lastcopy = *(copy->right);
        copy = copy->right = lastcopy;
      }
      copy->right = remove->right;
    }
  }
}

static void applyUNTIL(void)
{
  Cell *cond = getN(1), *fun = getN(2), *x = getN(3), *temp;

  evaluate(x);
  for(;;)
  {
    push(temp = newcell(APPLY));
    temp->left = cond;
    temp->right = x;
    eval();
    if(pop()->value)
    {
      push(x);
      return;
    }
    temp = newcell(APPLY);
    temp->left = fun;
    temp->right = x;
    setN(3, x = temp);
  }
}

static void applyAPPEND(void)
{
  Cell *l1 = getN(1), *l2 = getN(2), *temp;

  evaluate(l1);
  if(l1->tag != LIST)
  {
    push(l2);
    eval();
  }
  else
  {
    push(temp = newcell(LIST));
    temp->left = l1->left;
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_append->value;
    temp->left  = l2;
    temp->right = l1->right;
  }
}

static void applyCONCAT(void)
{
  Cell *l = getN(1), *temp;

  evaluate(l);
  if(l->tag != LIST)
    push(template_nil);
  else
  {
    push(temp = newcell(APPLICATION));
    temp->value = template_append->value;
    temp->right = l->left;
    temp = temp->left = newcell(APPLICATION);
    temp->value = template_concat->value;
    temp->right = l->right;
  }
}

static void applyFILTER(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp;

  for(;;)
  {
    evaluate(l);
    if(l->tag != LIST)
    {
      push(l);
      return;
    }
    push(temp = newcell(APPLY));
    temp->left  = fun;
    temp->right = l->left;
    eval();
    if(pop()->value) break;
    setN(2, l = l->right);
  }
  push(temp = newcell(LIST));
  temp->left = l->left;
  temp = temp->right = newcell(APPLICATION);
  temp->value = template_filter->value;
  temp->left  = l->right;
  temp->right = fun;
}

static void applyMAP(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp;

  evaluate(l);
  if(l->tag != LIST)
    push(template_nil);
  else
  {
    push(temp = newcell(LIST));
    temp->left = newcell(APPLY);
    temp->left->left = fun;
    temp->left->right = l->left;
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_map->value;
    temp->left  = l->right;
    temp->right = fun;
  }
}

static void applyFOLDL(void)
{
  Cell *fun = getN(1), *res = getN(2), *list = getN(3), *temp;

  evaluate(res);
  evaluate(list);
  while(list->tag == LIST)
  {
    push(temp = newcell(APPLY));
    temp->right = list->left;
    temp = temp->left = newcell(APPLY);
    temp->right = res;
    temp->left = fun;
    eval();
    res = pop();
    setN(2, res);
    setN(3, list = list->right);
    evaluate(list);
  }
  push(res);
}

static void applyFOLDL1(void)
{
  Cell *fun = getN(1), *list = getN(2), *res, *temp;

  evaluate(list);
  if(list->tag != LIST) runtimeerror(LIST, template_foldl1->value);
  push(res = list->left);
  evaluate(res);
  setN(3, list = list->right);
  evaluate(list);
  while(list->tag == LIST)
  {
    push(temp = newcell(APPLY));
    temp->right = list->left;
    temp = temp->left = newcell(APPLY);
    temp->right = res;
    temp->left = fun;
    eval();
    res = pop();
    settop(res);
    setN(3, list = list->right);
    evaluate(list);
  }
}

static void applyFOLDR(void)
{
  Cell *fun = getN(1), *zero = getN(2), *l = getN(3), *temp;

  evaluate(l);
  if(l->tag != LIST)
  {
    push(zero);
    eval();
  }
  else
  {
    push(temp = newcell(APPLY));
    temp->left = newcell(APPLY);
    temp->left->left = fun;
    temp->left->right = l->left;
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_foldr->value;
    temp->left = l->right;
    temp = temp->right = newcell(APPLICATION);
    temp->left  = zero;
    temp->right = fun;
  }
}

static void applyFOLDR1(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp;

  evaluate(l);
  if(l->tag != LIST) runtimeerror(LIST, template_foldr1->value);
  evaluate(l->right);
  if(l->right->tag == NIL)
  {
    push(l->left);
    eval();
  }
  else
  {
    push(temp = newcell(APPLY));
    temp->left = newcell(APPLY);
    temp->left->left = fun;
    temp->left->right = l->left;
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_foldr1->value;
    temp->left  = l->right;
    temp->right = fun;
  }
}

#define NATCOUNT 3

static void applyNAT(void)
{
  Cell *k = getN(1), *l = getN(2), *temp;
  Integer val_k = k->tag == INT ? integer(k) : evalint(k, template_nat->value);
  Integer val_l = l->tag == INT ? integer(l) : evalint(l, template_nat->value);
  int count;
  if(val_k > val_l)
    push(template_nil);
  else
  {
    push(temp = newcell(LIST));
    temp->left = k;
    for(count=0; count<NATCOUNT; count++)
    {
      if(++val_k > val_l)
      {
        temp->right = template_nil;
        return;
      }
      temp = temp->right = newcell(LIST);
      temp->left = newcell(INT);
      integer(temp->left) = val_k;
    }
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_nat->value;
    temp->left  = l;
    temp->right = newcell(INT);
    integer(temp->right) = val_k + 1;
  }
}

static void applyNATS(void)
{
  Cell *k = getN(1), *temp;
  Integer val_k = k->tag == INT ? integer(k) : evalint(k, template_nats->value);
  int count;
  push(temp = newcell(LIST));
  temp->left = k;
  for(count=0; count<NATCOUNT; count++)
  {
    temp = temp->right = newcell(LIST);
    temp->left = newcell(INT);
    integer(temp->left) = ++val_k;
  }
  temp = temp->right = newcell(APPLICATION);
  temp->value = template_nats->value;
  temp->right = newcell(INT);
  integer(temp->right) = val_k + 1;
}

static void applyGENNAT(void)
{
  Cell *k = getN(1), *l = getN(2), *m = getN(3), *temp;
  Integer val_k = k->tag == INT ? integer(k) : evalint(k, template_gennat->value);
  Integer val_l = l->tag == INT ? integer(l) : evalint(l, template_gennat->value);
  Integer val_m = m->tag == INT ? integer(m) : evalint(m, template_gennat->value);
  Integer step = val_l - val_k;
  int count;
  if(step > 0 && val_k > val_m)
    push(template_nil);
  else if(step < 0 && val_k < val_m)
    push(template_nil);
  else
  {
    push(temp = newcell(LIST));
    temp->left = k;
    for(count=0; count<NATCOUNT; count++)
    {
      val_k = val_l;
      val_l += step;
      if(step > 0 && val_k > val_m)
      {
        temp->right = template_nil;
        return;
      }
      else if(step < 0 && val_k < val_m)
      {
        temp->right = template_nil;
        return;
      }
      temp = temp->right = newcell(LIST);
      if(count == 0)
        temp->left = l;
      else
      {
        temp->left = newcell(INT);
        integer(temp->left) = val_k;
      }
    }
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_gennat->value;
    temp->left = m;
    temp = temp->right = newcell(APPLICATION);
    temp->left = newcell(INT);
    integer(temp->left) = val_l + step;
    temp->right = newcell(INT);
    integer(temp->right) = val_l;
  }
}

static void applyGENNATS(void)
{
  Cell *k = getN(1), *l = getN(2), *temp;
  Integer val_k = k->tag == INT ? integer(k) : evalint(k, template_gennats->value);
  Integer val_l = l->tag == INT ? integer(l) : evalint(l, template_gennats->value);
  Integer step = val_l - val_k;
  int count;
  push(temp = newcell(LIST));
  temp->left = k;
  for(count=0; count<NATCOUNT; count++)
  {
    val_k = val_l;
    val_l += step;
    temp = temp->right = newcell(LIST);
    if(count == 0)
      temp->left = l;
    else
    {
      temp->left = newcell(INT);
      integer(temp->left) = val_k;
    }
  }
  temp = temp->right = newcell(APPLICATION);
  temp->value = template_gennats->value;
  temp->left = newcell(INT);
  integer(temp->left) = val_l + step;
  temp->right = newcell(INT);
  integer(temp->right) = val_l;
}

static void applySEQ(void)
{
  eval();
  push(getN(2));
  eval();
}

static void applyTAKE(void)
{
  Cell *l = getN(2), *temp;
  Integer val_n = evalint(getN(1), template_take->value);
  if(val_n <= 0)
    push(template_nil);
  else
  {
    evaluate(l);
    if(l->tag != LIST)
      push(template_nil);
    else
    {
      push(temp = newcell(LIST));
      temp->left = l->left;
      temp = temp->right = newcell(APPLICATION);
      temp->value = template_take->value;
      temp->left = l->right;
      temp->right = newcell(INT);
      integer(temp->right) = val_n - 1;
    }
  }
}

static void applyTAKEWHILE(void)
{
  Cell *fun = getN(1), *l = getN(2), *temp;

  evaluate(l);
  if(l->tag != LIST)
    push(template_nil);
  else
  {
    push(temp = newcell(APPLY));
    temp->left  = fun;
    temp->right = l->left;
    eval();
    if(pop()->value)
    {
      push(temp = newcell(LIST));
      temp->left = l->left;
      temp = temp->right = newcell(APPLICATION);
      temp->value = template_takewhile->value;
      temp->left  = l->right;
      temp->right = fun;
    }
    else
      push(template_nil);
  }
}

static void applyZIP2(void)
{
  Cell *l1 = getN(1), *l2 = getN(2), *temp;

  evaluate(l1);
  evaluate(l2);
  if(l1->tag != LIST || l2->tag != LIST)
    push(template_nil);
  else
  {
    push(temp = newcell(LIST));
    temp->right = newcell(APPLICATION);
    temp->right->value = template_zip2->value;
    temp->right->left  = l2->right;
    temp->right->right = l1->right;
    temp = temp->left = newcell(PAIR);
    temp->value = 2;
    temp->left = l1->left;
    temp = temp->right = newcell(PAIR);
    temp->left = l2->left;
    temp->right = template_match;
  }
}

static void applyZIP3(void)
{
  Cell *l1 = getN(1), *l2 = getN(2), *l3 = getN(3), *temp;

  evaluate(l1);
  evaluate(l2);
  evaluate(l3);
  if(l1->tag != LIST || l2->tag != LIST || l3->tag != LIST)
    push(template_nil);
  else
  {
    push(temp = newcell(LIST));
    temp->right = newcell(APPLICATION);
    temp->right->value = template_zip3->value;
    temp->right->left  = l3->right;
    temp->right->right = newcell(APPLICATION);
    temp->right->right->left  = l2->right;
    temp->right->right->right = l1->right;
    temp = temp->left = newcell(PAIR);
    temp->value = 3;
    temp->left = l1->left;
    temp = temp->right = newcell(PAIR);
    temp->left = l2->left;
    temp = temp->right = newcell(PAIR);
    temp->left = l3->left;
    temp->right = template_match;
  }
}

static void applyZIP(void)
{
  Cell *arg1 = getN(1), *temp;
  evaluate(arg1);
  push(temp = newcell(APPLICATION));
  temp->value = template_zip2->value;
  temp->left  = arg1->right->left;
  temp->right = arg1->left;
}


/************************************************************************
  pushes a list of characters (based on s)
*************************************************************************/
static void buildstring(char *s)
{
  Cell *temp;

  if(*s == '\0')
  {
    push(template_nil);
    return;
  }
  push(temp = newcell(LIST));
  for(;;)
  {
    temp->left = newcell(CHAR);
    temp->left->value = *s++;
    if(*s == '\0')
    {
      temp->right = template_nil;
      return;
    }
    temp = temp->right = newcell(LIST);
  }
}

/************************************************************************
  c must evaluate to a list of characters
  the characters are put in s
*************************************************************************/
static void fillstring(Cell *c, char *s, int size)
{
  char *t = s;

  size--;
  evaluate(c);
  while(c->tag == LIST)
  {
    evaluate(c->left);
    *t++ = c->left->value;
    if(t-s >= size) break;
    c = c->right;
    evaluate(c);
  }
  *t = '\0';
}

static void applyFREAD(void)
{
  Cell *temp;
  char s[stringsize];
  int filenr;

  fillstring(top(), s, stringsize);
  filenr = OpenIOFileRead(s);
  if(filenr < 0)
  {
    push(template_nil);
    return;
  }
  push(temp = newcell(SYSFUNC1));
  temp->value = sysvalue_fread;
  temp->left = newcell(INT);
  integer(temp->left) = filenr;
}

static void apply_FREAD(void)
{
  Cell *nr = getN(1), *temp;
  int ch = ReadIOFile(integer(nr));
  if(ch == EOF)
  {
    CloseIOFile(integer(nr));
    push(template_nil);
    return;
  }
  push(temp = newcell(LIST));
  temp->left = newcell(CHAR);
  temp->left->value = ch;
  temp = temp->right = newcell(SYSFUNC1);
  temp->value = sysvalue_fread;
  temp->left = nr;
}

static void applyFWRITE(void)
{
  Cell *fname = getN(1), *l = getN(2);
  char s[stringsize];
  int filenr;

  fillstring(fname, s, stringsize);
  filenr = OpenIOFileWrite(s);
  if(filenr < 0)
  {
    push(template_false);
    return;
  }
  evaluate(l);
  while(l->tag == LIST)
  {
    evaluate(l->left);
    WriteIOFile(filenr, l->left->value);
    setN(2, l = l->right);
    evaluate(l);
  }
  CloseIOFile(filenr);
  push(template_true);
}

static void applyFAPPEND(void)
{
  Cell *fname = getN(1), *l = getN(2);
  char s[stringsize];
  int filenr;

  fillstring(fname, s, stringsize);
  filenr = OpenIOFileAppend(s);
  if(filenr < 0)
  {
    push(template_false);
    return;
  }
  evaluate(l);
  while(l->tag == LIST)
  {
    evaluate(l->left);
    WriteIOFile(filenr, l->left->value);
    setN(2, l = l->right);
    evaluate(l);
  }
  CloseIOFile(filenr);
  push(template_true);
}

void GraphDisplay(char string[]);

static void applyGRAPHDISPLAY(void)
{
  Cell *l = getN(1);
  char s[stringsize];

  evaluate(l);
  while(l->tag == LIST)
  {
    fillstring(l->left, s, stringsize);
    GraphDisplay(s);
    setN(1, l = l->right);
    evaluate(l);
  }
  push(template_true);
}

static void applyMIN2(void)
{
  push(comparecell(getN(1), getN(2))<0 ? getN(1) : getN(2));
}

static void applyMAX2(void)
{
  push(comparecell(getN(1), getN(2))>0 ? getN(1) : getN(2));
}

static void applySCAN(void)
{
  Cell *arg1 = getN(1), *arg2 = getN(2), *arg3 = getN(3), *temp;

  evaluate(arg3);
  if(arg3->tag != LIST)
  {
    push(temp = newcell(LIST));
    temp->left = arg2;
    temp->right = template_nil;
  }
  else
  {
    push(temp = newcell(LIST));
    temp->left = arg2;
    temp = temp->right = newcell(APPLICATION);
    temp->value = template_scan->value;
    temp->left = arg3->right;
    temp = temp->right = newcell(APPLICATION);
    temp->right = arg1;
    temp = temp->left = newcell(APPLY);
    temp->right = arg3->left;
    temp = temp->left = newcell(APPLY);
    temp->right = arg2;
    temp->left = arg1;
  }
}

static void applyITERATE(void)
{
  Cell *arg1 = getN(1), *arg2 = getN(2), *temp;

  push(temp = newcell(LIST));
  temp->left = arg2;
  temp = temp->right = newcell(APPLICATION);
  temp->value = template_iterate->value;
  temp->right = arg1;
  temp = temp->left = newcell(APPLY);
  temp->right = arg2;
  temp->left = arg1;
}

static void applyATOI(void)
{
  char s[stringsize];
  fillstring(top(), s, stringsize);
  makeINT(atol(s));
}

static void applyITOA(void)
{
  char s[stringsize];
  sprintf(s, "%ld", evalint(top(), template_itoa->value));
  buildstring(s);
}

static void applyATOF(void)
{
  Integer i;
  Real r;
  char s[stringsize];
  fillstring(top(), s, stringsize);
  r = atof(s);
  if(r >= LONG_MIN && r <= LONG_MAX && r == (i = r))
    makeINT(i);
  else
    makeREAL(r);
}

static void applyFTOA(void)
{
  char s[stringsize];
  sprintf(s, "%lg", evalreal(top()));
  buildstring(s);
}

static void applyRANDOM(void)
{
  Integer r = evalint(getN(1), template_random->value);
  if(r<=0) runtimeerror(ERROR, template_random->value);
  makeINT(rand()%r);
}

static void applyCODE(void)
{
  Cell *temp = top();
  evaluate(temp);
  makeINT(temp->value);
}

static void applyDECODE(void)
{
  Integer i = evalint(getN(1), template_decode->value);
  makeconstant(CHAR, i);
}

static void applyABS(void)
{
  Cell *temp = getN(1);
  evaluate(temp);
  if(temp->tag == INT)
  {
    if(integer(temp) < 0)
      makeINT(-integer(temp));
    else
      push(temp);
  }
  else
  {
    if(real(temp) < 0)
      makeREAL(-real(temp));
    else
      push(temp);
  }
}

static void applyROUND(void)
{
  Real r = evalreal(getN(1));
  makeINT(r >= 0 ? r+0.5 : r-0.5);
}

static void applyTRUNC(void)
{
  Real r = evalreal(getN(1));
  makeINT(r);
}

static void applyCOS(void)
{
  Real r = evalreal(getN(1));
  makeREAL(cos(r));
}

static void applySIN(void)
{
  Real r = evalreal(getN(1));
  makeREAL(sin(r));
}

static void applyEXP(void)
{
  Real r = evalreal(getN(1));
  makeREAL(exp(r));
}

static void applyLOG(void)
{
  Real r = evalreal(getN(1));
  if(r<=0) runtimeerror(ERROR, template_log->value);
  makeREAL(log(r));
}

static void applySQRT(void)
{
  Real r = evalreal(getN(1));
  if(r<0) runtimeerror(ERROR, template_sqrt->value);
  makeREAL(sqrt(r));
}

static void applyATAN(void)
{
  Real r = evalreal(getN(1));
  makeREAL(atan(r));
}


/********************************************************************
  initialisation of hashtable with standard functions
*********************************************************************/
void initlib(void)
{
  srand((unsigned)time(NULL));

  insert("error"           , 1, FUNC, NULL, applyERROR);
  insert("debug"           , 2, FUNC, NULL, applyDEBUG);
  insert("timedate"        , 0, FUNC, NULL, applyTIMEDATE);
  insert("empty"           , 1, FUNC, NULL, applyEMPTY);
  insert("hd"              , 1, FUNC, NULL, applyHD);
  insert("tl"              , 1, FUNC, NULL, applyTL);
  insert("fst"             , 1, FUNC, NULL, applyFST);
  insert("snd"             , 1, FUNC, NULL, applySND);
  insert("and"             , 1, FUNC, NULL, applyAND);
  insert("drop"            , 2, FUNC, NULL, applyDROP);
  insert("dropwhile"       , 2, FUNC, NULL, applyDROPWHILE);
  insert("member"          , 2, FUNC, NULL, applyMEMBER);
  insert("min"             , 1, FUNC, NULL, applyMIN);
  insert("max"             , 1, FUNC, NULL, applyMAX);
  insert("or"              , 1, FUNC, NULL, applyOR);
  insert("prod"            , 1, FUNC, NULL, applyPROD);
  insert("reverse"         , 1, FUNC, NULL, applyREVERSE);
  insert("seq"             , 2, FUNC, NULL, applySEQ);
  insert("sum"             , 1, FUNC, NULL, applySUM);
  insert("until"           , 3, FUNC, NULL, applyUNTIL);
  insert("foldl"           , 3, FUNC, NULL, applyFOLDL);
  insert("foldl1"          , 2, FUNC, NULL, applyFOLDL1);
  insert("foldr"           , 3, FUNC, NULL, applyFOLDR);
  insert("foldr1"          , 2, FUNC, NULL, applyFOLDR1);
  insert("concat"          , 1, FUNC, NULL, applyCONCAT);
  insert("filter"          , 2, FUNC, NULL, applyFILTER);
  insert("map"             , 2, FUNC, NULL, applyMAP);
  insert("nat"             , 2, FUNC, NULL, applyNAT);
  insert("nats"            , 1, FUNC, NULL, applyNATS);
  insert("gennat"          , 3, FUNC, NULL, applyGENNAT);
  insert("gennats"         , 2, FUNC, NULL, applyGENNATS);
  insert("split"           , 2, FUNC, NULL, applySPLIT);
  insert("splitwhile"      , 2, FUNC, NULL, applySPLITWHILE);
  insert("take"            , 2, FUNC, NULL, applyTAKE);
  insert("takewhile"       , 2, FUNC, NULL, applyTAKEWHILE);
  insert("zip2"            , 2, FUNC, NULL, applyZIP2);
  insert("zip3"            , 3, FUNC, NULL, applyZIP3);
  insert("zip"             , 1, FUNC, NULL, applyZIP);
  insert("."               , 3, FUNC, NULL, applyCOMPOSE);
  insert(":"               , 2, FUNC, NULL, applyCONS);
  insert("!"               , 2, FUNC, NULL, applyINDEX);
  insert("++"              , 2, FUNC, NULL, applyAPPEND);
  insert("--"              , 2, FUNC, NULL, applyREMOVE);
  insert("/\\"             , 2, FUNC, NULL, apply_AND);
  insert("\\/"             , 2, FUNC, NULL, apply_OR);
  insert("#"               , 1, FUNC, NULL, applyLENGTH);
  insert("~"               , 1, FUNC, NULL, apply_NOT);
  insert("fread"           , 1, FUNC, NULL, applyFREAD);
  insert("fwrite"          , 2, FUNC, NULL, applyFWRITE);
  insert("fappend"         , 2, FUNC, NULL, applyFAPPEND);
  insert("_fread"          , 1, FUNC, NULL, apply_FREAD);
  insert("graphdisplay"    , 1, FUNC, NULL, applyGRAPHDISPLAY);
  insert("min2"            , 2, FUNC, NULL, applyMIN2);
  insert("max2"            , 2, FUNC, NULL, applyMAX2);
  insert("iterate"         , 2, FUNC, NULL, applyITERATE);
  insert("scan"            , 3, FUNC, NULL, applySCAN);
  insert("itoa"            , 1, FUNC, NULL, applyITOA);
  insert("atoi"            , 1, FUNC, NULL, applyATOI);
  insert("ftoa"            , 1, FUNC, NULL, applyFTOA);
  insert("atof"            , 1, FUNC, NULL, applyATOF);
  insert("abs"             , 1, FUNC, NULL, applyABS);
  insert("round"           , 1, FUNC, NULL, applyROUND);
  insert("trunc"           , 1, FUNC, NULL, applyTRUNC);
  insert("decode"          , 1, FUNC, NULL, applyDECODE);
  insert("code"            , 1, FUNC, NULL, applyCODE);
  insert("random"          , 1, FUNC, NULL, applyRANDOM);
  insert("cos"             , 1, FUNC, NULL, applyCOS);
  insert("sin"             , 1, FUNC, NULL, applySIN);
  insert("exp"             , 1, FUNC, NULL, applyEXP);
  insert("log"             , 1, FUNC, NULL, applyLOG);
  insert("sqrt"            , 1, FUNC, NULL, applySQRT);
  insert("atan"            , 1, FUNC, NULL, applyATAN);

  inserttypestring("error"            , "[char] -> *");
  inserttypestring("debug"            , "** -> * -> *");
  inserttypestring("timedate"         , "(num, num, num, num, num, num)");
  inserttypestring("empty"            , "[*] -> bool");
  inserttypestring("hd"               , "[*] -> *");
  inserttypestring("tl"               , "[*] -> [*]");
  inserttypestring("fst"              , "(*, **) -> *");
  inserttypestring("snd"              , "(*, **) -> **");
  inserttypestring("and"              , "[bool] -> bool");
  inserttypestring("drop"             , "num -> [*] -> [*]");
  inserttypestring("dropwhile"        , "(* -> bool) -> [*] -> [*]");
  inserttypestring("member"           , "[*] -> * -> bool");
  inserttypestring("min"              , "[*] -> *");
  inserttypestring("max"              , "[*] -> *");
  inserttypestring("or"               , "[bool] -> bool");
  inserttypestring("prod"             , "[num] -> num");
  inserttypestring("reverse"          , "[*] -> [*]");
  inserttypestring("seq"              , "* -> ** -> **");
  inserttypestring("sum"              , "[num] -> num");
  inserttypestring("until"            , "(* -> bool) -> (* -> *) -> * -> *");
  inserttypestring("foldl"            , "(* -> ** -> *) -> * -> [**] -> *");
  inserttypestring("foldl1"           , "(* -> * -> *) -> [*] -> *");
  inserttypestring("foldr"            , "(** -> * -> *) -> * -> [**] -> *");
  inserttypestring("foldr1"           , "(* -> * -> *) -> [*] -> *");
  inserttypestring("concat"           , "[[*]] -> [*]");
  inserttypestring("filter"           , "(* -> bool) -> [*] -> [*]");
  inserttypestring("map"              , "(* -> **) -> [*] -> [**]");
  inserttypestring("nat"              , "num -> num -> [num]");
  inserttypestring("nats"             , "num -> [num]");
  inserttypestring("gennat"           , "num -> num -> num -> [num]");
  inserttypestring("gennats"          , "num -> num -> [num]");
  inserttypestring("split"            , "num -> [*] -> ([*], [*])");
  inserttypestring("splitwhile"       , "(* -> bool) -> [*] -> ([*], [*])");
  inserttypestring("take"             , "num -> [*] -> [*]");
  inserttypestring("takewhile"        , "(* -> bool) -> [*] -> [*]");
  inserttypestring("zip2"             , "[*] -> [**] -> [(*, **)]");
  inserttypestring("zip3"             , "[*] -> [**] -> [***] -> [(*, **, ***)]");
  inserttypestring("zip"              , "([*], [**]) -> [(*, **)]");
  inserttypestring("."                , "(** -> ***) -> (* -> **) -> * -> ***");
  inserttypestring(":"                , "* -> [*] -> [*]");
  inserttypestring("!"                , "[*] -> num -> *");
  inserttypestring("++"               , "[*] -> [*] -> [*]");
  inserttypestring("--"               , "[*] -> [*] -> [*]");
  inserttypestring("/\\"              , "bool -> bool -> bool");
  inserttypestring("\\/"              , "bool -> bool -> bool");
  inserttypestring("#"                , "[*] -> num");
  inserttypestring("~"                , "bool -> bool");
  inserttypestring("fread"            , "[char] -> [char]");
  inserttypestring("fwrite"           , "[char] -> [char] -> bool");
  inserttypestring("fappend"          , "[char] -> [char] -> bool");
  inserttypestring("graphdisplay"     , "[[char]] -> bool");
  inserttypestring("min2"             , "* -> * -> *");
  inserttypestring("max2"             , "* -> * -> *");
  inserttypestring("iterate"          , "(* -> *) -> * -> [*]");
  inserttypestring("scan"             , "(* -> ** -> *) -> * -> [**] -> [*]");
  inserttypestring("itoa"             , "num -> [char]");
  inserttypestring("atoi"             , "[char] -> num");
  inserttypestring("ftoa"             , "num -> [char]");
  inserttypestring("atof"             , "[char] -> num");
  inserttypestring("abs"              , "num -> num");
  inserttypestring("round"            , "num -> num");
  inserttypestring("trunc"            , "num -> num");
  inserttypestring("decode"           , "num -> char");
  inserttypestring("code"             , "char -> num");
  inserttypestring("random"           , "num -> num");
  inserttypestring("cos"              , "num -> num");
  inserttypestring("sin"              , "num -> num");
  inserttypestring("exp"              , "num -> num");
  inserttypestring("log"              , "num -> num");
  inserttypestring("sqrt"             , "num -> num");
  inserttypestring("atan"             , "num -> num");

  template_hd               = gettemplate("hd");
  template_tl               = gettemplate("tl");
  template_min              = gettemplate("min");
  template_max              = gettemplate("max");
  template_drop             = gettemplate("drop");
  template_split            = gettemplate("split");
  template_index            = gettemplate("!");
  template_itoa             = gettemplate("itoa");
  template_random           = gettemplate("random");
  template_decode           = gettemplate("decode");
  template_log              = gettemplate("log");
  template_sqrt             = gettemplate("sqrt");
  template_append           = gettemplate("++");
  template_concat           = gettemplate("concat");
  template_filter           = gettemplate("filter");
  template_foldl            = gettemplate("foldl");
  template_foldl1           = gettemplate("foldl1");
  template_foldr            = gettemplate("foldr");
  template_foldr1           = gettemplate("foldr1");
  template_iterate          = gettemplate("iterate");
  template_map              = gettemplate("map");
  template_nat              = gettemplate("nat");
  template_nats             = gettemplate("nats");
  template_gennat           = gettemplate("gennat");
  template_gennats          = gettemplate("gennats");
  template_remove           = gettemplate("--");
  template_scan             = gettemplate("scan");
  template_take             = gettemplate("take");
  template_takewhile        = gettemplate("takewhile");
  template_zip2             = gettemplate("zip2");
  template_zip3             = gettemplate("zip3");

  insertsys("_fread");
}
