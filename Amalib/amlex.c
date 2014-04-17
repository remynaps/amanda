/**********************************************************************
  Author : Dick Bruin
  Date   : 19/12/99
  Version: 2.02
  File   : amlex.c

  Description:

  the lexical analyser of the Amanda interpreter
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bool.h"
#include "amlex.h"
#include "amerror.h"
#include "amprint.h"
#include "amsyslib.h"
#include "amtable.h"

#define stringsize        256
#define filenamesize      256
#define INPUTPUTBACKSIZE   20


TokenType tokentype;

char tokenval[stringsize];

int tokenindent, tokenoffside;


typedef enum { FILEINPUT, EXPRESSIONINPUT, CLOSED } InputState;

typedef struct INPUTFILE
{
  FILE *fp;
  char name[filenamesize];
  int linenr;
  int columnnr;
  struct INPUTFILE *next, *link;
}
InputFile;

typedef struct SYNONYM
{
  char synonym[stringsize];
  char name[stringsize];
  TokenType type;
  struct SYNONYM *next;
}
Synonym;


static InputState inputstate;

static char inputbuffer[stringsize];

static int inputputbackcount, inputputback[INPUTPUTBACKSIZE];

static InputFile firstInputFile, *current = &firstInputFile, *inputFiles = &firstInputFile;

static Synonym *synonyms = NULL;


void initlex(void)
{
  inputstate = CLOSED;
  firstInputFile.fp   = NULL;
  firstInputFile.next = NULL;
  firstInputFile.link = NULL;
  while(inputFiles->next)
  {
    InputFile *i = inputFiles;
    inputFiles = inputFiles->next;
    if(i->fp) fclose(i->fp);
    free(i);
  }
  current = inputFiles;
  while(synonyms)
  {
    Synonym *s = synonyms;
    synonyms = s->next;
    free(s);
  }
}

void closeinput(void)
{
  if(inputstate == FILEINPUT)
  {
    if(current->fp)
    {
      fclose(current->fp);
      current->fp = NULL;
    }
    if(current->link)
      current = current->link;
    else
      inputstate = CLOSED;
  }
  else
    inputstate = CLOSED;
  inputbuffer[0] = '\0';
  inputputbackcount = 0;
}

void lexerror(void)
{
  if(inputstate == EXPRESSIONINPUT || strcmp(current->name, "") == 0)
  {
    int k, l = current->columnnr + strlen(GetOption("ConPrompt"));
    for(k=1; k<l; k++) Write(" ");
    Write("^");
  }
  else
  {
    Write("file  : %s\n", current->name);
    Write("line  : %d\n", current->linenr);
    Write("column: %d\n", current->columnnr);
    while(inputstate != CLOSED) closeinput();
  }
}

int getPositionCode(void)
{
  if(inputstate == FILEINPUT)
  {
    int k = 0;
    InputFile *i = inputFiles;
    while(i && i != current)
    {
      i = i->next;
      k++;
    }
    return i ? k + i->linenr * 10 : 0;
  }
  return 0;
}

char *posCodeFileName(int positionCode)
{
  int k = 0, pos = positionCode % 10;
  InputFile *i = inputFiles;
  while(i && k != pos)
  {
    i = i->next;
    k++;
  }
  return i ? i->name : "";
}

int posCodeLinenr(int positionCode)
{
  return positionCode / 10;
}

void openfileinput(char filename[])
{
  if(inputstate != CLOSED && inputstate != FILEINPUT)
    parseerror(20);
  if(inputstate == FILEINPUT)
  {
    InputFile *i = malloc(sizeof(InputFile));
    if(!i) parseerror(21);
    i->link = current;
    current = i;
    i->next = inputFiles;
    inputFiles = i;
  }
  inputstate = FILEINPUT;
  strncat(strcpy(current->name, ""), filename, stringsize-1);
  current->linenr = current->columnnr = 0;
  current->fp = fopen(current->name, "r");
  if(current->fp == NULL)
    Write("\nWARNING: file %s not found\n", current->name);
  inputbuffer[0] = '\0';
  inputputbackcount = 0;
}

void openinput(char s[])
{
  while(inputstate != CLOSED) closeinput();
  inputstate = EXPRESSIONINPUT;
  strcpy(current->name, "");
  strncat(strcpy(inputbuffer, ""), s, sizeof(inputbuffer)-1);
  current->columnnr = 0;
  inputputbackcount = 0;
}

static void getprimarytoken(void);

static bool preprocess(void)
{
  int globaltokenoffside = tokenoffside;
  char *s = inputbuffer;
  if(strncmp(s, "####", 4) == 0)
  {
    char *t = inputbuffer;
    int k = 0;
    s = inputbuffer+4;
    while(*s)
    {
      *t = 0 < *s && *s <= 26 ? *s : *s - 1 - k % 10;
      t++;
      s++;
      k++;
    }
    *t = '\0';
  }
  s = inputbuffer;
  while(isspace(*s)) s++;
  if(strncmp(s, "#import", 7) == 0)
  {
    InputFile *i = inputFiles;
    tokenoffside = current->columnnr = s-inputbuffer+7;
    gettoken();
    if(tokentype != STRING) parseerror(22);
    tokenoffside = globaltokenoffside;
    inputbuffer[0] = '\0';
    current->columnnr = 0;
    while(i->next && strcmp(tokenval, i->name) != 0) i = i->next;
    if(!(i->next)) openfileinput(tokenval);
    return True;
  }
  else if(strncmp(s, "#synonym", 8) == 0)
  {
    Synonym *syn = malloc(sizeof(Synonym));
    if(syn == NULL) systemerror(4);
    syn->next = synonyms;
    synonyms = syn;
    tokenoffside = current->columnnr = s-inputbuffer+8;
    getprimarytoken();
    strcpy(syn->synonym, tokenval);
    gettoken();
    strcpy(syn->name, tokenval);
    syn->type = tokentype;
    tokenoffside = globaltokenoffside;
    inputbuffer[0] = '\0';
    current->columnnr = 0;
    return True;
  }
  else if(strncmp(s, "#operator", 9) == 0)
  {
    Assoc assoc;
    int prio;
    tokenoffside = current->columnnr = s-inputbuffer+9;
    gettoken();
    if(strcmp(tokenval, "r") == 0)
      assoc = Right;
    else if(strcmp(tokenval, "l") == 0)
      assoc = Left;
    else
      parseerror(34);
    gettoken();
    if(tokentype != NUMBER) parseerror(34);
    prio = atoi(tokenval);
    gettoken();
    if(tokentype != OPERATOR) parseerror(34);
    insertoperator(tokenval, prio, assoc);
    tokenoffside = globaltokenoffside;
    inputbuffer[0] = '\0';
    current->columnnr = 0;
    return True;
  }
  else
    return False;
}

/*********************************************************************
  return: next character from current input stream
**********************************************************************/
static int getnext(void)
{
  int ch;

  if(inputputbackcount > 0)
  {
    ch = inputputback[--inputputbackcount];
    (current->columnnr)++;
    return ch;
  }
  if(inputstate == CLOSED)
    systemerror(9);
  if(current->columnnr > 0 && inputbuffer[current->columnnr-1] == '\0')
  {
    if(inputstate == EXPRESSIONINPUT)
      return EOF;
    else if(current->fp == NULL || !fgets(inputbuffer, stringsize, current->fp))
    {
      closeinput();
      return inputstate==FILEINPUT ? getnext() : EOF;
    }
    (current->linenr)++;
    current->columnnr = 0;
    if(preprocess()) return getnext();
  }
  ch = inputbuffer[(current->columnnr)++];
  if(ch == '\0') ch = '\n';
  return ch;
}

static void ungetnext(int ch)
{
  if(inputputbackcount >= INPUTPUTBACKSIZE) parseerror(23);
  inputputback[inputputbackcount++] = ch;
  (current->columnnr)--;
}

static char makehex(char ch)
{
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  else if(ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  else if(ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  else
    parseerror(24);
  return ch;
}

static void getprimarytoken(void)
{
  static int wildcard = 0;
  int count = 0, ch;

  while(isspace(ch=getnext()));
  tokenindent = current->columnnr;
  if(tokenindent < tokenoffside)
  {
    ungetnext(ch);
    tokentype = offside;
    tokenval[count++] = '\0';
    return;
  }
  if(isdigit(ch))
  {
    tokentype = NUMBER;
    while(isdigit(ch))
    {
      tokenval[count++] = ch;
      ch = getnext();
    }
    if(ch == '.')
    {
      ch = getnext();
      if(isdigit(ch))
        tokenval[count++] = '.';
      else
      {
        ungetnext(ch);
        ch = '.';
      }
      while(isdigit(ch))
      {
        tokenval[count++] = ch;
        ch = getnext();
      }
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    return;
  }
  if(isalpha(ch) || ch == '_')
  {
    if('A' <= ch && ch <= 'Z')
      tokentype = TYPEID;
    else
      tokentype = IDENTIFIER;
    while(isalpha(ch) || isdigit(ch) || ch == '_')
    {
      tokenval[count++] = ch;
      ch = getnext();
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    if(strcmp(tokenval, "where")     == 0) tokentype = WHERE;
    if(strcmp(tokenval, "otherwise") == 0) tokentype = OTHERWISE;
    if(strcmp(tokenval, "abstype")   == 0) tokentype = ABSTYPE;
    if(strcmp(tokenval, "with")      == 0) tokentype = WITH;
    if(strcmp(tokenval, "generic")   == 0) tokentype = GENERIC;
    if(strcmp(tokenval, "True")      == 0) tokentype = IDENTIFIER;
    if(strcmp(tokenval, "False")     == 0) tokentype = IDENTIFIER;
    if(strcmp(tokenval, "Nil")       == 0) tokentype = IDENTIFIER;
    if(strcmp(tokenval, "_")         == 0) sprintf(tokenval, "_??_%d", wildcard++);
    return;
  }
  if(ch == '$')
  {
    ch = getnext();
    if(('a' <= ch && ch <= 'z') || ch == '_')
    {
      tokentype = OPERATOR;
      while(isalpha(ch) || isdigit(ch) || ch == '_')
      {
        tokenval[count++] = ch;
        ch = getnext();
      }
      tokenval[count++] = '\0';
      ungetnext(ch);
    }
    else
      parseerror(25);
    return;
  }
  if(ch == '\'')
  {
    tokentype = CHARACTER;
    ch = getnext();
    if(!isprint(ch)) parseerror(26);
    if(ch == '\\')
    {
      ch = getnext();
      switch(ch)
      {
        case 'a':
          ch = '\a';
          break;
        case 'b':
          ch = '\b';
          break;
        case 'f':
          ch = '\f';
          break;
        case 'n':
          ch = '\n';
          break;
        case 'r':
          ch = '\r';
          break;
        case 't':
          ch = '\t';
          break;
        case 'v':
          ch = '\v';
          break;
        case 'e':
          ch = 27;
          break;
        case 'x':
          ch = 16 * makehex(getnext()) + makehex(getnext());
          break;
      }
    }
    tokenval[count++] = ch;
    ch = getnext();
    if(ch != '\'') parseerror(27);
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '\"')
  {
    tokentype = STRING;
    ch = getnext();
    while(isprint(ch) && ch != '\"')
    {
      if(ch == '\\')
      {
        ch = getnext();
        switch(ch)
        {
          case 'a':
            ch = '\a';
            break;
          case 'b':
            ch = '\b';
            break;
          case 'f':
            ch = '\f';
            break;
          case 'n':
            ch = '\n';
            break;
          case 'r':
            ch = '\r';
            break;
          case 't':
            ch = '\t';
            break;
          case 'v':
            ch = '\v';
            break;
          case 'e':
            ch = 27;
            break;
          case 'x':
            ch = 16 * makehex(getnext()) + makehex(getnext());
            break;
        }
      }
      tokenval[count++] = ch;
      ch = getnext();
    }
    if(ch != '\"') parseerror(28);
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '(')
  {
    tokentype = LPAR;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == ')')
  {
    tokentype = RPAR;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == ';')
  {
    tokentype = SEP;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == ',')
  {
    tokentype = COMMA;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '[')
  {
    tokentype = LBRACK;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == ']')
  {
    tokentype = RBRACK;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '{')
  {
    tokentype = LACC;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '}')
  {
    tokentype = RACC;
    tokenval[count++] = ch;
    tokenval[count++] = '\0';
    return;
  }
  if(ch == '|')
  {
    tokentype = BAR;
    tokenval[count++] = ch;
    ch = getnext();
    if(ch == '|')
    {
      while(ch != '\n')
      {
        ch = getnext();
        if(ch == EOF)
        {
          tokentype = empty;
          tokenval[count++] = '\0';
          return;
        }
      }
      gettoken();
      return;
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    return;
  }
  if(ch == '.')
  {
    tokentype = OPERATOR;
    tokenval[count++] = ch;
    ch = getnext();
    if(ch == '.')
    {
      tokentype = POINTS;
      tokenval[count++] = ch;
      ch = getnext();
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    return;
  }
  if(ch == '/')
  {
    ch = getnext();
    if(ch == '*')
    {
      int comments = 1;
      while(comments > 0)
      {
        ch = getnext();
        if(ch == EOF) parseerror(29);
        else if(ch == '/')
        {
          ch = getnext();
          if(ch == '*') comments++;
          else ungetnext(ch);
        }
        else if(ch == '*')
        {
          ch = getnext();
          if(ch == '/') comments--;
          else ungetnext(ch);
        }
      }
      gettoken();
      return;
    }
    tokentype = OPERATOR;
    tokenval[count++] = '/';
    while(ispunct(ch) && ch!='('  && ch!=')'  && ch!='['
                      && ch!=']'  && ch!='{'  && ch!='}'
                      && ch!='|'  && ch!=';'  && ch!=','
                      && ch!='\'' && ch!='\"' && ch!='.'
                      && ch!='_')
    {
      tokenval[count++] = ch;
      ch = getnext();
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    return;
  }
  if(ispunct(ch))
  {
    tokentype = OPERATOR;
    while(ispunct(ch) && ch!='('  && ch!=')'  && ch!='['
                      && ch!=']'  && ch!='{'  && ch!='}'
                      && ch!='|'  && ch!=';'  && ch!=','
                      && ch!='\'' && ch!='\"' && ch!='.'
                      && ch!='_')
    {
      tokenval[count++] = ch;
      ch = getnext();
    }
    tokenval[count++] = '\0';
    ungetnext(ch);
    if     (strcmp(tokenval, "<-")  == 0) tokentype = GENER;
    else if(strcmp(tokenval, ":=")  == 0) tokentype = ASSIGNMENT;
    else if(strcmp(tokenval, "->")  == 0) tokentype = ARROW;
    else if(strcmp(tokenval, "::")  == 0) tokentype = COLONS;
    else if(strcmp(tokenval, "::=") == 0) tokentype = DEF;
    else if(strcmp(tokenval, "==")  == 0) tokentype = SYN;
    else if(strcmp(tokenval, "~")   == 0) tokentype = IDENTIFIER;
    else if(strcmp(tokenval, "#")   == 0) tokentype = IDENTIFIER;
    return;
  }
  tokentype = empty;
  tokenval[count++] = '\0';
  ungetnext(ch);
}

void gettoken(void)
{
  Synonym *syn;
  getprimarytoken();
  if(tokentype != STRING && tokentype != CHARACTER)
    for(syn=synonyms; syn; syn=syn->next)
      if(strcmp(tokenval, syn->synonym)==0)
      {
        strcpy(tokenval, syn->name);
        tokentype = syn->type;
        return;
      }
}
