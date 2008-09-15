/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbMediacoreSequencer.h"

#include <nsIURI.h>
#include <nsIURL.h>

#include <nsAutoLock.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbTArrayStringEnumerator.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreSequencer, 
                              sbIMediacoreSequencer)

sbMediacoreSequencer::sbMediacoreSequencer()
: mMode(sbIMediacoreSequencer::MODE_FORWARD)
{
}

sbMediacoreSequencer::~sbMediacoreSequencer()
{
  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult
sbMediacoreSequencer::Init() 
{
  mMonitor = nsAutoMonitor::NewMonitor("sbMediacoreSequencer::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult 
sbMediacoreSequencer::RecalculateSequence()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbMediacoreSequencer::GetMode(PRUint32 *aMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMode);

  nsAutoMonitor mon(mMonitor);
  *aMode = mMode;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::SetMode(PRUint32 aMode)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  PRBool validMode = PR_FALSE;
  switch(aMode) {
    case sbIMediacoreSequencer::MODE_FORWARD:
    case sbIMediacoreSequencer::MODE_REVERSE:
    case sbIMediacoreSequencer::MODE_REPEAT_ALL:
    case sbIMediacoreSequencer::MODE_REPEAT_ONE:
    case sbIMediacoreSequencer::MODE_SHUFFLE:
    case sbIMediacoreSequencer::MODE_CUSTOM:
      validMode = PR_TRUE;
    break;
  }
  NS_ENSURE_TRUE(validMode, NS_ERROR_INVALID_ARG);

  nsAutoMonitor mon(mMonitor);

  if(mMode != aMode) {
    mMode = aMode;
    
    nsresult rv = RecalculateSequence();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::GetView(sbIMediaListView * *aView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);
  NS_IF_ADDREF(*aView = mView);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::SetView(sbIMediaListView * aView)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aView);

  nsAutoMonitor mon(mMonitor);
  if(mView != aView) {
    mView = aView;

    nsresult rv = RecalculateSequence();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::GetCustomGenerator(
                        sbIMediacoreSequenceGenerator * *aCustomGenerator)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCustomGenerator);

  nsAutoMonitor mon(mMonitor);
  NS_IF_ADDREF(*aCustomGenerator = mCustomGenerator);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreSequencer::SetCustomGenerator(
                        sbIMediacoreSequenceGenerator * aCustomGenerator)
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aCustomGenerator);

  nsAutoMonitor mon(mMonitor);
  if(mCustomGenerator != aCustomGenerator) {
    mCustomGenerator = aCustomGenerator;
    
    // Custom generator changed and mode is custom, recalculate sequence
    if(mMode == sbIMediacoreSequencer::MODE_CUSTOM) {
      nsresult rv = RecalculateSequence();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}
