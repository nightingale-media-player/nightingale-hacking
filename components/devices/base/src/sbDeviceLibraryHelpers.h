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

/**
 * \file Listener helpers for sbDeviceLibrary
 */
#ifndef __SBDEVICELIBRARYHELPERS_H__
#define __SBDEVICELIBRARYHELPERS_H__

#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#include <sbILocalDatabaseSmartMediaList.h>
#include <sbIMediaListListener.h>
#include <sbLibraryUtils.h>

class sbILibrary;
class sbIMediaList;

class sbDeviceLibrary;

/**
  bool mSyncPlaylists;
 * Listens to item update events to propagate into a second library
 * The target library must register this listener before going away.
 */
class sbLibraryUpdateListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbLibraryUpdateListener(sbILibrary * aTargetLibrary,
                          bool aIgnorePlaylists,
                          sbIDevice * aDevice);

protected:
  virtual ~sbLibraryUpdateListener();

  /**
   * The target library holds a reference to us (to unregister), so it is
   * safe to not own a reference
   */
  sbILibrary* mTargetLibrary;
  bool mIgnorePlaylists;
  sbIDevice* mDevice;
};

#endif
