/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

// This keeps a list of objects that can act as drag and drop sources, so a string representing the index of the object
// in the tracker can be passed into a TransferData object for Drag and Drop operations. This is necessary until someone
// either documents or is able to explain how it is possible to pass arbitrary objects instead of strings.
var sbDnDSourceTracker = {

  m_list : null,

  registerDnDSource : function (source) {
    if (this.m_list == null) this.m_list = Array();
    for (var i in this.m_list) {
      if (this.m_list[i] == source) return;
    }
    this.m_list.push(source);
  },

  unregisterDnDSource : function (source) {
    if (this.m_list == null) return;
    for (var i in this.m_list) {
      if (this.m_list[i] == source) {
        this.m_list.splice(i, 1);
        return;
      }
    }
  },
  
  getDnDSourceIndex : function (source) {
    if (this.m_list == null) return -1;
    for (var i in this.m_list) {
      if (this.m_list[i] == source) return i;
    }
    return -1;
  },
  
  getDnDSource : function (index) {
    if (this.m_list == null) return null;
    return this.m_list[index];
  }

};

