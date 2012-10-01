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
 * \file  sbMediacoreEvent.h
 * \brief Songbird Mediacore Event Definition.
 */

#ifndef __SB_MEDIACOREEVENT_H__
#define __SB_MEDIACOREEVENT_H__

#include <sbIMediacoreEvent.h>

#include <nsIVariant.h>

#include <sbIMediacore.h>
#include <sbIMediacoreError.h>
#include <sbIMediacoreEventTarget.h>

#include <nsCOMPtr.h>

class sbMediacoreEvent : public sbIMediacoreEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREEVENT

  sbMediacoreEvent();

  nsresult Init(PRUint32 aType,
                sbIMediacoreError *aError,
                nsIVariant *aData,
                sbIMediacore *aOrigin);

  nsresult SetTarget(sbIMediacoreEventTarget *aTarget);

  void Dispatch();
  bool WasDispatched();

  /**
   * Used to create an instance of a mediacore event with an error
   */
  static nsresult CreateEvent(PRUint32 aType,
                              sbIMediacoreError * aError,
                              nsIVariant *aData,
                              sbIMediacore *aOrigin,
                              sbIMediacoreEvent **retval);

protected:
  virtual ~sbMediacoreEvent();

  PRLock*  mLock;
  PRUint32 mType;

  nsCOMPtr<sbIMediacoreError> mError;
  nsCOMPtr<nsIVariant>        mData;

  nsCOMPtr<sbIMediacore>            mOrigin;
  nsCOMPtr<sbIMediacoreEventTarget> mTarget;

  bool mDispatched;
};

#endif /* __SB_MEDIACOREEVENT_H__ */
