/*
 * Written by Logan F. Smyth © 2009
 * http://logansmyth.com
 * me@logansmyth.com
 * 
 * Feel free to use/modify this code, but if you do
 * then please at least tell me!
 *
 */


#include "nsIGenericFactory.h"
#include "sbDbusConnection.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(sbDbusConnection)


static nsModuleComponentInfo components[] =
{
	{
		"Dbus Protocol plugin", 
		{ 0x939381dc, 0x1747, 0x4d3e, { 0xb2, 0xf7, 0x50, 0x53, 0xe5, 0xfb, 0x7d, 0xa4 } },
		"@logansmyth.com/Songbird/DbusConnection;1",
		sbDbusConnectionConstructor,
	}
};

NS_IMPL_NSGETMODULE("sbDbusConnectionModule", components)

