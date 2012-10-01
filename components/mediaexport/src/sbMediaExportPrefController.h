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

#ifndef sbMediaExportPrefController_h_
#define sbMediaExportPrefController_h_

#include <nsIObserver.h>


//------------------------------------------------------------------------------
// Pure virtual listener class for listening to pref changes.

class sbMediaExportPrefListener
{
public:
  NS_IMETHOD OnBoolPrefChanged(const nsAString & aPrefName,
                               const bool aNewPrefValue) = 0;
};

//------------------------------------------------------------------------------
// Utility class to abstract all the pref listening and management out of the
// export media service.

class sbMediaExportPrefController : public nsIObserver
{
public:
  sbMediaExportPrefController();
  virtual ~sbMediaExportPrefController();

  nsresult Init(sbMediaExportPrefListener *aListener);
  nsresult Shutdown();

  // Utility method, returns true if any media should be exported.
  bool GetShouldExportAnyMedia();
    
  // Utility method, returns true if any playlist type should be exported.
  bool GetShouldExportAnyPlaylists();

  // Utility method, used to determine if the pref has been set to prevent
  // the export agent from starting.
  bool GetShouldStartExportAgent();

  bool GetShouldExportTracks();
  bool GetShouldExportPlaylists();
  bool GetShouldExportSmartPlaylists();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

private:
  bool mShouldExportTracks;
  bool mShouldExportPlaylists;
  bool mShouldExportSmartPlaylists;
  bool mShouldStartExportAgent;

  sbMediaExportPrefListener *mListener;  // weak
};

#endif  // sbMediaExportPrefController_h_

