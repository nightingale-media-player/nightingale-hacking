/* vim: le=unix sw=2 : */

#ifndef _REGHANDLER_ERROR_H__
#define _REGHANDLER_ERROR_H__

#include "regparse.h"

struct driverLoc_t {
  enum insert_type_t { INSERT_AT_FRONT, INSERT_AFTER_NODE } insertAt;
  const regValue_t *loc;

  driverLoc_t(insert_type_t _i, const regValue_t *_l) : insertAt(_i), loc(_l) {} 
  // convenience constructor for INSERT_AT_FRONT
  driverLoc_t(insert_type_t _i) : insertAt(_i), loc(NULL) {}
};

LPTSTR GetSystemErrorMessage(DWORD errNum);
BOOL LoggedSUCCEEDED(LONG rv, LPCTSTR message);
int RegInstallKeys(void);
int RegRemoveKeys(void);
int AddFilteredDriver(LPCTSTR k, const regValue_t &nk, driverLoc_t l); 

#endif /* _REGHANDLER_ERROR_H__ */
