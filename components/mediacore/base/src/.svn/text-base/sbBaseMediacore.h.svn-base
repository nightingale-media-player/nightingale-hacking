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
 * \file  sbBaseMediacore.h
 * \brief Songbird Base Mediacore Definition.
 */

#ifndef __SB_BASEMEDIACORE_H__
#define __SB_BASEMEDIACORE_H__

#include <sbIMediacore.h>

#include <nsIClassInfo.h>

#include <sbIMediacoreCapabilities.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbBaseMediacore : public sbIMediacore,
                        public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIMEDIACORE

  sbBaseMediacore();

  // you have to call this before calling anything else.
  nsresult InitBaseMediacore();

  nsresult SetInstanceName(const nsAString &aInstanceName);
  nsresult SetCapabilities(sbIMediacoreCapabilities *aCapabilities);
  nsresult SetStatus(sbIMediacoreStatus *aStatus);

  //Override me, see cpp file for implementation notes.
  virtual nsresult OnInitBaseMediacore();

  //Override me, see cpp file for implementation notes.
  virtual nsresult OnGetCapabilities();

  //Override me, see cpp file for implementation notes.
  virtual nsresult OnSetSequencer(sbIMediacoreSequencer *aSequencer);

  //Override me, see cpp file for implementation notes.
  virtual nsresult OnShutdown();

protected:
  virtual ~sbBaseMediacore();

  PRMonitor  *mMonitor;
  
  nsString mInstanceName;

  nsCOMPtr<sbIMediacoreCapabilities> mCapabilities;
  nsCOMPtr<sbIMediacoreStatus>       mStatus;
  nsCOMPtr<sbIMediacoreSequencer>    mSequencer;
};

#endif /* __SB_BASEMEDIACORE_H__ */
