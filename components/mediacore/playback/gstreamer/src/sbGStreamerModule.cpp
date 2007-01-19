#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsServiceManagerUtils.h"
#include "sbGStreamerService.h"
#include "sbGStreamerSimple.h"
#include "prlog.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbGStreamerService)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbGStreamerSimple)

static NS_METHOD
RegisterGStreamerService(nsIComponentManager *aCompMgr,
                         nsIFile *aPath,
                         const char *registryLocation,
                         const char *componentType,
                         const nsModuleComponentInfo *info)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry("app-startup",
                                "GStreamer Service",
                                SBGSTREAMERSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

//----------------------------------------------------------

static const nsModuleComponentInfo components[] =
{
	{
		SBGSTREAMERSERVICE_CLASSNAME,
		SBGSTREAMERSERVICE_CID,
		SBGSTREAMERSERVICE_CONTRACTID,
		sbGStreamerServiceConstructor,
    RegisterGStreamerService
	},
	{
		SBGSTREAMERSIMPLE_CLASSNAME,
		SBGSTREAMERSIMPLE_CID,
		SBGSTREAMERSIMPLE_CONTRACTID,
		sbGStreamerSimpleConstructor,
    nsnull
	}
};

//-----------------------------------------------------------------------------
#if defined( PR_LOGGING )
PRLogModuleInfo *gGStreamerLog;

// setup nspr logging ...
PR_STATIC_CALLBACK(nsresult)
InitGStreamer(nsIModule *self)
{
  gGStreamerLog = PR_NewLogModule("gstreamer");
  return NS_OK;
}
#else
#define InitGStreamer nsnull
#endif

NS_IMPL_NSGETMODULE_WITH_CTOR(sbGStreamerModule, components, InitGStreamer)

