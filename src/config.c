#include <stdlib.h>
#include <string.h>
#include "config.h"

char *getTempFilePath()
{
  #ifdef _WIN32
	char *env = (char *)malloc(256 * sizeof(char));
	strcpy(env, getenv("TEMP"));
	strcat(env, "\\temp.ama");
	return env;
  #else
	return "/tmp/temp.ama";
  #endif
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
