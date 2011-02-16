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
 * \file  sbPlaylistCommandsHelper.h
 * \brief Songbird PlaylistCommandsHelper Component Definition.
 */

#ifndef __SBIDENTITY_SERVICE_H__
#define __SBIDENTITY_SERVICE_H__

#include "sbIIdentityService.h"

#include <nsIComponentManager.h>
#include <nsIGenericFactory.h>
#include <nsCOMArray.h>
#include <nsStringAPI.h>
#include <sbIMediaItem.h>

// DEFINES ====================================================================
#define SONGBIRD_IDENTITY_SERVICE_CONTRACTID            \
  "@songbirdnest.com/Songbird/IdentityService;1"
#define SONGBIRD_IDENTITY_SERVICE_CLASSNAME             \
  "Songbird Unique Identifier Service"
#define SONGBIRD_IDENTITY_SERVICE_CID                   \
{ /* 94717ce5-adb7-4661-8044-530c4b4330ce */            \
  0x94717ce5, 0xadb7, 0x4661,                           \
  { 0x80, 0x44, 0x53, 0x0c, 0x4b, 0x43, 0x30, 0xce }    \
}
// CLASSES ====================================================================
class sbIdentityService : public sbIIdentityService
{
NS_DECL_ISUPPORTS
NS_DECL_SBIIDENTITYSERVICE
public:

  sbIdentityService();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

private:
  virtual ~sbIdentityService();

  // internal method to generate an md5 hash of the param aString
  nsresult HashString(const nsAString  &aString,
                      nsAString        &_retval);

protected:
  /* additional members */
};
#endif // __SBIDENTITY_SERVICE_H__
