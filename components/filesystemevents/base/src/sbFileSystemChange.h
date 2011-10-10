/*
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

#ifndef sbFileSystemChange_h_
#define sbFileSystemChange_h_

#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include "sbFileSystemTreeListener.h"

class sbFileSystemNode;


//
// \brief Abstract class for changes.
//
class sbFileSystemChange : public nsISupports
{
public:
  sbFileSystemChange();
  virtual ~sbFileSystemChange();

  NS_DECL_ISUPPORTS

  //
  // \brief Getter and setter for the change type of the change.
  //
  NS_IMETHOD SetChangeType(EChangeType aChangeType);
  NS_IMETHOD GetChangeType(EChangeType *aChangeType);

protected:
  EChangeType mChangeType;
};

//------------------------------------------------------------------------------

//
// \brief Simple class for mapping a change type and a file system node.
//        This class is used to determine diffs between file system snapshots.
//
class sbFileSystemNodeChange : public sbFileSystemChange 
{
public:
  sbFileSystemNodeChange();
  sbFileSystemNodeChange(sbFileSystemNode *aNode, EChangeType aChangeType);
  virtual ~sbFileSystemNodeChange();

  //
  // \brief Getter and setter for the changed node.
  //
  nsresult SetNode(sbFileSystemNode *aNode);
  nsresult GetNode(sbFileSystemNode **aRetVal);

private:
  nsRefPtr<sbFileSystemNode> mNode;
};

//------------------------------------------------------------------------------

class sbFileSystemPathChange : public sbFileSystemChange
{
public:
  sbFileSystemPathChange();
  sbFileSystemPathChange(const nsAString & aChangePath, 
                         EChangeType aChangeType);
  virtual ~sbFileSystemPathChange();

  //
  // \brief Getter and setter for the changed path.
  //
  nsresult SetChangePath(const nsAString & aChangePath);
  nsresult GetChangePath(nsAString & aChangePath);

private:
  nsString    mChangePath;
};

#endif  // sbFileSystemChange_h_

