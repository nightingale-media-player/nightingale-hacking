/*
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

#include "sbFileSystemTree.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsIRunnable.h>
#include <nsIThreadManager.h>
#include <nsThreadUtils.h>
#include <nsAutoLock.h>
#include <sbStringUtils.h>
#include <nsCRT.h>
#include "sbFileSystemChange.h"
#include <stack>

// Save ourselves some pain by getting the path seperator char.
// NOTE: FILE_PATH_SEPARATOR is always going to one char long.
#define PATH_SEPERATOR_CHAR \
  NS_LITERAL_STRING(FILE_PATH_SEPARATOR).CharAt(0)


//------------------------------------------------------------------------------
// Utility container, helps prevent running up the tree to find the 
// ful path of a node.

struct NodeContext
{
  NodeContext(const nsAString & aFullPath, sbFileSystemNode *aNode)
    : fullPath(aFullPath), node(aNode)
  {
  }

  nsString fullPath;
  nsRefPtr<sbFileSystemNode> node;
};

//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileSystemTree, nsISupports)

sbFileSystemTree::sbFileSystemTree()
  : mIsIntialized(PR_FALSE)
  , mRootNodeLock(PR_NewLock())
  , mListenersLock(PR_NewLock())
{
  NS_ASSERTION(mRootNodeLock, "Failed to create mRootNodeLock!");
  NS_ASSERTION(mListenersLock, "Failed to create mListenersLock!");
}

sbFileSystemTree::~sbFileSystemTree()
{
  if (mRootNodeLock) {
    PR_DestroyLock(mRootNodeLock);
  }
  if (mListenersLock) {
    PR_DestroyLock(mListenersLock);
  }
}

nsresult
sbFileSystemTree::Init(const nsAString & aPath, PRBool aIsRecursive)
{
  if (mIsIntialized) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mIsIntialized = PR_TRUE;

  mRootPath.Assign(aPath);
  mIsRecursiveBuild = aIsRecursive;
  
  nsresult rv;
  mRootFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mRootFile->InitWithPath(mRootPath);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsAutoLock rootNodeLock(mRootNodeLock);

    rv = CreateNode(mRootFile, nsnull, getter_AddRefs(mRootNode));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIThreadManager> threadMgr = 
    do_GetService("@mozilla.org/thread-manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Save the threading context for notifying the listeners on the current
  // thread once the build operation has completed.
  rv = threadMgr->GetCurrentThread(getter_AddRefs(mTreesCurrentThread));
  NS_ENSURE_SUCCESS(rv, rv);

  // Does this need to be a member?
  nsCOMPtr<nsIThread> treeThread;
  rv = threadMgr->NewThread(0, getter_AddRefs(treeThread));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, RunBuildThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  rv = treeThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbFileSystemTree::RunBuildThread()
{
  {
    // Don't allow any changes to structure
    nsAutoLock rootNodeLock(mRootNodeLock);

    // Build the tree and get the added dir paths to report back to the
    // tree listener once the build finishes.
    nsresult rv = AddChildren(mRootPath, mRootNode, PR_TRUE, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to add children to root node!");
  }

  // Now notify our listeners on the main thread
  nsCOMPtr<nsIRunnable> runnable = 
    NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, NotifyBuildComplete);
  NS_ASSERTION(runnable, 
               "Could not create a runnable for NotifyBuildComplete()!!");

  nsresult rv = mTreesCurrentThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Could not dispatch NotifyBuildComplete()!");
}

void
sbFileSystemTree::NotifyBuildComplete()
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbFileSystemTree::NotifyBuildComplete() not called on the main thread!");

  nsAutoLock listenerLock(mListenersLock);

  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; i++) {
    mListeners[i]->OnTreeReady(mDiscoveredDirs);
  }

  // Don't hang on to the values in |mDiscoveredDirs|.
  mDiscoveredDirs.Clear();
}

nsresult
sbFileSystemTree::Update(const nsAString & aPath)
{
  nsRefPtr<sbFileSystemNode> pathNode;
  nsresult rv = GetNode(aPath, getter_AddRefs(pathNode));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the saved snapshot of the child nodes at the passed in path.
  sbNodeMap *savedChildMap = pathNode->GetChildren();

  // Get the current snapshot of the child nodes at the passed in path.
  sbNodeMap newChildMap;
  rv = GetChildren(aPath, pathNode, newChildMap);
  NS_ENSURE_SUCCESS(rv, rv);

  // Compare the two maps.
  sbChangeArray pathChangesArray;
  rv = CompareNodeMaps(*savedChildMap, newChildMap, pathChangesArray);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString path(aPath);
  PRUint32 numChanges = pathChangesArray.Length();
  for (PRUint32 i = 0; i < numChanges; i++) {
    nsRefPtr<sbFileSystemChange> curChange = pathChangesArray[i];

    EChangeType curChangeType;
    rv = curChange->GetChangeType(&curChangeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbFileSystemNode> curChangeNode;
    rv = curChange->GetNode(getter_AddRefs(curChangeNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString curLeafName;
    rv = curChangeNode->GetLeafName(curLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString curNodeFullPath = EnsureTrailingPath(aPath);
    curNodeFullPath.Append(curLeafName);
    
    PRBool isDir;
    rv = curChangeNode->GetIsDir(&isDir);
    NS_ENSURE_SUCCESS(rv, rv);

    switch (curChangeType) {
      case eChanged:
        // Since the the native file system hooks are supposed to watch
        // only directories - do not handle directory changes. The event
        // will be processed later (possibly next).
        if (!isDir) {
          // Files are simple, just replace the the current node with the
          // change node.
          nsString curChangeLeafName;
          rv = curChangeNode->GetLeafName(curChangeLeafName);
          NS_ENSURE_SUCCESS(rv, rv);

          // Replace the node
          rv = pathNode->ReplaceNode(curChangeLeafName, curChangeNode);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

      case eAdded:
        if (isDir) {
          rv = NotifyDirAdded(curChangeNode, curNodeFullPath);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        
        // Simply add this child to the path node's children.
        rv = pathNode->AddChild(curChangeNode);
        NS_ENSURE_SUCCESS(rv, rv);
        break;

      case eRemoved:
        if (isDir) {
          rv = NotifyDirRemoved(curChangeNode, curNodeFullPath);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = pathNode->RemoveChild(curChangeNode);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
    }

    // Inform the listeners of the change
    rv = NotifyChanges(curNodeFullPath, curChangeType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::AddListener(sbFileSystemTreeListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
  nsAutoLock listenerLock(mListenersLock);

  NS_ENSURE_TRUE(mListeners.AppendElement(aListener), NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult
sbFileSystemTree::RemoveListener(sbFileSystemTreeListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  nsAutoLock listeners(mListenersLock);

  NS_ENSURE_TRUE(mListeners.RemoveElement(aListener), NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult
sbFileSystemTree::AddChildren(const nsAString & aPath,
                              sbFileSystemNode *aParentNode,
                              PRBool aBuildDiscoveredDirArray,
                              PRBool aNotifyListeners)
{
  std::stack<NodeContext> nodeContextStack;
  nodeContextStack.push(NodeContext(aPath, aParentNode));

  while (!nodeContextStack.empty()) {
    NodeContext curNodeContext = nodeContextStack.top();
    nodeContextStack.pop();

    sbNodeMap childNodes;
    nsresult rv = GetChildren(curNodeContext.fullPath, 
                              curNodeContext.node,
                              childNodes);

    sbNodeMapIter begin = childNodes.begin();
    sbNodeMapIter end = childNodes.end();
    sbNodeMapIter next;
    for (next = begin; next != end; ++next) {
      nsRefPtr<sbFileSystemNode> curNode(next->second);
      if (!curNode) {
        continue;
      }

      rv = curNodeContext.node->AddChild(curNode);
      if (NS_FAILED(rv)) {
        continue;
      }

      PRBool isDir = PR_FALSE;
      rv = curNode->GetIsDir(&isDir);
      if (NS_FAILED(rv)) {
        continue;
      }

      if (aNotifyListeners || isDir) {
        nsString curNodeLeafName(next->first);

        // Format the next child path
        nsString curNodePath = EnsureTrailingPath(curNodeContext.fullPath);
        curNodePath.Append(curNodeLeafName);

        if (mIsRecursiveBuild && isDir) {
          nodeContextStack.push(NodeContext(curNodePath, curNode));

          if (aBuildDiscoveredDirArray) {
            mDiscoveredDirs.AppendElement(curNodePath);
          }
        }

        if (aNotifyListeners) {
          rv = NotifyChanges(curNodePath, eAdded);
          if (NS_FAILED(rv)) {
            NS_WARNING("Could not notify listener of change!");
          }
        }
      }
    }
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::GetChildren(const nsAString & aPath,
                              sbFileSystemNode *aParentNode,
                              sbNodeMap & aNodeMap)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> pathFile = 
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pathFile->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> pathEnum;
  rv = pathFile->GetDirectoryEntries(getter_AddRefs(pathEnum));
  NS_ENSURE_SUCCESS(rv, rv);  
  
  PRBool hasMore = PR_FALSE;
  while ((NS_SUCCEEDED(pathEnum->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> curItem;
    rv = pathEnum->GetNext(getter_AddRefs(curItem));
    if (NS_FAILED(rv) || !curItem)
      continue;

    nsCOMPtr<nsIFile> curFile = do_QueryInterface(curItem, &rv);
    if (NS_FAILED(rv) || !curFile)
      continue;

    // Don't track symlinks or special stuff
    PRBool isSpecial;
    rv = curFile->IsSpecial(&isSpecial);
    if (NS_FAILED(rv) || isSpecial)
      continue;
    PRBool isSymlink;
    rv = curFile->IsSymlink(&isSymlink);
    if (NS_FAILED(rv) || isSymlink)
      continue;

    nsRefPtr<sbFileSystemNode> curNode;
    rv = CreateNode(curFile, aParentNode, getter_AddRefs(curNode));
    if (NS_FAILED(rv) || !curNode)
      continue;

    nsString curNodeLeafName;
    rv = curNode->GetLeafName(curNodeLeafName);
    if (NS_FAILED(rv))
      continue;

    aNodeMap.insert(sbNodeMapPair(curNodeLeafName, curNode));
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::GetNode(const nsAString & aPath,
                          sbFileSystemNode **aNodeRetVal)
{
  NS_ENSURE_ARG_POINTER(aNodeRetVal);
  NS_ENSURE_ARG(StringBeginsWith(aPath, mRootPath));

  *aNodeRetVal = nsnull;
  nsresult rv;

  // Trim off the trailing '/' if one exists
  nsString path(aPath);
  if (StringEndsWith(path, NS_LITERAL_STRING(FILE_PATH_SEPARATOR))) {
    path.Cut(path.Length() - 1, 1);
  }

  // If this is the root path, simply return the root node.
  if (path.Equals(mRootPath)) {
    NS_IF_ADDREF(*aNodeRetVal = mRootNode);
    return NS_OK;
  }

  // Only search path components that |aPath| and |mRootPath| don't have.
  PRInt32 strRange = path.Find(mRootPath);
  NS_ENSURE_TRUE(strRange >= 0, NS_ERROR_FAILURE);
  strRange += mRootPath.Length();

  // If |searchPath| starts with a '/', remove it
  nsString searchPath(Substring(path, strRange, path.Length() - strRange));
  if (searchPath[0] == PATH_SEPERATOR_CHAR) {
    searchPath.Cut(0, 1);
  }

  // Get a list of each patch component to search the tree with
  nsTArray<nsString> pathComponents;
  nsString_Split(searchPath, 
                 NS_LITERAL_STRING(FILE_PATH_SEPARATOR), 
                 pathComponents);

  // Start searching at the root node
  nsRefPtr<sbFileSystemNode> curSearchNode = mRootNode;
  
  PRBool foundTargetNode = PR_TRUE;  // assume true, prove below
  PRUint32 numComponents = pathComponents.Length();
  for (PRUint32 i = 0; i < numComponents; i++) {
    nsString curPathComponent(pathComponents[i]);
 
    sbNodeMap *curChildren = curSearchNode->GetChildren();
    if (!curChildren) {
      continue;
    }

    // If the current component was not found in the child nodes, bail.    
    sbNodeMapIter foundNodeIter = curChildren->find(curPathComponent);
    if (foundNodeIter == curChildren->end()) {
      foundTargetNode = PR_FALSE;
      break;
    }

    // This is the found component node
    curSearchNode = foundNodeIter->second;
  }  // end for
  
  if (foundTargetNode) {
    NS_ADDREF(*aNodeRetVal = curSearchNode);
    rv = NS_OK;
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult 
sbFileSystemTree::CreateNode(nsIFile *aFile, 
                             sbFileSystemNode *aParentNode,
                             sbFileSystemNode **aNodeRetVal)
{
  NS_ENSURE_ARG_POINTER(aFile);
  
  nsString leafName;
  nsresult rv = aFile->GetLeafName(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isDir;
  rv = aFile->IsDirectory(&isDir);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 lastModify;
  rv = aFile->GetLastModifiedTime(&lastModify);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbFileSystemNode> node = new sbFileSystemNode();
  NS_ENSURE_TRUE(node, NS_ERROR_OUT_OF_MEMORY);
  
  rv = node->Init(leafName, isDir, lastModify, aParentNode);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aNodeRetVal = node);
  return NS_OK;
}

nsresult
sbFileSystemTree::CompareLeafNames(sbFileSystemNode *aNode1,
                                   sbFileSystemNode *aNode2,
                                   PRBool *aOutResult)
{
  NS_ENSURE_ARG_POINTER(aNode1);
  NS_ENSURE_ARG_POINTER(aNode2);
  NS_ENSURE_ARG_POINTER(aOutResult);

  nsresult rv;
  nsString node1Name;
  rv = aNode1->GetLeafName(node1Name);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString node2Name;
  rv = aNode2->GetLeafName(node2Name);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOutResult = node1Name.Equals(node2Name);
  return NS_OK;
}

nsresult
sbFileSystemTree::CompareTimeStamps(sbFileSystemNode *aNode1,
                                    sbFileSystemNode *aNode2,
                                    PRBool *aOutResult)
{
  NS_ENSURE_ARG_POINTER(aNode1);
  NS_ENSURE_ARG_POINTER(aNode2);
  NS_ENSURE_ARG_POINTER(aOutResult);

  nsresult rv;
  PRInt64 node1ts;
  rv = aNode1->GetLastModify(&node1ts);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt64 node2ts;
  rv = aNode2->GetLastModify(&node2ts);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOutResult = (node1ts == node2ts);
  return NS_OK;
}

nsresult
sbFileSystemTree::CompareNodeMaps(sbNodeMap & aOriginalNodeMap,
                                  sbNodeMap & aCompareNodeMap,
                                  sbChangeArray & aOutChangeArray)
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbFileSystemTree::CompareNodeArray() not called on the main thread!");
  nsresult rv;
  
  // Make a copy of the working node maps.
  sbNodeMap originalSnapshot(aOriginalNodeMap);
  sbNodeMap compareSnapshot(aCompareNodeMap);

  // TODO: Search don't use two node maps for finding changes, simply iterate
  //       through the nsIFile.directoryEntries enumerator.
  // @see bug 14703
  
  // Loop through the original map. Pop items out of the duplicate maps
  // as needed to help figure out the added/removed nodes.
  sbNodeMapIter begin = aOriginalNodeMap.begin();
  sbNodeMapIter end = aOriginalNodeMap.end();
  sbNodeMapIter next;
  for (next = begin; next != end; ++next) {
    // Compare the node at the current index with what is in the passed
    // in compare node map to look for changes in timestamps.
    nsRefPtr<sbFileSystemNode> curOriginalNode(next->second);

    // Look for the matching node name (by leaf name) in the compare map.
    nsString curLeafName(next->first);
    if (curLeafName.IsEmpty()) {
      continue;
    }

    sbNodeMapIter foundNodeIter = aCompareNodeMap.find(curLeafName);
    if (foundNodeIter == aCompareNodeMap.end()) {
      // The node is not in the compare map, continue.
      continue;
    }

    nsRefPtr<sbFileSystemNode> curCompareNode(foundNodeIter->second);
    if (!curCompareNode) {
      continue;
    }

    PRBool hasSameTs = PR_FALSE;
    rv = CompareTimeStamps(curOriginalNode, curCompareNode, &hasSameTs);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!hasSameTs) {
      // The original node has been modified, create a change object for
      // this mutation event.
      nsRefPtr<sbFileSystemChange> curChange =
        new sbFileSystemChange(curCompareNode, eChanged);
      NS_ASSERTION(curChange, "Could not create a sbFileSystemChange!");
      if (!curChange) {
        continue;
      }

      // Add the change the array of changes.
      nsRefPtr<sbFileSystemChange> *resultChange =
        aOutChangeArray.AppendElement(curChange);
      NS_ASSERTION(resultChange, 
                   "Coult not add a change to the change array!");
    }

    // It's ok to fall-through since we want to remove these matched
    // nodes from the original and compare snapshot maps.

    // Since a match has been found, remove these current nodes from both
    // node maps. This determines which nodes have been added or removed.
    originalSnapshot.erase(curLeafName);
    compareSnapshot.erase(curLeafName);
  }

  // Now that all the matching elements have been trimmed from the cloned
  // node maps, go through and find the added and removed nodes.
  
  // Added items:
  begin = compareSnapshot.begin();
  end = compareSnapshot.end();
  for (next = begin; next != end; ++next) {
    nsRefPtr<sbFileSystemNode> curNode(next->second);
    NS_ASSERTION(curNode, "Could not get the current compare snapshot node!");
    if (!curNode) {
      continue;
    }

    nsRefPtr<sbFileSystemChange> curChange = 
      new sbFileSystemChange(curNode, eAdded);
    NS_ASSERTION(curChange, "Could not create a sbFileSystemChange!");
    if (!curChange) {
      continue;
    }

    NS_ASSERTION(aOutChangeArray.AppendElement(curChange),
                 "Could not add a change to the change array!");  
  }

  // Removed items:
  begin = originalSnapshot.begin();
  end = originalSnapshot.end();
  for (next = begin; next != end; ++next) {
    nsRefPtr<sbFileSystemNode> curNode(next->second);
    NS_ASSERTION(curNode, "Could not get the current compare snapshot node!");
    if (!curNode) {
      continue;
    }

    nsRefPtr<sbFileSystemChange> curChange = 
      new sbFileSystemChange(curNode, eRemoved);
    NS_ASSERTION(curChange, "Could not create a sbFileSystemChange!");
    if (!curChange) {
      continue;
    }

    NS_ASSERTION(aOutChangeArray.AppendElement(curChange),
                 "Could not add a change to the change array!");  
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::NotifyDirAdded(sbFileSystemNode *aAddedDirNode,
                                 nsAString & aFullPath)
{
  NS_ENSURE_ARG_POINTER(aAddedDirNode);

  // This is a little different than removal, since I have to add the new
  // child nodes.
  nsString fullPath = EnsureTrailingPath(aFullPath);

  // This is a shortcut for calling |AddChildren()| but toggling the
  // notification flag to inform listeners. Keeping this here in case
  // this needs to be threaded. If performance becomes a problem
  // (i.e. a large folder was moved into the watch path) this should be
  // moved to a background thread.
  nsresult rv = AddChildren(fullPath, aAddedDirNode, PR_FALSE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbFileSystemTree::NotifyDirRemoved(sbFileSystemNode *aRemovedDirNode,
                                   nsAString & aFullPath)
{
  NS_ENSURE_ARG_POINTER(aRemovedDirNode);

  nsString fullPath = EnsureTrailingPath(aFullPath);

  // Loop through all the children in |aRemovedDirNode| and notify of
  // removed events for each node.
  sbNodeMap *dirChildren = aRemovedDirNode->GetChildren();
  NS_ENSURE_TRUE(dirChildren, NS_ERROR_UNEXPECTED);

  sbNodeMapIter begin = dirChildren->begin();
  sbNodeMapIter end = dirChildren->end();
  sbNodeMapIter next;
  for (next = begin; next != end; ++next) {
    nsRefPtr<sbFileSystemNode> curNode(next->second);
    if (!curNode) {
      continue;
    }

    nsString curNodeLeafName(next->first);

    nsString curNodePath(fullPath);
    curNodePath.Append(curNodeLeafName);

    PRBool isDir;
    nsresult rv = curNode->GetIsDir(&isDir);
    NS_ENSURE_SUCCESS(rv, rv);

    // This is a dir, call out to have its children notified of their removal.
    if (isDir) {
      rv = NotifyDirRemoved(curNode, curNodePath);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Now notify the listeners
    rv = NotifyChanges(curNodePath, eRemoved);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::NotifyChanges(nsAString & aChangePath,
                                EChangeType aChangeType)
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbFileSystemTree::NotifyChanges() not called on the main thread!");

  nsAutoLock listenerLock(mListenersLock);

  PRUint32 listenerCount = mListeners.Length();
  for (PRUint32 i = 0; i < listenerCount; i++) {
    mListeners[i]->OnChangeFound(aChangePath, aChangeType);
  }

  return NS_OK;
}

nsString
sbFileSystemTree::EnsureTrailingPath(const nsAString & aFilePath) 
{
  nsString resultPath(aFilePath);
  PRUint32 length = resultPath.Length();
  if (length > 0 && (resultPath[length - 1] != PATH_SEPERATOR_CHAR)) {
    resultPath.AppendLiteral(FILE_PATH_SEPARATOR);
  }

  return resultPath;
}

