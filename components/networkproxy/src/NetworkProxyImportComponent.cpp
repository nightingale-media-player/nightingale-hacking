/*
 * BEGIN NIGHTINGALE GPL
 * 
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 * 
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 * 
 * Software distributed under the License is distributed 
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 * express or implied. See the GPL for the specific language 
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this 
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * END NIGHTINGALE GPL
 */

/** 
* \file  NetworkProxyImportComponent.cpp
* \brief Songbird NetworkProxyImport Component Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>
#include "NetworkProxyImport.h"

#define NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR( _Interface )                  \
  static _Interface * m_Simpleton = NULL;                                       \
  static _Interface * _Interface##SimpletonConstructor( void )                  \
  {                                                                             \
    NS_IF_ADDREF( m_Simpleton ? m_Simpleton : ( NS_IF_ADDREF( m_Simpleton = new _Interface() ), m_Simpleton ) ); \
    return m_Simpleton;                                                         \
  }                                                                             \
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR( _Interface, _Interface##SimpletonConstructor );

NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR(CNetworkProxyImport);

static NS_IMETHODIMP CNetworkProxyImportFactoryDestructor()
{
  NS_IF_RELEASE(m_Simpleton);
  return NS_OK;
}


NS_DEFINE_NAMED_CID(SONGBIRD_NetworkProxyImport_CID);

static const mozilla::Module::CIDEntry kNetworkProxyImportCIDs[] = {
  { &kSONGBIRD_NetworkProxyImport_CID, false, NULL, CNetworkProxyImportConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kNetworkProxyImportContracts[] = {
  { SONGBIRD_NetworkProxyImport_CONTRACTID, &kSONGBIRD_NetworkProxyImport_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kNetworkProxyImportCategories[] = {
  { "app-startup", SONGBIRD_NetworkProxyImport_CLASSNAME, SONGBIRD_NetworkProxyImport_CONTRACTID },
  { NULL }
};

static const mozilla::Module kNetworkProxyImportModule = {
  mozilla::Module::kVersion,
  kNetworkProxyImportCIDs,
  kNetworkProxyImportContracts,
  kNetworkProxyImportCategories
};

NSMODULE_DEFN(sbNetworkProxyImportComponent) = &kNetworkProxyImportModule;
