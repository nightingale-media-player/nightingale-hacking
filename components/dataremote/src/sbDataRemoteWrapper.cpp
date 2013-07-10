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
#include <nsMemory.h> 
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <prlog.h>

#include "sbDataRemoteWrapper.h"


/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDataRemoteWrapper:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDataRemoteWrapperLog = nsnull;
#define LOG(args) PR_LOG(gDataRemoteWrapperLog, PR_LOG_DEBUG, args)
#else
#define LOG(args) /* nothing */
#endif


// CID for the original dataremote implementation, to which
// this object will delegate.
// {e0990420-e9c0-11dd-ba2f-0800200c9a66}
#define SB_DATAREMOTE_CID \
{ 0xe0990420, 0xe9c0, 0x11dd, \
  { 0xba, 0x2f, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }
static NS_DEFINE_CID(kDataRemoteCID, SB_DATAREMOTE_CID); 


NS_IMPL_CLASSINFO(sbDataRemoteWrapper, NULL, nsIClassInfo::THREADSAFE, SB_DATAREMOTEWRAPPER_CID);

NS_IMPL_ISUPPORTS3(sbDataRemoteWrapper, sbIDataRemote, nsIObserver, nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbDataRemoteWrapper,
                             sbIDataRemote,
                             nsIClassInfo)

NS_IMPL_THREADSAFE_CI(sbDataRemoteWrapper)


sbDataRemoteWrapper::sbDataRemoteWrapper()
: mInnerDataRemote(nsnull), 
  mObserver(nsnull)
{
#ifdef PR_LOGGING
  if (!gDataRemoteWrapperLog)
    gDataRemoteWrapperLog = PR_NewLogModule("sbDataRemoteWrapper");
#endif

  LOG(("DataRemoteWrapper[0x%x] - Created", this));
}

sbDataRemoteWrapper::~sbDataRemoteWrapper()
{
  nsresult rv;
  if (mObserver) {
    // Destructor won't be called unless unbound, but
    // do this anyway.
    rv = Unbind();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to unbind");
  }
  mInnerDataRemote = nsnull;

  LOG(("DataRemoteWrapper[0x%x] - Destroyed", this));
}

nsresult sbDataRemoteWrapper::InitWrapper()
{
  LOG(("DataRemoteWrapper[0x%x] - InitWrapper", this));

  nsresult rv;
  // Forward everything to the original dataremote implementation
  mInnerDataRemote =
     do_CreateInstance(kDataRemoteCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

//
// sbIDataRemote
//
// Redirect getter/setters to methods rather than attributes
// in order to work around BMO 304048.
//

/* attribute AString stringValue; */
NS_IMETHODIMP sbDataRemoteWrapper::GetStringValue(nsAString & aStringValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->GetAsString(aStringValue);
}
NS_IMETHODIMP sbDataRemoteWrapper::SetStringValue(const nsAString & aStringValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->SetAsString(aStringValue);
}

/* attribute boolean boolValue; */
NS_IMETHODIMP sbDataRemoteWrapper::GetBoolValue(PRBool *aBoolValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->GetAsBool(aBoolValue);
}
NS_IMETHODIMP sbDataRemoteWrapper::SetBoolValue(PRBool aBoolValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->SetAsBool(aBoolValue);
}

/* attribute long long intValue; */
NS_IMETHODIMP sbDataRemoteWrapper::GetIntValue(PRInt64 *aIntValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->GetAsInt(aIntValue);
}
NS_IMETHODIMP sbDataRemoteWrapper::SetIntValue(PRInt64 aIntValue)
{
  NS_ENSURE_STATE(mInnerDataRemote);
  return mInnerDataRemote->SetAsInt(aIntValue);
}

/* void bindObserver (in nsIObserver aObserver, [optional] in boolean aSuppressFirst); */
NS_IMETHODIMP sbDataRemoteWrapper::BindObserver(nsIObserver *aObserver, PRBool aSuppressFirst)
{
  LOG(("DataRemoteWrapper[0x%x] - BindObserver", this));

  NS_ENSURE_STATE(mInnerDataRemote);
  NS_ENSURE_ARG_POINTER(aObserver);
  mObserver = aObserver;  
  return mInnerDataRemote->BindObserver(this, aSuppressFirst);
}

/* void bindRemoteObserver (in sbIRemoteObserver, [optional] in boolean aSuppressFirst); */
NS_IMETHODIMP sbDataRemoteWrapper::BindRemoteObserver(sbIRemoteObserver *aObserver, PRBool aSuppressFirst)
{
  LOG(("DataRemoteWrapper[0x%x] - BindRemoteObserver", this));

  NS_ENSURE_STATE(mInnerDataRemote);
  NS_ENSURE_ARG_POINTER(aObserver);
  
  // We don't pretend like we are this observer. It's not required by the
  // security model.
  return mInnerDataRemote->BindRemoteObserver(aObserver, aSuppressFirst);
}

/* void unbind (); */
NS_IMETHODIMP sbDataRemoteWrapper::Unbind()
{
  LOG(("DataRemoteWrapper[0x%x] - Unbind", this));

  NS_ENSURE_STATE(mInnerDataRemote);
  nsresult rv = mInnerDataRemote->Unbind();
  mObserver = nsnull;
  return rv;
}

//
// nsIObserver
//
// Intercept Observer callbacks in order to update the subject param
// to point to this object.
//

NS_IMETHODIMP sbDataRemoteWrapper::Observe( nsISupports *aSubject,
                                            const char *aTopic,
                                            const PRUnichar *aData )
{
  LOG(("DataRemoteWrapper[0x%x] - Observe: %s", this, aTopic));

  NS_ENSURE_STATE(mObserver);
  // Make sure our observer thinks the message is coming
  // from this instance
  return mObserver->Observe(NS_ISUPPORTS_CAST(sbIDataRemote*, this), aTopic, aData);
}
