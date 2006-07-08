/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */

/** 
 * \file  Servicesource.h
 * \brief Songbird Servicesource Component Definition.
 */

#ifndef __SERVICE_SOURCE_H__
#define __SERVICE_SOURCE_H__

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIStringBundle.h"
#include "nsIRDFLiteral.h"
#include "sbIServicesource.h"

#include "sbIDatabaseQuery.h"

#include <map>
#include <set>

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_Servicesource_CONTRACTID                 \
  "@mozilla.org/rdf/datasource;1?name=Servicesource"
#define SONGBIRD_Servicesource_CLASSNAME                  \
  "Songbird Service Source Component"
#define SONGBIRD_Servicesource_CID                        \
{ /* 5c0ebb3e-ae05-4b26-8cdf-483573bca888 */              \
  0x5c0ebb3e,                                             \
  0xae05,                                                 \
  0x4b26,                                                 \
  {0x8c, 0xdf, 0x48, 0x35, 0x73, 0xbc, 0xa8, 0x88}        \
}
// CLASSES ====================================================================
class CServicesource : public sbIServicesource
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISERVICESOURCE
  NS_DECL_NSIRDFDATASOURCE

  // Call the observers from the main thread.
  void Update( void );

  CServicesource();
  virtual ~CServicesource();

private:
  void Init( void );
  void DeInit( void );

  class observers_t : public std::set< nsCOMPtr<nsIRDFObserver> > {};
  observers_t m_Observers;

  // pseudo-constants
  nsIRDFResource       *kNC_Servicesource;
  nsIRDFResource       *kNC_ServicesourceFlat;

  nsIRDFResource       *kNC_Label;
  nsIRDFResource       *kNC_Icon;
  nsIRDFResource       *kNC_URL;
  nsIRDFResource       *kNC_Properties;
  nsIRDFResource       *kNC_Open; // THIS DOESN'T WORK, STOP IMPLEMENTING IT.

  // Cheezy hardcoded hierarchy
#define NUM_PARENTS 9
#define MAX_CHILDREN 20
  nsIRDFResource       *kNC_Parent[ NUM_PARENTS ];
  nsIRDFResource       *kNC_Child[ NUM_PARENTS ][ MAX_CHILDREN ];

  nsIRDFResource       *kNC_child;
  nsIRDFResource       *kNC_pulse;

  nsIRDFResource       *kRDF_InstanceOf;
  nsIRDFResource       *kRDF_type;
  nsIRDFResource       *kRDF_nextVal;
  nsIRDFResource       *kRDF_Seq;

  nsIRDFLiteral        *kLiteralTrue;
  nsIRDFLiteral        *kLiteralFalse;

  nsCOMPtr< sbIDatabaseQuery >  m_PlaylistsQuery;
  nsCOMPtr< nsIStringBundle >   m_StringBundle;

  class resmap_t : public std::map< nsIRDFResource *, int > {};
  resmap_t m_PlaylistMap;
};

#endif // __SERVICE_SOURCE_H__

