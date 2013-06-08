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

/** 
 * \file  sbBaseMediacorePlaybackControl.h
 * \brief Songbird Base Mediacore Playback Control Definition.
 */

#ifndef __SB_BASEMEDIACOREPLAYBACKCONTROL_H__
#define __SB_BASEMEDIACOREPLAYBACKCONTROL_H__

#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreEventTarget.h>

#include <nsIURI.h>

#include <mozilla/Monitor.h>
#include <nsCOMPtr.h>

class sbBaseMediacorePlaybackControl : public sbIMediacorePlaybackControl,
                                       public sbIMediacoreEventTarget
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREPLAYBACKCONTROL

  sbBaseMediacorePlaybackControl();

  nsresult InitBaseMediacorePlaybackControl();

  // Override me, see cpp file for implementation notes.
  virtual nsresult OnInitBaseMediacorePlaybackControl();

  // Override me, see cpp file for implementation notes.
  virtual nsresult OnSetUri(nsIURI *aURI);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnGetDuration(PRUint64 *aPosition);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnGetPosition(PRUint64 *aPosition);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnSetPosition(PRUint64 aPosition);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnGetIsPlayingAudio(PRBool *aIsPlayingAudio);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnGetIsPlayingVideo(PRBool *aIsPlayingVideo);
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnPlay();
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnPause();
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnStop();
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnSeek(PRUint64 aPosition, PRUint32 aFlags);

protected:
  virtual ~sbBaseMediacorePlaybackControl();

  nsresult DispatchPlaybackControlEvent(PRUint32 aType);

  mozilla::Monitor mMonitor;

  nsCOMPtr<nsIURI> mUri;

  // Use for caching if you wish, lock mLock before using.
  PRUint64 mPosition;
  // Use for caching if you wish, lock mLock before using.
  PRUint64 mDuration;
};

#endif /* __SB_BASEMEDIACOREPLAYBACKCONTROL_H__ */
