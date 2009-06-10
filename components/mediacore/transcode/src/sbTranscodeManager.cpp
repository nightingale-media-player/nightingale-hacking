/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbTranscodeManager.h"

#include <xpcom/nsAutoLock.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsAutoPtr.h>

#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsIComponentRegistrar.h>
#include <xpcom/nsISupportsPrimitives.h>

#include <nsStringGlue.h>
#include <nsISimpleEnumerator.h>

/* Global transcode manager (singleton) */
sbTranscodeManager *gTranscodeManager = nsnull;

static const char TranscodeContractIDPrefix[] =
    "@songbirdnest.com/Songbird/Mediacore/Transcode/";

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeManager, sbITranscodeManager)

sbTranscodeManager::sbTranscodeManager()
{
  m_pContractListLock = PR_NewLock();
  NS_ASSERTION(m_pContractListLock,
          "Failed to create sbTranscodeManager::m_pContractListLock! "
          "Object *not* threadsafe!");

  // Find the list of handlers for this object.
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS (rv, /* void */);

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  NS_ENSURE_SUCCESS (rv, /* void */);

  PRBool moreAvailable = PR_FALSE;
  while(simpleEnumerator->HasMoreElements(&moreAvailable) == NS_OK && 
        moreAvailable)
  {
    nsCOMPtr<nsISupportsCString> contractString;
    rv = simpleEnumerator->GetNext(getter_AddRefs(contractString));
    if (NS_SUCCEEDED (rv))
    {
      nsCString contractID;
      contractString->GetData(contractID);
      if (contractID.Find(TranscodeContractIDPrefix) != -1)
      {
        m_ContractList.push_back(contractID);
      }
    }
  }
}

//-----------------------------------------------------------------------------
sbTranscodeManager::~sbTranscodeManager()
{
  if(m_pContractListLock)
  {
    PR_DestroyLock(m_pContractListLock);
    m_pContractListLock = nsnull;
  }
}

//-----------------------------------------------------------------------------
sbTranscodeManager *sbTranscodeManager::GetSingleton()
{
  if (gTranscodeManager) {
    NS_ADDREF(gTranscodeManager);
    return gTranscodeManager;
  }

  NS_NEWXPCOM(gTranscodeManager, sbTranscodeManager);
  if (!gTranscodeManager)
    return nsnull;

  NS_ADDREF(gTranscodeManager);
  NS_ADDREF(gTranscodeManager);

  return gTranscodeManager;
}

//-----------------------------------------------------------------------------
void sbTranscodeManager::DestroySingleton()
{
  nsRefPtr<sbTranscodeManager> dummy;
  dummy.swap(gTranscodeManager);
}

NS_IMETHODIMP
sbTranscodeManager::GetTranscoderForMediaItem(sbIMediaItem *aMediaItem,
        sbITranscodeProfile *aProfile, sbITranscodeJob **_retval)
{
  nsresult rv;
  int bestVote = -1;
  nsCOMPtr<sbITranscodeJob> bestHandler;

  for (contractlist_t::iterator contractid = m_ContractList.begin();
       contractid != m_ContractList.end();
       contractid++ )
  {
    nsCOMPtr<sbITranscodeJob> handler = do_CreateInstance(
            (*contractid).get(), &rv);

    if(NS_SUCCEEDED(rv) && handler)
    {
      PRInt32 vote;
      rv = handler->Vote( aMediaItem, aProfile, &vote);
      if (NS_FAILED (rv))
        continue;
      
      /* Negative values are not ever permissible */
      if (vote >= bestVote) {
        bestVote = vote;
        bestHandler = handler;
      }
    }
  }

  if (bestHandler) {
    NS_IF_ADDREF (*_retval = bestHandler);
    return NS_OK;
  }
  else 
    return NS_ERROR_FAILURE;
}

