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

#ifndef sbWatchFolderUtils_h_
#define sbWatchFolderUtils_h_

#include <nsCOMPtr.h>
#include <nsStringAPI.h>

class sbIWatchFolderService;


//==============================================================================
//
// @interface sbAutoIgnoreWatchFolderPath
// @brief A class to automatically set and remove an ignored path with the
//        watch folder service when the class goes out of scope.
//
//==============================================================================

class sbAutoIgnoreWatchFolderPath : public nsISupports
{
public:
  sbAutoIgnoreWatchFolderPath();
  virtual ~sbAutoIgnoreWatchFolderPath();

  NS_DECL_ISUPPORTS

  //
  // @brief Initialize an auto-release path with the watch folder service. This
  //        method will add the passed in file path to the ignore list on the
  //        watch folder service and remove the ignore path when the object is
  //        destructed.
  //
  nsresult Init(nsAString const & aWatchPath);

private:
  nsCOMPtr<sbIWatchFolderService> mWFService;
  nsString                        mWatchPath;
  bool                          mIsIgnoring;
};

#endif  // sbWatchFolderUtils_h_

