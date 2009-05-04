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

#include "sbiTunesImporterBatchCreateListener.h"

#include <nsIArray.h>

NS_IMPL_ISUPPORTS1(sbiTunesImporterBatchCreateListener, sbIBatchCreateMediaItemsListener)

sbiTunesImporterBatchCreateListener * sbiTunesImporterBatchCreateListener::New()
{
  return new sbiTunesImporterBatchCreateListener;
}

sbiTunesImporterBatchCreateListener::sbiTunesImporterBatchCreateListener() :
  mCreateResult(NS_ERROR_FAILURE),
  mCompleted(PR_FALSE)
{
  /* member initializers and constructor code */
}

sbiTunesImporterBatchCreateListener::~sbiTunesImporterBatchCreateListener()
{
  /* destructor code */
}

/* void onProgress (in unsigned long aIndex); */
NS_IMETHODIMP sbiTunesImporterBatchCreateListener::OnProgress(PRUint32 aIndex)
{
  return NS_OK;
}

/* void onComplete (in nsIArray aMediaItems, in unsigned long aResult); */
NS_IMETHODIMP sbiTunesImporterBatchCreateListener::OnComplete(nsIArray *aMediaItems, PRUint32 aResult)
{
  mCompleted = PR_TRUE;
  mCreatedMediaItems = aMediaItems;
  mCreateResult = aResult;
  return NS_OK;
}
