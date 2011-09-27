#ifndef SB_CLIPBOARD_HELPER_H__
#define SB_CLIPBOARD_HELPER_H__

#include <sbIClipboardHelper.h>

#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#define SB_CLIPBOARD_HELPER_CLASSNAME "sbClipboardHelper"
#define SB_CLIPBOARD_HELPER_CID \
{ 0x6063116b, 0x2b98, 0x44d5, \
  { 0x8f, 0x6e, 0x0a, 0x70, 0x43, 0xf0, 0x1f, 0xc3 } }

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
