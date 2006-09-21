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

/** 
* \file  PlaylistReaderPLSComponent.cpp
* \brief Songbird PLS Playlist Reader Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"
#include "PlaylistReaderPLS.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(CPlaylistReaderPLS)

static nsModuleComponentInfo sbPlaylistReaderPLS[] =
{
  {
    SONGBIRD_PLREADERPLS_CLASSNAME,
    SONGBIRD_PLREADERPLS_CID,
    SONGBIRD_PLREADERPLS_CONTRACTID,
    CPlaylistReaderPLSConstructor
  },

};

NS_IMPL_NSGETMODULE("SongbirdPlaylistReaderPLSComponent", sbPlaylistReaderPLS)
