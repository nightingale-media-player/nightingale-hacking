/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbFileSystemTreeState.h"
#include "sbFileSystemTree.h"
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIPipe.h>
#include <nsIAsyncOutputStream.h>
#include <nsIAsyncInputStream.h>
#include <nsILocalFile.h>
#include <nsIFileStreams.h>
#include <nsIProperties.h>
#include <nsAppDirectoryServiceDefs.h>
#include <nsMemory.h>
#include <stack>

#define TREE_FOLDER_NAME           "fstrees"
#define SESSION_FILENAME_EXTENSION ".tree"
#define TREE_SCHEMA_VERSION        1


//
// To serialize the tree structure, the parent/child dependency is broken
// down and recorded with ID references to their genealogy. When the tree
// is de-serialized, the relationship is set back up using the ID of the 
// parent and child nodes. 
//
// See below for a example of the sequence of data:
//
//   FILENAME: '{sessionid}.tree'
//   -----------------------------------------------------------
//   | 1.) Serialization schema version (PRUint32)             |
//   -----------------------------------------------------------
//   | 2.) Tree root absolute path (nsString)                  |
//   -----------------------------------------------------------
//   | 3.) Is tree recursive watch (PRBool)                    |
//   -----------------------------------------------------------
//   | 4.) Number of nodes (PRUint32)                          |
//   -----------------------------------------------------------
//   | 5.) Node (sbFileSystemNode)                             |
//   -----------------------------------------------------------
//   | ..  Node (sbFileSystemNode)                             |
//   -----------------------------------------------------------
//   | ...                                                     |
//   -----------------------------------------------------------
//   | -> EOF                                                  |
//   -----------------------------------------------------------
//  

NS_IMPL_THREADSAFE_ISUPPORTS0(sbFileSystemTreeState)

sbFileSystemTreeState::sbFileSystemTreeState()
{
}

sbFileSystemTreeState::~sbFileSystemTreeState()
{
}

nsresult
sbFileSystemTreeState::SaveTreeState(sbFileSystemTree *aTree,
                                     const nsID & aSessionID)
{
  NS_ENSURE_ARG_POINTER(aTree);

  // Setup and write the serialized data as defined above.
  nsresult rv;
  nsCOMPtr<nsIFile> savedSessionFile;
  rv = GetTreeSessionFile(aSessionID, 
                          PR_TRUE,  // do create
                          getter_AddRefs(savedSessionFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbFileObjectOutputStream> fileObjectStream =
    new sbFileObjectOutputStream();
  NS_ENSURE_TRUE(fileObjectStream, NS_ERROR_OUT_OF_MEMORY);

  rv = fileObjectStream->InitWithFile(savedSessionFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now begin to write out the data in the sequence described above:
  // 1.) The tree schema version.
  rv = fileObjectStream->WriteUint32(TREE_SCHEMA_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  // 2.) Tree root absolute path
  rv = fileObjectStream->WriteString(aTree->mRootPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // 3.) Is tree recursive watch.
  rv = fileObjectStream->WritePRBool(aTree->mIsRecursiveBuild);
  NS_ENSURE_SUCCESS(rv, rv);

  // 4.) Number of nodes
  PRUint32 nodeCount = 0;
  rv = GetTreeNodeCount(aTree->mRootNode, &nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileObjectStream->WriteUint32(nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // 5.) Node data
  std::stack<nsRefPtr<sbFileSystemNode> > nodeStack;
  nodeStack.push(aTree->mRootNode);

  // Used to create a unique identifier for a parent node.
  // NOTE: The root node will always be 0.
  PRUint32 curNodeID = 0;

  while (!nodeStack.empty()) {
    nsRefPtr<sbFileSystemNode> curNode = nodeStack.top();
    nodeStack.pop();

    if (!curNode) {
      NS_WARNING("Coult not get the node from the node stack!");
      continue;
    }

    // Set the id of the current node.
    rv = curNode->SetNodeID(curNodeID);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not set the node ID!");
      continue;
    }

    rv = WriteNode(fileObjectStream, curNode);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not write curNode to disk!");
      continue;
    }

    sbNodeMap *curNodeChildren = curNode->GetChildren();
    if (curNodeChildren && curNodeChildren->size() > 0) {
      // Iterate through the entire child node map to set the parent ID 
      // and push each node into the node stack.
      sbNodeMapIter begin = curNodeChildren->begin();
      sbNodeMapIter end = curNodeChildren->end();
      sbNodeMapIter next;
      for (next = begin; next != end; ++next) {
        nsRefPtr<sbFileSystemNode> curChildNode(next->second);
        if (!curChildNode) {
          NS_WARNING("Could not get get curChildNode!");
          continue;
        }

        rv = curChildNode->SetParentID(curNodeID);
        if (NS_FAILED(rv)) {
          NS_WARNING("Could not set the parent GUID!");
          continue;
        }

        nodeStack.push(curChildNode);
      } 
    }

    // Bump the node ID count.
    ++curNodeID;
  }

  rv = fileObjectStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbFileSystemTreeState::LoadTreeState(nsID & aSessionID,
                                     nsString & aSessionAbsolutePath,
                                     PRBool *aIsRecursiveWatch,
                                     sbFileSystemNode **aOutRootNode)
{
  NS_ENSURE_ARG_POINTER(aOutRootNode);

  // Setup and read the serialized data as defined above.
  nsresult rv;
  nsCOMPtr<nsIFile> savedSessionFile;
  rv = GetTreeSessionFile(aSessionID,
                          PR_FALSE,  // don't create
                          getter_AddRefs(savedSessionFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Ensure that the session file exists.
  PRBool exists = PR_FALSE;
  if (NS_FAILED(savedSessionFile->Exists(&exists)) || !exists) {
    NS_WARNING("The saved session file no longer exists!");
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<sbFileObjectInputStream> fileObjectStream =
    new sbFileObjectInputStream();
  NS_ENSURE_TRUE(fileObjectStream, NS_ERROR_OUT_OF_MEMORY);

  rv = fileObjectStream->InitWithFile(savedSessionFile);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now begin to read in the data in the sequence defined above:
  // 1.) The tree schema version.
  PRUint32 schemaVersion = 0;
  rv = fileObjectStream->ReadUint32(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  // For now, just ensure that the schema version is the same.
  // In the future, a migration handler will need to be written.
  if (schemaVersion != TREE_SCHEMA_VERSION) {
    return NS_ERROR_FAILURE;
  }

  // 2.) Tree root absolute path
  rv = fileObjectStream->ReadString(aSessionAbsolutePath);
  NS_ENSURE_SUCCESS(rv, rv);
 
  // 3.) Is tree recursive watch.
  rv = fileObjectStream->ReadPRBool(aIsRecursiveWatch);
  NS_ENSURE_SUCCESS(rv, rv);

  // 4.) Number of nodes
  PRUint32 nodeCount = 0;
  rv = fileObjectStream->ReadUint32(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // 5.) Node data
  // Use the map to store guids to help rebuild the parent/child relationship.
  nsRefPtr<sbFileSystemNode> savedRootNode;
  sbNodeIDMap nodeIDMap;
  for (PRUint32 i = 0; i < nodeCount; i++) {
    nsRefPtr<sbFileSystemNode> curNode;
    rv = ReadNode(fileObjectStream, getter_AddRefs(curNode));
    // If one of the nodes is missing, it could corrupt the entire tree.
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(curNode, NS_ERROR_UNEXPECTED);

    // Assign this node into the node ID map.
    PRUint32 curNodeID;
    rv = curNode->GetNodeID(&curNodeID);
    // Once again, this will corrupt the entire tree if it fails.
    NS_ENSURE_SUCCESS(rv, rv);
    
    nodeIDMap.insert(sbNodeIDMapPair(curNodeID, curNode));

    // If this is the first node read, it is the root node. Simply stash the
    // references for later and continue.
    if (i == 0) {
      savedRootNode = curNode;
      continue;
    }

    // Setup the relationship between parent and child.
    rv = AssignRelationships(curNode, nodeIDMap);
    // If this fails, it will also corrupt the entire tree. 
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = fileObjectStream->Close();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not close the file object stream!");

  savedRootNode.forget(aOutRootNode);
  return NS_OK;
}

/* static */ nsresult
sbFileSystemTreeState::DeleteSavedTreeState(const nsID & aSessionID)
{
  nsresult rv;
  nsCOMPtr<nsIFile> sessionFile;
  rv = GetTreeSessionFile(aSessionID, PR_FALSE, getter_AddRefs(sessionFile));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool fileExists = PR_FALSE;
  if (NS_SUCCEEDED(sessionFile->Exists(&fileExists)) && fileExists) {
    rv = sessionFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbFileSystemTreeState::WriteNode(sbFileObjectOutputStream *aOutputStream,
                                 sbFileSystemNode *aOutNode)
{
  NS_ENSURE_ARG_POINTER(aOutputStream);
  NS_ENSURE_ARG_POINTER(aOutNode);

  nsresult rv;
  nsCOMPtr<nsISupports> writeSupports =
    do_QueryInterface(NS_ISUPPORTS_CAST(nsISerializable *, aOutNode), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return aOutputStream->WriteObject(writeSupports, PR_TRUE);
}

nsresult
sbFileSystemTreeState::ReadNode(sbFileObjectInputStream *aInputStream,
                                sbFileSystemNode **aOutNode)
{
  NS_ENSURE_ARG_POINTER(aInputStream);
  NS_ENSURE_ARG_POINTER(aOutNode);

  nsresult rv;
  nsCOMPtr<nsISupports> readSupports;
  rv = aInputStream->ReadObject(PR_TRUE, getter_AddRefs(readSupports));
  NS_ENSURE_SUCCESS(rv, rv);

  // Cast to a nsISerializable to work around ambigious issue with 
  // the sbFileSystemNode class.
  nsCOMPtr<nsISerializable> serializable = 
    do_QueryInterface(readSupports, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOutNode = static_cast<sbFileSystemNode *>(serializable.get());
  NS_IF_ADDREF(*aOutNode);

  return NS_OK;
}

nsresult
sbFileSystemTreeState::AssignRelationships(sbFileSystemNode *aChildNode,
                                           sbNodeIDMap & aParentGuidMap)
{
  NS_ENSURE_ARG_POINTER(aChildNode);

  nsresult rv;
  PRUint32 parentID;
  rv = aChildNode->GetParentID(&parentID);
  NS_ENSURE_SUCCESS(rv, rv);

  sbNodeIDMapIter found = aParentGuidMap.find(parentID);
  if (found == aParentGuidMap.end()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<sbFileSystemNode> parentNode(found->second);
  NS_ENSURE_TRUE(parentNode, NS_ERROR_UNEXPECTED);
  
  rv = parentNode->AddChild(aChildNode);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/* static */ nsresult
sbFileSystemTreeState::GetTreeSessionFile(const nsID & aSessionID,
                                          PRBool aShouldCreate,
                                          nsIFile **aOutFile)
{
  char idChars[NSID_LENGTH];
  aSessionID.ToProvidedString(idChars);
  nsString sessionFilename;
  sessionFilename.Append(NS_ConvertASCIItoUTF16(idChars));
  sessionFilename.Append(NS_LITERAL_STRING(SESSION_FILENAME_EXTENSION));

  nsresult rv;
  nsCOMPtr<nsIProperties> dirService = 
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> profileDir;
  rv = dirService->Get(NS_APP_PREFS_50_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(profileDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> treeFolder;
  rv = profileDir->Clone(getter_AddRefs(treeFolder));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = treeFolder->Append(NS_LITERAL_STRING(TREE_FOLDER_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool folderExists = PR_FALSE;
  if (NS_SUCCEEDED(treeFolder->Exists(&folderExists)) && !folderExists) {
    rv = treeFolder->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIFile> newFile;
  rv = treeFolder->Clone(getter_AddRefs(newFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = newFile->Append(sessionFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aShouldCreate) {
    PRBool exists = PR_FALSE;
    if (NS_SUCCEEDED(newFile->Exists(&exists)) && exists) {
      rv = newFile->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = newFile->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  newFile.swap(*aOutFile);
  return NS_OK;
}

nsresult
sbFileSystemTreeState::GetTreeNodeCount(sbFileSystemNode *aRootNode,
                                        PRUint32 *aNodeCount)
{
  NS_ENSURE_ARG_POINTER(aRootNode);
  NS_ENSURE_ARG_POINTER(aNodeCount);

  PRUint32 nodeCount = 0;

  std::stack<nsRefPtr<sbFileSystemNode> > nodeStack;
  nodeStack.push(aRootNode);

  while (!nodeStack.empty()) {
    nsRefPtr<sbFileSystemNode> curNode = nodeStack.top();
    nodeStack.pop();

    // Increment the node count.
    nodeCount++;

    sbNodeMap *childMap = curNode->GetChildren();
    if (!childMap || childMap->size() == 0) {
      continue;
    }
    
    // Push all children into the stack to count.
    sbNodeMapIter begin = childMap->begin();
    sbNodeMapIter end = childMap->end();
    sbNodeMapIter next;
    for (next = begin; next != end; ++next) {
      nodeStack.push(next->second);
    }
  }

  *aNodeCount = nodeCount;
  return NS_OK;
}

