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

protected:
  
  //
  // \brief Add children to a given parent node at a specified path.
  // \param aPath The path of the children to add.
  // \param aNotifyListener Option to notify listeners of each node that
  //                        is added to the child list.
  //
  nsresult AddChildren(const nsAString & aPath,
                       sbFileSystemNode *aParentNode,
                       PRBool aNotifyListener);

  //
  // \brief Get the children that are currently at the given path and 
  //        assign them a parent node.
  // \param aPath The path to get the children at.
  // \param aParentNode The parent node to assign to each of the nodes.
  // \param aNodeArray The array to assign the path's children to.
  //
  nsresult GetChildren(const nsAString & aPath, 
                       sbFileSystemNode *aParentNode,
                       sbNodeArray & aNodeArray);

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
  // \brief Compare two nodes using their leaf names. This is simply a
  //        comparision of the leaf names, this check does not care about
  //        the parents of each node.
  // \param aNode1 The first node to compare.
  // \param aNode2 The second node to compare.
  // \param aOutResult The out-param result of the leaf name comparison.
  //
  nsresult CompareLeafNames(sbFileSystemNode *aNode1,
                            sbFileSystemNode *aNode2,
                            PRBool *aOutResult);

  //
  // \brief Compare two nodes using their time specs.
  // \param aNode1 The first node to compare.
  // \param aNode2 The second node to compare.
  // \param aOutResult The out-param result of the time-spec comparison.
  //
  nsresult CompareTimeStamps(sbFileSystemNode *aNode1,
                             sbFileSystemNode *aNode2,
                             PRBool *aOutResult);

  //
  // \brief Compare two node arrays to get a resulting changelog.
  // \param aOriginalArray The original array of nodes to compare.
  // \param aCompareArray The new array to compare agains the original array.
  // \param aOutChangeArray The change array to append all discovered
  //                        differences between the two node arrays.
  //
  nsresult CompareNodeArray(sbNodeArray & aOriginalArray,
                            sbNodeArray & aCompareArray,
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
  // \brief Get a string that has a native trailing path component.
  //        For example:
  //          INPUT: "/foo/bar"
  //          OUTPUT: "/foo/bar/"
  //
  nsString EnsureTrailingPath(const nsAString & aFilePath); 
 
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

  // Temporary member to store discovered paths while |AddChildren()| is
  // still recursive.
  // @see bug 14666 
  sbStringArray                        mDiscoveredDirs;
};

#endif

