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

#ifndef sbFileSystemTreeListener_h_
#define sbFileSystemTreeListener_h_

#include <nsStringAPI.h>


//
// \brief Enum for node change types used when diffing snapshots of nodes.
//
typedef enum {
  eUnchanged = 0,
  eChanged   = 1,
  eAdded     = 2,
  eRemoved   = 3,
} EChangeType;


//------------------------------------------------------------------------------
// A pure virtual class for listening to tree events.
//------------------------------------------------------------------------------
class sbFileSystemTreeListener
{
public:
  //
  // \brief Callback function for when changes are found in the tree.
  //
  virtual void OnChangeFound(nsAString & aChangePath, 
                             EChangeType aChangeType) = 0;

  //
  // \brief Callback function for when the tree has been full intialized.
  //
  virtual void OnTreeReady() = 0;
};

#endif  // sbFileSystemTreeListener_h_

