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

#define SONGBIRD_CLIPBOARD_HELPER_CLASSNAME "sbClipboardHelper"
#define SONGBIRD_CLIPBOARD_HELPER_CID \
  /* {6063116b-2b98-44d5-8f6e-0a7043f01fc3} */ \
  { 0x6063116b, 0x2b98, 0x44d5, \
    { 0x8f, 0x6e, 0x0a, 0x70, 0x43, 0xf0, 0x1f, 0xc3 } \
  }
#define SONGBIRD_CLIPBOARD_HELPER_CONTRACTID \
  "@songbirdnest.com/moz/clipboard/helper;1"

#endif // SB_CLIPBOARD_HELPER_H__
