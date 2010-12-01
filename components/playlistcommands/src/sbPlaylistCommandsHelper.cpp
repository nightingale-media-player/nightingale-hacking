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
* \file  sbPlaylistCommandsHelper.cpp
* \brief Songbird PlaylistCommandsHelper Component Implementation.
*/

#include "sbPlaylistCommandsHelper.h"

#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include <nsICategoryManager.h>

#include "nsEnumeratorUtils.h"
#include "nsArrayEnumerator.h"

#include <nsStringGlue.h>
#include <sbStringUtils.h>

#include "PlaylistCommandsManager.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaylistCommandsHelper, sbIPlaylistCommandsHelper)

//-----------------------------------------------------------------------------
sbPlaylistCommandsHelper::sbPlaylistCommandsHelper()
{
}

//-----------------------------------------------------------------------------
sbPlaylistCommandsHelper::~sbPlaylistCommandsHelper()
{
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, createCommandObjectForAction */
NS_IMETHODIMP
sbPlaylistCommandsHelper::CreateCommandObjectForAction
                          (const nsAString                          &aCommandId,
                           const nsAString                          &aLabel,
                           const nsAString                          &aTooltipText,
                           sbIPlaylistCommandsBuilderSimpleCallback *aCallback,
                           sbIPlaylistCommands                      **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, addCommandObjectForType */
NS_IMETHODIMP
sbPlaylistCommandsHelper::AddCommandObjectForType
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, addCommandObjectForGUID */
NS_IMETHODIMP
sbPlaylistCommandsHelper::AddCommandObjectForGUID
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           sbIPlaylistCommands *aCommandObject)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, removeCommandObjectForType */
NS_IMETHODIMP
sbPlaylistCommandsHelper::RemoveCommandObjectForType
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, removeCommandObjectForGUID */
NS_IMETHODIMP
sbPlaylistCommandsHelper::RemoveCommandObjectForGUID
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           sbIPlaylistCommands *aCommandObject)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, getCommandObjectForType */
NS_IMETHODIMP
sbPlaylistCommandsHelper::GetCommandObjectForType
                          (PRUint16             aTargetFlag,
                           const nsAString      &aMediaListType,
                           const nsAString      &aCommandId,
                           sbIPlaylistCommands  **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, getCommandObjectForGUID */
NS_IMETHODIMP
sbPlaylistCommandsHelper::GetCommandObjectForGUID
                          (PRUint16             aTargetFlag,
                           const nsAString      &aMediaListGUID,
                           const nsAString      &aCommandId,
                           sbIPlaylistCommands  **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbPlaylistCommandsHelper::RegisterSelf(nsIComponentManager *aCompMgr,
                                       nsIFile *aPath,
                                       const char *aLoaderStr,
                                       const char *aType,
                                       const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_PLAYLISTCOMMANDSHELPER_CLASSNAME,
                                "service,"
                                SONGBIRD_PLAYLISTCOMMANDSHELPER_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
