/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file  PlaylistsourceComponent.cpp
* \brief Songbird Playlistsource Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"
#include "Playlistsource.h"

#define NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR( _Interface )                  \
  static _Interface * _Interface##SimpletonConstructor( void )                  \
  {                                                                             \
    static _Interface * m_Simpleton = NULL;                                     \
    NS_IF_ADDREF( m_Simpleton ? m_Simpleton : ( NS_IF_ADDREF( m_Simpleton = new _Interface() ), m_Simpleton ) ); \
    return m_Simpleton;                                                         \
  }                                                                             \
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR( _Interface, _Interface##SimpletonConstructor )

NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR(sbPlaylistsource)

static nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_PLAYLISTSOURCE_CLASSNAME, 
    SONGBIRD_PLAYLISTSOURCE_CID,
    SONGBIRD_PLAYLISTSOURCE_CONTRACTID,
    sbPlaylistsourceConstructor,
  }
};

NS_IMPL_NSGETMODULE("SongbirdPlaylistsourceComponent", components)

