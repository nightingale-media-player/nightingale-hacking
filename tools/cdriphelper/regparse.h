/* vim: set sw=2 :miv */

#include <windows.h>
#include <list>

#ifndef _REGHELPER_REGPARSE_H__
#define _REGHELPER_REGPARSE_H__

struct multiSzBuffer_t {
  LPBYTE value;
  DWORD cbBytes;
  multiSzBuffer_t(LPBYTE _v, DWORD _b)
    :value(_v), cbBytes(_b) {}
};

struct regValue_t {
  const TCHAR* p;
  size_t len; // in characters, no terminating null
  regValue_t(const TCHAR* _p, size_t _len)
    :p(_p), len(_len) {}
};

typedef std::list<regValue_t> regValueList_t;

regValueList_t ParseValues(const multiSzBuffer_t& data, DWORD regKeyType);
void MakeMultiSzRegString(multiSzBuffer_t& data, const regValueList_t& values); 

#endif /* _REGHELPER_REGPARSE_H__ */
