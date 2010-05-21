/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef sbSongkickDBService_h_
#define sbSongkickDBService_h_

#include <Songkick.h>

#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsIArray.h>

class sbSongkickQuery;
class sbSongkickResultEnumerator;


class sbSongkickDBService : public sbPISongkickDBService
{
public:
  sbSongkickDBService();
  virtual ~sbSongkickDBService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBPISONGKICKDBSERVICE
};

#define SONGBIRD_SONGKICKDBSERVICE_CONTRACTID                  \
  "@songbirdnest.com/songkick/dbservice;1"
#define SONGBIRD_SONGKICKDBSERVICE_CLASSNAME                   \
  "Songkick DB Service"
#define SONGBIRD_SONGKICKDBSERVICE_CID                         \
  {0xe81479c8, 0x1dd1, 0x11b2, {0xa9, 0xa1, 0xae, 0x97, 0x1f, 0x22, 0xf7, 0xfc}}


//------------------------------------------------------------------------------

class sbSongkickConcertInfo : public sbISongkickConcertInfo
{
  friend class sbSongkickResultEnumerator;

public:
  sbSongkickConcertInfo();
  virtual ~sbSongkickConcertInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_SBISONGKICKCONCERTINFO

  nsresult Init(const nsAString & aArtistname,
                const nsAString & aArtistURL,
                const nsAString & aID,
                const nsAString & aTS,
                const nsAString & aVenue,
                const nsAString & aCity,
                const nsAString & aTitle,
                const nsAString & aURL,
                const nsAString & aVenueURL,
                const nsAString & aTickets,
                nsIArray *aArtistsConcertInfo,
                const nsAString & aLibartist);

protected:
  nsString           mArtistname;
  nsString           mArtistURL;
  nsString           mID;
  nsString           mTS;
  nsString           mVenue;
  nsString           mCity;
  nsString           mTitle;
  nsString           mURL;
  nsString           mVenueURL;
  nsString           mTickets;
  nsCOMPtr<nsIArray> mArtistConcertInfoArray;
  nsString           mLibArtist;
};

//------------------------------------------------------------------------------

class sbSongkickArtistConcertInfo : public sbISongkickArtistConcertInfo
{
public:
  sbSongkickArtistConcertInfo();
  virtual ~sbSongkickArtistConcertInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_SBISONGKICKARTISTCONCERTINFO

  nsresult Init(const nsAString & aArtistName,
                const nsAString & aArtistURL);

private:
  nsString mArtistName;
  nsString mArtistURL;
};

#endif  // sbSongkickDBService_h_

