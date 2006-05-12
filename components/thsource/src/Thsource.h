/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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
 * \file  Thsource.h
 * \brief Songbird Thsource Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIStringBundle.h"
#include "nsIRDFLiteral.h"
#include "sbIThsource.h"

#include "IDatabaseQuery.h"

#include <map>
#include <set>

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_Thsource_CONTRACTID  "@mozilla.org/rdf/datasource;1?name=Servicesource"
#define SONGBIRD_Thsource_CLASSNAME   "Songbird Service Source Component"
// {558123E2-B42C-4b08-9415-A9C9735E5452}
#define SONGBIRD_Thsource_CID { 0x558123e2, 0xb42c, 0x4b08, { 0x94, 0x15, 0xa9, 0xc9, 0x73, 0x5e, 0x54, 0x52 } }

// CLASSES ====================================================================
class CThsource : public sbIThsource
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITHSOURCE
  NS_DECL_NSIRDFDATASOURCE

  // Call the observers from the main thread.
  void Update( void );

  CThsource();
  virtual ~CThsource();

private:
  void Init( void );
  void DeInit( void );

  class observers_t : public std::set< nsCOMPtr<nsIRDFObserver> > {};
  observers_t m_Observers;

  // pseudo-constants
  nsIRDFResource       *kNC_Thsource;
  nsIRDFResource       *kNC_ThsourceFlat;

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
