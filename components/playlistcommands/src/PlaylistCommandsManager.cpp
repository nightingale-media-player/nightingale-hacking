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


#define MODULE_SHORTCIRCUIT 0

#if MODULE_SHORTCIRCUIT
# define METHOD_SHORTCIRCUIT return NS_OK;
# define VMETHOD_SHORTCIRCUIT return;
#else
# define METHOD_SHORTCIRCUIT
# define VMETHOD_SHORTCIRCUIT
#endif

// forward declaration
class sbIPlaylistCommandsBuilder;

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
    rootCommand =  iter->second;
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
    }
  }

  if (!key.IsEmpty()) {
    commandobjmap_t::iterator iter = map->find(key);
    if (iter != map->end()) {
      rootCommand = iter->second;
      rv = rootCommand->RemoveCommandObject(aCommandObj);
      NS_ENSURE_SUCCESS(rv, rv);
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

