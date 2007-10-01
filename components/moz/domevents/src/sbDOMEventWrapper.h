#ifndef SB_DOM_EVENT_WRAPPER_H__
#define SB_DOM_EVENT_WRAPPER_H__

#include "sbIDOMEventWrapper.h"

#include <nsCOMPtr.h>
#include <nsIDOMNSEvent.h>
#include <nsIPrivateDOMEvent.h>

class sbDOMEventWrapper : public sbIDOMEventWrapper,
                          public nsIPrivateDOMEvent,
                          public nsIDOMNSEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDOMEVENTWRAPPER

  NS_FORWARD_SAFE_NSIDOMEVENT(mDOMEvent);
  NS_FORWARD_SAFE_NSIDOMNSEVENT(mNSEvent);

/** nsIPrivateDOMEvent - this is non-XPCOM so no forward macro **/
  NS_IMETHOD DuplicatePrivateData();
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD SetCurrentTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD SetOriginalTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD IsDispatchStopped(PRBool* aIsDispatchPrevented);
  NS_IMETHOD GetInternalNSEvent(nsEvent** aNSEvent);
  NS_IMETHOD HasOriginalTarget(PRBool* aResult);
  NS_IMETHOD SetTrusted(PRBool aTrusted);

  sbDOMEventWrapper();

private:
  ~sbDOMEventWrapper();

protected:
  nsCOMPtr<nsIDOMEvent> mDOMEvent;
  nsCOMPtr<nsIDOMNSEvent> mNSEvent;
  nsCOMPtr<nsISupports> mData;
};

#define SONGBIRD_DOM_EVENT_WRAPPER_CLASSNAME "sbDOMEventWrapper"
#define SONGBIRD_DOM_EVENT_WRAPPER_CID \
  /* {059A9FA9-7B73-480a-9333-5E1C519F4FAF} */ \
  { 0x59a9fa9, 0x7b73, 0x480a, \
    { 0x93, 0x33, 0x5e, 0x1c, 0x51, 0x9f, 0x4f, 0xaf } \
  }
#define SONGBIRD_DOM_EVENT_WRAPPER_CONTRACTID \
  "@songbirdnest.com/moz/domevents/wrapper;1"

#endif // SB_DOM_EVENT_WRAPPER_H__
