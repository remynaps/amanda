/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amstack.c

  Description:

  evaluation stack for the Amanda interpreter
*******************************************************************/
#include <stdlib.h>
#include "amtypes.h"
#include "amerror.h"
#include "amstack.h"
#include "ammem.h"
#include "amtable.h"

Cell **stack = NULL;
int stackpointer = 0, basepointer;

void createstack(void)
{
  if(stack == NULL) stack = malloc(stacksize * sizeof(Cell *));
  if(stack == NULL) systemerror(4);
  stackpointer = 0;
}

void forallstack(void (*fun)(Cell *))
{
  int sp = stackpointer;
  while(sp-- > 0) (fun)(stack[sp]);
}

void squeeze(int between, int count)
{
  int k = stackpointer - count, l = k - between;
  stackpointer -= between;
  while(l < stackpointer)
    stack[l++] = stack[k++];
}

void slide(int n)
{
  Cell *temp = pop();
  popN(n);
  push(temp);
}

void make(TagType tag)
{
  Cell *temp = newcell(tag);
  temp->left  = pop();
  temp->right = pop();
  push(temp);
}

void makecompound(TagType tag, int n)
{
  push(template_match);
  while(n-- > 0)
  {
    Cell *temp = newcell(tag);
    temp->right = pop();
    temp->left  = pop();
    push(temp);
  }
}

void makeset(TagType tag, int n)
{
  Cell *temp;
  makecompound(PAIR, n+2);
  temp = newcell(tag);
  temp->value = n;
  temp->left = pop();
  temp->right = template_nil;
  push(temp);
}

void makeAPPLICATION(int value, int n)
{
  Cell *temp;
  if(n == 0)
    push(newcell(APPLICATION));
  else if(n == 1)
  {
    temp = newcell(APPLICATION);
    temp->right = pop();
    push(temp);
  }
  else
    while(n-- > 1)
    {
      temp = newcell(APPLICATION);
      temp->right = pop();
      temp->left  = pop();
      push(temp);
    }
  top()->value = value;
}

void make_IF(void)
{
  Cell *temp = newcell(_IF);
  temp->right = newcell(_IF);
  temp->right->right = pop();
  temp->right->left  = pop();
  temp->left = pop();
  push(temp);
}

void makeinverse(TagType tag)
{
  Cell *temp  = newcell(tag);
  temp->right = pop();
  temp->left  = pop();
  push(temp);
}

void rotatestack(void)
{
  Cell *s0 = pop(), *s1 = pop();
  push(s0);
  push(s1);
}

