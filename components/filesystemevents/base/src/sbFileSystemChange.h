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

#ifndef sbFileSystemChange_h_
#define sbFileSystemChange_h_

class sbFileSystemNode;
#include "sbFileSystemTreeListener.h"


//
// \brief Simple class for mapping a change type and a file system node.
//        This class is used to determine diffs between file system snapshots.
//
class sbFileSystemChange : public nsISupports
{
public:
  sbFileSystemChange();
  sbFileSystemChange(sbFileSystemNode *aNode, EChangeType aChangeType);
  virtual ~sbFileSystemChange();

  NS_DECL_ISUPPORTS

  nsresult SetNode(sbFileSystemNode *aNode)
  {
    NS_ENSURE_ARG_POINTER(aNode);
    mNode = aNode;
    return NS_OK;
  }

  nsresult GetNode(sbFileSystemNode **aRetVal)
  {
    NS_ENSURE_ARG_POINTER(aRetVal);
    NS_IF_ADDREF(*aRetVal = mNode);
    return NS_OK;
  }

  nsresult SetChangeType(EChangeType aChangeType)
  {
    mChangeType = aChangeType;
    return NS_OK;
  }

  nsresult GetChangeType(EChangeType *aRetVal)
  {
    NS_ENSURE_ARG_POINTER(aRetVal);
    *aRetVal = mChangeType;
    return NS_OK;
  }

private:
  nsRefPtr<sbFileSystemNode> mNode;
  EChangeType                mChangeType; 
};

NS_IMPL_ISUPPORTS0(sbFileSystemChange)

sbFileSystemChange::sbFileSystemChange()
{
}

sbFileSystemChange::sbFileSystemChange(sbFileSystemNode *aNode, 
                                       EChangeType aChangeType)
  : mNode(aNode), mChangeType(aChangeType)
{
  NS_ASSERTION(aNode, "Invalid node passed in to constructor!");
}

sbFileSystemChange::~sbFileSystemChange()
{
}

#endif  // sbFileSystemChange_h_

