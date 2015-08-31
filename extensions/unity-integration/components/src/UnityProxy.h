#ifndef _UNITY_PROXY_H_
#define _UNITY_PROXY_H_

#include "IUnityProxy.h"

#include <gtk/gtk.h>
#include <unity.h>

#define UNITY_PROXY_CONTRACTID "@LookingMan.org/Songbird-Nightingale/UnityProxy;1"
#define UNITY_PROXY_CLASSNAME  "Proxy for libunity"

// 76af963f-bbbf-4558-a938-1d079d4cda56
#define UNITY_PROXY_CID        { 0x76af963f, 0xbbbf, 0x4558, { 0xa9, 0x38, 0x1d, 0x07, 0x9d, 0x4c, 0xda, 0x56 } }

class UnityProxy : public IUnityProxy
{
	                 
public:

  NS_DECL_ISUPPORTS

  NS_DECL_IUNITYPROXY

  bool isUnityRunning();

  UnityProxy();
  virtual ~UnityProxy();
};

#endif //_UNITY_PROXY_H_
