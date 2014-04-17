/**********************************************************************
  Author : Dick Bruin
  Date   : 25/09/2000
  Version: 2.03
  File   : amcon.c

  Description:

  Amanda interpreter (default version)
  
  Usage:
    ama
    ama filename
    ama -obj filename
  
  amaobj commands:
    object objectname
    call functionname {parameter}
    echo
    time
    exit
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef AMA_READLINE
  #include <readline/readline.h>
  #include <readline/history.h>
#endif

#include "amcon.h"
#include "amerror.h"

#define stringsize 256

void WriteString(char string[])
{
  fputs(string, stdout);
  fflush(stdout);
}

static void initgetstring(void)
{
  #ifdef AMA_READLINE
    using_history();
  #endif
}

static void getstring(char prompt[], char string[])
{
  #ifdef AMA_READLINE
    char *s = readline(prompt);
    add_history(s);
    strncat(strcpy(string, ""), s, stringsize-1);
    free(s);
  #else
    WriteString(prompt);
    if(fgets(string, stringsize, stdin) == NULL) exit(0);
  #endif
}

void CheckIO(void)
{
}

void GraphDisplay(char string[])
{
}

static int FindWords(char line[], char *words[], int maxwords)
{
  enum { fwStart, fwWord, fwString } state = fwStart;
  int k, count = 0;
  for(k=0; line[k] && count<=maxwords; k++)
    if(isspace(line[k]) && state != fwString)
    {
      line[k] = '\0';
      state = fwStart;
    }
    else if(isspace(line[k]) && state == fwString)
      ;
    else if(line[k] == '\"' && state == fwString)
    {
      line[k] = '\0';
      state = fwStart;
    }
    else if(line[k] == '\"' && state != fwString)
    {
      line[k] = '\0';
      if(count < maxwords) words[count++] = &line[k+1];
      state = fwString;
    }
    else if(state == fwStart)
    {
      if(count < maxwords) words[count++] = &line[k];
      state = fwWord;
    }
  return count;
}

static void amaproc(char path[])
{
  InitOptions(False, path);
  CreateInterpreter();
  for(;;)
  {
    char expr[stringsize] = "";
    getstring(">> ", expr);
    WriteString("\n<\n");
    Interpret(expr);
    WriteString("\n>\n");
  }
}

static void amaobj(char path[], char filename[])
{
  char command[stringsize], s[stringsize], *words[stringsize];
  int k, handle = -1, count;
  bool echo = False;
  
  InitOptions(False, path);
  CreateInterpreter();
  if(!Load(filename) || !InitRemote()) return;
  for(;;)
  {
    getstring(">> ", command);
    if(echo) WriteString(command);
    WriteString("\n");
    count = FindWords(command, words, stringsize);
    if(count == 2 && strcmp(words[0], "object") == 0)
    {
      DropRemote(handle);
      handle = CreateRemote(words[1]);
    }
    else if(count >= 2 && strcmp(words[0], "call") == 0)
    {
      if(handle < 0)
        WriteString("No object selected");
      else
      {
        starttiming();
        for(k=2; k < count; k++) PutRemote(handle, words[k]);
        CallRemote(handle, words[1]);
        WriteString("<\n");
        while(GetRemote(handle, s, stringsize))
        {
          WriteString(s);
          WriteString("\n");
        }
        WriteString(">\n");
        stoptiming();
      }
    }
    else if(count == 1 && strcmp(words[0], "echo") == 0)
      echo = !echo;
    else if(count == 1 && strcmp(words[0], "time") == 0)
      timing = !timing;
    else if(count == 1 && strcmp(words[0], "exit") == 0)
      break;
    else
      WriteString("???\n");
    WriteString("\n");
  }
}

void main(int argc, char *argv[])
{
  initgetstring();
  if(argc > 1 && strcmp(argv[1], "-proc") == 0) 
  {
    amaproc(argv[0]);
    return;
  }
  if(argc > 2 && strcmp(argv[1], "-obj") == 0) 
  {
    amaobj(argv[0], argv[2]);
    return;
  }
  InitOptions(True, argv[0]);
  CreateInterpreter();
  if(argc > 1 && !Load(argv[1])) WriteString("\n");
  for(;;)
  {
    char expr[stringsize] = "";
    getstring(GetOption("ConPrompt"), expr);
    Interpret(expr);
  }
}
