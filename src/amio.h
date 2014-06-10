/**********************************************************************
  Author : Dick Bruin
  Date   : 02/10/98
  Version: 2.01
  File   : amio.h

  Description:

  file IO functions for the Amanda interpreter
  every open file has a unique filenr
  a negative filenr means an unsuccessful attempt to open a file
*************************************************************/

#ifndef AMIO_H
#define AMIO_H

#include "bool.h"

void createIO(void);

/********************* signals *******************************/

void SetIntSignal(bool enable);

/********************* file io *******************************/

int  OpenIOFileRead(char filename[]);

int  OpenIOFileWrite(char filename[]);

int  OpenIOFileAppend(char filename[]);

int  ReadIOFile(int filenr);

void WriteIOFile(int filenr, char ch);

void CloseIOFile(int filenr);

void CloseAllIOFiles(void);

void MarkIOFile(int filenr);

#endif
