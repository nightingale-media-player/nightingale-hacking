/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

//
//  sbMessageBox
//

function sbMessageBox( title, message )
{
  try
  {
    var messagebox_data = new Object();
    messagebox_data.title = title;
    messagebox_data.message = message;
    window.openDialog( "chrome://rmp_demo/content/messagebox.xul", "messagebox", "chrome,modal=yes, centerscreen", messagebox_data );
  }
  catch ( err )
  {
    alert("sbMessageBox - " + err);
  }
}

