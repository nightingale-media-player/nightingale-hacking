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
 * \file  PlaylistCommandsManager.h
 * \brief Songbird PlaylistCommandsManager Component Definition.
 */

#ifndef __PLAYLISTCOMMANDS_MANAGER_H__
#define __PLAYLISTCOMMANDS_MANAGER_H__

#include <nsISupportsImpl.h>
#include <nsISupportsUtils.h>
#include <nsIStringBundle.h>
#include <nsStringGlue.h>
#include "sbIPlaylistCommands.h"
#include <nsCOMPtr.h>
#include <map>
#include <set>
#include <vector>

#ifndef NS_DECL_ISUPPORTS
#error
#endif
// DEFINES ====================================================================
#define SONGBIRD_PlaylistCommandsManager_CONTRACTID                 \
  "@songbirdnest.com/Songbird/PlaylistCommandsManager;1"
#define SONGBIRD_PlaylistCommandsManager_CLASSNAME                  \
  "Songbird Playlist Commands Manager Component"
#define SONGBIRD_PlaylistCommandsManager_CID                        \
{ /* f89d8c28-406d-4d4b-9130-ecabe66a71e2 */              \
  0xf89d8c28, 0x406d, 0x4d4b,                             \
  { 0x91, 0x30, 0xec, 0xab, 0xe6, 0x6a, 0x71, 0xe2 }      \
}
// CLASSES ====================================================================
class CPlaylistCommandsManager : public sbIPlaylistCommandsManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPLAYLISTCOMMANDSMANAGER

  CPlaylistCommandsManager();
  virtual ~CPlaylistCommandsManager();

private:
  class commandlist_t : public std::vector< nsCOMPtr<sbIPlaylistCommands> > {};
  class commandmap_t  : public std::map<nsString, commandlist_t > {};
  commandmap_t m_MediaListMap;
  commandmap_t m_MediaItemMap;
  std::map<nsString, nsCOMPtr<sbIPlaylistCommands> > m_publishedCommands;

  NS_IMETHODIMP RegisterPlaylistCommands(commandmap_t *map,
                                         const nsAString     &aContextGUID,
                                         const nsAString     &aPlaylistType,
                                         sbIPlaylistCommands *aCommandObj);
  NS_IMETHODIMP UnregisterPlaylistCommands(commandmap_t *map,
                                         const nsAString     &aContextGUID,
                                         const nsAString     &aPlaylistType,
                                         sbIPlaylistCommands *aCommandObj);
  NS_IMETHODIMP GetPlaylistCommands(commandmap_t *map,
                                    const nsAString      &aContextGUID,
                                    const nsAString      &aPlaylistType,
                                    nsISimpleEnumerator  **_retval);

};

#endif // __PLAYLISTCOMMANDS_MANAGER_H__


