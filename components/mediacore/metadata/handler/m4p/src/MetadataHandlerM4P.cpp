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
* \file MetadataHandlerM4P.cpp
* \brief 
*/

#include "MetadataHandlerM4P.h"

#include <sbIMetadataValues.h>
#include <sbQuickTimeUtils.h>

#include <nsComponentManagerUtils.h>
#include <nsIChannel.h>

#include <prlog.h>

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbMetadataHandlerM4P");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif

static const char* kMetadataKeys[] = {
  SB_METADATA_KEY_TITLE,      SB_METADATA_KEY_ARTIST,
  SB_METADATA_KEY_ALBUM,      SB_METADATA_KEY_GENRE,
  SB_METADATA_KEY_COMMENT,    SB_METADATA_KEY_COMPOSER,
  SB_METADATA_KEY_LENGTH,     SB_METADATA_KEY_SRCURL,
  SB_METADATA_KEY_TRACKNO
};

#define kMetadataArrayCount countof(kMetadataKeys)

NS_IMPL_ISUPPORTS1(sbMetadataHandlerM4P, sbIMetadataHandler)

NS_IMETHODIMP
sbMetadataHandlerM4P::Initialize()
{
  // Get a new values object.
  nsresult rv;
  mValues =
    do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::SupportedMIMETypes(PRUint32* aMIMECount,
                                         PRUnichar*** aMIMETypes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::SupportedFileExtensions(PRUint32* eExtCount,
                                              PRUnichar*** aExts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::GetChannel(nsIChannel** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_IF_ADDREF(*_retval = mChannel);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::SetChannel(nsIChannel* aURLChannel)
{
  mChannel = aURLChannel;
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::OnChannelData(nsISupports* aChannel)
{
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::GetCompleted(PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mCompleted;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::Close()
{
  mValues = nsnull;
  mChannel = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::Vote(const nsAString& aURL,
                           PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCAutoString str = NS_LossyConvertUTF16toASCII(aURL);
  ToLowerCase(str);
  
  if (StringTail(str, 4).EqualsLiteral(".m4p"))
    *_retval = 1;
  else
    *_retval = -1;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::Read(PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;

  NS_ENSURE_STATE(mChannel);

  // Make this a sync read
  mCompleted = PR_TRUE;

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = mChannel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri, &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("M4A metadata is available for local files only at this time");
    return NS_OK;
  }
  
  // Open the file with QuickTime
  Movie movie;
  rv = SB_CreateMovieFromURI(uri, &movie);
  NS_ENSURE_SUCCESS(rv, rv);

  // And get all the metadata we can
  PRUint32 count = kMetadataArrayCount;
  for (PRUint32 index = 0; index < count; index++) {
    nsAutoString key;
    key.AssignLiteral(kMetadataKeys[index]);
    
    nsAutoString value;
    PRUint32 valueType;
    rv = SB_GetQuickTimeMetadataForKey(movie, key, &valueType, value);
    
    if (NS_FAILED(rv) || value.IsEmpty())
      continue;
    
    rv = mValues->SetValue(key, value, valueType);
    if (NS_SUCCEEDED(rv))
      *_retval += 1;
    else
      NS_WARNING("SetValue failed!");
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::Write(PRInt32* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::GetValues(sbIMetadataValues** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_STATE(mValues);
  NS_ADDREF(*_retval = mValues);
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataHandlerM4P::SetValues(sbIMetadataValues* aValues)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
