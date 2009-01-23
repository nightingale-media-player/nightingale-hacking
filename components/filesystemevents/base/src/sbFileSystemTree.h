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

#ifndef sbFileSystemTree_h_
#define sbFileSystemTree_h_

#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include <nsILocalFile.h>
#include <nsIThread.h>
#include <nsTArray.h>
#include <prlock.h>
#include "sbFileSystemNode.h"
#include "sbFileSystemTreeListener.h"

class sbFileSystemChange;
typedef nsTArray<nsRefPtr<sbFileSystemChange> > sbChangeArray;


//------------------------------------------------------------------------------
// A class to build a tree snapsot of a filesystem specified at a root.
//------------------------------------------------------------------------------
class sbFileSystemTree : public nsISupports
{
public:
  sbFileSystemTree();
  virtual ~sbFileSystemTree();

  NS_DECL_ISUPPORTS
  
  //
  // \brief Initialize the tree with a given path.
  // \param aPath The file path to root from.
  // \param aIsRecursive If the tree should watch recursively from the 
  //        given path.
  //
  nsresult Init(const nsAString & aPath, PRBool aIsRecursive);

  //
  // \brief Init a tree from a saved session.
  // \param aSessionGuid The saved tree session GUID.
  //
  nsresult InitWithTreeSession(const nsAString & aSessionGuid);
  
  //
  // \brief Inform the tree to sync at a given path.
  // \param aPath The path to sync at.
  // 
  nsresult Update(const nsAString & aPath);

  //
  // \brief Add a tree listener.
  // \param The listener to add.
  //
  nsresult AddListener(sbFileSystemTreeListener *aListener);

  //
  // \brief Remove a tree listener.
  // \param The listener to remove.
  //
  nsresult RemoveListener(sbFileSystemTreeListener *aListener);

  //
  // \brief Save the current state of a tree. The session GUID passed in
  //        to this method can be used in |InitWithTreeSession()| to restore
  //        a tree from a save performed in this method.
  // \param aSessionGuid The session GUID to associate to the saved tree.
  //
  nsresult SaveTreeSession(const nsAString & aSessionGuid);

  //
  // \brief Get a string that has a native trailing path component.
  //        For example:
  //          INPUT: "/foo/bar"
  //          OUTPUT: "/foo/bar/"
  //
  nsString EnsureTrailingPath(const nsAString & aFilePath);

protected:
  
  //
  // \brief Add children to a given parent node at a specified path.
  // \param aPath The path of the children to add.
  // \param aParentNode The parent node of the children to add.
  // \param aBuildDiscoveredDirArray Option to enable logging all directories
  //        discovered into |mDiscoveredDirs|. This is usually only done on 
  //        the initial tree build.
  // \param aNotifyListener Option to notify listeners of each node that
  //                        is added to the child list.
  //
  nsresult AddChildren(const nsAString & aPath,
                       sbFileSystemNode *aParentNode,
                       PRBool aBuildDiscoveredDirArray,
                       PRBool aNotifyListener);

  //
  // \brief Get the children that are currently at the given path and 
  //        assign them a parent node.
  // \param aPath The path to get the children at.
  // \param aParentNode The parent node to assign to each of the nodes.
  // \param aNodeMap The child node map to assign the path's children to.
  //
  nsresult GetChildren(const nsAString & aPath, 
                       sbFileSystemNode *aParentNode,
                       sbNodeMap & aNodeMap);

  //
  // \brief Get a node from the from the current saved snapshot at a 
  //        specified path.
  // \param aPath The path of the node to find
  // \param aNodeRetVal The result node of the search.
  //
  nsresult GetNode(const nsAString & aPath,
                   sbFileSystemNode **aNodeRetVal);

  //
  // \brief Create a node for a given |nsIFile| spec.
  // \param aFile The file spec to use for the node creation
  // \param aParentNode The parent node to assign to the new node.
  // \param aNodeRetVal The out-param result node.
  //
  nsresult CreateNode(nsIFile *aFile,
                      sbFileSystemNode *aParentNode,
                      sbFileSystemNode **aNodeRetVal);

  //
  // \brief This method looks at all the current directory entries and
  //        generates a change log of all the differences between the
  //        children that are currently stored in |aNode|.
  // \param aNode The path node that is used for comparing it's children
  //              against.
  // \param aNodePath The absolute path of the |aNode|.
  // \param aOutChangeArray The arrary of changes found during the compare.
  //
  nsresult GetNodeChanges(sbFileSystemNode *aNode,
                          const nsAString & aNodePath,
                          sbChangeArray & aOutChangeArray);  
  
  //
  // \brief Notify the tree listeners that a directory was added by informing
  //        them of all the children inside the new directory. This function
  //        will not report the change event specified at the passed in full
  //        path.
  // \param aAddedDirNode The node representing the directory that was added.
  // \param aFullPath The absolute path of the new directory.
  //
  nsresult NotifyDirAdded(sbFileSystemNode *aAddedDirNode,
                          nsAString & aFullPath);

  //
  // \brief Notify the tree listeners that a directory was removed by
  //        informing them off all the children inside the directory. This
  //        function will not report the change event specified at the passed
  //        in full path.
  // \param aRemovedDirNode The node representing the directory that was 
  //        removed.
  // \param aFullPath The absolute path of the removed directory.
  //
  nsresult NotifyDirRemoved(sbFileSystemNode *aRemovedDirNode,
                            nsAString & aFullPath);

  //
  // \brief Notify the tree listeners that a resource has changed (could
  //        be either a file or a directory) with a given change type.
  // \param aChangePath The absolute path of the changed resource.
  // \param aChangeType The type of change for the given resource path.
  //
  nsresult NotifyChanges(nsAString & aChangePath,
                         EChangeType aChangeType);

  //
  // \brief Utility method for getting a enumerator of the entries in a
  //        directory at a given path.
  // \param aPath The path to get the entries at.
  // \param aResultEnum The out-param enumerator for the entries.
  //
  static nsresult GetPathEntries(const nsAString & aPath,
                                 nsISimpleEnumerator **aResultEnum);

  //
  // \brief Utility method for creating and appending a change item to
  //        a change array with a given node and change type.
  // \param aChangedNode The node that has changed to use with the change item.
  // \param aChangeType The change type for the change item.
  // \param aChangeArray The change array to append the change item onto.
  //
  static nsresult AppendCreateChangeItem(sbFileSystemNode *aChangedNode,
                                         EChangeType aChangeType,
                                         sbChangeArray & aChangeArray);
  
  //
  // \brief Background thread method for doing the initial build of the
  //        file system tree. Once the build is complete, the listeners
  //        will be notified via the |OnTreeReady()| function.
  //
  void RunBuildThread();

  //
  // \brief Internal method for informing tree listeners that the tree has been
  //        built on the main thread. 
  //
  void NotifyBuildComplete();

private:
  nsRefPtr<sbFileSystemNode>           mRootNode;
  nsTArray<sbFileSystemTreeListener *> mListeners;
  nsCOMPtr<nsIThread>                  mTreesCurrentThread;
  nsCOMPtr<nsILocalFile>               mRootFile;
  nsString                             mRootPath;
  PRBool                               mIsRecursiveBuild;
  PRBool                               mIsIntialized;
  PRLock                               *mRootNodeLock;
  PRLock                               *mListenersLock;
  sbStringArray                        mDiscoveredDirs;
};

#endif

