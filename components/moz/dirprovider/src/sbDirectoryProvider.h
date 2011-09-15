/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef SB_DIRECTORY_PROVIDER_H__
#define SB_DIRECTORY_PROVIDER_H__

#include <nsIDirectoryService.h>

#define NS_WIN_COMMON_DOCUMENTS             "CmDocs"
#define NS_WIN_COMMON_PICTURES              "CmPics"
#define NS_WIN_COMMON_MUSIC                 "CmMusic"
#define NS_WIN_COMMON_VIDEO                 "CmVideo"
#define NS_WIN_DOCUMENTS                    "Docs"
#define NS_WIN_PICTURES                     "Pics"
#define NS_WIN_MUSIC                        "Music"
#define NS_WIN_VIDEO                        "Video"
#define NS_WIN_DISCBURNING                  "DiscBurning"

/**
 * This class implements a Songbird directory service provider, to have
 * directory service return special directories Songbird needs. Mozilla won't
 * add these directories by default (moz bug 431880).
 */

class sbDirectoryProvider : public nsIDirectoryServiceProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER

  sbDirectoryProvider();

  nsresult Init();

private:
  ~sbDirectoryProvider();

protected:
};

#define SONGBIRD_DIRECTORY_PROVIDER_CLASSNAME "sbDirectoryProvider"
#define SONGBIRD_DIRECTORY_PROVIDER_CID \
  /* {d1d8f5b0-c207-11de-8a39-0800200c9a66} */ \
  { 0xd1d8f5b0, 0xc207, 0x11de, \
    { 0x8a, 0x39, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } \
  }
#define SONGBIRD_DIRECTORY_PROVIDER_CONTRACTID \
  "@songbirdnest.com/moz/directory/provider;1"

#endif // SB_DIRECTORY_PROVIDER_H__
