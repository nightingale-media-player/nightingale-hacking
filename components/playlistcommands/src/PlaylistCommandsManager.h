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
#include "sbIPlaylistCommandsBuilder.h"
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <map>

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
  /* the key string will be a medialist's guid or medialist listType
   * (e.g. "simple") depending on which the caller of register wants to
   * register the sbIPlaylistCommandsBuilder for */
  typedef std::map<nsString, nsCOMPtr<sbIPlaylistCommandsBuilder> > commandobjmap_t;
  commandobjmap_t m_ServicePaneCommandObjMap;
  commandobjmap_t m_PlaylistCommandObjMap;
  std::map<nsString, nsCOMPtr<sbIPlaylistCommands> > m_publishedCommands;

  typedef std::map<nsString, nsCOMArray<sbIPlaylistCommandsListener> > listenermap_t;
  listenermap_t m_ListenerMap;

 /* Find a root playlist command in the param map registered for the param
  * aSearchString.  If no root playlist command is already registered, one
  * is created and registered for aSearchString.  The root command, found or
  * newly created, is then returned.
  *
  *@param map The commandobjmap_t to look for the root command registered for
  *           aSearchString within.  If a command is not found, a new command
  *           is created and added to this map as the value and aSearchString
  *           as key.
  *@param aSearchString The string, probably a medialist type or guid, for
  *                     which the root sbIPlaylistCommands object is desired.
  *@return The root sbIPlaylistCommands object registered in the param map
  *        to the key aSearchString.  If one was not present before this
  *        function was called, this function creates a new root comand, adds
  *        it to the map for the key aSearchString and returns the added command
  *        object
  */
  nsresult FindOrCreateRootCommand(commandobjmap_t            *map,
                                   const nsAString            &aSearchString,
                                   sbIPlaylistCommandsBuilder **_retval);

 /* Register a playlist command for a medialist guid and/or a medialist listType
  * (e.g. "simple" or "smart"). An sbIPlaylistCommands object must be initialized
  * with a unique id before it can be registered.  This function will error if
  * init() is not called on the param aCommandObj before attempting to register
  * it.
  * If null or an empty string is passed as the param guid or type, that param
  * will be ignored.  If both type and guid are null, this command does
  * nothing.  If both a type and guid are specified, the command object will
  * be registered for both.
  *
  *@param map The commandobjmap_t to register the command object to.  The options
  *           are m_PlaylistCommandObjMap for toolbar and media item context menus
  *           and m_ServicePaneCommandObjMap for context menus for service pane
  *           nodes
  *@param aContextGUID The guid of a medialist for which the param command object
  *                    should be registered to.
  *@param aPlaylistType A medialist listType (e.g. "simple") for which the param
  *                     command object should be registered.  Thus appearing
  *                     for all medialists of that type.
  *@param aCommandObj The command object to be registered
  */
  NS_IMETHODIMP RegisterPlaylistCommands(commandobjmap_t     *map,
                                         const nsAString     &aContextGUID,
                                         const nsAString     &aPlaylistType,
                                         sbIPlaylistCommands *aCommandObj);

 /* Unregister a playlist command for a medialist guid and/or a medialist listType
  * (e.g. "simple" or "smart"). If null or an empty string is passed as the
  * param guid or type, that param will be ignored.  If both type and guid
  * are null, this command does nothing.  If both a type and guid are specified,
  * the command object will be unregistered from both.
  *
  *@param map The commandobjmap_t to unregister the command object from.  The options
  *           are m_PlaylistCommandObjMap for toolbar and media item context menus
  *           and m_ServicePaneCommandObjMap for context menus for service pane
  *           nodes
  *@param aContextGUID The guid of a medialist for which the param command object
  *                    should be unregistered from.
  *@param aPlaylistType A medialist listType (e.g. "simple") for which the param
  *                     command object should be unregistered.
  *@param aCommandObj The command object to be unregistered
  */
  NS_IMETHODIMP UnregisterPlaylistCommands(commandobjmap_t   *map,
                                           const nsAString     &aContextGUID,
                                           const nsAString     &aPlaylistType,
                                           sbIPlaylistCommands *aCommandObj);

 /* Get the root sbIPlaylistCommands object for a medialist guid or type
  * (e.g. "simple" or "smart").  If null or an empty string is passed as the
  * param guid or type, that param will be ignored.  If both type and guid
  * are null, this command does nothing and will return null. If both a type
  * and guid are specified, the guid object is returned first, if present,
  * then the type is searched for
  *
  *@param map The commandobjmap_t to retrieve the command object from.  The options
  *           are m_PlaylistCommandObjMap for toolbar and media item context menus
  *           and m_ServicePaneCommandObjMap for context menus for service pane
  *           nodes
  *@param aContextGUID The guid of a medialist for which the root command object
  *                    should be retrieved.
  *@param aPlaylistType A medialist listType (e.g. "simple") for which the root
  *                     command object should be retrieved.
  *@return The root command object found for the guid or type specified.
  *        Returns nsnull if not found.
  */
  NS_IMETHODIMP GetPlaylistCommands(commandobjmap_t      *map,
                                    const nsAString      &aContextGUID,
                                    const nsAString      &aPlaylistType,
                                    sbIPlaylistCommands  **_retval);

 /* Finds all root playlist commands that correspond to the param aContextGUID
  * and/or aContextType, representing a medialist guid or listType respectively.
  * There are two root sbIPlaylistCommands object for each medialist guid and
  * type.  One is the container for all servicepane menu commands, in the
  * m_ServicePaneCommandObjMap, while the other is for mediaitem context menu
  * and toolbar commands, stored in m_PlaylistCommandObjMap.
  *
  * If null or an empty string is provided as one of the params, guid or type,
  * that param is ignored and only those root commands for the provided param
  * are returned.
  *
  *@param aContextGUID The guid of a medialist for which the root command objects
  *                    should be retrieved.
  *@param aPlaylistType A medialist listType (e.g. "simple") for which the root
  *                     command objects should be retrieved.
  *@return An enumerator containing all root sbIPlaylistCommands objects that
  *        correspond to the provided aContextGuid and/or aContextType
  */
  nsresult FindAllRootCommands(const nsAString &aContextGUID,
                               const nsAString &aContextType,
                               nsISimpleEnumerator **_retval);

 /* Finds all root sbIPlaylistCommands objects for the param medialist guid
  * and/or type and removes the param listener from those root commands.
  *
  * If null or an empty string is provided as one of guid or type, that param
  * is ignored and the listener will only be removed from those root commands
  * found for the provided param
  *
  *@param aContextGUID The guid of a medialist for which the root command objects
  *                    should have the param aListener removed.
  *@param aPlaylistType A medialist listType (e.g. "simple") for which the root
  *                     command objects should have the param aListener removed.
  *@param aListener The listener to be removed
  */

  nsresult RemoveListenerFromRootCommands(const nsString     &aContextGUID,
                                          const nsString     &aPlaylistType,
                                          sbIPlaylistCommandsListener *aListener);

 /* Removes the param aListener that is mapped to the key aSearchString in
  * the map of saved listeners
  *
  *@param aSearchString The key in m_ListenerMap for which aListener should
  *                     be removed as a saved listener.
  *@param aListener The listener to be removed
  */
  nsresult  RemoveListenerInListenerMap(const nsString     &aSearchString,
                                        sbIPlaylistCommandsListener *aListener);

};

#endif // __PLAYLISTCOMMANDS_MANAGER_H__


