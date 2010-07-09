/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/** 
* \file  sbMediacoreWrapper.cpp
* \brief Songbird Mediacore Wrapper Implementation.
*/
#include "sbMediacoreWrapper.h"

#include <nsIClassInfoImpl.h>
#include <nsIDOMDocument.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>

#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsXPCOMCID.h>
#include <prlog.h>

#include <IWindowCloak.h>
#include <sbIPropertyArray.h>

#include <sbProxiedComponentManager.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

#include "sbBaseMediacoreEventTarget.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreWrapper:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreWrapper = nsnull;
#define TRACE(args) PR_LOG(gMediacoreWrapper, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreWrapper, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ADDREF(sbMediacoreWrapper)
NS_IMPL_THREADSAFE_RELEASE(sbMediacoreWrapper)

NS_IMPL_QUERY_INTERFACE8_CI(sbMediacoreWrapper,
                            sbIMediacore,
                            sbIMediacorePlaybackControl,
                            sbIMediacoreVolumeControl,
                            sbIMediacoreVotingParticipant,
                            sbIMediacoreEventTarget,
                            sbIMediacoreWrapper,
                            nsIDOMEventListener,
                            nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER7(sbMediacoreWrapper,
                             sbIMediacore,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIMediacoreEventTarget,
                             sbIMediacoreWrapper,
                             nsIClassInfo)

NS_DECL_CLASSINFO(sbMediacoreWrapper)
NS_IMPL_THREADSAFE_CI(sbMediacoreWrapper)

sbMediacoreWrapper::sbMediacoreWrapper()
: mBaseEventTarget(new sbBaseMediacoreEventTarget(this))
, mWindowIsReady(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gMediacoreWrapper)
    gMediacoreWrapper = PR_NewLogModule("sbMediacoreWrapper");
#endif
  TRACE(("sbMediacoreWrapper[0x%x] - Created", this));
}

sbMediacoreWrapper::~sbMediacoreWrapper()
{
  TRACE(("sbMediacoreWrapper[0x%x] - Destroyed", this));
}

nsresult
sbMediacoreWrapper::Init()
{
  TRACE(("sbMediacoreWrapper[0x%x] - Init", this));

  nsresult rv = sbBaseMediacore::InitBaseMediacore();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacorePlaybackControl::InitBaseMediacorePlaybackControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacoreVolumeControl::InitBaseMediacoreVolumeControl();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//
// sbBaseMediacore overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacore()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetCapabilities()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnShutdown()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIWindowCloak> windowCloak = 
    do_ProxiedGetService("@songbirdnest.com/Songbird/WindowCloak;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = windowCloak->Uncloak(mPluginHostWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveSelfDOMListener();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrompter->Cancel();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//
// sbBaseMediacorePlaybackControl overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacorePlaybackControl()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetUri(nsIURI *aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCString uriSpec;
  nsresult rv = aURI->GetSpec(uriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = 
    SendDOMEvent(NS_LITERAL_STRING("seturi"), uriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreWrapper::OnGetDuration(PRUint64 *aDuration) 
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent;

  nsresult rv = SendDOMEvent(NS_LITERAL_STRING("getduration"), 
                             EmptyString(), 
                             getter_AddRefs(dataEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString retvalStr;
  rv = GetRetvalFromEvent(dataEvent, retvalStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aDuration = 0;

  if(!retvalStr.IsEmpty()) {
    PRUint64 duration = nsString_ToUint64(retvalStr, &rv);
    if(NS_SUCCEEDED(rv)) {
      *aDuration = duration;
    }
  }

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetPosition(PRUint64 *aPosition)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent;

  nsresult rv = SendDOMEvent(NS_LITERAL_STRING("getposition"), 
                             EmptyString(),
                             getter_AddRefs(dataEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString retvalStr;
  rv = GetRetvalFromEvent(dataEvent, retvalStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aPosition = 0;

  if(!retvalStr.IsEmpty()) {
    PRUint64 position = nsString_ToUint64(retvalStr, &rv);
    if(NS_SUCCEEDED(rv)) {
      *aPosition = position;
    }
  }

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetPosition(PRUint64 aPosition)
{
  nsresult rv = 
    SendDOMEvent(NS_LITERAL_STRING("setposition"), sbAutoString(aPosition));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetIsPlayingAudio(PRBool *aIsPlayingAudio)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent;

  nsresult rv = SendDOMEvent(NS_LITERAL_STRING("getisplayingaudio"), 
                             EmptyString(),
                             getter_AddRefs(dataEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString retvalStr;
  rv = GetRetvalFromEvent(dataEvent, retvalStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsPlayingAudio = PR_FALSE;

  if(retvalStr.EqualsLiteral("true") || retvalStr.EqualsLiteral("1")) {
    *aIsPlayingAudio = PR_TRUE;
  }

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnGetIsPlayingVideo(PRBool *aIsPlayingVideo)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent;

  nsresult rv = SendDOMEvent(NS_LITERAL_STRING("getisplayingvideo"), 
                             EmptyString(),
                             getter_AddRefs(dataEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString retvalStr;
  rv = GetRetvalFromEvent(dataEvent, retvalStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsPlayingVideo = PR_FALSE;

  if(retvalStr.EqualsLiteral("true") || retvalStr.EqualsLiteral("1")) {
    *aIsPlayingVideo = PR_TRUE;
  }

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnPlay()
{
  nsresult rv = 
    SendDOMEvent(NS_LITERAL_STRING("play"), EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_START);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnPause()
{
  nsresult rv = 
    SendDOMEvent(NS_LITERAL_STRING("pause"), EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_PAUSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnStop()
{
  nsresult rv = 
    SendDOMEvent(NS_LITERAL_STRING("stop"), EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_STOP);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreWrapper::OnSeek(PRUint64 aPosition, PRUint32 aFlags)
{
  return OnSetPosition(aPosition);
}

//
// sbBaseMediacoreVolumeControl overrides
//

/*virtual*/ nsresult 
sbMediacoreWrapper::OnInitBaseMediacoreVolumeControl()
{
  return NS_OK;
}

/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetMute(PRBool aMute)
{
  nsresult rv = 
    SendDOMEvent(NS_LITERAL_STRING("setmute"), sbAutoString(aMute));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
  
/*virtual*/ nsresult 
sbMediacoreWrapper::OnSetVolume(PRFloat64 aVolume)
{
  nsCString volStr;
  SB_ConvertFloatVolToJSStringValue(aVolume, volStr);
  
  nsresult rv = SendDOMEvent(NS_LITERAL_STRING("setvolume"), volStr);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

//
// sbIMediacoreVotingParticipant
//

NS_IMETHODIMP
sbMediacoreWrapper::VoteWithURI(nsIURI *aURI, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCString uriSpec;
  nsresult rv = aURI->GetSpec(uriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent;
  rv = SendDOMEvent(NS_LITERAL_STRING("votewithuri"), 
                    uriSpec, 
                    getter_AddRefs(dataEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString retvalStr;
  rv = GetRetvalFromEvent(dataEvent, retvalStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = 0;

  if(!retvalStr.IsEmpty()) {
    PRUint32 voteResult = static_cast<PRUint32>(retvalStr.ToInteger(&rv, 10));
    if(NS_SUCCEEDED(rv)) {
      *_retval = voteResult;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreWrapper::VoteWithChannel(nsIChannel *aChannel, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//
// sbIMediacoreWrapper
//

NS_IMETHODIMP
sbMediacoreWrapper::Initialize(const nsAString &aInstanceName, 
                               sbIMediacoreCapabilities *aCapabilities,
                               const nsACString &aChromePageURL)
{
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv = SetInstanceName(aInstanceName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetCapabilities(aCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPrompter> prompter = 
    do_CreateInstance(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  prompter.swap(mPrompter);

  rv = mPrompter->SetParentWindowType(NS_LITERAL_STRING("Songbird:Main"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> domWindow;
  rv = mPrompter->OpenWindow(nsnull, 
                             NS_ConvertUTF8toUTF16(aChromePageURL), 
                             aInstanceName,
                             NS_LITERAL_STRING("chrome,centerscreen"),
                             nsnull,
                             getter_AddRefs(domWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  domWindow.swap(mPluginHostWindow);

  nsCOMPtr<nsIDOMEventTarget> domEventTarget = 
    do_QueryInterface(mPluginHostWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  domEventTarget.swap(mDOMEventTarget);

  rv = AddSelfDOMListener();
  NS_ENSURE_SUCCESS(rv, rv);

  // Wait for window to be ready.
  nsCOMPtr<nsIThread> target;
  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool processed = PR_FALSE;
  while(!mWindowIsReady) {
    rv = target->ProcessNextEvent(PR_FALSE, &processed);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIDOMDocument> document;
  rv = mPluginHostWindow->GetDocument(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentEvent> documentEvent = 
    do_QueryInterface(document, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  documentEvent.swap(mDocumentEvent);

  return NS_OK;
}

//
// nsIDOMEventListener
//

NS_IMETHODIMP
sbMediacoreWrapper::HandleEvent(nsIDOMEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsString eventType;
  nsresult rv = aEvent->GetType(eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  if(eventType.EqualsLiteral("resize")) {
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPluginHostWindow, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    mWindowIsReady = PR_TRUE;
    
    nsCOMPtr<sbIWindowCloak> windowCloak = 
      do_ProxiedGetService("@songbirdnest.com/Songbird/WindowCloak;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // For debugging, you can comment this out to see the window which is
    // hosting the plugin. 
    //rv = windowCloak->Cloak(mPluginHostWindow);
    //NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(eventType.EqualsLiteral("mediacore-buffering-begin")) {
    rv = DispatchMediacoreEvent(sbIMediacoreEvent::BUFFERING, 
                                sbNewVariant(0.0).get());
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(eventType.EqualsLiteral("mediacore-buffering-end")) {
    rv = DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_START);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(eventType.EqualsLiteral("mediacore-error")) {
    // XXXAus: TODO
  }
  else if(eventType.EqualsLiteral("mediacore-eos")) {
    rv = DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_END);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(eventType.EqualsLiteral("mediacore-metadata")) {
    nsCOMPtr<nsIDOMDataContainerEvent> dataEvent = 
      do_QueryInterface(aEvent, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> variant;
    rv = dataEvent->GetData(NS_LITERAL_STRING("properties"), 
                            getter_AddRefs(variant));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DispatchMediacoreEvent(sbIMediacoreEvent::METADATA_CHANGE, variant);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//
// sbMediacoreWrapper
//
nsresult 
sbMediacoreWrapper::AddSelfDOMListener()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPluginHostWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AddEventListener(NS_LITERAL_STRING("mediacore-error"), 
                                this, 
                                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AddEventListener(NS_LITERAL_STRING("mediacore-eos"),
                                this,
                                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AddEventListener(NS_LITERAL_STRING("mediacore-metadata"),
                                this,
                                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AddEventListener(NS_LITERAL_STRING("mediacore-buffering-begin"),
                                this,
                                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->AddEventListener(NS_LITERAL_STRING("mediacore-buffering-end"),
                                this,
                                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbMediacoreWrapper::RemoveSelfDOMListener()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPluginHostWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->RemoveEventListener(NS_LITERAL_STRING("mediacore-error"), 
                                   this, 
                                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->RemoveEventListener(NS_LITERAL_STRING("mediacore-eos"),
                                   this,
                                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = target->RemoveEventListener(NS_LITERAL_STRING("mediacore-metadata"),
                                   this,
                                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->RemoveEventListener(NS_LITERAL_STRING("mediacore-buffering-begin"),
                                   this,
                                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = target->RemoveEventListener(NS_LITERAL_STRING("mediacore-buffering-end"),
                                   this,
                                   PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbMediacoreWrapper::SendDOMEvent(const nsAString &aEventName, 
                                 const nsAString &aEventData,
                                 nsIDOMDataContainerEvent **aEvent)
{
  nsCOMPtr<nsIDOMEvent> domEvent;
  nsresult rv = mDocumentEvent->CreateEvent(NS_LITERAL_STRING("DataContainerEvent"), 
                                            getter_AddRefs(domEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent = 
    do_QueryInterface(domEvent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = domEvent->InitEvent(aEventName, PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataEvent->SetData(NS_LITERAL_STRING("data"), 
                          sbNewVariant(aEventData).get());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> retval = 
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataEvent->SetData(NS_LITERAL_STRING("retval"),
                          sbNewVariant(retval).get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool handled = PR_FALSE;
  rv = mDOMEventTarget->DispatchEvent(dataEvent, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(handled, NS_ERROR_UNEXPECTED);

  if(aEvent) {
    NS_ADDREF(*aEvent = dataEvent);
  }

  return NS_OK;
}

nsresult 
sbMediacoreWrapper::SendDOMEvent(const nsAString &aEventName, 
                                 const nsACString &aEventData,
                                 nsIDOMDataContainerEvent **aEvent)
{
  nsresult rv = SendDOMEvent(aEventName, 
                             NS_ConvertUTF8toUTF16(aEventData),
                             aEvent);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult 
sbMediacoreWrapper::GetRetvalFromEvent(nsIDOMDataContainerEvent *aEvent, 
                                       nsAString &aRetval)
{
  nsCOMPtr<nsIVariant> variant;
  nsresult rv = aEvent->GetData(NS_LITERAL_STRING("retval"), 
                                getter_AddRefs(variant));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports;
  rv = variant->GetAsISupports(getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> supportsString = 
    do_QueryInterface(supports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = supportsString->GetData(aRetval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbMediacoreWrapper::DispatchMediacoreEvent(PRUint32 aType, 
                                           nsIVariant *aData,
                                           sbIMediacoreError *aError)
{
  nsCOMPtr<sbIMediacoreEvent> event;
  nsresult rv = sbMediacoreEvent::CreateEvent(aType, 
                                              aError, 
                                              aData, 
                                              this, 
                                              getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DispatchEvent(event, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
