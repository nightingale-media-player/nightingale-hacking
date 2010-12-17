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
* \file  PlaylistCommands.cpp
* \brief Songbird PlaylistCommandsManager Component Implementation.
*/

#include "PlaylistCommandsManager.h"
#include "nscore.h"

#include "nspr.h"
#include "nsCOMPtr.h"
#include "rdf.h"

#include "nsIInterfaceRequestor.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"

#include "nsITimer.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMArray.h"
#include "nsEnumeratorUtils.h"
#include "nsArrayEnumerator.h"

#include "nsCRTGlue.h"
#include <nsStringGlue.h>

#include <sbStringUtils.h>
#include <sbIMediaList.h>

#define MODULE_SHORTCIRCUIT 0

#if MODULE_SHORTCIRCUIT
# define METHOD_SHORTCIRCUIT return NS_OK;
# define VMETHOD_SHORTCIRCUIT return;
#else
# define METHOD_SHORTCIRCUIT
# define VMETHOD_SHORTCIRCUIT
#endif

NS_IMPL_ISUPPORTS1(CPlaylistCommandsManager, sbIPlaylistCommandsManager);

//-----------------------------------------------------------------------------
CPlaylistCommandsManager::CPlaylistCommandsManager()
{
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CPlaylistCommandsManager::~CPlaylistCommandsManager()
{
} //dtor

//-----------------------------------------------------------------------------
nsresult
CPlaylistCommandsManager::FindOrCreateRootCommand(commandobjmap_t *map,
                                                  const nsAString &aSearchString,
                                                  sbIPlaylistCommandsBuilder **_retval)
{
  NS_ENSURE_ARG_POINTER(map);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsString searchString(aSearchString);
  nsCOMPtr<sbIPlaylistCommandsBuilder> rootCommand;
  commandobjmap_t::iterator iter = map->find(searchString);
  if (iter != map->end()) {
    // if we find a root playlistCommands object, return it
    rootCommand = iter->second;
  }
  else {
    // if we can't find a root playlistCommands object, make one
    rootCommand = do_CreateInstance
                  ("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // initialize the new playlistcommand with an id of the search param
    rv = rootCommand->Init(searchString);
    NS_ENSURE_SUCCESS(rv, rv);
    (*map)[searchString] = rootCommand;

    // Check if there are listeners waiting for this root command to be created.
    // If there are, add the listeners to the newly created command.
    listenermap_t::iterator it;
    it = m_ListenerMap.find(searchString);
    if (it != m_ListenerMap.end())
    {
      nsCOMArray<sbIPlaylistCommandsListener> listeners = it->second;
      PRUint32 length = listeners.Count();
      for (PRUint32 i=0; i < length; i++)
      {
        rv = rootCommand->AddListener(listeners[i]);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  NS_ADDREF(*_retval = rootCommand);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommands
                          (commandobjmap_t *map,
                           const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(map);
  NS_ENSURE_ARG_POINTER(aCommandObj);

  METHOD_SHORTCIRCUIT;

  nsString key(aContextGUID);
  nsString type(aPlaylistType);
  nsString id;

  nsresult rv = aCommandObj->GetId(id);
  if ( !NS_SUCCEEDED(rv) || id.IsEmpty() )
  {
    NS_ERROR("PlaylistCommandsManager::Cannot register a playlist command without an id");
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<sbIPlaylistCommandsBuilder> rootCommand;

  // check if the caller gave a type
  if (!type.IsEmpty()) {
    rv = FindOrCreateRootCommand(map, type, getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = rootCommand->AddCommandObject(aCommandObj);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // check if the caller gave a guid
  if (!key.IsEmpty()) {
    rv = FindOrCreateRootCommand(map, key, getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = rootCommand->AddCommandObject(aCommandObj);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommands
                          (commandobjmap_t *map,
                           const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(map);
  NS_ENSURE_ARG_POINTER(aCommandObj);
  nsresult rv;

  METHOD_SHORTCIRCUIT;

  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  nsCOMPtr<sbIPlaylistCommands> rootCommand;

  if (!type.IsEmpty()) {
    commandobjmap_t::iterator iter = map->find(type);
    if (iter != map->end()) {
      rootCommand = iter->second;
      rv = rootCommand->RemoveCommandObject(aCommandObj);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 numCommands;
      rv = rootCommand->GetNumCommands(SBVoidString(),
                                       SBVoidString(),
                                       &numCommands);
      NS_ENSURE_SUCCESS(rv, rv);

      if (numCommands == 0) {
        rv = rootCommand->ShutdownCommands();
        NS_ENSURE_SUCCESS(rv, rv);

        map->erase(iter);
      }
    }
  }

  if (!key.IsEmpty()) {
    commandobjmap_t::iterator iter = map->find(key);
    if (iter != map->end()) {
      rootCommand = iter->second;
      rv = rootCommand->RemoveCommandObject(aCommandObj);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 numCommands;
      rv = rootCommand->GetNumCommands(SBVoidString(),
                                       SBVoidString(),
                                       &numCommands);
      NS_ENSURE_SUCCESS(rv, rv);

      if (numCommands == 0) {
        rv = rootCommand->ShutdownCommands();
        NS_ENSURE_SUCCESS(rv, rv);

        map->erase(iter);
      }
    }
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommands(commandobjmap_t *map,
                                              const nsAString      &aContextGUID,
                                              const nsAString      &aPlaylistType,
                                              sbIPlaylistCommands  **_retval)
{
  NS_ENSURE_ARG_POINTER(map);
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;

  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  nsCOMPtr<sbIPlaylistCommands> rootCommand;

  commandobjmap_t::iterator iterGUID = map->find(key);
  if (iterGUID != map->end())
  {
    // if we find the rootCommand for the guid, return it
    NS_ADDREF(*_retval = iterGUID->second);
    return NS_OK;
  }

  commandobjmap_t::iterator iterType = map->find(type);
  if (iterType != map->end())
  {
    // if we find the rootCommand for the type, return it
    NS_ADDREF(*_retval = iterType->second);
    return NS_OK;
  }

  *_retval = nsnull;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommandsMediaList
                          (const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  return RegisterPlaylistCommands(&m_ServicePaneCommandObjMap,
                                  aContextGUID,
                                  aPlaylistType,
                                  aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommandsMediaList
                          (const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  return UnregisterPlaylistCommands(&m_ServicePaneCommandObjMap,
                                    aContextGUID,
                                    aPlaylistType,
                                    aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommandsMediaList
                          (const nsAString      &aContextGUID,
                           const nsAString      &aPlaylistType,
                           sbIPlaylistCommands  **_retval)
{
  return GetPlaylistCommands(&m_ServicePaneCommandObjMap,
                             aContextGUID,
                             aPlaylistType,
                             _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommandsMediaItem
                          (const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  return RegisterPlaylistCommands(&m_PlaylistCommandObjMap,
                                  aContextGUID,
                                  aPlaylistType,
                                  aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommandsMediaItem
                          (const nsAString     &aContextGUID,
                           const nsAString     &aPlaylistType,
                           sbIPlaylistCommands *aCommandObj)
{
  return UnregisterPlaylistCommands(&m_PlaylistCommandObjMap,
                                    aContextGUID,
                                    aPlaylistType,
                                    aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommandsMediaItem
                          (const nsAString      &aContextGUID,
                           const nsAString      &aPlaylistType,
                           sbIPlaylistCommands  **_retval)
{
  return GetPlaylistCommands(&m_PlaylistCommandObjMap,
                             aContextGUID,
                             aPlaylistType,
                             _retval);
}

//-----------------------------------------------------------------------------
nsresult
CPlaylistCommandsManager::FindAllRootCommands(const nsAString &aContextGUID,
                                              const nsAString &aContextType,
                                              nsISimpleEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsString guid(aContextGUID);
  nsString type(aContextType);
  nsCOMArray<sbIPlaylistCommands> array;
  nsCOMPtr<sbIPlaylistCommands> rootCommand;

  // check if we got a guid to search for
  if (!guid.IsEmpty())
  {
    // check if a root command exists in the medialist map for the guid
    rv = GetPlaylistCommandsMediaList(guid,
                                      SBVoidString(),
                                      getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);

    if (rootCommand)
    {
      // if we find a root command, add it to the array and we'll return it
      array.AppendObject(rootCommand);
    }

    // check if a root command exists in the mediaitem map for the guid
    rv = GetPlaylistCommandsMediaItem(guid,
                                      SBVoidString(),
                                      getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);
    if (rootCommand)
    {
      // if we find a root command, add it to the array and we'll return it
      array.AppendObject(rootCommand);
    }
  }

  // check if we got a type to search for
  if (!type.IsEmpty())
  {
    // check if a root command exists in the medialist map for the type
    rv = GetPlaylistCommandsMediaList(SBVoidString(),
                                      type,
                                      getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);

    if (rootCommand)
    {
      // if we find a root command, add it to the array and we'll return it
      array.AppendObject(rootCommand);
    }

    // check if a root command exists in the mediaitem map for the type
    rv = GetPlaylistCommandsMediaItem(SBVoidString(),
                                      type,
                                      getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);

    if (rootCommand)
    {
      // if we find a root command, add it to the array and we'll return it
      array.AppendObject(rootCommand);
    }
  }

  return NS_NewArrayEnumerator(_retval, array);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::AddListenerForMediaList(sbIMediaList *aMediaList,
                                                  sbIPlaylistCommandsListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsString guid;
  nsString type;

  rv = aMediaList->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aMediaList->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  /* Add listeners to the listener map for guid and type.  This map ensures that
   * if a root command does not exist during this function call, the saved
   * listeners will be added when the root command is created in
   * FindOrCreateRootCommand. */
  m_ListenerMap[guid].AppendObject(aListener);
  m_ListenerMap[type].AppendObject(aListener);

  /* Get all the root commands that are registered for this medialist's
   * guid and type.  This function searches both the mediaitem and medialist
   * maps and returns the found root commands in an enumerator */
  nsCOMPtr<nsISimpleEnumerator> cmdEnum;
  rv = FindAllRootCommands(guid, type, getter_AddRefs(cmdEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // for each root command that we found, add the listener
  PRBool hasMore;
  while (NS_SUCCEEDED(cmdEnum->HasMoreElements(&hasMore)) && hasMore)
  {
    nsCOMPtr<sbIPlaylistCommands> rootCommand;
    if (NS_SUCCEEDED(cmdEnum->GetNext(getter_AddRefs(rootCommand))) && rootCommand)
    {
      // add the param listener to the found root commands
      rv = rootCommand->AddListener(aListener);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
nsresult
CPlaylistCommandsManager::RemoveListenerFromRootCommands
                          (const nsString     &aContextGUID,
                           const nsString     &aPlaylistType,
                           sbIPlaylistCommandsListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  // get all of the root sbIPlaylistCommands that fit the param guid and/or type
  nsCOMPtr<nsISimpleEnumerator> rootCmdEnum;
  nsresult rv = FindAllRootCommands(aContextGUID, aPlaylistType, getter_AddRefs(rootCmdEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // for each root command found, remove the param aListener
  PRBool hasMore;
  while (NS_SUCCEEDED(rootCmdEnum->HasMoreElements(&hasMore)) && hasMore)
  {
    nsCOMPtr<sbIPlaylistCommands> rootCommand;
    if (NS_SUCCEEDED(rootCmdEnum->GetNext(getter_AddRefs(rootCommand))) && rootCommand)
    {
      rv = rootCommand->RemoveListener(aListener);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

nsresult
CPlaylistCommandsManager::RemoveListenerInListenerMap
                          (const nsString     &aSearchString,
                           sbIPlaylistCommandsListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  listenermap_t::iterator foundListeners;
  // get the vector of listeners saved for aSearchString
  foundListeners = m_ListenerMap.find(aSearchString);

  // make sure there are listeners saved for the search string
  if (foundListeners != m_ListenerMap.end())
  {
    // the m_ListenerMap stores an nsCOMArray of listeners that we need to scan
    // to find the listener that we want to remove
    nsCOMArray<sbIPlaylistCommandsListener> *listeners = &foundListeners->second;
    PRUint32 length = listeners->Count();
    for (PRUint32 i=0; i < length; i++)
    {
      if ((*listeners)[i] == aListener)
      {
        listeners->RemoveObjectAt(i);
        i--;
        length--;

        // Check if we removed the last element in that array of listeners.
        // If we did, remove that array from the map.
        if (length == 0)
        {
          m_ListenerMap.erase(foundListeners);
          return NS_OK;
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CPlaylistCommandsManager::RemoveListenerForMediaList(sbIMediaList *aMediaList,
                                                     sbIPlaylistCommandsListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsString guid;
  nsString type;

  rv = aMediaList->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aMediaList->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  /* first remove the listener from the m_ListenerMap.  It was saved for both
   * the medialist's guid and type, so it needs to be removed for both */
  rv = RemoveListenerInListenerMap(guid, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveListenerInListenerMap(type, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  // remove the listener from any root commands that it was attached to
  rv = RemoveListenerFromRootCommands(guid, type, aListener);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::Publish(const nsAString     &aCommandsGUID,
                                  sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(aCommandObj);
  nsString guid(aCommandsGUID);
  if (m_publishedCommands[guid]) return NS_ERROR_FAILURE;
  m_publishedCommands[guid] = aCommandObj;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::Withdraw(const nsAString     &aCommandsGUID,
                                   sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(aCommandObj);
  nsString guid(aCommandsGUID);
  if (m_publishedCommands[guid] != aCommandObj) return NS_ERROR_FAILURE;
  m_publishedCommands.erase(guid);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::Request(const nsAString      &aCommandsGUID,
                                  sbIPlaylistCommands  **_retval)
{
  nsString guid(aCommandsGUID);
  sbIPlaylistCommands *dup = NULL;
  sbIPlaylistCommands *cmds;
  cmds = m_publishedCommands[guid];
  if (cmds) cmds->Duplicate(&dup);
  *_retval = dup;
  return NS_OK;
}
