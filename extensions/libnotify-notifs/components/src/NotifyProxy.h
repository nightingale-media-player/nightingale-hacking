#ifndef _NOTIFY_PROXY
#define _NOTIFY_PROXY

#include "INotifyProxy.h"
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#define NOTIFY_PROXY_CONTRACTID "@getnightingale.com/Songbird-Nightingale/NotifsProxy;1"
#define NOTIFY_PROXY_CLASSNAME  "NotifyProxy"

/* 649f3733-12fd-485b-bd55-5106b6dafb80 */
#define NOTIFY_PROXY_CID \
{ 0x649f3733, 0x12fd, 0x485b, { 0xbd, 0x55, 0x51, 0x06, 0xb6, 0xda, 0xfb, 0x80 } }

class NotifyProxy : public INotifyProxy
{
    public:
        NS_DECL_ISUPPORTS

        NS_DECL_INOTIFYPROXY

        NotifyProxy();
        virtual ~NotifyProxy();
};

#endif
