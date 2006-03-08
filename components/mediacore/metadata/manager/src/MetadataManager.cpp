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
* \file MetadataManager.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataManager.h"

#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsXPCOM.h>

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>

#include <necko/nsNetUtil.h>

#include <string/nsStringAPI.h>
#include <string/nsString.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataManager, sbIMetadataManager)

//-----------------------------------------------------------------------------
sbMetadataManager::sbMetadataManager()
{
}

//-----------------------------------------------------------------------------
sbMetadataManager::~sbMetadataManager()
{
}

//-----------------------------------------------------------------------------
/* sbIMetadataHandler GetHandlerForMediaURL (in wstring strURL); */
NS_IMETHODIMP sbMetadataManager::GetHandlerForMediaURL(const PRUnichar *strURL, sbIMetadataHandler **_retval)
{
  *_retval = nsnull;
  nsresult nRet = NS_ERROR_UNEXPECTED;
  if(!_retval) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<sbIMetadataHandler> pHandler;
  nsCOMPtr<nsIChannel> pChannel;
  nsCOMPtr<nsIIOService> pIOService = do_GetIOService(&nRet);
  if(NS_FAILED(nRet)) return nRet;

  NS_ConvertUTF16toUTF8 cstrURL(strURL);
  //nRet = pURI->SetSpec(cstrURL);
  //if(NS_FAILED(nRet)) return nRet;

  nsCOMPtr<nsIURI> pURI;
  pIOService->NewURI(cstrURL, nsnull, nsnull, getter_AddRefs(pURI));
  if(!pURI) return nRet;

  nsCString cstrScheme;
  nRet = pURI->GetScheme(cstrScheme);
  if(NS_FAILED(nRet)) return nRet;
  if(cstrScheme.Length() <= 1)
  {
    nsCString cstrFixedURL = NS_LITERAL_CSTRING("file://");
    cstrFixedURL += cstrURL;

    pIOService->NewURI(cstrURL, nsnull, nsnull, getter_AddRefs(pURI));
  }

  //Local File
  if(cstrScheme.Equals(NS_LITERAL_CSTRING("file")))
  {
    PRInt32 nHandlerRet = 0;
    nRet = pIOService->NewChannelFromURI(pURI, getter_AddRefs(pChannel));
    if(NS_FAILED(nRet)) return nRet;

    pHandler = do_GetService("@songbird.org/Songbird/MetadataHandler/ID3;1");
    if(!pHandler)
    {
      nRet = NS_ERROR_UNEXPECTED;
      return nRet;
    }

    nRet = pHandler->SetChannel(pChannel, &nHandlerRet);
    if(nRet != 0) return nRet;

    *_retval = pHandler.get();
    (*_retval)->AddRef();

    nRet = NS_OK;
  }
  //Remote File
  else
  {

  }

  return nRet;
}
