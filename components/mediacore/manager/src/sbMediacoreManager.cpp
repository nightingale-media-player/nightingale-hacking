/* vim: set sw=2 :miv */
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

#include "sbMediacoreManager.h"

#include <nsIAppStartupNotifier.h>
#include <nsIClassInfoImpl.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIDOMXULElement.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>
#include <nsIThread.h>

#include <nsArrayUtils.h>
#include <mozilla/Monitor.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

#include <prprf.h>

#include <sbIMediacore.h>
#include <sbIMediacoreBalanceControl.h>
#include <sbIMediacoreCapabilities.h>
#include <sbIMediacoreFactory.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreSimpleEqualizer.h>
#include <sbIMediacoreVolumeControl.h>
#include <sbIMediacoreVotingParticipant.h>

#include <sbIPrompter.h>
#include <sbIWindowWatcher.h>

#include <sbBaseMediacoreEventTarget.h>
#include <sbMediacoreVotingChain.h>

#include <sbProxiedComponentManager.h>

#include "sbMediacoreDataRemotes.h"
#include "sbMediacoreSequencer.h"

/* observer topics */
#define NS_PROFILE_STARTUP_OBSERVER_ID          "profile-after-change"
#define NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID "quit-application-granted"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID         "profile-before-change"

/* default size of hashtable for active core instances */
#define SB_CORE_HASHTABLE_SIZE    (4)
#define SB_FACTORY_HASHTABLE_SIZE (4)

/* default base instance name */
#define SB_CORE_BASE_NAME   "mediacore"
#define SB_CORE_NAME_SUFFIX "@core.songbirdnest.com"

#define SB_EQUALIZER_DEFAULT_BAND_COUNT \
  sbBaseMediacoreMultibandEqualizer::EQUALIZER_BAND_COUNT_DEFAULT

#define SB_EQUALIZER_BANDS \
  sbBaseMediacoreMultibandEqualizer::EQUALIZER_BANDS_10

#define SB_EQUALIZER_DEFAULT_BAND_GAIN  (0.0)

/* primary video window chrome url */
#define SB_PVW_CHROME_URL "chrome://songbird/content/xul/videoWindow.xul"
/* primary video window name, this doesn't need to be localizable */
#define SB_PVW_NAME "VideoWindow"
/* primary video window element id */
#define SB_PVW_ELEMENT_ID "video-box"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreManager:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreManager = nsnull;
#define TRACE(args) PR_LOG(gMediacoreManager, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreManager, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif


NS_IMPL_CLASSINFO(sbMediacoreManager, NULL, nsIClassInfo::THREADSAFE, SB_MEDIACOREMANAGER_CID);

NS_IMPL_ISUPPORTS11_CI(sbMediacoreManager,
                            sbIMediacoreManager,
                            sbPIMediacoreManager,
                            sbIMediacoreEventTarget,
                            sbIMediacoreFactoryRegistrar,
                            sbIMediacoreVideoWindow,
                            sbIMediacoreMultibandEqualizer,
                            sbIMediacoreVolumeControl,
                            sbIMediacoreVoting,
                            nsISupportsWeakReference,
                            nsIClassInfo,
                            nsIObserver);

//NS_IMPL_THREADSAFE_ADDREF(sbMediacoreManager)
//NS_IMPL_THREADSAFE_RELEASE(sbMediacoreManager)
//NS_IMPL_QUERY_INTERFACE11_CI(sbMediacoreManager,
//                            sbIMediacoreManager,
//                            sbPIMediacoreManager,
//                            sbIMediacoreEventTarget,
//                            sbIMediacoreFactoryRegistrar,
//                            sbIMediacoreVideoWindow,
//                            sbIMediacoreMultibandEqualizer,
//                            sbIMediacoreVolumeControl,
//                            sbIMediacoreVoting,
//                            nsISupportsWeakReference,
//                            nsIClassInfo,
//                            nsIObserver)
//NS_IMPL_CI_INTERFACE_GETTER5(sbMediacoreManager,
//                             sbIMediacoreManager,
//                             sbIMediacoreEventTarget,
//                             sbIMediacoreFactoryRegistrar,
//                             sbIMediacoreVoting,
//                             nsISupportsWeakReference)
//
//NS_DECL_CLASSINFO(sbMediacoreManager)
NS_IMPL_THREADSAFE_CI(sbMediacoreManager)

/**
 * "this" is used to construct mBaseEventTarget but it's not accessed till
 * after construction is complete so this is safe.
 */
sbMediacoreManager::sbMediacoreManager()
: mMonitor(nsnull)
, mLastCore(0)
, mFullscreen(PR_FALSE)
, mVideoWindowMonitor(nsnull)
, mLastVideoWindow(0)
{
  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
  // mBaseEventTarget being null is handled on access
  NS_WARN_IF_FALSE(mBaseEventTarget, "mBaseEventTarget is null, may be out of memory");

#ifdef PR_LOGGING
  if (!gMediacoreManager)
    gMediacoreManager = PR_NewLogModule("sbMediacoreManager");
#endif

  TRACE(("sbMediacoreManager[0x%x] - Created", this));
}

sbMediacoreManager::~sbMediacoreManager()
{
  TRACE(("sbMediacoreManager[0x%x] - Destroyed", this));
}

template<class T>
PLDHashOperator appendElementToArray(T* aData, void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator sbMediacoreManager::EnumerateIntoArrayStringKey(
                                      const nsAString& aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbMediacoreManager[0x%x] - EnumerateIntoArray (String)"));
  return appendElementToArray(aData, aArray);
}

template<class T>
PLDHashOperator sbMediacoreManager::EnumerateIntoArrayISupportsKey(
                                      nsISupports* aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbMediacoreManager[0x%x] - EnumerateIntoArray (nsISupports)"));
  return appendElementToArray(aData, aArray);
}

template<class T>
PLDHashOperator sbMediacoreManager::EnumerateIntoArrayUint32Key(
                                      const PRUint32 &aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbMediacoreManager[0x%x] - EnumerateIntoArray (PRUint32)"));
  return appendElementToArray(aData, aArray);
}

nsresult
sbMediacoreManager::Init()
{
  TRACE(("sbMediacoreManager[0x%x] - Init", this));

  PRBool success = mCores.Init(SB_CORE_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mFactories.Init(SB_FACTORY_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Register all factories.
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsISimpleEnumerator> categoryEnum;

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cm->EnumerateCategory(SB_MEDIACORE_FACTORY_CATEGORY,
                             getter_AddRefs(categoryEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(categoryEnum->HasMoreElements(&hasMore)) &&
         hasMore) {

    nsCOMPtr<nsISupports> ptr;
    if (NS_SUCCEEDED(categoryEnum->GetNext(getter_AddRefs(ptr))) &&
        ptr) {

      nsCOMPtr<nsISupportsCString> stringValue(do_QueryInterface(ptr));

      nsCString factoryName;
      nsresult rv = NS_ERROR_UNEXPECTED;

      if (stringValue &&
          NS_SUCCEEDED(stringValue->GetData(factoryName))) {

        char * contractId;
        rv = cm->GetCategoryEntry(SB_MEDIACORE_FACTORY_CATEGORY,
                                  factoryName.get(), &contractId);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIMediacoreFactory> factory =
          do_CreateInstance(contractId , &rv);
        NS_Free(contractId);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = RegisterFactory(factory);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  nsRefPtr<sbMediacoreSequencer> sequencer;
  sequencer = new sbMediacoreSequencer;
  NS_ENSURE_TRUE(sequencer, NS_ERROR_OUT_OF_MEMORY);

  rv = sequencer->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mSequencer = sequencer;

  rv = InitBaseMediacoreMultibandEqualizer();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitBaseMediacoreVolumeControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitVideoDataRemotes();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreManager::PreShutdown()
{
  TRACE(("sbMediacoreManager[0x%x] - PreShutdown", this));

  mozilla::MonitorAutoLock mon(mMonitor);

  if(mPrimaryCore) {
    nsCOMPtr<sbIMediacoreStatus> status;

    nsresult rv = GetStatus(getter_AddRefs(status));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 state = sbIMediacoreStatus::STATUS_UNKNOWN;
    rv = status->GetState(&state);
    NS_ENSURE_SUCCESS(rv, rv);

    if(state != sbIMediacoreStatus::STATUS_STOPPED) {
      nsCOMPtr<sbIMediacorePlaybackControl> playbackControl;
      rv = GetPlaybackControl(getter_AddRefs(playbackControl));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = playbackControl->Stop();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbMediacoreManager::Shutdown()
{
  TRACE(("sbMediacoreManager[0x%x] - Shutdown", this));

  mozilla::MonitorAutoLock mon(mMonitor);

  nsresult rv;
  if (mSequencer) {
    rv = mSequencer->Stop(PR_TRUE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to stop sequencer.");
    mSequencer = nsnull;
  }

  if (mDataRemoteEqualizerEnabled) {
    rv = mDataRemoteEqualizerEnabled->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateVolume) {
    rv = mDataRemoteFaceplateVolume->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteFaceplateMute) {
    rv = mDataRemoteFaceplateMute->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mDataRemoteVideoFullscreen) {
    rv = mDataRemoteVideoFullscreen->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDataRemoteEqualizerBands.IsInitialized()) {
    mDataRemoteEqualizerBands.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayUint32Key,
                                            mutableArray.get());
  }

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIDataRemote> dataRemote = do_QueryElementAt(mutableArray, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = dataRemote->Unbind();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mutableArray->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  mCores.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayStringKey,
                       mutableArray.get());

  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIMediacore> core = do_QueryElementAt(mutableArray, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = core->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv),
      "Failed to Shutdown a Mediacore. This may cause problems during final shutdown.");
  }

  mPrimaryCore = nsnull;

  mFactories.Clear();
  mCores.Clear();

  return NS_OK;
}

nsresult
sbMediacoreManager::GenerateInstanceName(nsAString &aInstanceName)
{
  TRACE(("sbMediacoreManager[0x%x] - GenerateInstanceName", this));

  mozilla::MonitorAutoLock mon(mMonitor);

  aInstanceName.AssignLiteral(SB_CORE_BASE_NAME);

  aInstanceName.AppendInt(mLastCore);
  ++mLastCore;

  aInstanceName.AppendLiteral(SB_CORE_NAME_SUFFIX);

  return NS_OK;
}

nsresult
sbMediacoreManager::VoteWithURIOrChannel(nsIURI *aURI,
                                         nsIChannel *aChannel,
                                         sbIMediacoreVotingChain **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - VoteWithURIOrChannel", this));
  NS_ENSURE_TRUE(aURI || aChannel, NS_ERROR_INVALID_ARG);

  nsRefPtr<sbMediacoreVotingChain> votingChain;
  votingChain = new sbMediacoreVotingChain;
  NS_ENSURE_TRUE(votingChain, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = votingChain->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> instances;

  // First go through the active instances to see if one of them
  // can handle what we wish to play.
  rv = GetInstances(getter_AddRefs(instances));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = instances->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 found = 0;
  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIMediacoreVotingParticipant> votingParticipant =
      do_QueryElementAt(instances, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 result = 0;

    if(aURI) {
      rv = votingParticipant->VoteWithURI(aURI, &result);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = votingParticipant->VoteWithChannel(aChannel, &result);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if(result > 0) {
      nsCOMPtr<sbIMediacore> mediacore =
        do_QueryInterface(votingParticipant, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = votingChain->AddVoteResult(result, mediacore);
      NS_ENSURE_SUCCESS(rv, rv);

      ++found;
    }
  }

  // Always prefer already instantiated objects, even if they may potentially
  // have a lower rank than registered factories.
  if(found) {
    NS_ADDREF(*_retval = votingChain);
    return NS_OK;
  }

  // If we haven't seen anything that can play this yet, try going through
  // all the factories, create a core and have it vote.
  nsCOMPtr<nsIArray> factories;
  rv = GetFactories(getter_AddRefs(factories));
  NS_ENSURE_SUCCESS(rv, rv);

  length = 0;
  rv = factories->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIMediacoreFactory> factory =
      do_QueryElementAt(factories, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString mediacoreInstanceName;
    GenerateInstanceName(mediacoreInstanceName);

    nsCOMPtr<sbIMediacore> mediacore;
    rv = CreateMediacoreWithFactory(factory,
                                    mediacoreInstanceName,
                                    getter_AddRefs(mediacore));
#if defined(DEBUG)
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create mediacore.");
#endif

    // Creation of core failed. Just move along to the next one.
    // This can often happen if dependecies for the mediacore are missing.
    if(NS_FAILED(rv)) {
      continue;
    }

    nsCOMPtr<sbIMediacoreVotingParticipant> votingParticipant =
      do_QueryInterface(mediacore, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 result = 0;

    if(aURI) {
      rv = votingParticipant->VoteWithURI(aURI, &result);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = votingParticipant->VoteWithChannel(aChannel, &result);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if(result > 0) {
      rv = votingChain->AddVoteResult(result, mediacore);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  NS_ADDREF(*_retval = votingChain);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbBaseMediacoreMultibandEqualizer overrides
// ----------------------------------------------------------------------------

/*virtual*/ nsresult
sbMediacoreManager::OnInitBaseMediacoreMultibandEqualizer()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);

  PRBool success = mDataRemoteEqualizerBands.Init(10);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mDataRemoteEqualizerEnabled =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteEqualizerEnabled->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_EQ_ENABLED),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString eqEnabledStr;
  rv = mDataRemoteEqualizerEnabled->GetStringValue(eqEnabledStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool eqEnabled = PR_FALSE;
  if(!eqEnabledStr.IsEmpty()) {
    rv = mDataRemoteEqualizerEnabled->GetBoolValue(&eqEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mEqEnabled = eqEnabled;

  rv = mDataRemoteEqualizerEnabled->SetBoolValue(mEqEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("[sbMediacoreManager] - Initializing eq enabled from data remote, enabled: %s",
      eqEnabled ? "true" : "false"));

  // Initialize the eq band data remotes
  for(PRUint32 i = 0; i < SB_EQUALIZER_DEFAULT_BAND_COUNT; ++i) {
    nsCOMPtr<sbIMediacoreEqualizerBand> band;
    rv = GetBand(i, getter_AddRefs(band));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnSetEqEnabled(PRBool aEqEnabled)
{

  nsresult rv = NS_ERROR_UNEXPECTED;
  {
	  mozilla::MonitorAutoLock mon(mMonitor);

	  if(mPrimaryCore) {
		  nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer =
				  do_QueryInterface(mPrimaryCore, &rv);
		  NS_ENSURE_SUCCESS(rv, rv);

		  {
			  mozilla::MonitorAutoUnlock mon(mMonitor);

			  rv = equalizer->SetEqEnabled(aEqEnabled);
			  NS_ENSURE_SUCCESS(rv, rv);

			  // If the EQ wasn't enabled before, set the bands.
			  if(!mEqEnabled && aEqEnabled) {
				  nsCOMPtr<nsISimpleEnumerator> bands;
				  rv = GetBands(getter_AddRefs(bands));
				  NS_ENSURE_SUCCESS(rv, rv);

				  rv = equalizer->SetBands(bands);
				  NS_ENSURE_SUCCESS(rv, rv);
			  }
		  }
	  }
  }

  rv = mDataRemoteEqualizerEnabled->SetBoolValue(aEqEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnGetBandCount(PRUint32 *aBandCount)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  {
		mozilla::MonitorAutoLock mon(mMonitor);

		if(mPrimaryCore) {
			nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer =
				do_QueryInterface(mPrimaryCore, &rv);
			NS_ENSURE_SUCCESS(rv, rv);

//			mon.Exit();

			rv = equalizer->GetBandCount(aBandCount);
			NS_ENSURE_SUCCESS(rv, rv);
		} else {
//			mon.Exit();

		  *aBandCount = SB_EQUALIZER_DEFAULT_BAND_COUNT;
		}
  }


  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnGetBand(PRUint32 aBandIndex, sbIMediacoreEqualizerBand *aBand)
{
  NS_ENSURE_ARG_RANGE(aBandIndex, 0, SB_EQUALIZER_DEFAULT_BAND_COUNT);

  nsresult rv = NS_ERROR_UNEXPECTED;

  {
		mozilla::MonitorAutoLock mon(mMonitor);

		if(mPrimaryCore) {
			nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer =
				do_QueryInterface(mPrimaryCore, &rv);
			NS_ENSURE_SUCCESS(rv, rv);

//			mon.Exit();

			nsCOMPtr<sbIMediacoreEqualizerBand> band;
			rv = equalizer->GetBand(aBandIndex, getter_AddRefs(band));
			NS_ENSURE_SUCCESS(rv, rv);

			PRUint32 bandIndex = 0, bandFrequency = 0;
			double bandGain = 0.0;

			rv = band->GetValues(&bandIndex, &bandFrequency, &bandGain);
			NS_ENSURE_SUCCESS(rv, rv);

			rv = aBand->Init(bandIndex, bandFrequency, bandGain);
			NS_ENSURE_SUCCESS(rv, rv);
		}	else {
			nsCOMPtr<sbIDataRemote> bandRemote;
			rv = GetAndEnsureEQBandHasDataRemote(aBandIndex, getter_AddRefs(bandRemote));
			NS_ENSURE_SUCCESS(rv, rv);

			nsString bandRemoteValue;
			rv = bandRemote->GetStringValue(bandRemoteValue);
			NS_ENSURE_SUCCESS(rv, rv);

			NS_ConvertUTF16toUTF8 gainStr(bandRemoteValue);
			PRFloat64 gain = 0;

			if((PR_sscanf(gainStr.BeginReading(), "%lg", &gain) != 1) ||
				 (gain > 1.0 || gain < -1.0)) {
				gain = SB_EQUALIZER_DEFAULT_BAND_GAIN;
				SB_ConvertFloatEqGainToJSStringValue(SB_EQUALIZER_DEFAULT_BAND_GAIN, gainStr);
				rv = bandRemote->SetStringValue(NS_ConvertUTF8toUTF16(gainStr));
				NS_ENSURE_SUCCESS(rv, rv);
			}

			rv = aBand->Init(aBandIndex, SB_EQUALIZER_BANDS[aBandIndex], gain);
			NS_ENSURE_SUCCESS(rv, rv);
		}
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnSetBand(sbIMediacoreEqualizerBand *aBand)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  {
		mozilla::MonitorAutoLock mon(mMonitor);

		if(mPrimaryCore) {
			nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer =
				do_QueryInterface(mPrimaryCore, &rv);
			NS_ENSURE_SUCCESS(rv, rv);

//			mon.Exit();

			rv = equalizer->SetBand(aBand);
			NS_ENSURE_SUCCESS(rv, rv);
		}

	}

  rv = SetAndEnsureEQBandHasDataRemote(aBand);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreManager::GetAndEnsureEQBandHasDataRemote(PRUint32 aBandIndex,
                                                    sbIDataRemote **aRemote)
{
  NS_ENSURE_ARG_RANGE(aBandIndex, 0, SB_EQUALIZER_DEFAULT_BAND_COUNT);
  NS_ENSURE_ARG_POINTER(aRemote);
  NS_ENSURE_TRUE(mDataRemoteEqualizerBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIDataRemote> bandRemote;
  PRBool success = mDataRemoteEqualizerBands.Get(aBandIndex, getter_AddRefs(bandRemote));

  if(!success) {
    rv = CreateDataRemoteForEqualizerBand(aBandIndex, getter_AddRefs(bandRemote));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  bandRemote.forget(aRemote);

  return NS_OK;
}

nsresult
sbMediacoreManager::SetAndEnsureEQBandHasDataRemote(sbIMediacoreEqualizerBand *aBand)
{
  NS_ENSURE_ARG_POINTER(aBand);
  NS_ENSURE_TRUE(mDataRemoteEqualizerBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

  PRUint32 bandIndex = 0, bandFrequency = 0;
  double bandGain = 0.0;

  nsresult rv = aBand->GetValues(&bandIndex, &bandFrequency, &bandGain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDataRemote> bandRemote;
  PRBool success = mDataRemoteEqualizerBands.Get(bandIndex, getter_AddRefs(bandRemote));

  if(!success) {
    rv = CreateDataRemoteForEqualizerBand(bandIndex, getter_AddRefs(bandRemote));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString bandGainStr;
  SB_ConvertFloatEqGainToJSStringValue(bandGain, bandGainStr);

  NS_ConvertUTF8toUTF16 gainStr(bandGainStr);
  rv = bandRemote->SetStringValue(gainStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreManager::CreateDataRemoteForEqualizerBand(PRUint32 aBandIndex,
                                                     sbIDataRemote **aRemote)
{
  NS_ENSURE_ARG_RANGE(aBandIndex, 0, SB_EQUALIZER_DEFAULT_BAND_COUNT);
  NS_ENSURE_ARG_POINTER(aRemote);
  NS_ENSURE_TRUE(mDataRemoteEqualizerBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);

  nsCOMPtr<sbIDataRemote> bandRemote =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString bandRemoteName(NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_EQ_BAND_PREFIX));
  bandRemoteName.AppendInt(aBandIndex);

  rv = bandRemote->Init(bandRemoteName, nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mDataRemoteEqualizerBands.Put(aBandIndex, bandRemote);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  bandRemote.forget(aRemote);

  return NS_OK;
}

nsresult
sbMediacoreManager::InitVideoDataRemotes()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);

  mDataRemoteVideoFullscreen =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteVideoFullscreen->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_VIDEO_FULLSCREEN), nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteVideoFullscreen->SetBoolValue(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreManager::VideoWindowUnloaded()
{
  mozilla::MonitorAutoLock mon(mVideoWindowMonitor);
  mVideoWindow = nsnull;
  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbBaseMediacoreVolumeControl overrides
// ----------------------------------------------------------------------------

/*virtual*/ nsresult
sbMediacoreManager::OnInitBaseMediacoreVolumeControl()
{
  nsString nullString;
  nullString.SetIsVoid(PR_TRUE);

  nsresult rv = NS_ERROR_UNEXPECTED;
  mDataRemoteFaceplateVolume =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateVolume->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_VOLUME),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString volumeStr;
  rv = mDataRemoteFaceplateVolume->GetStringValue(volumeStr);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF16toUTF8 volStr(volumeStr);
  PRFloat64 volume = 0;

  if((PR_sscanf(volStr.BeginReading(), "%lg", &volume) != 1) ||
     (volume > 1 || volume < 0)) {
    volume = 0.5;
  }

  mVolume = volume;

  LOG(("[sbMediacoreManager] - Initializing volume from data remote, volume: %s",
         volStr.BeginReading()));

  rv = SetVolumeDataRemote(mVolume);
  NS_ENSURE_SUCCESS(rv, rv);

  mDataRemoteFaceplateMute =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataRemoteFaceplateMute->Init(
    NS_LITERAL_STRING(SB_MEDIACORE_DATAREMOTE_FACEPLATE_MUTE),
    nullString);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString muteStr;
  rv = mDataRemoteFaceplateMute->GetStringValue(muteStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool mute = PR_FALSE;
  if(!muteStr.IsEmpty()) {
    rv = mDataRemoteFaceplateMute->GetBoolValue(&mute);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mMute = mute;

  rv = mDataRemoteFaceplateMute->SetBoolValue(mMute);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("[sbMediacoreManager] - Initializing mute from data remote, mute: %d",
         mute));

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnSetMute(PRBool aMute)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  {
  	mozilla::MonitorAutoLock mon(mMonitor);

		if(mPrimaryCore) {
			nsCOMPtr<sbIMediacoreVolumeControl> volumeControl =
				do_QueryInterface(mPrimaryCore, &rv);
			NS_ENSURE_SUCCESS(rv, rv);

//			mon.Exit();

			rv = volumeControl->SetMute(aMute);
			NS_ENSURE_SUCCESS(rv, rv);
		}
  }

	rv = mDataRemoteFaceplateMute->SetBoolValue(aMute);
	NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbMediacoreManager::OnSetVolume(double aVolume)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  {
		mozilla::MonitorAutoLock mon(mMonitor);

		if(mPrimaryCore) {

			nsCOMPtr<sbIMediacoreVolumeControl> volumeControl =
				do_QueryInterface(mPrimaryCore, &rv);
			NS_ENSURE_SUCCESS(rv, rv);

//			mon.Exit();

			rv = volumeControl->SetVolume(aVolume);
			NS_ENSURE_SUCCESS(rv, rv);
		}
  }

  rv = SetVolumeDataRemote(aVolume);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediacoreManager::SetVolumeDataRemote(PRFloat64 aVolume)
{
  NS_ENSURE_STATE(mDataRemoteFaceplateVolume);

  nsCString volume;
  SB_ConvertFloatVolToJSStringValue(aVolume, volume);

  NS_ConvertUTF8toUTF16 volumeStr(volume);
  nsresult rv = mDataRemoteFaceplateVolume->SetStringValue(volumeStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbIMediacoreManager Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreManager::GetPrimaryCore(sbIMediacore * *aPrimaryCore)
{
  TRACE(("sbMediacoreManager[0x%x] - GetPrimaryCore", this));
  NS_ENSURE_ARG_POINTER(aPrimaryCore);

  mozilla::MonitorAutoLock mon(mMonitor);
  NS_IF_ADDREF(*aPrimaryCore = mPrimaryCore);

  return NS_OK;
}
NS_IMETHODIMP
sbMediacoreManager::GetBalanceControl(
                      sbIMediacoreBalanceControl * *aBalanceControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetBalanceControl", this));
  NS_ENSURE_ARG_POINTER(aBalanceControl);

  *aBalanceControl = nsnull;

  mozilla::MonitorAutoLock mon(mMonitor);

  if(!mPrimaryCore) {
    return nsnull;
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreBalanceControl> balanceControl =
    do_QueryInterface(mPrimaryCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  balanceControl.forget(aBalanceControl);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetVolumeControl(
                      sbIMediacoreVolumeControl * *aVolumeControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetVolumeControl", this));
  NS_ENSURE_ARG_POINTER(aVolumeControl);

  mozilla::MonitorAutoLock mon(mMonitor);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreVolumeControl> volumeControl =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreManager *, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  volumeControl.forget(aVolumeControl);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetEqualizer(
                      sbIMediacoreMultibandEqualizer * *aEqualizer)
{
  TRACE(("sbMediacoreManager[0x%x] - GetEqualizer", this));
  NS_ENSURE_ARG_POINTER(aEqualizer);

  *aEqualizer = nsnull;

  mozilla::MonitorAutoLock mon(mMonitor);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreManager *, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  equalizer.forget(aEqualizer);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetPlaybackControl(
                      sbIMediacorePlaybackControl * *aPlaybackControl)
{
  TRACE(("sbMediacoreManager[0x%x] - GetPlaybackControl", this));
  NS_ENSURE_ARG_POINTER(aPlaybackControl);

  *aPlaybackControl = nsnull;

  mozilla::MonitorAutoLock mon(mMonitor);

  if(!mPrimaryCore) {
    return NS_OK;
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacorePlaybackControl> playbackControl =
    do_QueryInterface(mPrimaryCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  playbackControl.forget(aPlaybackControl);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetCapabilities(
                      sbIMediacoreCapabilities * *aCapabilities)
{
  TRACE(("sbMediacoreManager[0x%x] - GetCapabilities", this));
  NS_ENSURE_ARG_POINTER(aCapabilities);

  *aCapabilities = nsnull;

  mozilla::MonitorAutoLock mon(mMonitor);

  if(!mPrimaryCore) {
    return NS_OK;
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreCapabilities> volumeControl =
    do_QueryInterface(mPrimaryCore, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  volumeControl.forget(aCapabilities);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetStatus(sbIMediacoreStatus * *aStatus)
{
  TRACE(("sbMediacoreManager[0x%x] - GetStatus", this));
  NS_ENSURE_ARG_POINTER(aStatus);

  nsresult rv = NS_ERROR_UNEXPECTED;
  mozilla::MonitorAutoLock mon(mMonitor);
  nsCOMPtr<sbIMediacoreStatus> status =
    do_QueryInterface(mSequencer, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  status.forget(aStatus);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetVideo(sbIMediacoreVideoWindow * *aVideo)
{
  TRACE(("sbMediacoreManager[0x%x] - GetVideo", this));
  NS_ENSURE_ARG_POINTER(aVideo);

  *aVideo = nsnull;

  mozilla::MonitorAutoLock mon(mMonitor);

  if(!mPrimaryCore) {
    return NS_OK;
  }

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMediacoreVideoWindow> videoWindow =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreManager *, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  videoWindow.forget(aVideo);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetSequencer(
                      sbIMediacoreSequencer * *aSequencer)
{
  TRACE(("sbMediacoreManager[0x%x] - GetSequencer", this));
  NS_ENSURE_ARG_POINTER(aSequencer);

  nsresult rv = NS_ERROR_UNEXPECTED;
  mozilla::MonitorAutoLock mon(mMonitor);
  nsCOMPtr<sbIMediacoreSequencer> sequencer =
    do_QueryInterface(mSequencer, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sequencer.forget(aSequencer);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::SetSequencer(
                      sbIMediacoreSequencer * aSequencer)
{
  TRACE(("sbMediacoreManager[0x%x] - SetSequencer", this));
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbMediacoreManager::GetPrimaryVideoWindow(PRBool aCreate,
                                          PRUint32 aWidthHint,
                                          PRUint32 aHeightHint,
                                          sbIMediacoreVideoWindow **aVideo)
{
  TRACE(("sbMediacoreManager[0x%x] - GetVideoWindow", this));
  NS_ENSURE_ARG_POINTER(aVideo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  *aVideo = nsnull;

  PRBool hintValid = PR_FALSE;
  if(aWidthHint > 0 && aHeightHint > 0) {
    hintValid = PR_TRUE;
  }

  {
  	mozilla::MonitorAutoLock mon(mVideoWindowMonitor);

    if(mVideoWindow) {
      nsCOMPtr<sbIMediacoreVideoWindow> videoWindow =
        do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreManager *, this), &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      videoWindow.forget(aVideo);

      return NS_OK;
    }

    if(!aCreate) {
      return NS_OK;
    }
  }

  nsCOMPtr<sbIPrompter> prompter =
    do_GetService(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->SetParentWindowType(NS_LITERAL_STRING("Songbird:Main"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a unique window name. This circumvents race conditions that can
  // occur while a window is requested and the video window is currently
  // closing by avoiding reuse of the closing window.
  nsString windowName;
  windowName.AssignLiteral(SB_PVW_NAME);
  windowName.AppendInt(mLastVideoWindow++);

  nsCOMPtr<nsIDOMWindow> domWindow;
  rv = prompter->OpenWindow(nsnull,
                            NS_LITERAL_STRING(SB_PVW_CHROME_URL),
                            windowName,
                            NS_LITERAL_STRING("chrome,centerscreen,resizable"),
                            nsnull,
                            getter_AddRefs(domWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  // If we're not on the main thread, we'll have to proxy calls
  // to quite a few objects.
  PRBool mainThread = NS_IsMainThread();
  nsCOMPtr<nsIThread> target;

  rv = NS_GetMainThread(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  // Now we have to wait for the window to finish loading before we
  // continue. If we don't let it finish loading we will never be
  // able to find the correct element in the window.
  if(!mainThread) {
    // If we're not on the main thread we can use the songbird window
    // watcher to wait for a window of type "Songbird:Core" to come up.
    nsCOMPtr<sbIWindowWatcher> windowWatcher =
      do_GetService(SB_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = windowWatcher->WaitForWindow(NS_LITERAL_STRING("Songbird:Core"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMWindow> grip;
    domWindow.swap(grip);

    rv = do_GetProxyForObject(target,
                              NS_GET_IID(nsIDOMWindow),
                              grip,
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(domWindow));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEventTarget> domTarget = do_QueryInterface(domWindow, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEventTarget> proxiedDomTarget;
    rv = do_GetProxyForObject(target, 
                              NS_GET_IID(nsIDOMEventTarget), 
                              domTarget, 
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS, 
                              getter_AddRefs(proxiedDomTarget));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbMediacoreVideoWindowListener> videoWindowListener;
    videoWindowListener = new sbMediacoreVideoWindowListener;
    NS_ENSURE_TRUE(videoWindowListener, NS_ERROR_OUT_OF_MEMORY);

    rv = videoWindowListener->Init(this, domTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = proxiedDomTarget->AddEventListener(NS_LITERAL_STRING("unload"),
                                            videoWindowListener,
                                            PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // On the main thread, we'll have to add a listener to the window
    // and process events on the main thread while we wait for the video
    // window to finish loading.
    nsRefPtr<sbMediacoreVideoWindowListener> videoWindowListener;
    videoWindowListener = new sbMediacoreVideoWindowListener;
    NS_ENSURE_TRUE(videoWindowListener, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIDOMEventTarget> domTarget = do_QueryInterface(domWindow, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = videoWindowListener->Init(this, domTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = domTarget->AddEventListener(NS_LITERAL_STRING("resize"),
                                     videoWindowListener,
                                     PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool processed = PR_FALSE;
    while(!videoWindowListener->IsWindowReady()) {
      rv = target->ProcessNextEvent(PR_FALSE, &processed);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = domTarget->RemoveEventListener(NS_LITERAL_STRING("resize"),
                                        videoWindowListener,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    // If the window isn't ready yet, error out.
    NS_ENSURE_TRUE(videoWindowListener->IsWindowReady(), NS_ERROR_FAILURE);

    rv = domTarget->AddEventListener(NS_LITERAL_STRING("unload"),
                                     videoWindowListener,
                                     PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIDOMDocument> domDoc;

  rv = domWindow->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  if(!mainThread) {
    nsCOMPtr<nsIDOMDocument> grip;
    domDoc.swap(grip);

    rv = do_GetProxyForObject(target,
                              NS_GET_IID(nsIDOMDocument),
                              grip,
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(domDoc));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIDOMElement> domElement;
  rv = domDoc->GetElementById(NS_LITERAL_STRING(SB_PVW_ELEMENT_ID),
                              getter_AddRefs(domElement));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(domElement, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMXULElement> domXulElement =
    do_QueryInterface(domElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  {
  	mozilla::MonitorAutoLock mon(mVideoWindowMonitor);
    domXulElement.swap(mVideoWindow);
  }

  nsCOMPtr<sbIMediacoreVideoWindow> videoWindow =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediacoreManager *, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  videoWindow.forget(aVideo);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbPIMediacoreManager Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreManager::SetPrimaryCore(sbIMediacore * aPrimaryCore)
{
  TRACE(("sbMediacoreManager[0x%x] - SetPrimaryCore", this));
  NS_ENSURE_ARG_POINTER(aPrimaryCore);

  nsresult rv;
  nsCOMPtr<sbIMediacoreVolumeControl> volumeControl;
  nsCOMPtr<sbIMediacoreMultibandEqualizer> equalizer;

  {
   mozilla::MonitorAutoLock mon(mMonitor);
		mPrimaryCore = aPrimaryCore;

		rv = NS_ERROR_UNEXPECTED;

		volumeControl = do_QueryInterface(mPrimaryCore, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		// No equalizer interface on the primary core is not a fatal error.
		equalizer = do_QueryInterface(mPrimaryCore, &rv);
		if(NS_FAILED(rv)) {
			equalizer = nsnull;
		}
	}

  {
		mozilla::MonitorAutoLock volMon(sbBaseMediacoreVolumeControl::mMonitor);

		rv = volumeControl->SetVolume(mVolume);
		NS_ENSURE_SUCCESS(rv, rv);

		rv = volumeControl->SetMute(mMute);
		NS_ENSURE_SUCCESS(rv, rv);
  }

  if(equalizer) {
  	PRBool eqEnabled;

  	{
  		mozilla::MonitorAutoLock eqMon(sbBaseMediacoreMultibandEqualizer::mMonitor);

			eqEnabled = mEqEnabled;
			rv = equalizer->SetEqEnabled(mEqEnabled);
			NS_ENSURE_SUCCESS(rv, rv);

  	}

    if(eqEnabled) {
      nsCOMPtr<nsISimpleEnumerator> bands;
      rv = GetBands(getter_AddRefs(bands));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = equalizer->SetBands(bands);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbIMediacoreFactoryRegistrar Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreManager::GetFactories(nsIArray * *aFactories)
{
  TRACE(("sbMediacoreManager[0x%x] - GetFactories", this));
  NS_ENSURE_ARG_POINTER(aFactories);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock mon(mMonitor);
  mFactories.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayISupportsKey,
                           mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if(length < mFactories.Count()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIArray> array = do_QueryInterface(mutableArray, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  array.forget(aFactories);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetInstances(nsIArray * *aInstances)
{
  TRACE(("sbMediacoreManager[0x%x] - GetInstances", this));
  NS_ENSURE_ARG_POINTER(aInstances);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock mon(mMonitor);

  mCores.EnumerateRead(sbMediacoreManager::EnumerateIntoArrayStringKey,
                       mutableArray.get());

  PRUint32 length = 0;
  rv = mutableArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if(length < mCores.Count()) {
    return NS_ERROR_FAILURE;
  }

  rv = CallQueryInterface(mutableArray, aInstances);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::CreateMediacore(const nsAString & aContractID,
                                    const nsAString & aInstanceName,
                                    sbIMediacore **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - CreateMediacore", this));
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;
  NS_ConvertUTF16toUTF8 contractId(aContractID);

  nsCOMPtr<sbIMediacoreFactory> coreFactory =
    do_CreateInstance(contractId.BeginReading(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacore> core;
  rv = GetMediacore(aInstanceName, getter_AddRefs(core));

  if(NS_SUCCEEDED(rv)) {
    core.forget(_retval);
    return NS_OK;
  }

  mozilla::MonitorAutoLock mon(mMonitor);

  rv = coreFactory->Create(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mCores.Put(aInstanceName, *_retval);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::CreateMediacoreWithFactory(sbIMediacoreFactory *aFactory,
                                               const nsAString & aInstanceName,
                                               sbIMediacore **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - CreateMediacoreWithFactory", this));
  NS_ENSURE_ARG_POINTER(aFactory);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediacore> core;
  nsresult rv = GetMediacore(aInstanceName, getter_AddRefs(core));

  if(NS_SUCCEEDED(rv)) {
    core.forget(_retval);
    return NS_OK;
  }

  rv = aFactory->Create(aInstanceName, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mCores.Put(aInstanceName, *_retval);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetMediacore(const nsAString & aInstanceName,
                                 sbIMediacore **_retval)
{
  TRACE(("%s[%p] (%s)", __FUNCTION__, this,
         NS_ConvertUTF16toUTF8(aInstanceName).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIMediacore> core;

  mozilla::MonitorAutoLock mon(mMonitor);

  PRBool success = mCores.Get(aInstanceName, getter_AddRefs(core));
  NS_ENSURE_TRUE(success, NS_ERROR_NOT_AVAILABLE);

  core.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::DestroyMediacore(const nsAString & aInstanceName)
{
  TRACE(("sbMediacoreManager[0x%x] - DestroyMediacore", this));

  nsCOMPtr<sbIMediacore> core;

  mozilla::MonitorAutoLock mon(mMonitor);

  PRBool success = mCores.Get(aInstanceName, getter_AddRefs(core));
  NS_ENSURE_TRUE(success, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(core, NS_ERROR_UNEXPECTED);

  nsresult rv = core->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  mCores.Remove(aInstanceName);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::RegisterFactory(sbIMediacoreFactory *aFactory)
{
  TRACE(("sbMediacoreManager[0x%x] - RegisterFactory", this));
  NS_ENSURE_ARG_POINTER(aFactory);

  mozilla::MonitorAutoLock mon(mMonitor);

  PRBool success = mFactories.Put(aFactory, aFactory);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::UnregisterFactory(sbIMediacoreFactory *aFactory)
{
  TRACE(("sbMediacoreManager[0x%x] - UnregisterFactory", this));
  NS_ENSURE_ARG_POINTER(aFactory);

  mozilla::MonitorAutoLock mon(mMonitor);

  mFactories.Remove(aFactory);

  return NS_OK;
}


// ----------------------------------------------------------------------------
// sbIMediacoreVideoWindow Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreManager::GetFullscreen(PRBool *aFullscreen)
{
  TRACE(("sbMediacoreManager[0x%x] - GetFullscreen", this));
  NS_ENSURE_ARG_POINTER(aFullscreen);

  mozilla::MonitorAutoLock mon(mMonitor);

  if(mPrimaryCore) {
    nsresult rv = NS_ERROR_UNEXPECTED;;

    nsCOMPtr<sbIMediacoreVideoWindow> videoWindow =
      do_QueryInterface(mPrimaryCore, &rv);

    if(NS_SUCCEEDED(rv)) {
      rv = videoWindow->GetFullscreen(aFullscreen);
      NS_ENSURE_SUCCESS(rv, rv);

      // If for some reason we get out of sync we should update now.
      if(*aFullscreen != mFullscreen) {
        mFullscreen = *aFullscreen;

        rv = mDataRemoteVideoFullscreen->SetBoolValue(mFullscreen);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      return NS_OK;
    }
  }

  *aFullscreen = mFullscreen;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::SetFullscreen(PRBool aFullscreen)
{
  TRACE(("sbMediacoreManager[0x%x] - SetFullscreen", this));

  nsresult rv = NS_ERROR_UNEXPECTED;

  mozilla::MonitorAutoLock mon(mMonitor);
  if(mPrimaryCore) {
    nsCOMPtr<sbIMediacoreVideoWindow> videoWindow =
      do_QueryInterface(mPrimaryCore, &rv);

    if(NS_SUCCEEDED(rv)) {
      rv = videoWindow->SetFullscreen(aFullscreen);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  mFullscreen = aFullscreen;

  rv = mDataRemoteVideoFullscreen->SetBoolValue(mFullscreen);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::GetVideoWindow(nsIDOMXULElement * *aVideoWindow)
{
  TRACE(("sbMediacoreManager[0x%x] - GetVideoWindow", this));
  NS_ENSURE_ARG_POINTER(aVideoWindow);

  mozilla::MonitorAutoLock mon(mMonitor);
  NS_IF_ADDREF(*aVideoWindow = mVideoWindow);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::SetVideoWindow(nsIDOMXULElement * aVideoWindow)
{
  TRACE(("sbMediacoreManager[0x%x] - SetVideoWindow", this));
  NS_ENSURE_ARG_POINTER(aVideoWindow);

  mozilla::MonitorAutoLock mon(mMonitor);
  mVideoWindow = aVideoWindow;

  return NS_OK;
}


// ----------------------------------------------------------------------------
// sbIMediacoreVoting Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbMediacoreManager::VoteWithURI(nsIURI *aURI,
                                sbIMediacoreVotingChain **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - VoteWithURI", this));
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = VoteWithURIOrChannel(aURI, nsnull, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::VoteWithChannel(nsIChannel *aChannel,
                                    sbIMediacoreVotingChain **_retval)
{
  TRACE(("sbMediacoreManager[0x%x] - VoteWithChannel", this));
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = VoteWithURIOrChannel(nsnull, aChannel, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// ----------------------------------------------------------------------------
// nsIObserver Interface
// ----------------------------------------------------------------------------

NS_IMETHODIMP sbMediacoreManager::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  TRACE(("sbMediacoreManager[0x%x] - Observe", this));

  nsresult rv = NS_ERROR_UNEXPECTED;

  if (!strcmp(aTopic, APPSTARTUP_CATEGORY)) {
    // listen for profile startup and profile shutdown messages
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

  }
  else if (!strcmp(NS_PROFILE_STARTUP_OBSERVER_ID, aTopic)) {

    // Called after the profile has been loaded, so prefs and such are available
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);

  }
  else if (!strcmp(NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, aTopic)) {

    // Called when the request to shutdown has been granted
    // We can't watch the -requested notification since it may not always be
    // fired :(

    // Pre-shutdown.

    // Remove the observer so we don't get called a second time, since we are
    // shutting down anyways this should not cause problems.
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = PreShutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (!strcmp(NS_PROFILE_SHUTDOWN_OBSERVER_ID, aTopic)) {

    // Final shutdown.

    // remove all the observers
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreManager::CreateEvent(PRUint32 aType,
                                sbIMediacore *aOrigin,
                                nsIVariant *aData,
                                sbIMediacoreError *aError,
                                sbIMediacoreEvent **retval)
{
  NS_ENSURE_ARG_POINTER(aOrigin);
  NS_ENSURE_ARG_POINTER(retval);

  return sbMediacoreEvent::CreateEvent(aType, aError, aData, aOrigin, retval);
}

// Forwarding functions for sbIMediacoreEventTarget interface

NS_IMETHODIMP
sbMediacoreManager::DispatchEvent(sbIMediacoreEvent *aEvent,
                                  PRBool aAsync,
                                  PRBool* _retval)
{
  return mBaseEventTarget ? mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbMediacoreManager::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->AddListener(aListener) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbMediacoreManager::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ? mBaseEventTarget->RemoveListener(aListener) : NS_ERROR_NULL_POINTER;
}

//-----------------------------------------------------------------------------
// sbMediacoreVideoWindowListener class
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ADDREF(sbMediacoreVideoWindowListener)
NS_IMPL_THREADSAFE_RELEASE(sbMediacoreVideoWindowListener)
NS_IMPL_QUERY_INTERFACE1(sbMediacoreVideoWindowListener,
                         nsIDOMEventListener)

sbMediacoreVideoWindowListener::sbMediacoreVideoWindowListener()
: mWindowReady(PR_FALSE)
{
}

sbMediacoreVideoWindowListener::~sbMediacoreVideoWindowListener()
{
}

nsresult
sbMediacoreVideoWindowListener::Init(sbMediacoreManager *aManager,
                                     nsIDOMEventTarget *aTarget)
{
  NS_ENSURE_ARG_POINTER(aManager);
  NS_ENSURE_ARG_POINTER(aTarget);

  mManager = aManager;
  mTarget = aTarget;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreVideoWindowListener::HandleEvent(nsIDOMEvent *aEvent)
{
  NS_ENSURE_TRUE(mManager, NS_ERROR_NOT_INITIALIZED);

  nsString eventType;
  nsresult rv = aEvent->GetType(eventType);
  NS_ENSURE_SUCCESS(rv, rv);

  if(eventType.EqualsLiteral("resize")) {
    mWindowReady = PR_TRUE;
  }
  else if(eventType.EqualsLiteral("unload")) {
    rv = mManager->VideoWindowUnloaded();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMEventListener> grip(this);
    rv = mTarget->RemoveEventListener(NS_LITERAL_STRING("unload"),
                                      this,
                                      PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

PRBool
sbMediacoreVideoWindowListener::IsWindowReady()
{
  return mWindowReady;
}
