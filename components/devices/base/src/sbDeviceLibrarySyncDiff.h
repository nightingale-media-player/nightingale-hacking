/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#ifndef __SB_DEVICELIBRARYSYNCDIFF_H__
#define __SB_DEVICELIBRARYSYNCDIFF_H__

#include <nsIClassInfo.h>

#include <sbIDeviceLibrarySyncDiff.h>
#include <sbILibrary.h>
#include <sbILibraryChangeset.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsInterfaceHashtable.h>
#include <nsTArray.h>

#include <sbLibraryChangeset.h>

class sbDeviceLibrarySyncDiff : public sbIDeviceLibrarySyncDiff
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARYSYNCDIFF

  sbDeviceLibrarySyncDiff();

private:
  ~sbDeviceLibrarySyncDiff();

};

#endif /* __SB_DEVICELIBRARYSYNCDIFF_H__ */
