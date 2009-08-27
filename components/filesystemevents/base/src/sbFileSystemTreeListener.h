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

#ifndef sbFileSystemTreeListener_h_
#define sbFileSystemTreeListener_h_

#include <nsStringAPI.h>
#include <nsTArray.h>

typedef nsTArray<nsString> sbStringArray;


//
// \brief Enum for node change types used when diffing snapshots of nodes.
//
typedef enum {
  eChanged   = 0,
  eAdded     = 1,
  eRemoved   = 2,
} EChangeType;


//------------------------------------------------------------------------------
// A pure virtual class for listening to tree events.
//------------------------------------------------------------------------------
class sbFileSystemTreeListener
{
public:
  virtual ~sbFileSystemTreeListener() {};
  //
  // \brief Callback function for when changes are found in the tree.
  //
  NS_IMETHOD OnChangeFound(const nsAString & aChangePath, 
                           EChangeType aChangeType) = 0;

  //
  // \brief Callback function for when the tree has been full intialized.
  // \param aDirPathArray A string array containing all the directory paths
  //                      that were discovered during the initial build.
  //                      NOTE: This does not include the root directory.
  //
  NS_IMETHOD OnTreeReady(const nsAString & aTreeRootPath,
                         sbStringArray & aDirPathArray) = 0;

  //
  // \brief Callback method when the tree can not build itself at the 
  //        specified root path.
  //
  NS_IMETHOD OnRootPathMissing() = 0;

  //
  // \brief Callback function for when a tree failed to de-serialize. When 
  //        this method is sent, the tree is in a non-initialized state and
  //        will need to be re-initialized in order to become active. 
  //
  NS_IMETHOD OnTreeSessionLoadError() = 0;
};

#endif  // sbFileSystemTreeListener_h_

