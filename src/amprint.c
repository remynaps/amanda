/**********************************************************************
  Author : Dick Bruin
  Date   : 26/10/99
  Version: 2.02
  File   : amprint.c

  Description:

  printing of the contents of a graph for the Amanda interpreter
*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "ameval.h"
#include "amerror.h"
#include "amtypes.h"
#include "amprint.h"
#include "amtable.h"
#include "amstack.h"
#include "amsyslib.h"

void Write(char *fmt, ...)
{
  va_list args;
  char buffer[256];
  va_start(args, fmt);
  vsprintf(buffer, fmt, args);
  va_end(args);
  WriteString(buffer);
}

static void WriteC(Cell *c, bool parentheses);

static void WriteDirector(int bitstring, TagType tag)
{
  while(bitstring>1)
  {
    if(bitstring&1    && tag==STRICTDIRECTOR) WriteString(".L");
    if(!(bitstring&1) && tag==STRICTDIRECTOR) WriteString(".R");
    if(bitstring&1    && tag==LAZYDIRECTOR)   WriteString(".l");
    if(!(bitstring&1) && tag==LAZYDIRECTOR)   WriteString(".r");
    bitstring >>= 1;
  }
}

static void WriteElems(Cell *c, TagType tag, char start[], char separator[], char stop[], bool parentheses)
{
  WriteString(start);
  for(; c->tag == tag; c=c->right)
  {
    WriteC(c->left, parentheses);
    if(c->right->tag == tag) WriteString(separator);
  }
  WriteString(stop);
}

static void WriteList(Cell *c, bool parentheses)
{
  bool charlist = True;
  Cell *temp;
  for(temp=c; temp->tag==LIST; temp=temp->right)
    if(temp->left->tag != CHAR) charlist = False;
  if(temp->tag != NIL)
  {
    if(parentheses) WriteString("(");
    for(temp=c; temp->tag==LIST; temp=temp->right)
    {
      WriteC(temp->left, True);
      WriteString(":");
    }
    WriteC(temp, True);
    if(parentheses) WriteString(")");
  }
  else if(charlist)
  {
    WriteString("\"");
    for(temp=c; temp->tag==LIST; temp=temp->right)
      Write("%c", temp->left->value);
    WriteString("\"");
  }
  else
    WriteElems(c, LIST, "[", ", ", "]", False);
}

static void WriteFunc(char name[])
{
  bool isOperator = !(isalpha(name[0]) || name[0] == '_');
  if(isOperator) WriteString("(");
  WriteString(name);
  if(isOperator) WriteString(")");
}

static void WriteApply(Cell *c)
{
  int k;
  char *name;
  for(k=0; c->tag==APPLY; k++,c=c->left) push(c->right);
  if(k == 2 
  && c->tag == FUNC 
  && (name = getfunction(c->value)->name) != NULL
  && !(isalpha(name[0]) || name[0] == '_'))
  {
    WriteC(pop(), True);
    WriteString(" ");
    WriteString(name);
    WriteString(" ");
    WriteC(pop(), True);
  }
  else
  {
    WriteC(c, True);
    for(; k>0; k--)
    {
      WriteString(" ");
      WriteC(pop(), True);
    }
  }
}

static void WriteC(Cell *c, bool parentheses)
{
  int k;
  FuncDef *fun;

  if(c==NULL) return;
  switch(c->tag)
  {
    case APPLY:
      if(parentheses) WriteString("(");
      WriteApply(c);
      if(parentheses) WriteString(")");
      break;
    case ARG:
      if(c->value>0)
        Write("ARG(%d)", c->value);
      else
        Write("LOCAL(%d)", -c->value);
      break;
    case INT:
      Write("%ld", integer(c));
      break;
    case REAL:
      Write("%lg", real(c));
      break;
    case CHAR:
      Write("'%c'", c->value);
      break;
    case BOOLEAN:
      WriteString(c->value ? "True" : "False");
      break;
    case NULLTUPLE:
      WriteString("()");
      break;
    case LIST:
      WriteList(c, parentheses);
      break;
    case NIL:
      WriteString("Nil");
      break;
    case STRUCT:
      WriteElems(c, STRUCT, parentheses ? "(" : "", " ", parentheses ? ")" : "", True);
      break;
    case PAIR:
      WriteElems(c, PAIR, "(", ", ", ")", False);
      break;
    case RECORD:
      WriteElems(c, RECORD, "{", ", ", "}", False);
      break;
    case _IF:
      if(parentheses) WriteString("(");
      WriteString("_if ");
      WriteC(c->left, True);
      WriteString(" ");
      WriteC(c->right->left, True);
      WriteString(" ");
      WriteC(c->right->right, True);
      if(parentheses) WriteString(")");
      break;
    case MATCH:
      if(parentheses) WriteString("(");
      WriteString("_match ");
      WriteC(c->left, True);
      WriteString(" ");
      WriteC(c->right, True);
      if(parentheses) WriteString(")");
      break;
    case MATCHARG:
      if(parentheses) WriteString("(");
      for(;;)
      {
        WriteString("_match ");
        WriteC(c->left, True);
        WriteString(" ");
        if(c->value>0)
          Write("ARG(%d)", c->value);
        else
          Write("LOCAL(%d)", -c->value);
        c = c->right;
        if(c == NULL) break;
        WriteString(" /\\ ");
      }
      if(parentheses) WriteString(")");
      break;
    case MATCHTYPE:
      if(c->value == INT)
        WriteString("num");
      else if(c->value == BOOLEAN)
        WriteString("bool");
      else if(c->value == CHAR)
        WriteString("char");
      else
        WriteString("...");
      break;
    case ALIAS:
      if(parentheses) WriteString("(");
      WriteC(c->left, False);
      WriteString(" = ");
      WriteC(c->right, False);
      if(parentheses) WriteString(")");
      break;
    case UNDEFINED:
      WriteString("undefined");
      break;
    case GENERATOR:
      WriteString("[");
      WriteElems(c->left, LIST, "", ", ", "", False);
      WriteString(" | ");
      for(c=c->right; c->tag==GENERATOR; c=c->right)
      {
        if(c->left->right)
        {
          WriteElems(c->left->left, LIST, "", ", ", "", False);
          WriteString(" <- ");
          WriteElems(c->left->right, LIST, "", ", ", "", False);
        }
        else
          WriteC(c->left->left, False);
        if(c->right->tag==GENERATOR) WriteString("; ");
      }
      WriteString("]");
      break;
    case SYSFUNC1:
      fun = getfunction(c->value);
      if(parentheses) WriteString("(");
      WriteString(fun->name);
      WriteString(" ");
      WriteC(c->left, True);
      if(parentheses) WriteString(")");
      break;
    case SYSFUNC2:
      fun = getfunction(c->value);
      if(parentheses) WriteString("(");
      WriteC(c->left, True);
      WriteString(" ");
      WriteString(fun->name);
      WriteString(" ");
      WriteC(c->right, True);
      if(parentheses) WriteString(")");
      break;
    case APPLICATION:
      fun = getfunction(c->value);
      if(parentheses) WriteString("(");
      WriteString(fun->name);
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
      for(k=fun->argcount; k>0; k--)
      {
        WriteString(" ");
        WriteC(pop(), True);
      }
      if(parentheses) WriteString(")");
      break;
    case FUNC: case TYPE:
      WriteFunc(getfunction(c->value)->name);
      break;
    case ERROR:
      Write("error(%s)", getfunction(c->value)->name);
      break;
    case CONST:
      WriteString("(Const ");
      WriteC(c->left, False);
      WriteString(")");
      break;
    case STRICTDIRECTOR: case LAZYDIRECTOR:
      WriteDirector(c->value, c->tag);
      if(parentheses) WriteString("(");
      WriteC(c->left, True);
      if(parentheses) WriteString(")");
      break;
    case LETREC:
      if(parentheses) WriteString("(");
      WriteC(c->right, False);
      WriteString(" WHERE ");
      k = 0;
      for(c=c->left; c->tag==LIST; c=c->right)
      {
        Write("LOCAL(%d) = ", -(k--));
        WriteC(c->left, False);
        WriteString("; ");
      }
      WriteString("ENDWHERE");
      if(parentheses) WriteString(")");
      break;
    case LAMBDA:
      WriteC(c->left, False);
      WriteString(" -> ");
      WriteC(c->right, False);
      break;
    case LAMBDAS:
      WriteElems(c, LAMBDAS, "(", " | ", ")", False);
      break;
    case VARIABLE:
      WriteString(getfunction(c->left->value)->name);
      break;
    case SET1: case SET2:
      WriteString("[");
      WriteC(c->left->left, False);
      for(k=1; k<=c->value; k++) Write(" x%d", k);
      WriteString(" | (x1");
      for(k=2; k<=c->value; k++) Write(", x%d", k);
      WriteString(") <- ");
      WriteC(c->left->right->right, False);
      if(c->left->right->left)
      {
        WriteString("; ");
        WriteC(c->left->right->left, False);
        for(k=1; k<=c->value; k++) Write(" x%d", k);
      }
      WriteString("]");
      break;
    default:
      systemerror(7);
  }
}

void WriteCell(Cell *c)
{
  WriteC(c, False);
}

static void WriteT(Cell *c, bool parentheses)
{
  int k;
  if(c == NULL) return;
  switch(c->tag)
  {
    case INT: case REAL:
      WriteString("num");
      break;
    case CHAR:
      WriteString("char");
      break;
    case BOOLEAN:
      WriteString("bool");
      break;
    case NULLTUPLE:
      WriteString("()");
      break;
    case LIST:
      WriteString("[");
      WriteT(c->left, False);
      WriteString("]");
      break;
    case PAIR:
      WriteString("(");
      WriteT(c->left, False);
      while(c->right->tag == PAIR)
      {
        WriteString(", ");
        c = c->right;
        WriteT(c->left, False);
      }
      WriteString(")");
      break;
    case RECORD:
      WriteString("{");
      WriteCell(c->left->left);
      WriteString(" :: ");
      WriteT(c->left->right, False);
      while(c->right->tag == RECORD)
      {
        WriteString(", ");
        c = c->right;
        WriteCell(c->left->left);
        WriteString(" :: ");
        WriteT(c->left->right, False);
      }
      WriteString("}");
      break;
    case APPLY:
      if(parentheses) WriteString("(");
      while(c->tag == APPLY)
      {
        WriteT(c->left, True);
        WriteString(" -> ");
        c = c->right;
      }
      WriteT(c, False);
      if(parentheses) WriteString(")");
      break;
    case TYPEVAR:
      for(k=1; k<=c->value; k++) WriteString("*");
      break;
    case TYPESYNONYM:
      WriteT(c->left, False);
      WriteString(" == ");
      WriteT(c->right, False);
      break;
    case TYPEDEF:
      WriteT(c->left, False);
      WriteString(" ::= ");
      WriteT(c->right, False);
      break;
    case STRUCT:
      if(parentheses) WriteString("(");
      WriteString(getfunction(c->left->value)->name);
      while(c->right->tag == STRUCT)
      {
        WriteString(" ");
        c = c->right;
        WriteT(c->left, True);
      }
      if(parentheses) WriteString(")");
      break;
    default:
      systemerror(8);
  }
}

void WriteType(Cell *c)
{
  WriteT(c, False);
}

static void WriteEval(bool parentheses)
{
  Cell *temp = pop();
  bool charlist;

  evaluate(temp);
  switch(temp->tag)
  {
    case NIL:
      WriteCell(temp);
      break;
    case LIST:
      push(temp->right);
      temp = temp->left;
      evaluate(temp);
      charlist = temp->tag == CHAR;
      if(!charlist)
      {
        WriteString("[");
        push(temp);
        WriteEval(False);
      }
      else
      {
        WriteString("\"");
        Write("%c", temp->value);
      }
      for(;;)
      {
        eval();
        temp = pop();
        if(temp->tag != LIST) break;
        push(temp->right);
        temp = temp->left;
        if(!charlist)
        {
          WriteString(", ");
          push(temp);
          WriteEval(False);
        }
        else
        {
          evaluate(temp);
          Write("%c", temp->value);
        }
      }
      if(!charlist)
        WriteString("]");
      else
        WriteString("\"");
      break;
    case STRUCT:
      if(parentheses) WriteString("(");
      push(temp->right);
      push(temp->left);
      WriteEval(True);
      for(;;)
      {
        temp = pop();
        if(temp->tag != STRUCT) break;
        push(temp->right);
        push(temp->left);
        WriteString(" ");
        WriteEval(True);
      }
      if(parentheses) WriteString(")");
      break;
    case PAIR:
      WriteString("(");
      push(temp->right);
      push(temp->left);
      WriteEval(False);
      for(;;)
      {
        temp = pop();
        if(temp->tag != PAIR) break;
        push(temp->right);
        push(temp->left);
        WriteString(", ");
        WriteEval(False);
      }
      WriteString(")");
      break;
    case RECORD:
      WriteString("{");
      push(temp->right);
      push(temp->left);
      WriteEval(False);
      for(;;)
      {
        temp = pop();
        if(temp->tag != RECORD) break;
        push(temp->right);
        push(temp->left);
        WriteString(", ");
        WriteEval(False);
      }
      WriteString("}");
      break;
    default:
      WriteCell(temp);
      break;
  }
}

/************************************************************************
  The toplevel of the interpreter:
  evaluate top of stack and print it
  the stack is popped
*************************************************************************/
void toplevel(void)
{
  WriteEval(False);
}
