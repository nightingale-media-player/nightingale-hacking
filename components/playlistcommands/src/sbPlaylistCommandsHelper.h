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
 * \file  sbPlaylistCommandsHelper.h
 * \brief Songbird PlaylistCommandsHelper Component Definition.
 */

#ifndef __SBPLAYLISTCOMMANDS_HELPER_H__
#define __SBPLAYLISTCOMMANDS_HELPER_H__

#include "sbIPlaylistCommands.h"
#include "sbIPlaylistCommandsHelper.h"

#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>

// DEFINES ====================================================================
#define SONGBIRD_PLAYLISTCOMMANDSHELPER_CONTRACTID                 \
  "@songbirdnest.com/Songbird/PlaylistCommandsHelper;1"
#define SONGBIRD_PLAYLISTCOMMANDSHELPER_CLASSNAME                  \
  "Songbird Playlist Commands Manager Simple Component"
#define SONGBIRD_PLAYLISTCOMMANDSHELPER_CID                        \
{                                                         \
  0xada0a2ac, 0x1dd1, 0x11b2,                             \
  {0xbb, 0x58, 0xce, 0xfe, 0xff, 0x88, 0x4e, 0xd4}        \
}
// CLASSES ====================================================================
class sbPlaylistCommandsHelper : public sbIPlaylistCommandsHelper
{
NS_DECL_ISUPPORTS
NS_DECL_SBIPLAYLISTCOMMANDSHELPER
public:

  sbPlaylistCommandsHelper();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

private:
  virtual ~sbPlaylistCommandsHelper();

  nsresult AddToServicePane(const nsAString     &aMediaListGUID,
                            const nsAString     &aMediaListType,
                            sbIPlaylistCommands *aCommandObject);

  nsresult AddToMediaItemContextMenu(const nsAString     &aMediaListGUID,
                                     const nsAString     &aMediaListType,
                                     sbIPlaylistCommands *aCommandObject);

  nsresult AddToToolbar(const nsAString     &aMediaListGUID,
                        const nsAString     &aMediaListType,
                        sbIPlaylistCommands *aCommandObject);

  nsresult AddCommandObject(PRUint16            aTargetFlags,
                            const nsAString     &aMediaListGUID,
                            const nsAString     &aMediaListType,
                            sbIPlaylistCommands *aCommandObject);

  nsresult SetRemainingFlags(PRUint16            aTargetFlags,
                             sbIPlaylistCommands *aCommandObject);

  nsresult RemoveCommandObject(PRUint16            aTargetFlags,
                               const nsAString     &aMediaListGUID,
                               const nsAString     &aMediaListType,
                               sbIPlaylistCommands *aCommandObject);

  nsresult GetCommandObject(PRUint16             aTargetFlag,
                            const nsAString      &aMediaListGUID,
                            const nsAString      &aMediaListType,
                            const nsAString      &aCommandId,
                            sbIPlaylistCommands  **_retval);

  nsresult GetChildCommandWithId(sbIPlaylistCommands *parentCommand,
                                 const nsAString     &aCommandId,
                                 sbIPlaylistCommands **_retval);

protected:
  /* additional members */
};
#endif // __SBPLAYLISTCOMMANDS_HELPER_H__
