/*d
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

#ifndef __SB_MEDIASNIFFER_H__
#define __SB_MEDIASNIFFER_H__

#include <nsIGenericFactory.h>
#include <nsIContentSniffer.h>
#include <nsStringGlue.h>

#define SONGBIRD_MEDIASNIFFER_DESCRIPTION                  \
  "Songbird Media Sniffer"
#define SONGBIRD_MEDIASNIFFER_CONTRACTID                   \
  "@songbirdnest.com/sontentsniffer/manager;1"
#define SONGBIRD_MEDIASNIFFER_CLASSNAME                    \
  "sbMediaSniffer"
#define SONGBIRD_MEDIASNIFFER_CID                          \
{ /* b4f06ad2-0252-4a2f-9d83-f4f25d0303a3 */               \
  0xb4f06ad2,                                              \
  0x0252,                                                  \
  0x4a2f,                                                  \
  { 0x9d, 0x83, 0xf4, 0xf2, 0x5d, 0x03, 0x03, 0xa3 }       \
}

#define TYPE_MAYBE_MEDIA "application/vnd.songbird.maybe.media"

class sbMediaSniffer : public nsIContentSniffer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTSNIFFER

  static NS_METHOD Register(nsIComponentManager* compMgr,
                            nsIFile* path, 
                            const char* registryLocation,
                            const char* componentType, 
                            const nsModuleComponentInfo *info);
  static NS_METHOD Unregister(nsIComponentManager* aCompMgr,
                              nsIFile* aPath, 
                              const char* aRegistryLocation,
                              const nsModuleComponentInfo* aInfo);
};

#endif /* __SB_MEDIASNIFFER_H__ */
