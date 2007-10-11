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

#ifndef __SBDOWNLOADBUTTONPROPERTYINFO_H__
#define __SBDOWNLOADBUTTONPROPERTYINFO_H__

#include "sbImmutablePropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbITreeViewPropertyInfo.h>
#include <sbIClickablePropertyInfo.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <stdio.h>

class sbDownloadButtonPropertyInfo : public sbImmutablePropertyInfo,
                                     public sbIClickablePropertyInfo,
                                     public sbITreeViewPropertyInfo
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBICLICKABLEPROPERTYINFO
  NS_DECL_SBITREEVIEWPROPERTYINFO

  sbDownloadButtonPropertyInfo(const nsAString& aPropertyName,
                               const nsAString& aDisplayName,
                               const nsAString& aLabel,
                               const PRBool aRemoteReadable,
                               const PRBool aRemoteWritable,
                               const PRBool aUserViewable,
                               const PRBool aUserEditable);

  NS_IMETHOD Format(const nsAString& aValue, nsAString& _retval);

  nsresult Init();

private:

  nsString mLabel;

};

class sbDownloadButtonPropertyValue
{
public:
  enum Mode {
    eNone        = 0,
    eNew         = 1,
    eStarting    = 2,
    eDownloading = 3,
    ePaused      = 4,
    eComplete    = 5
  };

  sbDownloadButtonPropertyValue(const nsAString& aValue) :
    mMode(eNone),
    mTotal(0),
    mCurrent(0),
    mIsDirty(PR_FALSE),
    mIsModeSet(PR_FALSE),
    mIsTotalSet(PR_FALSE),
    mIsCurrentSet(PR_FALSE)
  {
    mFirstPipe   = aValue.FindChar('|');
    mSecondPipe  = aValue.FindChar('|', mFirstPipe + 1);

    if (mFirstPipe >= 1 && mSecondPipe >= 2) {
      mValue = aValue;
    }
    else {
      mValue.SetIsVoid(PR_TRUE);
      mIsModeSet    = PR_TRUE;
      mIsTotalSet   = PR_TRUE;
      mIsCurrentSet = PR_TRUE;
    }
  }

  Mode GetMode()
  {
    if (!mIsModeSet) {
      nsresult rv;
      PRInt32 mode = Substring(mValue, 0, mFirstPipe).ToInteger(&rv);
      if (NS_SUCCEEDED(rv) && mode >= eNone && mode <= eComplete) {
        mMode = Mode(mode);
      }
      mIsModeSet = PR_TRUE;
    }
    return mMode;
  }

  void SetMode(Mode aMode)
  {
    mMode      = aMode;
    mIsDirty   = PR_TRUE;
    mIsModeSet = PR_TRUE;
  }

  PRUint32 GetTotal()
  {
    if (!mIsTotalSet) {
      nsresult rv;
      PRInt32 total =
        Substring(mValue, mFirstPipe + 1, mSecondPipe - mFirstPipe).ToInteger(&rv);
      if (NS_SUCCEEDED(rv) && total >= 0) {
        mTotal = total;
      }
      mIsTotalSet = PR_TRUE;
    }
    return mTotal;
  }

  void SetTotal(PRUint32 aTotal)
  {
    mTotal      = aTotal;
    mIsTotalSet = PR_TRUE;
    mIsDirty    = PR_TRUE;
  }

  PRUint32 GetCurrent()
  {
    if (!mIsCurrentSet) {
      nsresult rv;
      PRInt32 current = Substring(mValue, mSecondPipe + 1).ToInteger(&rv);
      if (NS_SUCCEEDED(rv) && current >= 0) {
        mCurrent = current;
      }
      mIsCurrentSet = PR_TRUE;
    }
    return mCurrent;
  }

  void SetCurrent(PRUint32 aCurrent)
  {
    mCurrent      = aCurrent;
    mIsCurrentSet = PR_TRUE;
    mIsDirty      = PR_TRUE;
  }

  void GetValue(nsAString& aValue)
  {
    if (mIsDirty) {
      aValue.Truncate();
      aValue.AppendInt(GetMode());
      aValue.AppendLiteral("|");
      aValue.AppendInt(GetTotal());
      aValue.AppendLiteral("|");
      aValue.AppendInt(GetCurrent());
    }
    else {
      aValue = mValue;
    }
  }

private:
  PRInt32 mFirstPipe;
  PRInt32 mSecondPipe;
  nsString mValue;

  Mode mMode;
  PRUint32 mTotal;
  PRUint32 mCurrent;

  PRPackedBool mIsDirty;
  PRPackedBool mIsModeSet;
  PRPackedBool mIsTotalSet;
  PRPackedBool mIsCurrentSet;
};

#endif /* __SBDOWNLOADBUTTONPROPERTYINFO_H__ */

