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


NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileSystemNode, nsISupports)

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
  NS_ADDREF(*aRetVal = mParentNode);
  return NS_OK;
}

nsresult
sbFileSystemNode::SetChildren(const sbNodeArray & aNodeArray)
{
  mChildren.Clear();
  mChildren.AppendElements(aNodeArray);
  return NS_OK;
}

nsresult
sbFileSystemNode::GetChildren(sbNodeArray & aNodeArray)
{
  aNodeArray.AppendElements(mChildren);
  return NS_OK;
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
  mChildren.AppendElement(nsRefPtr<sbFileSystemNode>(aNode));
  return NS_OK;
}

nsresult
sbFileSystemNode::RemoveChild(sbFileSystemNode *aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);
  
  nsString nodeFileName;
  nsresult rv = aNode->GetLeafName(nodeFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Loop through |mChildren| to find a leaf name that matches the leaf name
  // of |aNode|.
  for (PRUint32 i = 0; i < mChildren.Length(); i++) {
    nsString curChildLeafName;
    rv = mChildren[i]->GetLeafName(curChildLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (nodeFileName.Equals(curChildLeafName)) {
      mChildren.RemoveElementAt(i);
      break;
    }
  }

  return NS_OK;
}

nsresult
sbFileSystemNode::GetChildCount(PRUint32 *aChildCount)
{
  NS_ENSURE_ARG_POINTER(aChildCount);
  *aChildCount = mChildren.Length(); 
  return NS_OK;
}

nsresult
sbFileSystemNode::ReplaceNode(const nsAString & aLeafName,
                              sbFileSystemNode *aReplacementNode)
{
  NS_ENSURE_ARG_POINTER(aReplacementNode);

  // Find the node with the same leaf name
  nsresult rv;
  PRBool foundMatch = PR_FALSE;
  PRUint32 childCount = mChildren.Length();
  for (PRUint32 i = 0; i < childCount && !foundMatch; i++) {
    nsString curChildLeafName;
    rv = mChildren[i]->GetLeafName(curChildLeafName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (curChildLeafName.Equals(aLeafName)) {
      mChildren.ReplaceElementsAt(i, 1, aReplacementNode);
      foundMatch = PR_TRUE;
    }
  }

  return (foundMatch ? NS_OK : NS_ERROR_FAILURE);
}

