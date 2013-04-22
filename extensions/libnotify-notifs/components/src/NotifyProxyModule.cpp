#include "nsIGenericFactory.h"
#include "NotifyProxy.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(NotifyProxy)

static nsModuleComponentInfo components[] =
{
    {
       NOTIFY_PROXY_CLASSNAME,
       NOTIFY_PROXY_CID,
       NOTIFY_PROXY_CONTRACTID,
       NotifyProxyConstructor,
    }
};

NS_IMPL_NSGETMODULE("NotifyProxyModule", components)
