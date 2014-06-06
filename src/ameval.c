/***************************************************************************
  Author : Dick Bruin
  Date   : 29/09/98
  Version: 2.00
  File   : ameval.c

  Description:

  toplevel of the interpreter
  eval function
****************************************************************************/

#include <stdlib.h>
#include "amtypes.h"
#include "ameval.h"
#include "amerror.h"
#include "amsyslib.h"
#include "ammem.h"
#include "ampatter.h"
#include "amstack.h"
#include "amtable.h"
#include "amprint.h"


/**********************************************************************
  Evaluation of user defined functions
  PRE: arguments on stack with names: getBaseN(1), getBaseN(2) ...
  this recursive copying can loose cells when going out of memory
***********************************************************************/
static Cell *copystructure(Cell *c)
{
  Cell *temp, *left, *right;
  if(c == NULL)
    return NULL;
  if(c->tag < copytag)
    return c;
  switch(c->tag)
  {
    case ARG:
      return getBaseN(c->value);
    case CONST:
      return c->left;
    case STRICTDIRECTOR:
      return copydirector(c->value, getBaseN(c->left->value));
    case MATCH:
      temp        = newcell(MATCH);
      temp->value = c->value;
      temp->left  = c->left;
      temp->right = copystructure(c->right);
      return temp;
    case SYSFUNC1:
      left = copystructure(c->left);
      if(left->tag < nonevaltag)
      {
        push(left);
        (*(getfunction(c->value)->code))();
        temp = pop();
        popN(1);
        return temp;
      }
      temp        = newcell(SYSFUNC1);
      temp->value = c->value;
      temp->left  = left;
      return temp;
    case SYSFUNC2:
      left  = copystructure(c->left);
      right = copystructure(c->right);
      if((left->tag < nonevaltag)
      && (right->tag < nonevaltag))
      {
        push(right);
        push(left);
        (*(getfunction(c->value)->code))();
        temp = pop();
        popN(2);
        return temp;
      }
      temp        = newcell(SYSFUNC2);
      temp->value = c->value;
      temp->left  = left;
      temp->right = right;
      return temp;
  }
  if(c->tag > compositetag)
  {
    temp        = newcell(c->tag);
    temp->value = c->value;
    temp->left  = copystructure(c->left);
    temp->right = copystructure(c->right);
    return temp;
  }
  return c;
}

/**********************************************************************
  Evaluation of user defined functions
  PRE: arguments on stack with names: getBaseN(1), getBaseN(2) ...
       argcount denotes the number of popped off stackentries
       bp denotes the initial basepointer
***********************************************************************/
static Cell *copyreduce(Cell *c, int argcount, int bp)
{
  Cell *temp;
  FuncDef *fun;
  int k;

  for(;;)
  {
    if(interruptcount-- == 0 || interrupted) checkinterrupt();
    setbasepointer(bp);
    switch(c->tag)
    {
      case _IF:
        temp = copyreduce(c->left, 0, bp);
        evaluate(temp);
        if(temp->value)
          c = c->right->left;
        else
          c = c->right->right;
        break;
      case MATCH:
        push(temp = copyreduce(c->right, argcount, bp));
        if(!match(c->left, temp))
        {
          temp = newcell(ERROR);
          temp->value = c->value;
        }
        popN(1);
        return temp;
      case MATCHARG:
        do
        {
          if(!match(c->left, getBaseN(c->value)))
          {
            popN(argcount);
            return template_false;
          }
          c = c->right;
          setbasepointer(bp);
        }
        while(c);
        popN(argcount);
        return template_true;
      case SYSFUNC1:
        push(copyreduce(c->left, 0, bp));
        (*(getfunction(c->value)->code))();
        temp = pop();
        popN(1+argcount);
        return temp;
      case SYSFUNC2:
        push(copyreduce(c->right, 0, bp));
        push(copyreduce(c->left , 0, bp));
        (*(getfunction(c->value)->code))();
        temp = pop();
        popN(2+argcount);
        return temp;
      case ARG:
        temp = getBaseN(c->value);
        evaluate(temp);
        popN(argcount);
        return temp;
      case STRICTDIRECTOR:
        temp = copydirector(c->value, getBaseN(c->left->value));
        evaluate(temp);
        popN(argcount);
        return temp;
      case LAZYDIRECTOR:
        temp = copyreduce(c->left, argcount, bp);
        evaluate(temp);
        temp = copydirector(c->value, temp);
        evaluate(temp);
        return temp;
      case LETREC:
        for(k=0; k<c->value; k++) push(newcell(UNDEFINED));
        argcount += c->value;
        temp = c->left;
        for(k=0; k<c->value; k++)
        {
          checkmem();
          *(getBaseN(-k)) = *(copystructure(temp->left));
          temp = temp->right;
        }
        c = c->right;
        break;
      case CONST:
        popN(argcount);
        return c->left;
      case APPLICATION:
        stackcheck();
        fun = getfunction(c->value);
        if(fun->argcount == 0)
          ;
        else if(fun->argcount == 1)
        {
          checkmem();
          push(copystructure(c->right));
        }
        else
        {
          for(k=fun->argcount; k>1; k--)
          {
            checkmem();
            push(copystructure(c->left));
            c = c->right;
          }
          checkmem();
          push(copystructure(c));
        }
        if(argcount > 0) squeeze(argcount, fun->argcount);
        argcount = fun->argcount;
        bp = stackheight();
        if(fun->code)
        {
          (*(fun->code))();
          temp = pop();
          popN(argcount);
          return temp;
        }
        c = fun->def;
        break;
      default:
        if(c->tag > compositetag)
        {
          checkmem();
          c = copystructure(c);
        }
        popN(argcount);
        return c;
    }
  }
}

static void evalset(Cell *set)
{
  for(;;)
  {
    Cell *fun = set->left->left,
         *cond = set->left->right->left,
         *generators = set->left->right->right,
         *l, *temp;
    FuncDef *function;
    int k, argcount, generatorcount = set->value;

    if(cond)
    {
      for(k=1; k<=generatorcount; k++) push(NULL);
      argcount = 0;
      temp = cond;
      while(temp->tag == APPLY)
      {
        push(temp->right);
        argcount++;
        temp = temp->left;
      }
      function = getfunction(temp->value);

      for(;;)
      {
        l = generators;
        for(k=1; k<=generatorcount; k++)
        {
          evaluate(l->left);
          if(l->left->tag != LIST)
          {
            popN(argcount+generatorcount);
            goto nextset;
          }
          setN(argcount+k, l->left->left);
          l = l->right;
        }

        if(function->code)
        {
          (*(function->code))();
          temp = pop();
        }
        else
          temp = copyreduce(function->def, 0, stackheight());

        evaluate(temp);
        if(temp->value)
        {
          popN(argcount+generatorcount);
          goto generatevalue;
        }
        l = generators;
        for(k=1; k<=generatorcount; k++)
        {
          l->left = l->left->right;
          l = l->right;
        }
      }
    }
    else
    {
      l = generators;
      for(k=1; k<=generatorcount; k++)
      {
        evaluate(l->left);
        if(l->left->tag != LIST) goto nextset;
        l = l->right;
      }
      goto generatevalue;
    }

  nextset:
    *set = *(set->right);
    if(set->tag == NIL) return;
    continue;

  generatevalue:
    if(set->tag == SET1)
    {
      temp = newcell(SET1);
      *temp = *set;
      set->right = temp;
      set->tag = LIST;
      if(fun)
      {
        set->left = fun;
        l = generators;
        for(k=1; k<=generatorcount; k++)
        {
          temp = newcell(APPLY);
          temp->left = set->left;
          temp->right = l->left->left;
          set->left = temp;
          l->left = l->left->right;
          l = l->right;
        }
      }
      else
      {
        set->left = generators->left->left;
        generators->left = generators->left->right;
      }
      return;
    }
    else
    {
      temp = newcell(SET2);
      *temp = *set;
      set->right = temp;
      setstackheight(stackheight()+generatorcount);
      l = generators;
      for(k=1; k<=generatorcount; k++)
      {
        setN(k, l->left->left);
        l->left = l->left->right;
        l = l->right;
      }
      argcount = 0;
      temp = fun;
      while(temp->tag == APPLY)
      {
        push(temp->right);
        argcount++;
        temp = temp->left;
      }
      function = getfunction(temp->value);
      if(function->code)
      {
        (*(function->code))();
        temp = pop();
      }
      else
      {
        setbasepointer(stackheight());
        checkmem();
        temp = copystructure(function->def);
      }
      set->tag = temp->tag;
      set->value = temp->value;
      set->left = temp->left;
      popN(argcount+generatorcount);
    }
  }
}


/********************************************************************
  The heart of the matter: the graph reducer

  eval evaluates the top of the stack to head normal form
  the spine of the redex is unwound
  i.e. the arguments are put on the stack
  the function is called (and returns its value on the stack)
  the function result is copied onto the right application cell

  in case of a compiled function the corresponding code function is called
  in case of a user defined function the template (def) is copied

  invariant:       original top()--- arg(argcount)
                             |
                            APPLY--- arg(argcount-1)
                             |
                             ...
                             |
                            APPLY--- arg(1)
                             |
                            temp
*********************************************************************/
void eval(void)
{
  FuncDef *fun;
  Cell *c, *temp = top();
  int k, argcount = 0;

  if(interruptcount-- == 0 || interrupted) checkinterrupt();
  stackcheck();

  for(;;)
  {
    if(temp->tag < nonevaltag || temp->tag > evaltag)
    {
      if(argcount > 0) popN(argcount);
      return;
    }
    switch(temp->tag)
    {
      case SYSFUNC1:
        push(temp->left);
        (*(getfunction(temp->value)->code))();
        *temp = *(pop());
        popN(1);
        break;
      case SYSFUNC2:
        push(temp->right);
        push(temp->left);
        (*(getfunction(temp->value)->code))();
        *temp = *(pop());
        popN(2);
        break;
      case APPLICATION:
        c = temp;
        fun = getfunction(c->value);
        if(fun->argcount == 0)
          ;
        else if(fun->argcount == 1)
          push(c->right);
        else
        {
          for(k=fun->argcount; k>1; k--)
          {
            push(c->left);
            c = c->right;
          }
          push(c);
        }
        temp->tag = UNDEFINED;
        if(fun->code)
        {
          (*(fun->code))();
          *temp = *(pop());
          popN(fun->argcount);
        }
        else
          *temp = *(copyreduce(fun->def, fun->argcount, stackheight()));
        break;
      case APPLY:
        do
        {
          push(temp->right);
          argcount++;
          temp = temp->left;
        }
        while(temp->tag == APPLY);
        break;
      case FUNC:
        fun = getfunction(temp->value);
        if(argcount < fun->argcount)
        {
          popN(argcount);
          return;
        }
        temp = getN(argcount+1);
        k = (argcount -= fun->argcount);
        while(k-- > 0) temp = temp->left;
        temp->tag = UNDEFINED;
        if(fun->code)
        {
          (*(fun->code))();
          *temp = *(pop());
          popN(fun->argcount);
        }
        else
          *temp = *(copyreduce(fun->def, fun->argcount, stackheight()));
        break;
      case SET1: case SET2:
        evalset(temp);
        break;
      case LETREC:
        *temp = *(copyreduce(temp, 0, stackheight()));
        break;
      case LAZYDIRECTOR:
        evaluate(temp->left);
        temp->left = copydirector(temp->value, temp->left);
        evaluate(temp->left);
        *temp = *(temp->left);
        break;
      case MATCH:
        if(match(temp->left, temp->right))
          *temp = *(temp->right);
        else
          temp->tag = ERROR;
        break;
      case _IF:
        push(temp->right->right);
        push(temp->right->left);
        push(temp->left);
        applyIF();
        *temp = *(pop());
        popN(3);
        break;
      case UNDEFINED:
        runtimeerror(UNDEFINED, -1);
      case ERROR:
        runtimeerror(ERROR, temp->value);
      default:
        systemerror(11);
    }
  }
}


