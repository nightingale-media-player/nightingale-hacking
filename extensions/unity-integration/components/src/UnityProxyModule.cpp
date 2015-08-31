#include "nsIGenericFactory.h"
#include "UnityProxy.h"

// Create the constructor (factory) of our component. this macro
// creates a static function named <ArgumentName>Constructor, in our case
// UnityProxyConstructor, which is then used in the structure of the components.
NS_GENERIC_FACTORY_CONSTRUCTOR(UnityProxy)

// Structure containing module info
static nsModuleComponentInfo components[] =
{
    {
       UNITY_PROXY_CLASSNAME,
       UNITY_PROXY_CID,
       UNITY_PROXY_CONTRACTID,
       UnityProxyConstructor,
    }
};

// Create an entry point to the required nsIModule interface
NS_IMPL_NSGETMODULE("UnityProxyModule", components)
