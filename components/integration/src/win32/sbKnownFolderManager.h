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

#ifndef __SB_KNOWNFOLDERMANAGER_H__
#define __SB_KNOWNFOLDERMANAGER_H__

#include "sbIKnownFolderManager.h"

// Mozilla Includes
#include <nsAutoPtr.h>

// Windows Includes
#include <windows.h>

// Forward Definitions
class IKnownFolderManager;

class sbKnownFolderManager : public sbIKnownFolderManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIKNOWNFOLDERMANAGER

  sbKnownFolderManager();

  nsresult Init();
  nsresult GetDefaultDisplayName(const nsAString &aFolderPath, 
                                 nsAString &aDisplayName);

private:
  ~sbKnownFolderManager();

protected:
  HRESULT                       mCOMInitialized;
  nsRefPtr<IKnownFolderManager> mKnownFolderManager;
};

#define SONGBIRD_KNOWN_FOLDER_MANAGER_CONTRACTID          \
  "@songbirdnest.com/Songbird/KnownFolderManager;1"
#define SONGBIRD_KNOWN_FOLDER_MANAGER_CLASSNAME           \
  "Songbird Known Folder Manager"
#define SONGBIRD_KNOWN_FOLDER_MANAGER_CID                 \
{ /*{DACCB32D-CFED-442f-8B6A-698FD7B22315}*/              \
  0xdaccb32d,                                             \
  0xcfed,                                                 \
  0x442f,                                                 \
  { 0x8b, 0x6a, 0x69, 0x8f, 0xd7, 0xb2, 0x23, 0x15 }      \
}

#endif /* __SB_KNOWNFOLDERMANAGER_H__ */
