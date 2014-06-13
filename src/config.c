#include <stdlib.h>
#include "config.h"

#ifdef _WIN32
  char TEMPFILEPATH[] = strcat(getenv("TEMP"), "\\temp.ama");
  char AMAPATH[] = getenv("APPDATA");
#else
  char TEMPFILEPATH[] = "/tmp/temp.ama";
  char AMAPATH[] = "/usr/local/lib/";
#endif
char BANNER[] = "Amanda V3.0\n\n";
char AMAINI[] = "amanda.ini";
