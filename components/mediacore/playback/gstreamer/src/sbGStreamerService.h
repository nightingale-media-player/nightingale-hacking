#ifndef _SB_GSTREAMER_SERVICE_H_
#define _SB_GSTREAMER_SERVICE_H_

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "sbIGStreamerService.h"

#include <gst/gst.h>

class sbGStreamerService : public sbIGStreamerService,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIGSTREAMERSERVICE
  NS_DECL_NSIOBSERVER

  sbGStreamerService();

private:
  ~sbGStreamerService();

  PRBool mInitialized;

protected:
};

#endif // _SB_GSTREAMER_SERVICE_H_

