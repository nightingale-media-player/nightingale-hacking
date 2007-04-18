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
* \file MetadataJob.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsAutoLock.h>

#include <unicharutil/nsUnicharUtils.h>
#include <nsComponentManagerUtils.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsIObserverService.h>
#include <xpcom/nsCRTGlue.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>
#include <nsISupportsPrimitives.h>
#include <nsThreadUtils.h>
#include <nsMemory.h>

#include "MetadataJob.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

// DEFINES ====================================================================

// GLOBALS ====================================================================

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJob, sbIMetadataJob);

sbMetadataJob::sbMetadataJob()
{
  nsresult rv;

  mQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(mQuery, "Unable to create sbMetadataJob::mQuery");
}

sbMetadataJob::~sbMetadataJob()
{
  Cancel();
}

/* readonly attribute AString tableName; */
NS_IMETHODIMP sbMetadataJob::GetTableName(nsAString & aTableName)
{
    return NS_OK;
}

/* readonly attribute unsigned long itemsTotal; */
NS_IMETHODIMP sbMetadataJob::GetItemsTotal(PRUint32 *aItemsTotal)
{
    return NS_OK;
}

/* readonly attribute unsigned long itemsCompleted; */
NS_IMETHODIMP sbMetadataJob::GetItemsCompleted(PRUint32 *aItemsCompleted)
{
    return NS_OK;
}

/* void init (in AString aTableName, in nsIArray aMediaItemsArray); */
NS_IMETHODIMP sbMetadataJob::Init(const nsAString & aTableName, nsIArray *aMediaItemsArray, PRUint32 aSleepMS)
{
    mTableName = aTableName;

    // TODO:
    //  - Initialize the task table (library, item guid, url, is_scanned)
    //  - Append any items in the array to the task table
    //  - Launch a thread to handle "file://" and a timer for everything else

    return NS_OK;
}

/* void cancel (); */
NS_IMETHODIMP sbMetadataJob::Cancel()
{
    return NS_OK;
}

