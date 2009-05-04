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

#ifndef SBITUNESIMPORTERALBUMARTLISTENER_H_
#define SBITUNESIMPORTERALBUMARTLISTENER_H_

#include <sbIAlbumArtListener.h>

/**
 * Simple listener class used for album art updating of imported items
 */
class sbiTunesImporterAlbumArtListener : public sbIAlbumArtListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTLISTENER

  /**
   * Allocator for this class
   */
  static sbiTunesImporterAlbumArtListener * New();
private:
  /**
   * Just specified to prevent allocation outside of New
   */
  sbiTunesImporterAlbumArtListener();
  /**
   * Just specified to prevent deallocation outside of Release
   */
  ~sbiTunesImporterAlbumArtListener();
};

#endif /* SBITUNESIMPORTERALBUMARTLISTENER_H_ */
