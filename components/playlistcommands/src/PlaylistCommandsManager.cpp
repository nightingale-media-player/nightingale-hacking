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

/** 
* \file  PlaylistCommands.cpp
* \brief Songbird PlaylistCommandsManager Component Implementation.
*/

#include "nscore.h"

#include "nspr.h"
#include "nsCOMPtr.h"
#include "rdf.h"

#include "nsIEnumerator.h"
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

#include "PlaylistCommandsManager.h"

#define MODULE_SHORTCIRCUIT 0

#if MODULE_SHORTCIRCUIT
# define METHOD_SHORTCIRCUIT return NS_OK;
# define VMETHOD_SHORTCIRCUIT return;
#else
# define METHOD_SHORTCIRCUIT
# define VMETHOD_SHORTCIRCUIT
#endif

static  CPlaylistCommandsManager *gPlaylistCommandsManager = nsnull;

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
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommands(commandmap_t *map,
                                         const nsAString     &aContextGUID,
                                         const nsAString     &aPlaylistType,
                                         sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(aCommandObj);

  METHOD_SHORTCIRCUIT;

  // Yes, I'm copying strings 1 time too many here.  I can live with that.
  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  (*map)[type].push_back(aCommandObj);
  (*map)[key].push_back(aCommandObj);

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommands(commandmap_t *map,
                                           const nsAString     &aContextGUID,
                                           const nsAString     &aPlaylistType,
                                           sbIPlaylistCommands *aCommandObj)
{
  NS_ENSURE_ARG_POINTER(aCommandObj);

  METHOD_SHORTCIRCUIT;

  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  PRBool found = PR_FALSE;

  commandmap_t::iterator c = (*map).find(type);
  if (c != (*map).end()) {
    commandlist_t *typelist = &((*c).second);
    if (typelist) {
      for (commandlist_t::iterator ci = typelist->begin(); ci != typelist->end(); ci++) {
        if (*ci == aCommandObj) {
          typelist->erase(ci);
          found = PR_TRUE;
          break;
        }
      }
    }
  }

  c = (*map).find(key);
  if (c != (*map).end()) {
    commandlist_t *keylist = &((*c).second);
    if (keylist) {
      for (commandlist_t::iterator ci = keylist->begin(); ci != keylist->end(); ci++) {
        if (*ci == aCommandObj) {
          keylist->erase(ci);
          found = PR_TRUE;
          break;
        }
      }
    }
  }

  return found ? NS_OK : NS_ERROR_FAILURE;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommands(commandmap_t *map,
                                    const nsAString      &aContextGUID,
                                    const nsAString      &aPlaylistType,
                                    nsISimpleEnumerator  **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;

  nsString key(aContextGUID);
  nsString type(aPlaylistType);
  commandmap_t::iterator c;

  // guid takes precedence over type
  c = (*map).find(key);
  if (c != (*map).end()) {
    commandlist_t *keylist = &((*c).second);
    if (keylist && keylist->size() > 0) {
      nsCOMArray<sbIPlaylistCommands> array;
      for (int i=0;i<keylist->size();i++) {
        nsCOMPtr<sbIPlaylistCommands> cmds;
        (*keylist)[i]->Duplicate(getter_AddRefs(cmds));
        array.AppendObject(cmds);
      }
      return NS_NewArrayEnumerator(_retval, array);
    }
  }

  c = (*map).find(type);
  if (c != (*map).end()) {
    commandlist_t *typelist = &((*c).second);
    if (typelist && typelist->size() > 0) {
      nsCOMArray<sbIPlaylistCommands> array;
      for (int i=0;i<typelist->size();i++) {
        nsCOMPtr<sbIPlaylistCommands> cmds;
        (*typelist)[i]->Duplicate(getter_AddRefs(cmds));
        array.AppendObject(cmds);
      }
      return NS_NewArrayEnumerator(_retval, array);
    }
  }

  *_retval = nsnull;
  return NS_OK;
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommandsMediaList(const nsAString     &aContextGUID,
                                                            const nsAString     &aPlaylistType,
                                                            sbIPlaylistCommands *aCommandObj)
{
  return RegisterPlaylistCommands(&m_MediaListMap, aContextGUID, aPlaylistType, aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommandsMediaList(const nsAString     &aContextGUID,
                                                              const nsAString     &aPlaylistType,
                                                              sbIPlaylistCommands *aCommandObj)
{
  return UnregisterPlaylistCommands(&m_MediaListMap, aContextGUID, aPlaylistType, aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommandsMediaList(const nsAString      &aContextGUID,
                                                       const nsAString      &aPlaylistType,
                                                       nsISimpleEnumerator  **_retval)
{
  return GetPlaylistCommands(&m_MediaListMap, aContextGUID, aPlaylistType, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::RegisterPlaylistCommandsMediaItem(const nsAString     &aContextGUID,
                                                  const nsAString     &aPlaylistType,
                                                  sbIPlaylistCommands *aCommandObj)
{
  return RegisterPlaylistCommands(&m_MediaItemMap, aContextGUID, aPlaylistType, aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::UnregisterPlaylistCommandsMediaItem(const nsAString     &aContextGUID,
                                                    const nsAString     &aPlaylistType,
                                                    sbIPlaylistCommands *aCommandObj)
{
  return UnregisterPlaylistCommands(&m_MediaItemMap, aContextGUID, aPlaylistType, aCommandObj);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::GetPlaylistCommandsMediaItem(const nsAString      &aContextGUID,
                                             const nsAString      &aPlaylistType,
                                             nsISimpleEnumerator  **_retval)
{
  return GetPlaylistCommands(&m_MediaItemMap, aContextGUID, aPlaylistType, _retval);
}


//-----------------------------------------------------------------------------
NS_IMETHODIMP
CPlaylistCommandsManager::Publish(const nsAString     &aCommandsGUID,
                                  sbIPlaylistCommands *aCommandObj)
{
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

