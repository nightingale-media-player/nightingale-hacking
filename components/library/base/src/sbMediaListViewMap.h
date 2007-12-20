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

/** 
 * \file  sbMediaListViewMap.h
 * \brief Songbird Library Manager Definition.
 */

#ifndef __SB_MEDIALISTVIEWMAP_H__
#define __SB_MEDIALISTVIEWMAP_H__

#include <nsIObserver.h>
#include <nsWeakReference.h>
#include <sbILibrary.h>
#include <sbIMediaListViewMap.h>
#include <sbIMediaListView.h>

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsClassHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsTHashtable.h>
#include <prlock.h>

#define SONGBIRD_MEDIALISTVIEWMAP_DESCRIPTION                \
  "Songbird MediaListViewMap"
#define SONGBIRD_MEDIALISTVIEWMAP_CONTRACTID                 \
  "@songbirdnest.com/Songbird/library/MediaListViewMap;1"
#define SONGBIRD_MEDIALISTVIEWMAP_CLASSNAME                  \
  "Songbird MediaListViewMap"
#define SONGBIRD_MEDIALISTVIEWMAP_CID                        \
{ 0x16ea057c, 0xd4c2, 0x4921, { 0x99, 0x84, 0x13, 0xac, 0x42, 0x1a, 0x5f, 0x8d } }


class nsIComponentManager;
class nsIFile;
class nsIRDFDataSource;
class sbILibraryFactory;
class sbILibraryLoader;

struct nsModuleComponentInfo;

class sbMediaListViewMap : public sbIMediaListViewMap,
                           public nsIObserver,
                           public nsSupportsWeakReference
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIMEDIALISTVIEWMAP

  sbMediaListViewMap();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  nsresult Init();

private:
  ~sbMediaListViewMap();

private:
  struct sbViewStateInfo {
    sbViewStateInfo(const nsAString& aLibraryGuid,
                    const nsAString& aListGuid,
                    sbIMediaListViewState* aState)
    : libraryGuid(aLibraryGuid),
      listGuid(aListGuid),
      state(aState)
    {
      MOZ_COUNT_CTOR(sbViewStateInfo);
    }

    ~sbViewStateInfo()
    {
      MOZ_COUNT_DTOR(sbViewStateInfo);
    }

    nsString libraryGuid;
    nsString listGuid;
    nsCOMPtr<sbIMediaListViewState> state;
  };

  typedef nsClassHashtableMT< nsISupportsHashKey, sbViewStateInfo > sbViewMapInner;
  typedef nsClassHashtableMT< nsISupportsHashKey, sbViewMapInner > sbViewMap;

  sbViewMap mViewMap;
};

#endif /* __SB_MEDIALISTVIEWMAP_H__ */
