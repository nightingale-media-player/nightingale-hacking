/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

EXPORTED_SYMBOLS = ["BatchHelper", "MultiBatchHelper"];

function BatchHelper() {
  this._depth = 0;
}

BatchHelper.prototype.begin =
function BatchHelper_begin()
{
  this._depth++;
}

BatchHelper.prototype.end =
function BatchHelper_end()
{
  this._depth--;
  if (this._depth < 0) {
    throw new Error("Invalid batch depth!");
  }
}

BatchHelper.prototype.depth =
function BatchHelper_depth()
{
  return this._depth;
}

BatchHelper.prototype.isActive =
function BatchHelper_isActive()
{
  return this._depth > 0;
}

function MultiBatchHelper() {
  this._libraries = {};
}

MultiBatchHelper.prototype.get =
function MultiBatchHelper_get(aLibrary)
{
  var batch = this._libraries[aLibrary.guid];
  if (!batch) {
    batch = new BatchHelper();
    this._libraries[aLibrary.guid] = batch;
  }
  return batch;
}

MultiBatchHelper.prototype.begin =
function MultiBatchHelper_begin(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.begin();
}

MultiBatchHelper.prototype.end =
function MultiBatchHelper_end(aLibrary)
{
  var batch = this.get(aLibrary);
  batch.end();
}

MultiBatchHelper.prototype.depth =
function MultiBatchHelper_depth(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.depth();
}

MultiBatchHelper.prototype.isActive =
function MultiBatchHelper_isActive(aLibrary)
{
  var batch = this.get(aLibrary);
  return batch.isActive();
}

