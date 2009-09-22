/* vim: set sw=2 :miv */

#include <tchar.h>
#include "regparse.h"

regValueList_t ParseValues(const multiSzBuffer_t& data, DWORD aRegType) {
  regValueList_t values;

  const TCHAR* p = reinterpret_cast<const TCHAR*>(data.value);
  size_t bytes = static_cast<size_t>(data.cbBytes);
  size_t count = bytes / sizeof(TCHAR);
  
  if (aRegType == REG_SZ || aRegType == REG_EXPAND_SZ) {
    values.push_back(regValue_t(p, count - 1));
  } else if (aRegType == REG_MULTI_SZ ) {
    while (count > 0) {
      size_t len = _tcsnlen(p, count);
      if (!len) {
        // empty, meaning end of strings
        break;
      }
      values.push_back(regValue_t(p, len));
      p += len + 1; // include null terminator
    }
  }
  
  return values;
}

void MakeMultiSzRegString(multiSzBuffer_t& data, const regValueList_t& values) {
  size_t bytesLeft = static_cast<size_t>(data.cbBytes);
  TCHAR* p = reinterpret_cast<TCHAR*>(data.value);
  for (regValueList_t::const_iterator it = values.begin();
   it != values.end(); ++it) {
    size_t bytes = it->len * sizeof(TCHAR);
    if (bytesLeft < bytes) {
      // BAIL BAIL BAIL
      break;
    }
    memcpy(p, it->p, bytes);
    p += it->len;
    *p++ = 0;
    bytesLeft -= bytes + sizeof(TCHAR);
  }
  // add the null to terminate the whole list
  *p++ = 0;

  // remember to report the byte count back out
  data.cbBytes = reinterpret_cast<LPBYTE>(p) - data.value;
}
