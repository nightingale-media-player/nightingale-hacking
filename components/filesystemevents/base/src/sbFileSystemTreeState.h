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

#ifndef sbFileSystemTreeState_h_
#define sbFileSystemTreeState_h_

#include "sbFileSystemNode.h"
#include <sbFileObjectStreams.h>
#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsIUUIDGenerator.h>
#include <nsStringAPI.h>

class sbFileSystemNode;
class sbFileSystemTree;

typedef std::map<PRUint32, nsRefPtr<sbFileSystemNode> > sbNodeIDMap;
typedef sbNodeIDMap::value_type sbNodeIDMapPair;
typedef sbNodeIDMap::const_iterator sbNodeIDMapIter;


class sbFileSystemTreeState : public nsISupports
{
public:
  sbFileSystemTreeState();
  virtual ~sbFileSystemTreeState();

  NS_DECL_ISUPPORTS

  nsresult SaveTreeState(sbFileSystemTree *aFileSystemTree,
                         const nsID & aSessionID);

  nsresult LoadTreeState(nsID & aSessionID,
                         nsString & aSessionAbsolutePath,
                         PRBool *aIsRecursiveWatch,
                         sbFileSystemNode **aOutRootNode);

  static nsresult DeleteSavedTreeState(const nsID & aSessionID);

protected:
  nsresult WriteNode(sbFileObjectOutputStream *aOutputStream,
                     sbFileSystemNode *aOutNode);
  
  nsresult ReadNode(sbFileObjectInputStream *aInputStream,
                    sbFileSystemNode **aOutNode);

  nsresult AssignRelationships(sbFileSystemNode *aChildNode,
                               sbNodeIDMap & aParentGuidMap);

  static nsresult GetTreeSessionFile(const nsID & aSessionID,
                                     PRBool aShouldCreate,
                                     nsIFile **aOutFile);

  nsresult GetTreeNodeCount(sbFileSystemNode *aRootNode,
                            PRUint32 *aNodeCount);

private:
  nsCOMPtr<nsIUUIDGenerator> mUuidGen;
};

#endif  // sbFileSystemTreeState_h_

