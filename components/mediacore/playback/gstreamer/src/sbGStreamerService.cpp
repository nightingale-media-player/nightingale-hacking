#include "sbGStreamerService.h"
#include "prlog.h"

#if defined( PR_LOGGING )
extern PRLogModuleInfo* gGStreamerLog;
#define LOG(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

NS_IMPL_ISUPPORTS2(sbGStreamerService, sbIGStreamerService, nsIObserver)

sbGStreamerService::sbGStreamerService() :
  mInitialized(PR_FALSE)
{
  Init();
}

sbGStreamerService::~sbGStreamerService()
{
}

// gstIGStreamerService
NS_IMETHODIMP
sbGStreamerService::Init()
{
  nsresult rv;

  if(mInitialized) {
    return NS_OK;
  }

  // Does not do much, perhaps will eventually read stuff from prefs and
  // pass it in
  gst_init(NULL, NULL);

  mInitialized = PR_TRUE;

  return NS_OK;
}

// nsIObserver
NS_IMETHODIMP
sbGStreamerService::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
  return NS_OK;
}

