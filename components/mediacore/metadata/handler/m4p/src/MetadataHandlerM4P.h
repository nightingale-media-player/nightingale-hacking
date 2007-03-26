/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file MetadataHandlerM4P.h
* \brief 
*/

#ifndef __METADATA_HANDLER_M4P_H__
#define __METADATA_HANDLER_M4P_H__

#include <nsCOMPtr.h>

#include <sbIMetadataHandler.h>

#define SONGBIRD_METADATAHANDLERM4P_CONTRACTID             \
  "@songbirdnest.com/Songbird/MetadataHandler/M4P;1"

#define SONGBIRD_METADATAHANDLERM4P_CLASSNAME              \
  "Songbird M4A Metadata Handler Component"

#define SONGBIRD_METADATAHANDLERM4P_CID                    \
{ /* c576a09f-3dca-4e1c-b1d3-6ac948ede4d1 */               \
  0xc576a09f,                                              \
  0x3dca,                                                  \
  0x4e1c,                                                  \
  { 0xb1, 0xd3, 0x6a, 0xc9, 0x48, 0xed, 0xe4, 0xd1 }       \
}

class nsIChannel;
class sbIMetadataValues;

class sbMetadataHandlerM4P : public sbIMetadataHandler
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER

  sbMetadataHandlerM4P() : mCompleted(PR_FALSE) { }

  NS_IMETHOD Initialize();

protected:
  nsCOMPtr<sbIMetadataValues>  mValues;
  nsCOMPtr<nsIChannel>         mChannel;
  PRBool                       mCompleted;
};

#endif // __METADATA_HANDLER_M4P_H__
