#include <stdlib.h>
#include <string.h>
#include "config.h"

char *getTempFilePath()
{
  char *env = (char *)malloc(256 * sizeof(char));
  #ifdef _WIN32
    strcpy(env, getenv("TEMP"));
    strcat(env, "\\temp.ama");
  #else
    strcpy(env, "/tmp/temp.ama");
  #endif
  return env;
}

char *getAmaPath()
{
  #ifdef _WIN32
    return getenv("APPDATA");
  #else
    return "/usr/lib/";
  #endif
}

char BANNER[] = "Amanda V3.0\n\n";
char AMAINI[] = "amanda.ini";
