/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <sbIMediaFileManager.h>
#include <nsIIOService.h>
#include <nsStringGlue.h>
#include <nsCOMPtr.h>

#define SB_MEDIAMANAGEMENTSERVICE_DESCRIPTION              \
  "Songbird Media File Manager Implementation"

#define SB_MEDIAMANAGEMENTSERVICE_CID                      \
  {                                                        \
   0x7b901232,                                             \
   0x1dd2,                                                 \
   0x11b2,                                                 \
   {0x8d, 0x6a, 0xe1, 0x78, 0x4d, 0xbd, 0x2d, 0x89}        \
  }

class sbMediaFileManager : public sbIMediaFileManager {
public:
  sbMediaFileManager();
  NS_METHOD Init();
  
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAFILEMANAGER

protected:
  virtual ~sbMediaFileManager();
  
  nsresult GetNewPath(sbIMediaItem *aMediaItem,
                      nsIURI *aItemUri,
                      nsString &aPath, 
                      PRBool *aRetVal);

  nsresult GetNewFilename(sbIMediaItem *aMediaItem, 
                          nsIURI *aItemUri,
                          nsString &aFilename, 
                          PRBool *aRetVal);

  nsresult CopyRename(sbIMediaItem *aMediaItem, 
                      nsIURI *aItemUri,
                      const nsString &aFilename,
                      const nsString &aPath,
                      PRBool *aRetVal);
  
  nsresult GetExtension(const nsString &aPath, nsString &aRetVal);
  
  nsresult Delete(sbIMediaItem *aMediaItem, PRBool *aRetVal);
  
private:
  nsCOMPtr<nsIIOService> mIOService;
};

