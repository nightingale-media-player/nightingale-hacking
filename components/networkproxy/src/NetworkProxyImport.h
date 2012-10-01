/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/** 
 * \file  NetworkProxyImport.h
 * \brief Songbird NetworkProxyImport Component Definition.
 */

#ifndef __NETWORKPROXYIMPORT_H__
#define __NETWORKPROXYIMPORT_H__

#include <nsISupportsPrimitives.h>
#include "sbINetworkProxyImport.h"

class nsIPrefBranch;

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_NetworkProxyImport_CONTRACTID                 \
  "@songbirdnest.com/Songbird/NetworkProxyImport;1"
#define SONGBIRD_NetworkProxyImport_CLASSNAME                  \
  "Songbird NetworkProxy Import Component"
#define SONGBIRD_NetworkProxyImport_CID                        \
{ /* caece57b-6634-491e-a858-9c99ab873dc4 */              \
  0xcaece57b, 0x6634, 0x491e,                             \
  { 0xa8, 0x58, 0x9c, 0x99, 0xab, 0x87, 0x3d, 0xc4 }      \
}
// CLASSES ====================================================================
class CNetworkProxyImport : public sbINetworkProxyImport
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBINETWORKPROXYIMPORT

  CNetworkProxyImport();
  virtual ~CNetworkProxyImport();

  static void SetUnicharPref(const char* aPref, 
                             const nsAString& aValue,
                             nsIPrefBranch* aPrefs);
  static void ParseOverrideServers(const nsAString& aServers, 
                                   nsIPrefBranch* aBranch);
  static void SetProxyPref(const nsAString& aHostPort, 
                           const char* aPref, 
                           const char* aPortPref, 
                           nsIPrefBranch* aPrefs);
};

nsresult ImportProxySettings_Auto(bool *_retval);

#ifdef XP_WIN
nsresult ImportProxySettings_IE(bool *_retval);
#endif

#endif // __NETWORKPROXYIMPORT_H__


