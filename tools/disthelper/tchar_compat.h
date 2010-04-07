#pragma once
#define TCHAR char
#define LPTSTR char*
#define LPCTSTR const char*
#define _T(x) (x)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

static inline char *_tgetenv(const char* varname) {
  return getenv(varname);
}
static inline int _tputenv(char* envstring) {
  return putenv(envstring);
}

static inline int _tunlink(const char* filename) {
  return unlink(filename);
}

static inline int _unlink(const char* filename) {
  return unlink(filename);
}

static inline int _mkdir(const char* dirname) {
  return mkdir(dirname, 0755);
}

static inline void OutputDebugString(LPCTSTR msg) {
  fprintf(stderr, "%s\n", msg);
}

static inline FILE* _tfopen(const char* filename, const char* mode) {
  return fopen(filename, mode);
}

static inline int _tchdir(const char* dirname) {
  return chdir(dirname);
}
