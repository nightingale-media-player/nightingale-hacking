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

nsresult
sbFileSystemTree::InitWithTreeSession(const nsAString & aSessionGuid)
{
  //
  // TODO: Write Me!
  //
  return NS_ERROR_NOT_IMPLEMENTED;
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

  sbChangeArray pathChangesArray;
  rv = GetNodeChanges(pathNode, aPath, pathChangesArray);
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
sbFileSystemTree::SaveTreeSession(const nsAString & aSessionGuid)
{
  //
  // TODO: Write Me!
  //
  return NS_ERROR_NOT_IMPLEMENTED;
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
  nsCOMPtr<nsISimpleEnumerator> pathEnum;
  rv = GetPathEntries(aPath, getter_AddRefs(pathEnum));
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

    // Don't track symlinks
    PRBool isSymlink;
    rv = curFile->IsSymlink(&isSymlink);
    if (NS_FAILED(rv) || isSymlink)
      continue;

#if defined(XP_WIN)
    // This check only needs to be done on windows
    PRBool isSpecial;
    rv = curFile->IsSpecial(&isSpecial);
    if (NS_FAILED(rv) || isSpecial)
      continue;
#endif

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
 
  nsresult rv;

#if DEBUG
  // Sanity checks to make sure that the passed in parent makes sense.
  if (aParentNode) {
    nsString parentNodeLeafName;
    rv = aParentNode->GetLeafName(parentNodeLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> parentFile;
    rv = aFile->GetParent(getter_AddRefs(parentFile));
    if (NS_SUCCEEDED(rv) && parentFile) {
      nsString parentFileLeafName;
      rv = parentFile->GetLeafName(parentFileLeafName);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(parentFileLeafName.Equals(parentNodeLeafName),
                   "ERROR: CreateNode() Potential invalid parent used!");
    }
  }
#endif

  nsString leafName;
  rv = aFile->GetLeafName(leafName);
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
sbFileSystemTree::GetNodeChanges(sbFileSystemNode *aNode,
                                 const nsAString & aNodePath,
                                 sbChangeArray & aOutChangeArray)
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbFileSystemTree::GetNodeChanges() not called on the main thread!");

  // This is a copy-constructor:
  sbNodeMap childSnapshot(*aNode->GetChildren());

  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> pathEnum;
  rv = GetPathEntries(aNodePath, getter_AddRefs(pathEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while ((NS_SUCCEEDED(pathEnum->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> curItem;
    rv = pathEnum->GetNext(getter_AddRefs(curItem));
    if (NS_FAILED(rv) || !curItem) {
      NS_WARNING("ERROR: Could not GetNext() item in enumerator!");
      continue;
    }

    nsCOMPtr<nsIFile> curFile = do_QueryInterface(curItem, &rv);
    if (NS_FAILED(rv) || !curFile) {
      NS_WARNING("ERROR: Could not QI to a nsIFile!");
      continue;
    }

    nsString curFileLeafName;
    rv = curFile->GetLeafName(curFileLeafName);
    if (NS_FAILED(rv)) {
      NS_WARNING("ERROR: Could not get the leaf name from the file-spec!");
      continue;
    }

    // See if a node exists with this file name in the child snapshot.
    sbNodeMapIter foundNodeIter = childSnapshot.find(curFileLeafName);
    if (foundNodeIter == childSnapshot.end()) {
      // The current file entry is not in the child snapshot, which means
      // that it is a new addition to the file path. Add a change entry
      // for this event.
      nsRefPtr<sbFileSystemNode> newFileNode;
      rv = CreateNode(curFile, aNode, getter_AddRefs(newFileNode));
      if (NS_FAILED(rv) || !newFileNode) {
        NS_WARNING("ERROR: Could not create a sbFileSystemNode!");
        continue;
      }

      rv = AppendCreateChangeItem(newFileNode, eAdded, aOutChangeArray);
      if (NS_FAILED(rv)) {
        NS_WARNING("ERROR: Could not add create change item!");
        continue;
      }
    }
    else {
      // Found a node that matches the leaf name in the child snapshot.
      // Now look to see if the item has changed by comparing the last
      // modify time stamps.
      nsRefPtr<sbFileSystemNode> curChildNode(foundNodeIter->second);
      if (!curChildNode) {
        NS_WARNING("ERROR: Could not get node from sbNodeMapIter!");
        continue;
      }

      // Now compare the timestamps
      PRInt64 curFileLastModify;
      rv = curFile->GetLastModifiedTime(&curFileLastModify);
      if (NS_FAILED(rv)) {
        NS_WARNING("ERROR: Could not get file last modify time!");
        continue;
      }
      PRInt64 curChildNodeLastModify;
      rv = curChildNode->GetLastModify(&curChildNodeLastModify);
      if (NS_FAILED(rv)) {
        NS_WARNING("ERROR: Could not get node last modify time!");
        continue;
      }

      if (curFileLastModify != curChildNodeLastModify) {
        // The original node has been modified, create a change object for
        // this mutation event.
        nsRefPtr<sbFileSystemNode> changedNode;
        rv = CreateNode(curFile, aNode, getter_AddRefs(changedNode));
        if (NS_FAILED(rv) || !changedNode) {
          NS_WARNING("ERROR: Could not create a sbFileSystemNode!");
          continue;
        }

        rv = AppendCreateChangeItem(changedNode, eChanged, aOutChangeArray);
        if (NS_FAILED(rv)) {
          NS_WARNING("ERROR: Could not add create change item!");
          continue;
        }
      }

      // It's ok to fall-through since we want to remove the matched node
      // from the child snapshot. This determines which nodes have been removed.
      childSnapshot.erase(curFileLeafName);
    }
  }

  // Now that the current directory entries have been traversed, loop through
  // and report all the nodes still in the child snapshot as removed events.
  sbNodeMapIter begin = childSnapshot.begin();
  sbNodeMapIter end = childSnapshot.end();
  sbNodeMapIter next;
  for (next = begin; next != end; ++next) {
    nsRefPtr<sbFileSystemNode> curNode(next->second);
    NS_ASSERTION(curNode, "Could not get the current child snapshot node!");
    if (!curNode) {
      NS_WARNING("ERROR: Could not get node from sbNodeMapIter!");
      continue;
    }

    rv = AppendCreateChangeItem(curNode, eRemoved, aOutChangeArray);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Error: Could not add a change event!");
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

/* static */ nsresult
sbFileSystemTree::GetPathEntries(const nsAString & aPath,
                                 nsISimpleEnumerator **aResultEnum)
{
  NS_ENSURE_ARG_POINTER(aResultEnum);

  nsresult rv;
  nsCOMPtr<nsILocalFile> pathFile =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pathFile->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, rv);

  return pathFile->GetDirectoryEntries(aResultEnum);
}

/* static */ nsresult
sbFileSystemTree::AppendCreateChangeItem(sbFileSystemNode *aChangedNode,
                                         EChangeType aChangeType,
                                         sbChangeArray & aChangeArray)
{
  NS_ENSURE_ARG_POINTER(aChangedNode);

  nsRefPtr<sbFileSystemChange> changedItem =
    new sbFileSystemChange(aChangedNode, aChangeType);
  NS_ENSURE_TRUE(changedItem, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbFileSystemChange> *appendResult =
    aChangeArray.AppendElement(changedItem);

  return (appendResult ? NS_OK : NS_ERROR_FAILURE);
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

