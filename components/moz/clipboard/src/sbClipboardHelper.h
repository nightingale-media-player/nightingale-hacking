#ifndef SB_CLIPBOARD_HELPER_H__
#define SB_CLIPBOARD_HELPER_H__

#include <sbIClipboardHelper.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

class sbClipboardHelper : public sbIClipboardHelper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICLIPBOARDHELPER

  sbClipboardHelper();

private:
  ~sbClipboardHelper();

protected:
};
#endif // SB_CLIPBOARD_HELPER_H__
