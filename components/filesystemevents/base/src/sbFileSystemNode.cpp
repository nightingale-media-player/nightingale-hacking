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

#include "sbFileSystemNode.h"
#include <sbFileSystemCID.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsMemory.h>
#include <stack>


struct NodeContext
{
  NodeContext(const nsAString & aFullPath, sbFileSystemNode *aNode)
    : fullPath(aFullPath), node(aNode)
  {
  }

  nsString fullPath;
  nsRefPtr<sbFileSystemNode> node;
};


static NS_DEFINE_CID(kFileSystemTreeCID,
                     SONGBIRD_FILESYSTEMNODE_CID);

NS_IMPL_THREADSAFE_ISUPPORTS2(sbFileSystemNode,
                              nsISerializable,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbFileSystemNode, 
                             nsISerializable, 
                             nsIClassInfo)

sbFileSystemNode::sbFileSystemNode()
{
}

sbFileSystemNode::~sbFileSystemNode()
{
}

nsresult 
sbFileSystemNode::Init(const nsAString & aLeafName,
                       PRBool aIsDir,
                       PRUint64 aLastModify,
                       sbFileSystemNode *aParentNode)
{
  NS_ASSERTION(!aLeafName.IsEmpty(), "Error: Leaf name is empty!");

  mLeafName.Assign(aLeafName);
  mIsDir = aIsDir;
  mLastModify = aLastModify;
  mParentNode = aParentNode;
  return NS_OK;
}

nsresult
sbFileSystemNode::SetParentNode(sbFileSystemNode *aParentNode)
{
  NS_ENSURE_ARG_POINTER(aParentNode);
  mParentNode = aParentNode;
  return NS_OK;
}

nsresult
sbFileSystemNode::GetParentNode(sbFileSystemNode **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  if (mParentNode) {
    NS_ADDREF(*aRetVal = mParentNode);
  }
  else {
    *aRetVal = nsnull;
  }
  return NS_OK;
}

nsresult
sbFileSystemNode::SetChildren(const sbNodeMap & aNodeMap)
{
  mChildMap = aNodeMap;
  return NS_OK;
}

sbNodeMap*
sbFileSystemNode::GetChildren()
{
  return &mChildMap; 
}

nsresult
sbFileSystemNode::SetLeafName(const nsAString & aLeafName)
{
  mLeafName = aLeafName;
  return NS_OK;
}

nsresult
sbFileSystemNode::GetLeafName(nsAString & aLeafName)
{
  aLeafName.Assign(mLeafName);
  return NS_OK;
}

nsresult
sbFileSystemNode::SetIsDir(const PRBool aIsDir)
{
  mIsDir = aIsDir;
  return NS_OK;
}

nsresult
sbFileSystemNode::GetIsDir(PRBool *aIsDir)
{
  NS_ENSURE_ARG_POINTER(aIsDir);
  *aIsDir = mIsDir;
  return NS_OK;
}

nsresult
sbFileSystemNode::SetLastModify(const PRInt64 aLastModify)
{
  mLastModify = aLastModify;
  return NS_OK;
}

nsresult
sbFileSystemNode::GetLastModify(PRInt64 *aLastModify)
{
  NS_ENSURE_ARG_POINTER(aLastModify);
  *aLastModify = mLastModify;
  return NS_OK;
}

nsresult
sbFileSystemNode::AddChild(sbFileSystemNode *aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);

  nsString leafName;
  nsresult rv = aNode->GetLeafName(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  mChildMap.insert(sbNodeMapPair(leafName, aNode));

  return NS_OK;
}

nsresult
sbFileSystemNode::RemoveChild(sbFileSystemNode *aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);
  
  nsString nodeFileName;
  nsresult rv = aNode->GetLeafName(nodeFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  mChildMap.erase(nodeFileName);

  return NS_OK;
}

nsresult
sbFileSystemNode::GetChildCount(PRUint32 *aChildCount)
{
  NS_ENSURE_ARG_POINTER(aChildCount);
  *aChildCount = mChildMap.size();
  return NS_OK;
}

nsresult
sbFileSystemNode::ReplaceNode(const nsAString & aLeafName,
                              sbFileSystemNode *aReplacementNode)
{
  NS_ENSURE_ARG_POINTER(aReplacementNode);
  nsString leafName(aLeafName);
  mChildMap[leafName] = aReplacementNode;
  return NS_OK;
}

nsresult
sbFileSystemNode::SetParentID(const nsID & aID)
{
  mParentID = aID;
  return NS_OK;
}
 
nsresult 
sbFileSystemNode::GetParentID(nsID & aOutID)
{
  aOutID = mParentID;
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsISerializable

NS_IMETHODIMP 
sbFileSystemNode::Read(nsIObjectInputStream *aInputStream)
{
  NS_ENSURE_ARG_POINTER(aInputStream);

  nsresult rv;
  char *buffer;
  rv = aInputStream->ReadBytes(sizeof(nsID), &buffer);
  if (buffer) {
    mParentID = *((nsID *)buffer);
    NS_Free(buffer);
  }

  rv = aInputStream->ReadString(mLeafName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aInputStream->ReadBoolean(&mIsDir);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 lastmodify;
  rv = aInputStream->Read64(&lastmodify);
  if (NS_SUCCEEDED(rv)) {
    mLastModify = lastmodify;
  }
  else {
    mLastModify = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbFileSystemNode::Write(nsIObjectOutputStream *aOutputStream)
{
  NS_ENSURE_ARG_POINTER(aOutputStream);

  nsresult rv;
  rv = aOutputStream->WriteBytes(((char *)&mParentID), sizeof(nsID));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aOutputStream->WriteWStringZ(mLeafName.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aOutputStream->WriteBoolean(mIsDir);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 lastmodify = mLastModify;
  rv = aOutputStream->Write64(lastmodify);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIClassInfo

NS_IMETHODIMP
sbFileSystemNode::GetInterfaces(PRUint32* count, nsIID*** array)
{
  NS_ENSURE_ARG_POINTER(count);
  NS_ENSURE_ARG_POINTER(array);

  return NS_CI_INTERFACE_GETTER_NAME(sbFileSystemNode)(count, array);
}

NS_IMETHODIMP
sbFileSystemNode::GetHelperForLanguage(PRUint32 language,
                                    nsISupports** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetContractID(char** aContractID)
{
  NS_ENSURE_ARG_POINTER(aContractID);
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetClassDescription(char** aClassDescription)
{
  NS_ENSURE_ARG_POINTER(aClassDescription);
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetClassID(nsCID** aClassID)
{
  NS_ENSURE_ARG_POINTER(aClassID);
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  NS_ENSURE_ARG_POINTER(aImplementationLanguage);
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetFlags(PRUint32* aFlags)
{
  NS_ENSURE_ARG_POINTER(aFlags);
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemNode::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kFileSystemTreeCID;
  return NS_OK;
}

