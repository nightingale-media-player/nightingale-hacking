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

#ifndef SBDEVICESUPPORTSITEMHELPER_H_
#define SBDEVICESUPPORTSITEMHELPER_H_

#include "sbIJobProgress.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>

class sbIDeviceSupportsItemCallback;
class sbIMediaInspector;
class sbIMediaItem;

class sbBaseDevice;

/**
 * Helper class for sbBaseDevice to implement sbIDevice::supportsMediaItem
 *
 * This is the job progress listener code, split out so that sbBaseDevice does
 * not have to itself implement the interface, as that may interfere with
 * derived classes wishing to do so for other purposes.
 */
class sbDeviceSupportsItemHelper : public sbIJobProgressListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIJOBPROGRESSLISTENER

  sbDeviceSupportsItemHelper();

  nsresult Init(sbIMediaItem* aItem,
                sbBaseDevice* aDevice,
                sbIDeviceSupportsItemCallback* aCallback);

  void RunSupportsMediaItem();

  nsresult InitJobProgress(sbIMediaInspector* aInspector,
                           PRUint32 aTranscodeType);

private:
  virtual ~sbDeviceSupportsItemHelper();

protected:
  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIDeviceSupportsItemCallback> mCallback;

  // mDevice can't be a nsRefPtr because it's multiply derived from nsISupports
  // and the two possibilities of AddRef confuses MSVC
  sbBaseDevice* mDevice;

  nsCOMPtr<sbIMediaInspector> mInspector;
  PRUint32 mTranscodeType;
};

#endif /* SBDEVICESUPPORTSITEMHELPER_H_ */
