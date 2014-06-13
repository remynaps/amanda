#ifdef _WIN32
  static const char TEMPFILEPATH[] = strcat(getenv("TEMP"), "\\temp.ama");
  static const char AMAPATH[] = getenv("APPDATA");
#else
  static const char TEMPFILEPATH[] = "/tmp/temp.ama";
  static const char AMAPATH[] = "/usr/local/lib/";
#endif
static const char BANNER[] = "Amanda V3.0\n\n";
static const char AMAINI[] = "amanda.ini";
