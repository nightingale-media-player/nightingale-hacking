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

#include "sbFileSystemChange.h"
#include "sbFileSystemNode.h"


NS_IMPL_THREADSAFE_ISUPPORTS0(sbFileSystemChange)

sbFileSystemChange::sbFileSystemChange()
{
}

sbFileSystemChange::~sbFileSystemChange()
{
}

NS_IMETHODIMP
sbFileSystemChange::SetChangeType(EChangeType aChangeType)
{
  mChangeType = aChangeType;
  return NS_OK;
}

NS_IMETHODIMP
sbFileSystemChange::GetChangeType(EChangeType *aChangeType)
{
  NS_ENSURE_ARG_POINTER(aChangeType);
  *aChangeType = mChangeType;
  return NS_OK;
}

//------------------------------------------------------------------------------

sbFileSystemNodeChange::sbFileSystemNodeChange()
{
}

sbFileSystemNodeChange::sbFileSystemNodeChange(sbFileSystemNode *aNode,
                                               EChangeType aChangeType)
  : mNode(aNode) 
{
  mChangeType = aChangeType;
  NS_ASSERTION(aNode, "Invalid node passed into constructor!");
}

sbFileSystemNodeChange::~sbFileSystemNodeChange()
{
}

nsresult
sbFileSystemNodeChange::SetNode(sbFileSystemNode *aNode)
{
  NS_ENSURE_ARG_POINTER(aNode);
  mNode = aNode;
  return NS_OK;
}

nsresult
sbFileSystemNodeChange::GetNode(sbFileSystemNode **aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);
  NS_IF_ADDREF(*aRetVal = mNode);
  return NS_OK;
}

//------------------------------------------------------------------------------

sbFileSystemPathChange::sbFileSystemPathChange()
{
}

sbFileSystemPathChange::sbFileSystemPathChange(const nsAString & aChangePath,
                                               EChangeType aChangeType)
  : mChangePath(aChangePath)
{
  mChangeType = aChangeType;
}

sbFileSystemPathChange::~sbFileSystemPathChange()
{
}

nsresult
sbFileSystemPathChange::SetChangePath(const nsAString & aChangePath)
{
  mChangePath.Assign(aChangePath);
  return NS_OK;
}

nsresult
sbFileSystemPathChange::GetChangePath(nsAString & aChangePath)
{
  aChangePath.Assign(mChangePath);
  return NS_OK;
}

