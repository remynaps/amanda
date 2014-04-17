/**********************************************************************
  Author : Dick Bruin
  Date   : 16/10/98
  Version: 2.01
  File   : amsyslib.h

  Description:

  library with definitions of system functions
  initsyslib initialises the hashtable
*************************************************************/

#ifndef AMSYSLIB_H
#define AMSYSLIB_H

#include "amtypes.h"

void InitOptions(bool console, char *path);
char *GetOption(char option[]);

void CreateInterpreter(void);
void Interpret(char expr[]);

bool Load(char filename[]);

void applyIF(void);

bool InitRemote(void);
int CreateRemote(char s[]);
void DropRemote(int handle);
void PutRemote(int handle, char s[]);
bool CallRemote(int handle, char s[]);
bool GetRemote(int handle, char s[], int size);

/************************************************************
  comparison of cells
  returned values: -, 0, + indicating smaller, equal, bigger
*************************************************************/
int comparecell(Cell *c1, Cell *c2);

/************************************************************
  Evaluates its argument to an integer and returns its value
  on error the value hashtablenr identifies the erroneous function
*************************************************************/
Integer evalint(Cell *c, int hashtablenr);

/************************************************************
  Evaluates its argument to a real and returns its value
*************************************************************/
Real evalreal(Cell *c);

#endif
