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


#import <Foundation/Foundation.h>


//------------------------------------------------------------------------------
// Base class for stashing objects from iTunes.

@interface SBiTunesObject : NSObject
{
  AEDesc mDescriptor;
}

- (id)initWithAppleEvent:(AppleEvent *)aAppleEvent;
- (AEDesc *)descriptor;

@end

//------------------------------------------------------------------------------
// Playlist class for representing playlists from iTunes.

@interface SBiTunesPlaylist : SBiTunesObject
{
}

- (NSString *)playlistName;

@end

//------------------------------------------------------------------------------
// Manager class for modifying the users iTunes library via ObjC.

@interface SBiTunesLibraryManager : NSObject
{
}

+ (SBiTunesPlaylist *)mainLibraryPlaylist;
+ (SBiTunesPlaylist *)songbirdPlaylistFolder;

- (void)addTracks:(NSArray *)aFilePathArray
       toPlaylist:(SBiTunesPlaylist *)aTargetPlaylist;

- (void)addPlaylistNamed:(NSString *)aPlaylistName;

- (void)removePlaylistNamed:(NSString *)aPlaylistName;

@end

