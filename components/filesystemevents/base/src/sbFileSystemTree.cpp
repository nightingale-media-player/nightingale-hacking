/*
//
// Copyright(c) 2005-2009 POTI, Inc.
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
#include <nsCRT.h>
#include <nsThreadUtils.h>
#include <nsAutoLock.h>

#include <nsISimpleEnumerator.h>
#include <nsIProxyObjectManager.h>
#include <nsIRunnable.h>
#include <nsIThreadManager.h>
#include <nsIThreadPool.h>

#include <sbProxiedComponentManager.h>
#include <sbStringUtils.h>
#include "sbFileSystemChange.h"

// Save ourselves some pain by getting the path seperator char.
// NOTE: FILE_PATH_SEPARATOR is always going to one char long.
#define PATH_SEPERATOR_CHAR \
  NS_LITERAL_STRING(FILE_PATH_SEPARATOR).CharAt(0)

// Logging
#ifdef PR_LOGGING
static PRLogModuleInfo* gFSTreeLog = nsnull;
#define TRACE(args) PR_LOG(gFSTreeLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gFSTreeLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */

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

NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileSystemTree, sbPIFileSystemTree)

sbFileSystemTree::sbFileSystemTree()
  : mListener(nsnull)
  , mShouldLoadSession(PR_FALSE)
  , mIsIntialized(PR_FALSE)
  , mRootNodeLock(nsAutoLock::NewLock("sbFileSystemTree::mRootNodeLock"))
  , mListenerLock(nsAutoLock::NewLock("sbFileSystemTree::mListenerLock"))
{
#ifdef PR_LOGGING
  if (!gFSTreeLog) {
    gFSTreeLog = PR_NewLogModule("sbFSTree");
  }
#endif
  NS_ASSERTION(mRootNodeLock, "Failed to create mRootNodeLock!");
  NS_ASSERTION(mListenerLock, "Failed to create mListenerLock!");
}

sbFileSystemTree::~sbFileSystemTree()
{
  if (mRootNodeLock) {
    nsAutoLock::DestroyLock(mRootNodeLock);
  }
  if (mListenerLock) {
    nsAutoLock::DestroyLock(mListenerLock);
  }
}

nsresult
sbFileSystemTree::Init(const nsAString & aPath, PRBool aIsRecursive)
{
  if (mIsIntialized) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mIsIntialized = PR_TRUE;
  mShouldLoadSession = PR_FALSE;
  mRootPath.Assign(aPath);
  mIsRecursiveBuild = aIsRecursive;

  return InitTree();
}

nsresult
sbFileSystemTree::InitWithTreeSession(nsID & aSessionID)
{
  if (mIsIntialized) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mSavedSessionID = aSessionID;
  mShouldLoadSession = PR_TRUE;
  mIsIntialized = PR_FALSE;  // not really initialized until session is loaded.

  return InitTree();
}

nsresult
sbFileSystemTree::InitTree()
{
  // This method just sets up the initial build thread. All pre-build
  // initialization should have been done before calling this method.
  nsresult rv;
  nsCOMPtr<nsIThreadManager> threadMgr =
    do_GetService("@mozilla.org/thread-manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Save the threading context for notifying the listeners on the current
  // thread once the build operation has completed.
  rv = threadMgr->GetCurrentThread(getter_AddRefs(mOwnerContextThread));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, RunBuildThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  rv = threadPoolService->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbFileSystemTree::RunBuildThread()
{
  nsresult rv;

  // If the tree should compare itself from a previous state - load that now.
  nsRefPtr<sbFileSystemNode> savedRootNode;
  if (mShouldLoadSession) {
    nsRefPtr<sbFileSystemTreeState> savedTreeState = 
      new sbFileSystemTreeState();
    NS_ASSERTION(savedTreeState, "Could not create a sbFileSystemTreeState!");
  
    rv = savedTreeState->LoadTreeState(mSavedSessionID,
                                       mRootPath,
                                       &mIsRecursiveBuild,
                                       getter_AddRefs(savedRootNode));
    if (NS_FAILED(rv)) {
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to load saved tree session!");

      // In the event that a session failed to load from disk, inform all the
      // listeners of the error and return. The tree can not be built w/o a
      // root watch path, which is stored in the stored session data.
      nsCOMPtr<nsIRunnable> runnable = 
        NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, NotifySessionLoadError);
      NS_ASSERTION(runnable, 
          "Could not create a runnable for NotifySessionLoadError()!");
      rv = mOwnerContextThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      NS_ASSERTION(NS_SUCCEEDED(rv), 
          "Could not dispatch NotifySessionLoadError()!");
      
      return;
    }
    else {
      mIsIntialized = PR_TRUE;
    }
  }

  mRootFile = do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Could not create a nsILocalFile!");

  rv = mRootFile->InitWithPath(mRootPath);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Could not InitWithPath a nsILocalFile!");

  // Before building the tree, ensure that root file does exist.
  PRBool exists = PR_FALSE;
  if ((NS_FAILED(mRootFile->Exists(&exists))) || !exists) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, NotifyRootPathIsMissing);
    NS_ASSERTION(runnable,
                 "Could not create a runnable for NotifyRootPathIsMissing()!");

    rv = mOwnerContextThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    NS_ASSERTION(NS_SUCCEEDED(rv), 
                 "Could not Dispatch NotifyRootPathIsMissing()!");

    // Don't bother trying to build the rest of the tree
    return;
  }

  {
    // Don't allow any changes to structure
    nsAutoLock rootNodeLock(mRootNodeLock);

    rv = CreateNode(mRootFile, nsnull, getter_AddRefs(mRootNode));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Could not create a sbFileSystemNode!"); 

    // Build the tree and get the added dir paths to report back to the
    // tree listener once the build finishes.
    rv = AddChildren(mRootPath, mRootNode, PR_TRUE, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to add children to root node!");
  }

  if (mShouldLoadSession && savedRootNode) {
    // Now that the saved tree has been reloaded, and the current tree has
    // been built, build a change list.
    rv = GetTreeChanges(savedRootNode, mSessionChanges);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get the old session tree changes!");
    }
  }

  // Now notify our listeners on the main thread
  nsCOMPtr<nsIRunnable> runnable = 
    NS_NEW_RUNNABLE_METHOD(sbFileSystemTree, this, NotifyBuildComplete);
  NS_ASSERTION(runnable, 
               "Could not create a runnable for NotifyBuildComplete()!!");

  rv = mOwnerContextThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Could not dispatch NotifyBuildComplete()!");
}

void
sbFileSystemTree::NotifyBuildComplete()
{
  TRACE(("%s: build for [%s] complete",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(mRootPath).get()));
  // If the tree was initialized from a previous session, inform the listener
  // of all the changes that have been detected from between sessions before
  // noitifying the tree is ready. This is the documented behavior of
  // |sbIFileSystemWatcher|. 
  if (mShouldLoadSession && mSessionChanges.Length() > 0) {
    nsresult rv;
    for (PRUint32 i = 0; i < mSessionChanges.Length(); i++) {
      nsRefPtr<sbFileSystemPathChange> curPathChange(mSessionChanges[i]);
      if (!curPathChange) {
        NS_WARNING("Could not get current path change!");
        continue;
      }

      nsString curEventPath;
      rv = curPathChange->GetChangePath(curEventPath);
      if (NS_FAILED(rv)) {
        NS_WARNING("Could not get the current change event path!");
        continue;
      }

      EChangeType curChangeType;
      rv = curPathChange->GetChangeType(&curChangeType);
      if (NS_FAILED(rv)) {
        NS_WARNING("Could not get current change type!");
        continue;
      }

      rv = NotifyChanges(curEventPath, curChangeType);
      if (NS_FAILED(rv)) {
        NS_WARNING("Could not notify listeners of changes!");
      }
    }
    
    mSessionChanges.Clear();
  }

  {
    nsAutoLock listenerLock(mListenerLock);
    if (mListener) {
      mListener->OnTreeReady(mRootPath, mDiscoveredDirs);
    }
  }

  // Don't hang on to the values in |mDiscoveredDirs|.
  mDiscoveredDirs.Clear();
}

void
sbFileSystemTree::NotifyRootPathIsMissing()
{
  nsAutoLock listenerLock(mListenerLock);

  if (mListener) {
    mListener->OnRootPathMissing();
  }
}

void 
sbFileSystemTree::NotifySessionLoadError()
{
  nsAutoLock listenerLock(mListenerLock);

  if (mListener) {
    mListener->OnTreeSessionLoadError();
  }
}

nsresult
sbFileSystemTree::Update(const nsAString & aPath)
{
  nsRefPtr<sbFileSystemNode> pathNode;
  nsresult rv;
  { /* scope */
    nsAutoLock rootLock(mRootNodeLock);
    rv = GetNode(aPath, mRootNode, getter_AddRefs(pathNode));
  }
  if (NS_FAILED(rv)) {
    TRACE(("%s: Could not update the tree at path '%s'!!!",
          __FUNCTION__, NS_ConvertUTF16toUTF8(aPath).get()));
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  sbNodeChangeArray pathChangesArray;
  rv = GetNodeChanges(pathNode, aPath, pathChangesArray);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString path(aPath);
  PRUint32 numChanges = pathChangesArray.Length();
  for (PRUint32 i = 0; i < numChanges; i++) {
    nsRefPtr<sbFileSystemNodeChange> curChange = 
      static_cast<sbFileSystemNodeChange *>(pathChangesArray[i].get());

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
          { /* scope */
            nsAutoLock rootLock(mRootNodeLock);
            rv = pathNode->ReplaceNode(curChangeLeafName, curChangeNode);
          }
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

      case eAdded:
        if (isDir) {
          rv = NotifyDirAdded(curChangeNode, curNodeFullPath);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        
        // Simply add this child to the path node's children.
        { /* scope */
          nsAutoLock rootLock(mRootNodeLock);
          rv = pathNode->AddChild(curChangeNode);
        }
        NS_ENSURE_SUCCESS(rv, rv);
        break;

      case eRemoved:
        if (isDir) {
          rv = NotifyDirRemoved(curChangeNode, curNodeFullPath);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        { /* scope */
          nsAutoLock rootLock(mRootNodeLock);
          rv = pathNode->RemoveChild(curChangeNode);
        }
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
sbFileSystemTree::SetListener(sbFileSystemTreeListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
  nsAutoLock listenerLock(mListenerLock);

  mListener = aListener;
  return NS_OK;
}

nsresult
sbFileSystemTree::ClearListener()
{
  nsAutoLock listeners(mListenerLock);
  
  mListener = nsnull;
  return NS_OK;
}

nsresult
sbFileSystemTree::SaveTreeSession(const nsID & aSessionID)
{
  if (!mRootNode) {
    return NS_ERROR_UNEXPECTED;
  }

  // XXX Move this off of the main thread
  nsAutoLock rootNodeLock(mRootNodeLock);

  nsRefPtr<sbFileSystemTreeState> treeState = new sbFileSystemTreeState();
  NS_ENSURE_TRUE(treeState, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  rv = treeState->SaveTreeState(this, aSessionID);
  NS_ENSURE_SUCCESS(rv, rv);

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
                          sbFileSystemNode * aRootSearchNode,
                          sbFileSystemNode **aNodeRetVal)
{
  NS_ENSURE_ARG_POINTER(aRootSearchNode);
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
    NS_IF_ADDREF(*aNodeRetVal = aRootSearchNode);
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

  // Start searching at the passed in root node
  nsRefPtr<sbFileSystemNode> curSearchNode = aRootSearchNode;
  
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
  
  rv = node->Init(leafName, isDir, lastModify);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aNodeRetVal = node);
  return NS_OK;
}

nsresult
sbFileSystemTree::GetNodeChanges(sbFileSystemNode *aNode,
                                 const nsAString & aNodePath,
                                 sbNodeChangeArray & aOutChangeArray)
{
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

      rv = AppendCreateNodeChangeItem(newFileNode, eAdded, aOutChangeArray);
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

        rv = AppendCreateNodeChangeItem(changedNode, eChanged, aOutChangeArray);
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

    rv = AppendCreateNodeChangeItem(curNode, eRemoved, aOutChangeArray);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Error: Could not add a change event!");
  }

  return NS_OK;
}

nsresult
sbFileSystemTree::GetTreeChanges(sbFileSystemNode *aOldRootNode,
                                 sbPathChangeArray & aOutChangeArray)
{
  NS_ENSURE_ARG_POINTER(mRootNode);
  NS_ENSURE_ARG_POINTER(aOldRootNode);

  // This method is called from a background thread, prevent changes
  // to the root node until the changes have been found.
  nsAutoLock rootNodeLock(mRootNodeLock);

  // Both |mRootNode| and |aOldRootNode| are guarenteed, compare them and than
  // start the tree search.
  PRBool isSame = PR_FALSE;
  nsresult rv;
  rv = CompareNodes(mRootNode, aOldRootNode, &isSame);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isSame) {
    rv = AppendCreatePathChangeItem(mRootPath, eChanged, aOutChangeArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Need to keep the context of the node for building the path changes.
  std::stack<NodeContext> nodeContextStack;
  nodeContextStack.push(NodeContext(mRootPath, mRootNode));

  while (!nodeContextStack.empty()) {
    NodeContext curNodeContext = nodeContextStack.top();
    nodeContextStack.pop();

    // Lookup the old node in the tree (if it is not there, something is
    // really wrong!).
    nsRefPtr<sbFileSystemNode> oldNodeContext;
    rv = GetNode(curNodeContext.fullPath, 
                 aOldRootNode, 
                 getter_AddRefs(oldNodeContext));
    if (NS_FAILED(rv) || !oldNodeContext) {
      NS_WARNING("Could not find old context node!!! Something is really bad!");
      continue;
    }

    sbNodeMap *curNodeChildren = curNodeContext.node->GetChildren();
    sbNodeMap oldNodeChildSnapshot(*oldNodeContext->GetChildren());

    nsString curContextRootPath = EnsureTrailingPath(curNodeContext.fullPath);
    
    // Loop through all the added children to find changes.
    sbNodeMapIter begin = curNodeChildren->begin();
    sbNodeMapIter end = curNodeChildren->end();
    sbNodeMapIter next;
    for (next = begin; next != end; ++next) {
      nsString curChildPath(curContextRootPath);
      curChildPath.Append(next->first);
        
      // See if the current child is in the old child snapshot.
      sbNodeMapIter found = oldNodeChildSnapshot.find(next->first);
      if (found == oldNodeChildSnapshot.end()) {
        // The current child node is not in the current snapshot. Report 
        // this node and all of its children as added events.
        std::stack<NodeContext> addedNodeContext;
        addedNodeContext.push(NodeContext(curChildPath, next->second));

        rv = CreateTreeEvents(addedNodeContext, eAdded, aOutChangeArray);
        if (NS_FAILED(rv)) {
          NS_WARNING("Could not report tree added events!");
          continue;
        }
      }
      else {
        // The current child node has a match in the old snapshot. Look to see
        // if the nodes have changed. If so, report an event.
        isSame = PR_FALSE;
        rv = CompareNodes(next->second, found->second, &isSame);
        if (NS_FAILED(rv)) {
          NS_WARNING("Could not compare child nodes!");
          continue;
        }

        if (!isSame) {
          rv = AppendCreatePathChangeItem(curChildPath, 
                                          eChanged, 
                                          aOutChangeArray);
          if (NS_FAILED(rv)) {
            NS_WARNING("could not create change item!");
            continue;
          }
        }

        // Remove this node from the old child node snapshot so that removed
        // nodes can be determined below. Also push the current node onto the
        // context stack so that the next batch of children can be compared.
        oldNodeChildSnapshot.erase(found->first);

        // Push the current node into the node context.
        nsRefPtr<sbFileSystemNode> curChildNode(next->second);
        nodeContextStack.push(NodeContext(curChildPath, curChildNode));
      }
    }

    // If there are remaining children in the old node child snapshot, report
    // all of them and their children as deleted events.
    if (oldNodeChildSnapshot.size() > 0) {
      // Push all changes into the remove context stack.
      sbNodeContextStack removedNodeContext;

      sbNodeMapIter removeBegin = oldNodeChildSnapshot.begin();
      sbNodeMapIter removeEnd = oldNodeChildSnapshot.end();
      sbNodeMapIter removeNext;
      for (removeNext = removeBegin; removeNext != removeEnd; ++removeNext) {
        nsString curRemoveChildPath(curContextRootPath);
        curRemoveChildPath.Append(removeNext->first);

        removedNodeContext.push(NodeContext(curRemoveChildPath, 
                                            removeNext->second));
      }
    
      rv = CreateTreeEvents(removedNodeContext, eRemoved, aOutChangeArray);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }  // end while

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

NS_IMETHODIMP
sbFileSystemTree::NotifyChanges(const nsAString & aChangePath,
                                PRUint32 aChangeType)
{
  NS_ENSURE_TRUE(aChangeType == eChanged ||
                 aChangeType == eAdded ||
                 aChangeType == eRemoved,
                 NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIThread> currentThread;
  nsresult rv = NS_GetCurrentThread(getter_AddRefs(currentThread));
  NS_ENSURE_SUCCESS(rv, rv);
  if (currentThread != mOwnerContextThread) {
    nsCOMPtr<sbPIFileSystemTree> proxiedThis;
    nsresult rv = do_GetProxyForObject(mOwnerContextThread,
                                       NS_GET_IID(sbPIFileSystemTree),
                                       this,
                                       NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(proxiedThis));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = proxiedThis->NotifyChanges(aChangePath, aChangeType);
    return rv;
  }

  nsAutoLock listenerLock(mListenerLock);
  if (mListener) {
    mListener->OnChangeFound(aChangePath, EChangeType(aChangeType));
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
sbFileSystemTree::CompareNodes(sbFileSystemNode *aNode1,
                               sbFileSystemNode *aNode2,
                               PRBool *aIsSame)
{
  NS_ENSURE_ARG_POINTER(aNode1);
  NS_ENSURE_ARG_POINTER(aNode2);

  nsresult rv;
#if DEBUG
  // Sanity check the node leaf names.
  nsString node1Name;
  rv = aNode1->GetLeafName(node1Name);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString node2Name;
  rv = aNode2->GetLeafName(node2Name);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!node1Name.Equals(node2Name)) {
    NS_WARNING("CompareNodes() was given two nodes w/o the same leaf name!");
    return NS_ERROR_FAILURE;
  }
#endif

  PRInt64 node1Modify;
  rv = aNode1->GetLastModify(&node1Modify);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRInt64 node2Modify;
  rv = aNode2->GetLastModify(&node2Modify);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsSame = (node1Modify == node2Modify);
  return NS_OK;
}

nsresult
sbFileSystemTree::CreateTreeEvents(sbNodeContextStack & aContextStack,
                                   EChangeType aChangeType,
                                   sbPathChangeArray & aChangeArray)
{
  nsresult rv;
  while (!aContextStack.empty()) {
    NodeContext curNodeContext = aContextStack.top();
    aContextStack.pop();

    // First, create the change item.
    rv = AppendCreatePathChangeItem(curNodeContext.fullPath,
                                    aChangeType,
                                    aChangeArray);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not create a change item!");
      continue;
    }

    // Next, push all the child nodes of the current path onto the stack.
    sbNodeMap *childNodes = curNodeContext.node->GetChildren();
    if (!childNodes || childNodes->size() == 0) {
      continue;
    }

    nsString curContextPath = EnsureTrailingPath(curNodeContext.fullPath);

    sbNodeMapIter begin = childNodes->begin();
    sbNodeMapIter end = childNodes->end();
    sbNodeMapIter next;
    for (next = begin; next != end; ++next) {
      nsString curChildPath(curContextPath);
      curChildPath.Append(next->first);

      aContextStack.push(NodeContext(curChildPath, next->second));
    }
  }

  return NS_OK;
}

/* static */ nsresult
sbFileSystemTree::AppendCreateNodeChangeItem(sbFileSystemNode *aChangedNode,
                                             EChangeType aChangeType,
                                             sbNodeChangeArray & aChangeArray)
{
  NS_ENSURE_ARG_POINTER(aChangedNode);

  nsRefPtr<sbFileSystemNodeChange> changedItem =
    new sbFileSystemNodeChange(aChangedNode, aChangeType);
  NS_ENSURE_TRUE(changedItem, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbFileSystemNodeChange> *appendResult =
    aChangeArray.AppendElement(changedItem);

  return (appendResult ? NS_OK : NS_ERROR_FAILURE);
}

/* static */ nsresult
sbFileSystemTree::AppendCreatePathChangeItem(const nsAString & aEventPath,
                                             EChangeType aChangeType,
                                             sbPathChangeArray & aChangeArray)
{
  nsRefPtr<sbFileSystemPathChange> changedItem =
    new sbFileSystemPathChange(aEventPath, aChangeType);
  NS_ENSURE_TRUE(changedItem, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbFileSystemPathChange> *appendResult =
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

