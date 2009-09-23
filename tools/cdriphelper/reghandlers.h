/* vim: le=unix sw=2 : */

#ifndef _REGHANDLER_ERROR_H__
#define _REGHANDLER_ERROR_H__

LPTSTR GetSystemErrorMessage(DWORD errNum);
BOOL LoggedSUCCEEDED(LONG rv, LPCTSTR message);
int RegInstallKeys(void);
int RegRemoveKeys(void);

#endif /* _REGHANDLER_ERROR_H__ */
