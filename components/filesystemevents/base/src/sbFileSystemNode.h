/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
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

#ifndef sbFileSystemNode_h_
#define sbFileSystemNode_h_

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsTArray.h>
#include <nsISerializable.h>
#include <nsIClassInfo.h>
#include <map>

class sbFileSystemNode;
typedef nsTArray<nsRefPtr<sbFileSystemNode> > sbNodeArray;
typedef std::map<nsString, nsRefPtr<sbFileSystemNode> > sbNodeMap;
typedef sbNodeMap::value_type sbNodeMapPair;
typedef sbNodeMap::const_iterator sbNodeMapIter;


//------------------------------------------------------------------------------
// A class to model a file-system resource node (w/ children)
//------------------------------------------------------------------------------
class sbFileSystemNode : public nsISerializable,
                         public nsIClassInfo 
{
public:
  sbFileSystemNode();
  virtual ~sbFileSystemNode();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  //
  // \brief Initialize a node with a given leaf name, directory flag, last 
  //        modification time, and a parent node.
  // \param aLeafName The leaf name to assign to the node, this can not be
  //                  a empty string.
  // \param aIsDir If this node is a directory.
  // \param aLastModify The last modification time stamp for the node.
  // \param aParentNode The parent node to assign to this node.
  //
  nsresult Init(const nsAString & aLeafName,
                bool aIsDir,
                PRUint64 aLastModify);
 
  //
  // \brief Setters and getters for the child array of this node.
  //
  nsresult SetChildren(const sbNodeMap & aNodeMap);
  sbNodeMap* GetChildren();
  
  //
  // \brief Setters and getters for the leaf name of this node.
  //
  nsresult SetLeafName(const nsAString & aLeafName);
  nsresult GetLeafName(nsAString & aLeafName);
  
  //
  // \brief Setters and getters for the directory flag of this node.
  //
  nsresult SetIsDir(const bool aIsDir);
  nsresult GetIsDir(bool *aIsDir);
  
  //
  // \brief Setters and getters of the last modification time stamp of
  //        this node.
  //
  nsresult SetLastModify(const PRInt64 aLastModify);
  nsresult GetLastModify(PRInt64 *aLastModify);
 
  //
  // \brief Add or remove a child node from this node.
  //
  nsresult AddChild(sbFileSystemNode *aNode);
  nsresult RemoveChild(sbFileSystemNode *aNode);

  //
  // \brief Get the number of children this node has.
  // \param aChildCount The out-param of the number of children.
  //
  nsresult GetChildCount(PRUint32 *aChildCount);

  //
  // \brief Replace a child node of the given leaf name with a new node.
  // \pram aLeafName The leaf name of the node to replace
  // \param aReplacementNode The new node pointer to replace the child with.
  //
  nsresult ReplaceNode(const nsAString & aLeafName,
                       sbFileSystemNode *aReplacementNode);

  //
  // \brief Setter and getters of the node ID. 
  //        This is only used for serialization and the value is never 
  //        generated.
  //
  nsresult SetNodeID(PRUint32 aID);
  nsresult GetNodeID(PRUint32 *aID);

  //
  // \brief Getter and setters for the ID of the parent node. 
  //        This is only used for serialization and the value is never 
  //        generated.
  //
  nsresult SetParentID(const PRUint32 aID);
  nsresult GetParentID(PRUint32 *aOutID);

private:
  sbNodeMap                  mChildMap;
  nsString                   mLeafName;
  PRUint32                   mID;
  PRUint32                   mParentID;
  bool                     mIsDir;
  PRInt64                    mLastModify;
};

#endif

