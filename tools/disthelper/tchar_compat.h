#pragma once
#define TCHAR char
#define LPTSTR char*
#define LPCTSTR const char*
#define _T(x) (x)

#include <stdlib.h>
#include <stdio.h>

inline char *_tgetenv(const char* varname) {
  return getenv(varname);
}

inline void OutputDebugString(LPCTSTR msg) {
  fprintf(stderr, "%s\n", msg);
}

inline FILE* _tfopen(const char* filename, const char* mode) {
  return fopen(filename, mode);
}
