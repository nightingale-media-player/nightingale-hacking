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
* \file  NetworkProxyImport.cpp
* \brief Songbird NetworkProxyImport Component Implementation.
*/

#include "nspr.h"
#include "nsCOMPtr.h"

#include <nsNetUtil.h>
#include <nsIPrefService.h>
#include <nsIMutableArray.h>

#include "NetworkProxyImport.h"

#ifdef XP_WIN
#include <windows.h>
#include "nsIWindowsRegKey.h"
#endif

//-----------------------------------------------------------------------------
struct NetworkProxyImportSource {
  nsresult (*ImportProxySettingsFunction)(PRBool *);
  const char *sourceName;
};

NetworkProxyImportSource networkProxyImportSources[] = {
#ifdef XP_WIN
  { ImportProxySettings_IE,   "Internet Explorer" },
#endif
  { ImportProxySettings_Auto, "Automatic" },
};

struct NetworkProxyData {
  char*   prefix;
  PRInt32 prefixLength;
  PRBool  proxyConfigured;
  char*   hostPref;
  char*   portPref;
};

//-----------------------------------------------------------------------------
NS_IMPL_ISUPPORTS1(CNetworkProxyImport, sbINetworkProxyImport);

//-----------------------------------------------------------------------------
CNetworkProxyImport::CNetworkProxyImport()
{
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CNetworkProxyImport::~CNetworkProxyImport()
{
} //dtor

//-----------------------------------------------------------------------------
// Import proxy settings from the specified source
NS_IMETHODIMP CNetworkProxyImport::ImportProxySettings(const nsAString &aSource, 
                                                       PRBool *_retval) 
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  for (int i=0;i<NS_ARRAY_LENGTH(networkProxyImportSources);i++) {
    if (aSource.
      Equals(NS_ConvertASCIItoUTF16(networkProxyImportSources[i].sourceName))) {
      return networkProxyImportSources[i].ImportProxySettingsFunction(_retval);
    }
  }
  
  return NS_ERROR_INVALID_ARG;
}

//-----------------------------------------------------------------------------
// Return an array of nsISupportsString containing the proxy import source IDs
NS_IMETHODIMP CNetworkProxyImport::GetImportSources(nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (int i=0;i<NS_ARRAY_LENGTH(networkProxyImportSources);i++) {
    nsCOMPtr<nsISupportsString> source = 
      do_CreateInstance("@mozilla.org/supports-string;1", &rv);
    source->
      SetData(NS_ConvertASCIItoUTF16(networkProxyImportSources[i].sourceName));
    rv = array->AppendElement(source, PR_FALSE);
  }
  
  return CallQueryInterface(array, _retval);
}

//-----------------------------------------------------------------------------
void CNetworkProxyImport::SetUnicharPref(const char* aPref, 
                                         const nsAString& aValue,
                                         nsIPrefBranch* aPrefs)
{
  nsCOMPtr<nsISupportsString> supportsString =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);    
  if (supportsString) {
     supportsString->SetData(aValue); 
     aPrefs->SetComplexValue(aPref, NS_GET_IID(nsISupportsString),
                             supportsString);
  }
}

//-----------------------------------------------------------------------------
void CNetworkProxyImport::ParseOverrideServers(const nsAString& aServers, 
                                               nsIPrefBranch* aBranch)
{
  // Windows (and Opera) formats its proxy override list in the form:
  // server;server;server where server is a server name or ip address, 
  // or "<local>". Mozilla's format is server,server,server, and <local>
  // must be translated to "localhost,127.0.0.1"
  nsAutoString override(aServers);
  PRInt32 left = 0, right = 0;
  for (;;) {
    right = override.FindChar(';', right);
    const nsAString& host = 
      Substring(override, left, 
               (right < 0 ? override.Length() : right) - left);
    if (host.EqualsLiteral("<local>"))
      override.Replace(left, 7, NS_LITERAL_STRING("localhost,127.0.0.1"));
    if (right < 0)
      break;
    left = right + 1;
    override.Replace(right, 1, NS_LITERAL_STRING(","));
  }
  CNetworkProxyImport::SetUnicharPref("network.proxy.no_proxies_on", 
                                      override, 
                                      aBranch); 
}

//-----------------------------------------------------------------------------
void CNetworkProxyImport::SetProxyPref(const nsAString& aHostPort, 
                                       const char* aPref, 
                                       const char* aPortPref, 
                                       nsIPrefBranch* aPrefs) 
{
  nsCOMPtr<nsIURI> uri;
  nsCAutoString host;
  PRInt32 portValue;

  // try parsing it as a URI first
  if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), aHostPort))
      && NS_SUCCEEDED(uri->GetHost(host))
      && !host.IsEmpty()
      && NS_SUCCEEDED(uri->GetPort(&portValue))) {
    CNetworkProxyImport::SetUnicharPref(aPref, 
                                        NS_ConvertUTF8toUTF16(host), 
                                        aPrefs);
    aPrefs->SetIntPref(aPortPref, portValue);
  } else {
    nsAutoString hostPort(aHostPort);  
    PRInt32 portDelimOffset = hostPort.RFindChar(':');
    if (portDelimOffset > 0) {
      CNetworkProxyImport::
        SetUnicharPref(aPref, 
                       Substring(hostPort, 0, portDelimOffset), 
                       aPrefs);
      nsAutoString port(Substring(hostPort, portDelimOffset + 1));
      nsresult stringErr;
      portValue = port.ToInteger(&stringErr);
      if (NS_SUCCEEDED(stringErr))
        aPrefs->SetIntPref(aPortPref, portValue);
    } else {
      CNetworkProxyImport::SetUnicharPref(aPref, hostPort, aPrefs); 
    }
  }
}

//-----------------------------------------------------------------------------
// This "import" source simply sets the proxy settings to "automatic detection",
// it should be the last source in the list as it is the least likely to succeed
nsresult ImportProxySettings_Auto(PRBool *_retval)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));
  if (!prefs) {
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  prefs->SetIntPref("network.proxy.type", 4); 
  
  *_retval = PR_TRUE;
  
  return NS_OK;
}

//-----------------------------------------------------------------------------
// Import settings from IE (code from mozilla IE migration service). 
// This does not support the "automatic detection" setting (if that mode is set
// in IE, the settings will be imported as "no proxy").
#ifdef XP_WIN
nsresult ImportProxySettings_IE(PRBool *_retval)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));
  if (!prefs)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  NS_NAMED_LITERAL_STRING(key,
    "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
  if (regKey && 
      NS_SUCCEEDED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                                key, nsIWindowsRegKey::ACCESS_READ))) {
    nsAutoString buf; 

    PRUint32 proxyType = 0;
    // If there's an autoconfig URL specified in the registry at all, 
    // it is being used. 
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("AutoConfigURL"), buf))) {
      // make this future-proof (MS IE will support IDN eventually and
      // 'URL' will contain more than ASCII characters)
      CNetworkProxyImport::SetUnicharPref("network.proxy.autoconfig_url", 
                                          buf, 
                                          prefs);
      proxyType = 2;
    }

    // ProxyEnable
    PRUint32 enabled;
    if (NS_SUCCEEDED(regKey->
                     ReadIntValue(NS_LITERAL_STRING("ProxyEnable"), &enabled))) {
      if (enabled & 0x1)
        proxyType = 1;
    }
    
    prefs->SetIntPref("network.proxy.type", proxyType); 
    
    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyOverride"), buf)))
      CNetworkProxyImport::ParseOverrideServers(buf, prefs);

    if (NS_SUCCEEDED(regKey->
                     ReadStringValue(NS_LITERAL_STRING("ProxyServer"), buf))) {

      NetworkProxyData data[] = {
        { "ftp=",     4, PR_FALSE, "network.proxy.ftp",
          "network.proxy.ftp_port"    },
        { "gopher=",  7, PR_FALSE, "network.proxy.gopher",
          "network.proxy.gopher_port" },
        { "http=",    5, PR_FALSE, "network.proxy.http",
          "network.proxy.http_port"   },
        { "https=",   6, PR_FALSE, "network.proxy.ssl",
          "network.proxy.ssl_port"    },
        { "socks=",   6, PR_FALSE, "network.proxy.socks",
          "network.proxy.socks_port"  },
      };

      PRInt32 startIndex = 0, count = 0;
      PRBool foundSpecificProxy = PR_FALSE;
      for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(data); ++i) {
        PRInt32 offset = buf.Find(NS_ConvertASCIItoUTF16(data[i].prefix));
        if (offset >= 0) {
          foundSpecificProxy = PR_TRUE;

          data[i].proxyConfigured = PR_TRUE;

          startIndex = offset + data[i].prefixLength;

          PRInt32 terminal = buf.FindChar(';', offset);
          count = terminal > startIndex ? terminal - startIndex : 
                                          buf.Length() - startIndex;

          // hostPort now contains host:port
          CNetworkProxyImport::SetProxyPref(Substring(buf, startIndex, count), 
                                            data[i].hostPref,
                                            data[i].portPref, prefs);
        }
      }

      if (!foundSpecificProxy) {
        // No proxy config for any specific type was found, assume 
        // the ProxyServer value is of the form host:port and that 
        // it applies to all protocols.
        for (PRUint32 i = 0; i < 5; ++i)
          CNetworkProxyImport::SetProxyPref(buf, 
                                            data[i].hostPref, 
                                            data[i].portPref, 
                                            prefs);
        prefs->SetBoolPref("network.proxy.share_proxy_settings", PR_TRUE);
      }
    }
    *_retval = PR_TRUE;
  }
  return NS_OK;
}
#endif

