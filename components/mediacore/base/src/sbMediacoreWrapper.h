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
 * \file  sbMediacoreWrapper.h
 * \brief Songbird Mediacore Wrapper Definition.
 */

#ifndef __SB_MEDIACOREWRAPPER_H__
#define __SB_MEDIACOREWRAPPER_H__

#include <sbIMediacoreWrapper.h>

#include <nsIClassInfo.h>
#include <nsIDOMDataContainerEvent.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIDOMWindowInternal.h>

#include <sbIMediacoreCapabilities.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediacoreVotingParticipant.h>
#include <sbIPrompter.h>

#include "sbBaseMediacore.h"
#include "sbBaseMediacoreEventTarget.h"
#include "sbBaseMediacorePlaybackControl.h"
#include "sbBaseMediacoreVolumeControl.h"
#include "sbMediacoreEvent.h"

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbMediacoreWrapper : public sbBaseMediacore,
                           public sbBaseMediacorePlaybackControl,
                           public sbBaseMediacoreVolumeControl,
                           public sbIMediacoreVotingParticipant,
                           public sbIMediacoreWrapper,
                           public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIDOMEVENTLISTENER
  
  NS_DECL_SBIMEDIACOREVOTINGPARTICIPANT
  NS_DECL_SBIMEDIACOREWRAPPER

  NS_FORWARD_SAFE_SBIMEDIACOREEVENTTARGET(mBaseEventTarget)

  sbMediacoreWrapper();

  nsresult Init();

  // sbBaseMediacore overrides
  virtual nsresult OnInitBaseMediacore();
  virtual nsresult OnGetCapabilities();
  virtual nsresult OnShutdown();

  // sbBaseMediacorePlaybackControl overrides
  virtual nsresult OnInitBaseMediacorePlaybackControl();
  virtual nsresult OnSetUri(nsIURI *aURI);
  virtual nsresult OnGetDuration(PRUint64 *aDuration);
  virtual nsresult OnGetPosition(PRUint64 *aPosition);
  virtual nsresult OnSetPosition(PRUint64 aPosition);
  virtual nsresult OnGetIsPlayingAudio(PRBool *aIsPlayingAudio);
  virtual nsresult OnGetIsPlayingVideo(PRBool *aIsPlayingVideo);
  virtual nsresult OnPlay();
  virtual nsresult OnPause();
  virtual nsresult OnStop();
  virtual nsresult OnSeek(PRUint64 aPosition, PRUint32 aFlag);

  // sbBaseMediacoreVolumeControl overrides
  virtual nsresult OnInitBaseMediacoreVolumeControl();
  virtual nsresult OnSetMute(PRBool aMute);
  virtual nsresult OnSetVolume(PRFloat64 aVolume);

private:
  nsresult AddSelfDOMListener();
  nsresult RemoveSelfDOMListener();

  nsresult SendDOMEvent(const nsAString &aEventName, 
                        const nsAString &aEventData,
                        nsIDOMDataContainerEvent **aEvent = nsnull);

  nsresult SendDOMEvent(const nsAString &aEventName, 
                        const nsACString &aEventData,
                        nsIDOMDataContainerEvent **aEvent = nsnull);

  nsresult GetRetvalFromEvent(nsIDOMDataContainerEvent *aEvent, 
                              nsAString &aRetval);

  nsresult DispatchMediacoreEvent(PRUint32 aType, 
                                  nsIVariant *aData = nsnull,
                                  sbIMediacoreError *aError = nsnull);
  
protected:
  virtual ~sbMediacoreWrapper();

  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;

  nsCOMPtr<nsIDOMDocumentEvent> mDocumentEvent;
  nsCOMPtr<nsIDOMEventTarget>   mDOMEventTarget;
  
  PRMonitor *                   mProxiedObjectsMonitor;
  nsCOMPtr<nsIDOMEventTarget>   mProxiedDOMEventTarget;
  nsCOMPtr<nsIDOMDocumentEvent> mProxiedDocumentEvent;

  nsCOMPtr<nsIDOMWindow>        mPluginHostWindow;

  nsCOMPtr<sbIPrompter>         mPrompter;

  PRPackedBool                  mWindowIsReady;
};

#endif /* __SB_MEDIACOREWRAPPER_H__ */
