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
#include <sbDebugUtils.h>

#define SB_MENU_HOST "menu"
#define SB_TOOLBAR_HOST "toolbar"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaylistCommandsHelper,
                              sbIPlaylistCommandsHelper)

//-----------------------------------------------------------------------------
/*  Class used to specify a visibility callback function that makes a playlist
 *  command only visible in the location specified by the param aVisibleHost
 */
class sbPlaylistCommandsVisibility : public sbIPlaylistCommandsBuilderPCCallback
{
  private:
    nsString mVisibleHost;

  public:
    NS_DECL_ISUPPORTS;
    NS_DECL_SBIPLAYLISTCOMMANDSBUILDERPCCALLBACK;
    sbPlaylistCommandsVisibility(const char* aVisibleHost);
    virtual ~sbPlaylistCommandsVisibility();
};

sbPlaylistCommandsVisibility::sbPlaylistCommandsVisibility
                              (const char* aVisibleHost)
{
  mVisibleHost = NS_ConvertASCIItoUTF16(aVisibleHost);
}

/* virtual */ sbPlaylistCommandsVisibility::~sbPlaylistCommandsVisibility()
{
}

NS_IMETHODIMP
sbPlaylistCommandsVisibility::HandleCallback
                              (sbIPlaylistCommandsBuilderContext *aContext,
                               const nsAString &aHost,
                               const nsAString &aData,
                               PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aContext);
  NS_ENSURE_ARG_POINTER(_retval);
  nsString host(aHost);
  *_retval = (host.Equals(mVisibleHost));
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaylistCommandsVisibility,
                              sbIPlaylistCommandsBuilderPCCallback);

//-----------------------------------------------------------------------------
sbPlaylistCommandsHelper::sbPlaylistCommandsHelper()
{
  SB_PRLOG_SETUP(sbPlaylistCommandsHelper);
}

//-----------------------------------------------------------------------------
/* virtual */ sbPlaylistCommandsHelper::~sbPlaylistCommandsHelper()
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
                         sbIPlaylistCommandsBuilder               **_retval)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(!aCommandId.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(!aLabel.IsEmpty(), NS_ERROR_INVALID_ARG);

  TRACE_FUNCTION("creating commandobject id:\'%s\', label:\'%s\'",
                 NS_ConvertUTF16toUTF8(aCommandId).get(),
                 NS_ConvertUTF16toUTF8(aLabel).get());

  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsBuilder> newCommand =
    do_CreateInstance("@songbirdnest.com/Songbird/PlaylistCommandsBuilder;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // set the id of the newly created playlist command
  rv = newCommand->Init(aCommandId);
  NS_ENSURE_SUCCESS(rv, rv);

  // append an action to the created playlist command with the same id
  rv = newCommand->AppendAction(SBVoidString(),
                                aCommandId,
                                aLabel,
                                aTooltipText,
                                aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = newCommand);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Helper method that handles adding command objects to a medialist's
 * root commands such that it shows up for the servicepane menu for the
 * applicable medialist(s)
 * This method was implemented with the intention that one of aMediaListGUID
 * or aMediaListType is null */
nsresult
sbPlaylistCommandsHelper::AddToServicePane(const nsAString     &aMediaListGUID,
                                           const nsAString     &aMediaListType,
                                           sbIPlaylistCommands *aCommandObject)
{
  /* When adding a playlist command to be displayed in the service pane menu
   * the aCommandObject param is duplicated and the duplicate is registered
   * to the service pane menu.
   * This is done because RegisterPlaylistCommandsMediaList
   * (the function to add a playlist command to a service pane menu)
   * registers the param command object in a separate, servicepane-only map
   * (distinct from that used by RegisterPlaylistCommandsMediaItem).
   * The duplication means that no object added through this API should be
   * present in both maps, hopefully making modifications more obvious and
   * side effects less common.
   */

  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistCommands> dupCommand;
  rv = aCommandObject->Duplicate(getter_AddRefs(dupCommand));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dupCommand->SetTargetFlags
                   (sbIPlaylistCommandsHelper::TARGET_SERVICEPANE_MENU);
  NS_ENSURE_SUCCESS(rv, rv);

  // RegisterPlaylistCommandsMediaList is for the service pane
  rv = cmdsMgr->RegisterPlaylistCommandsMediaList(aMediaListGUID,
                                                  aMediaListType,
                                                  dupCommand);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Helper method that handles adding command objects to a medialist's
 * root commands such that it shows up in the context menu for mediaitems in
 * the applicable medialist(s)
 * This method was implemented with the intention that one of aMediaListGUID
 * or aMediaListType is null */
nsresult
sbPlaylistCommandsHelper::AddToMediaItemContextMenu
                          (const nsAString     &aMediaListGUID,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistCommandsBuilder> commandBuilder =
                                       do_QueryInterface(aCommandObject, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // tell aCommandObject, commandBuilder, that it will be in the context menu
  rv = commandBuilder->SetTargetFlags
                       (sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU);
  NS_ENSURE_SUCCESS(rv, rv);

  // set aCommandObject, commandBuilder, to only be visible in the context menu
  rv = commandBuilder->SetVisibleCallback
                       (new sbPlaylistCommandsVisibility(SB_MENU_HOST));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cmdsMgr->RegisterPlaylistCommandsMediaItem(aMediaListGUID,
                                                  aMediaListType,
                                                  commandBuilder);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Helper method that handles adding command objects to a medialist's
 * root commands such that it shows up in the toolbar for
 * the applicable medialist(s)
 * This method was implemented with the intention that one of aMediaListGUID
 * or aMediaListType is null */
nsresult
sbPlaylistCommandsHelper::AddToToolbar(const nsAString     &aMediaListGUID,
                                       const nsAString     &aMediaListType,
                                       sbIPlaylistCommands *aCommandObject)
{
  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistCommandsBuilder> commandBuilder =
                                       do_QueryInterface(aCommandObject, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // tell aCommandObject, now commandBuilder, that it will be in the toolbar
  rv = commandBuilder->SetTargetFlags
                       (sbIPlaylistCommandsHelper::TARGET_TOOLBAR);
  NS_ENSURE_SUCCESS(rv, rv);

  // set aCommandObject, now commandBuilder, to only be visible in the toolbar
  rv = commandBuilder->
       SetVisibleCallback(new sbPlaylistCommandsVisibility(SB_TOOLBAR_HOST));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cmdsMgr->RegisterPlaylistCommandsMediaItem(aMediaListGUID,
                                                  aMediaListType,
                                                  commandBuilder);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Helper method that handles adding command objects to a medialist's
 * root commands.
 * This method was implemented with the intention that one of aMediaListGUID
 * or aMediaListType is null, and the command object is then registered to
 * only one of the root commands (type and guid have their own root commands).
 * While this function could work with both params (it would register a command
 * object with both guid and type root commands), it is not recommended as it
 * creates great ambiguity when modifying/removing a command object as changes
 * to a command object registered to guid would be reflected in changes to that
 * same command object but displayed by type (and vice versa).
 */
nsresult
sbPlaylistCommandsHelper::AddCommandObject
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;

  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // check if servicepane menu is a target
  if (aTargetFlags &
      sbIPlaylistCommandsHelper::TARGET_SERVICEPANE_MENU)
  {
    rv = AddToServicePane(aMediaListGUID, aMediaListType, aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // check if both the mediaitem context menu and the toolbar are targets
  if ((aTargetFlags &
       sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU) &&
      (aTargetFlags &
       sbIPlaylistCommandsHelper::TARGET_TOOLBAR))
  {
    /* We handle additions to both the context menu and toolbar as a special
     * case because, if an object is to be displayed in both these places, it
     * doesn't require a visibility callback.  That callback defaults to true,
     * meaning the command is displayed in all places possible, which for
     * RegisterPlaylistCommandsMediaItem are the toolbar and context menu */
    unsigned short mediaItemMenuAndToolbar =
                   sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU |
                   sbIPlaylistCommandsHelper::TARGET_TOOLBAR;

    rv = aCommandObject->SetTargetFlags(mediaItemMenuAndToolbar);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cmdsMgr->RegisterPlaylistCommandsMediaItem(aMediaListGUID,
                                                    aMediaListType,
                                                    aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aTargetFlags &
           sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU)
  {
    // only the mediaitem context menu is a target for this playlist command
    rv = AddToMediaItemContextMenu(aMediaListGUID, aMediaListType, aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aTargetFlags &
           sbIPlaylistCommandsHelper::TARGET_TOOLBAR)
  {
    // only the toolbar is a target location for this playlist command
    rv = AddToToolbar(aMediaListGUID, aMediaListType, aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, addCommandObjectForType */
NS_IMETHODIMP
sbPlaylistCommandsHelper::AddCommandObjectForType
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;

#if PR_LOGGING
  {
    nsString id;
    rv = aCommandObject->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE_FUNCTION("adding command with id \'%s\' to medialist type \'%s\'",
                   NS_ConvertUTF16toUTF8(id).get(),
                   NS_ConvertUTF16toUTF8(aMediaListType).get());
  }
#endif /* PR_LOGGING */

  rv = AddCommandObject(aTargetFlags,
                        SBVoidString(),
                        aMediaListType,
                        aCommandObject);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, addCommandObjectForGUID */
NS_IMETHODIMP
sbPlaylistCommandsHelper::AddCommandObjectForGUID
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;

#if PR_LOGGING
  {
    nsString id;
    rv = aCommandObject->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE_FUNCTION("adding command with id \'%s\' to medialist guid \'%s\'",
                   NS_ConvertUTF16toUTF8(id).get(),
                   NS_ConvertUTF16toUTF8(aMediaListGUID).get());
  }
#endif /* PR_LOGGING */

  rv = AddCommandObject(aTargetFlags,
                        aMediaListGUID,
                        SBVoidString(),
                        aCommandObject);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Class used to wrap a visibility callback function so that it will return
 * false (not be visibile) for the host string (aHiddenHost) provided in the
 * constructor and return the normal visibilty for all other cases.
 */
class sbPlaylistCommandsHidden : public sbIPlaylistCommandsBuilderPCCallback
{
  public:
    NS_DECL_ISUPPORTS;
    NS_DECL_SBIPLAYLISTCOMMANDSBUILDERPCCALLBACK;

    sbPlaylistCommandsHidden(const char* aHiddenHost,
                             sbIPlaylistCommandsBuilderPCCallback *aCallback);
    virtual ~sbPlaylistCommandsHidden();
  private:
    nsString mHiddenHost;
    nsCOMPtr<sbIPlaylistCommandsBuilderPCCallback> mOriginalCallback;
};

sbPlaylistCommandsHidden::sbPlaylistCommandsHidden
                          (const char* aHiddenHost,
                           sbIPlaylistCommandsBuilderPCCallback *aCallback)
{
  mHiddenHost = NS_ConvertASCIItoUTF16(aHiddenHost);
  mOriginalCallback = aCallback;
}

/* virtual */ sbPlaylistCommandsHidden::~sbPlaylistCommandsHidden()
{
}

NS_IMETHODIMP
sbPlaylistCommandsHidden::HandleCallback
                          (sbIPlaylistCommandsBuilderContext *aContext,
                           const nsAString &aHost,
                           const nsAString &aData,
                           PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aContext);
  NS_ENSURE_ARG_POINTER(_retval);
  nsString host(aHost);

  // return false if the host string being checked is the one we want to hide
  if (host.Equals(mHiddenHost))
  {
    // we definitely don't want this to appear in the context menu
    *_retval = PR_FALSE;
  }
  else {
    /* for anything host string other than mHiddenHost, do what the
     * original callback did. */
    nsresult rv = mOriginalCallback->HandleCallback(aContext,
                                                    aHost,
                                                    aData,
                                                    _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaylistCommandsHidden,
                              sbIPlaylistCommandsBuilderPCCallback);

//-----------------------------------------------------------------------------
/* A helper function to adjust aCommandObject's flags following a remove.
 * aCommandObject.targetFlags will not include any of those flags specified by
 * aTargetFlags at the end of this function.  Depending on our knowledge of
 * aCommandObject's location (through aCommandObject.targetFlags at this
 * function's start), however, aCommandObject.targetFlags may or may not
 * accurately reflect aCommandObject's actual location at the end of this
 * function, but it will always accurately reflect where this function is _not_.
 */
nsresult
sbPlaylistCommandsHelper::SetRemainingFlags
                          (PRUint16            aTargetFlags,
                           sbIPlaylistCommands *aCommandObject)
{
  PRUint16 commandsLocation;
  nsresult rv = aCommandObject->GetTargetFlags(&commandsLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!commandsLocation || commandsLocation == 0)
  {
    /* The command object doesn't know where it is, but we know where it is not
     * Set the remaining flags to be the opposite of those that we removed,
     * but also only set those flags that we actually use (under the umbrella
     * of TARGET_ALL) */
    commandsLocation = (~aTargetFlags) & sbPlaylistCommandsHelper::TARGET_ALL;
  }
  else {
    /* The command object knows where it was, so we'll use that info to update.
     * Set the remaining flags to be those that the object was present in
     * that are not the ones that it was removed from.
     */
     commandsLocation = commandsLocation & (~aTargetFlags);
  }
  rv = aCommandObject->SetTargetFlags(commandsLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* Helper method that handles removing command objects from a medialist's
 * root commands.
 * This method was implemented with the intention that one of aMediaListGUID
 * or aMediaListType is null, and the command object is then only removed from
 * one of the root commands (type and guid have their own root commands).
 * While this function could work with both params (it would unregister
 * aCommandObject from both guid and type root commands), it is not recommended.
 */
nsresult
sbPlaylistCommandsHelper::RemoveCommandObject
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool removeFromServicePane =
         (aTargetFlags &
          sbIPlaylistCommandsHelper::TARGET_SERVICEPANE_MENU);

  PRBool removeFromContextMenu =
         (aTargetFlags &
          sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU);

  PRBool removeFromToolbar =
         (aTargetFlags &
          sbIPlaylistCommandsHelper::TARGET_TOOLBAR);

  // if we want to remove from the service pane, we can do that now
  if (removeFromServicePane)
  {
    /* Command objects registered to the service pane are distinct from those
     * registered to the context menu and/or toolbar as they are stored in their
     * own, servicepane-only map.  Thus, if we want to remove from the service
     * pane, we can do that fairly straightforwardly */

    // UnregisterPlaylistCommandsMediaList deals with the service pane
    rv = cmdsMgr->UnregisterPlaylistCommandsMediaList(aMediaListGUID,
                                                      aMediaListType,
                                                      aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
    // unregister does it's own onCommandRemoved signal, so no need here
  }

  // if we want to remove from the context menu and toolbar, we can do that now
  if (removeFromContextMenu && removeFromToolbar)
  {
    /* Removing from both the context menu and toolbar is fairly straightforward
     * because commands registered to those locations are stored in the same
     * map.  Thus, if we know we want to remove aCommandObject from both
     * the toolbar and context menu, we can just unregister the command from
     * that map */

    rv = cmdsMgr->UnregisterPlaylistCommandsMediaItem(aMediaListGUID,
                                                      aMediaListType,
                                                      aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
    // unregister does it's own onCommandRemoved signal, so no need here

    rv = SetRemainingFlags(aTargetFlags, aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

 /* We've already handled the service pane, and we don't want to remove from the
  * context menu or the toolbar, so we are done. */
  if (!removeFromContextMenu && !removeFromToolbar) {
    rv = SetRemainingFlags(aTargetFlags, aCommandObject);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  // at this point, we are targetting one of the context menu and toolbar
  PRUint16 commandsLocation;
  rv  = aCommandObject->GetTargetFlags(&commandsLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!commandsLocation || commandsLocation == 0)
  {
   /* The command object being removed does not know where it is.
    * This is most likely because aCommandObject was added with the old,
    * non-helper API that doesn't use these locations.
    * This is the bad case.  We know we want to remove the command from only
    * 1 of context menu and toolbar, but we can't be sure how to proceed.
    * Potentially, the command is present only in the place that we want
    * to remove it from.  In that case, we could safely unregister it.  We
    * don't, however, know where the command is. So we cannot safely unregister
    *
    * Rather, we assume that the command is present in both the context menu
    * and toolbar, and because we only want to remove it from one we must
    * adjust the visibility callback accordingly.
    */
    nsCOMPtr<sbIPlaylistCommandsBuilderPCCallback> oldCallback;
    rv = aCommandObject->GetVisibleCallback(getter_AddRefs(oldCallback));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPlaylistCommandsBuilder> commandBuilder =
                                         do_QueryInterface(aCommandObject,
                                                           &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* if we want to remove the command from the context menu, set our
     * visibility callback to be hidden in the context menu.  Otherwise,
     * we want this to be removed from the toolbar, so make it hidden there */
    nsCOMPtr<sbPlaylistCommandsHidden> visCallback =
    (removeFromContextMenu ? new sbPlaylistCommandsHidden(SB_MENU_HOST,
                                                          oldCallback) :
                             new sbPlaylistCommandsHidden(SB_TOOLBAR_HOST,
                                                          oldCallback));
    rv = commandBuilder->SetVisibleCallback(visCallback);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = commandBuilder->NotifyListeners
                         (NS_LITERAL_STRING("onCommandChanged"),
                          commandBuilder);
  }
  else {
    // The command object being removed knows where it is and will help us.
    // The service pane was handled first, so we don't worry about that here.
    PRBool isInContextMenu =
           (commandsLocation &
            sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU);

    PRBool isInToolbar =
           (commandsLocation &
            sbIPlaylistCommandsHelper::TARGET_TOOLBAR);

    if (isInContextMenu && isInToolbar)
    {
      /* the command object is in two places and we want to remove it from one
       * so we need to change it's visibility callback */
      nsCOMPtr<sbIPlaylistCommandsBuilder> commandBuilder =
                                           do_QueryInterface(aCommandObject,
                                                             &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      /* if we are removing from the context menu, set the visibilty to show
       * only in the toolbar.  Otherwise, we are removing from the toolbar so
       * set the visibility to only show in the context menu */
      nsCOMPtr<sbPlaylistCommandsVisibility> visCallback =
      (removeFromContextMenu ? new sbPlaylistCommandsVisibility(SB_TOOLBAR_HOST) :
                               new sbPlaylistCommandsVisibility(SB_MENU_HOST));

      rv = commandBuilder->SetVisibleCallback(visCallback);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = commandBuilder->NotifyListeners
                           (NS_LITERAL_STRING("onCommandChanged"),
                            commandBuilder);
      NS_ENSURE_SUCCESS(rv, rv);

    }
    else if ((isInContextMenu && removeFromContextMenu) ||
             (isInToolbar && removeFromToolbar)) {
      /* the command object is present in one of the context menu or toolbar
       * and we want to remove it from there, so we can unregister it */
      rv = cmdsMgr->UnregisterPlaylistCommandsMediaItem(aMediaListGUID,
                                                        aMediaListType,
                                                        aCommandObject);
      NS_ENSURE_SUCCESS(rv, rv);
      // unregister does it's own onCommandRemoved signal, so no need here
    }
  }
  rv = SetRemainingFlags(aTargetFlags, aCommandObject);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, removeCommandObjectForType */
NS_IMETHODIMP
sbPlaylistCommandsHelper::RemoveCommandObjectForType
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListType,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;

#if PR_LOGGING
  {
    nsString id;
    rv = aCommandObject->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE_FUNCTION("removing object with id \'%s\' from medialist type \'%s\'",
                   NS_ConvertUTF16toUTF8(id).get(),
                   NS_ConvertUTF16toUTF8(aMediaListType).get());
  }
#endif /* PR_LOGGING */

  rv = RemoveCommandObject(aTargetFlags,
                           SBVoidString(),
                           aMediaListType,
                           aCommandObject);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*  sbIPlaylistCommandsHelper.idl, removeCommandObjectForGUID */
NS_IMETHODIMP
sbPlaylistCommandsHelper::RemoveCommandObjectForGUID
                          (PRUint16            aTargetFlags,
                           const nsAString     &aMediaListGUID,
                           sbIPlaylistCommands *aCommandObject)
{
  NS_ENSURE_ARG_POINTER(aCommandObject);
  nsresult rv;

#if PR_LOGGING
  {
    nsString id;
    rv = aCommandObject->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE_FUNCTION("removing object with id \'%s\' from medialist guid \'%s\'",
                   NS_ConvertUTF16toUTF8(id).get(),
                   NS_ConvertUTF16toUTF8(aMediaListGUID).get());
  }
#endif /* PR_LOGGING */

  TRACE_FUNCTION("");
  rv = RemoveCommandObject(aTargetFlags,
                           aMediaListGUID,
                           SBVoidString(),
                           aCommandObject);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* A helper function to search a sbIPlaylistCommands object, aParentCommand,
 * for a command with an id of aCommandId and return it */
nsresult
sbPlaylistCommandsHelper::GetChildCommandWithId
                          (sbIPlaylistCommands *aParentCommand,
                           const nsAString     &aCommandId,
                           sbIPlaylistCommands **_retval)
{
  NS_ENSURE_ARG_POINTER(aParentCommand);
  NS_ENSURE_ARG_POINTER(_retval);

  nsString commandId(aCommandId);
  nsCOMPtr<nsISimpleEnumerator> cmdEnum;

  /* loop through the children of the parent command to look for one with the
   * param id */
  nsresult rv = aParentCommand->GetChildrenCommandObjects
                                (getter_AddRefs(cmdEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(cmdEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<sbIPlaylistCommands> next;
    if (NS_SUCCEEDED(cmdEnum->GetNext(getter_AddRefs(next))) && next) {
      nsString aId;
      rv = next->GetId(aId);
      NS_ENSURE_SUCCESS(rv, rv);

      // return the first playlist command with the param id
      if (aId == commandId) {
        NS_ADDREF(*_retval = next);
        return NS_OK;
      }
    }
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* A helper function to search the root sbIPlaylistCommands for a medialist
 * guid or type and return an sbIPlaylistCommands object with the param
 * aCommandId.  This method was implemented with the intention that one of
 * aMediaListGUID or aMediaListType is null, and then only the root commands
 * for the param provided are searched.
 * This function can be used with both aMediaListGUID and aMediaListType
 * params passed, and both guid and type root commands will be searched,
 * but it is not recommended.  If both params are specified, guid root
 * commands will be searched first, followed by those for type, and the first
 * match is returned.
 */
nsresult
sbPlaylistCommandsHelper::GetCommandObject(PRUint16             aTargetFlag,
                                           const nsAString      &aMediaListGUID,
                                           const nsAString      &aMediaListType,
                                           const nsAString      &aCommandId,
                                           sbIPlaylistCommands  **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<sbIPlaylistCommandsManager> cmdsMgr =
    do_GetService(SONGBIRD_PlaylistCommandsManager_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaylistCommands> foundCommand = nsnull;
  nsCOMPtr<sbIPlaylistCommands> rootCommand;

  if (aTargetFlag &
      sbIPlaylistCommandsHelper::TARGET_SERVICEPANE_MENU)
  {
    // servicepane menu is target
    rv = cmdsMgr->GetPlaylistCommandsMediaList(aMediaListGUID,
                                               aMediaListType,
                                               getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (aTargetFlag &
           (sbIPlaylistCommandsHelper::TARGET_MEDIAITEM_CONTEXT_MENU |
            sbIPlaylistCommandsHelper::TARGET_TOOLBAR)) {
    // mediaitem context menu or toolbar is target
    rv = cmdsMgr->GetPlaylistCommandsMediaItem(aMediaListGUID,
                                               aMediaListType,
                                               getter_AddRefs(rootCommand));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (rootCommand)
  {
    rv = GetChildCommandWithId(rootCommand,
                               aCommandId,
                               getter_AddRefs(foundCommand));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  /* We found a command, but we need to make sure that it's actually where the
   * caller asked us to look for it, i.e. in a place included in aTargetFlags.
   *
   * We'll try to get the targetFlags of our found command to confirm, but
   * it is possible that the command won't have them.  So check them if we can,
   * but if it doesn't have the flags just return it.
   */
  if (foundCommand)
  {
    PRUint16 foundFlags;
    rv = foundCommand->GetTargetFlags(&foundFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    if (foundFlags > 0  && (foundFlags & aTargetFlag) == 0)
    {
      foundCommand = nsnull;
    }
  }
  NS_IF_ADDREF(*_retval = foundCommand);
  return NS_OK;
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
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  TRACE_FUNCTION("getting object with id \'%s\' from medialist type \'%s\'",
                 NS_ConvertUTF16toUTF8(aCommandId).get(),
                 NS_ConvertUTF16toUTF8(aMediaListType).get());

  nsCOMPtr<sbIPlaylistCommands> foundCommand;
  rv = GetCommandObject(aTargetFlag,
                        SBVoidString(),
                        aMediaListType,
                        aCommandId,
                        getter_AddRefs(foundCommand));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_retval = foundCommand);
  return NS_OK;
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
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  TRACE_FUNCTION("getting object with id \'%s\' from medialist guid \'%s\'",
                 NS_ConvertUTF16toUTF8(aCommandId).get(),
                 NS_ConvertUTF16toUTF8(aMediaListGUID).get());

  nsCOMPtr<sbIPlaylistCommands> foundCommand;
  rv = GetCommandObject(aTargetFlag,
                        aMediaListGUID,
                        SBVoidString(),
                        aCommandId,
                        getter_AddRefs(foundCommand));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_retval = foundCommand);
  return NS_OK;
}

