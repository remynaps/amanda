/**********************************************************************
  Author : Dick Bruin
  Date   : 24/08/98
  Version: 2.00
  File   : amio.c

  Description:

  file IO functions for the Amanda interpreter
  every open file has a unique filenr
  a negative filenr means an unsuccessful attempt to open a file
*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "amio.h"
#include "amerror.h"
#include "ammem.h"

/********************* signals *******************************/

static void TrapIntSignal(int sig)
{
  interrupted = True;
  signal(SIGINT, TrapIntSignal);
}

void SetIntSignal(bool enable)
{
  signal(SIGINT, enable ? TrapIntSignal : SIG_DFL);
}

/********************* file io *******************************/

#define MAXFILES 10

typedef enum { READ_MODE, WRITE_MODE, CLOSED } FileType;

typedef struct
{
  FILE *handle;
  FileType mode;
  bool garbage;
} FileDescriptor;

typedef FileDescriptor FileDescriptors[MAXFILES];

static FileDescriptor *fileDescriptors = NULL;

static FileDescriptor *searchfd(void)
{
  FileDescriptor *fd;

  for(fd=fileDescriptors; fd-fileDescriptors < MAXFILES; fd++)
    if(fd->mode == CLOSED) return fd;
  for(fd=fileDescriptors; fd-fileDescriptors < MAXFILES; fd++)
    fd->garbage = True;
  reclaim();
  for(fd=fileDescriptors; fd-fileDescriptors < MAXFILES; fd++)
    if(fd->garbage) CloseIOFile(fd-fileDescriptors);
  for(fd=fileDescriptors; fd-fileDescriptors < MAXFILES; fd++)
    if(fd->mode == CLOSED) return fd;
  return NULL;
}

int OpenIOFileRead(char s[])
{
  FileDescriptor *fd = searchfd();

  if(fd == NULL) return -1;
  fd->handle = fopen(s, "r");
  if(fd->handle == NULL) return -1;
  fd->mode = READ_MODE;
  return fd-fileDescriptors;
}

int OpenIOFileWrite(char s[])
{
  FileDescriptor *fd = searchfd();

  if(fd == NULL) return -1;
  fd->handle = fopen(s, "w");
  if(fd->handle == NULL) return -1;
  fd->mode = WRITE_MODE;
  return fd-fileDescriptors;
}

int OpenIOFileAppend(char s[])
{
  FileDescriptor *fd = searchfd();

  if(fd == NULL) return -1;
  fd->handle = fopen(s, "a");
  if(fd->handle == NULL) return -1;
  fd->mode = WRITE_MODE;
  return fd-fileDescriptors;
}

void CloseIOFile(int filenr)
{
  FileDescriptor *fd;

  if(filenr < 0 || filenr >= MAXFILES) return;
  fd = &fileDescriptors[filenr];
  if(fd->mode == CLOSED) return;
  fd->mode = CLOSED;
  if(fd->handle) fclose(fd->handle);
}

void CloseAllIOFiles(void)
{
  int k;
  for(k=0; k<MAXFILES; k++) CloseIOFile(k);
}

int ReadIOFile(int filenr)
{
  FileDescriptor *fd;
  int ch;

  if(filenr < 0 || filenr >= MAXFILES) return EOF;
  fd = &fileDescriptors[filenr];
  if(fd->mode != READ_MODE) return EOF;
  ch = fgetc(fd->handle);
  if(ch == EOF) CloseIOFile(filenr);
  return ch;
}

void WriteIOFile(int filenr, char ch)
{
  FileDescriptor *fd;

  if(filenr < 0 || filenr >= MAXFILES) return;
  fd = &fileDescriptors[filenr];
  if(fd->mode != WRITE_MODE) return;
  fputc(ch, fd->handle);
}

void MarkIOFile(int filenr)
{
  if(filenr < 0 || filenr >= MAXFILES) return;
  fileDescriptors[filenr].garbage = False;
}

void createIO(void)
{
  int k;
  if(fileDescriptors == NULL) fileDescriptors = malloc(sizeof(FileDescriptors));
  if(fileDescriptors == NULL) systemerror(4);
  for(k=0; k<MAXFILES; k++) fileDescriptors[k].mode = CLOSED;
}
