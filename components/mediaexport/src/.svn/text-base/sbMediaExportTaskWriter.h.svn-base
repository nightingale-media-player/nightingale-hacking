/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbMediaExportTaskWriter_h_
#define sbMediaExportTaskWriter_h_

#include "sbMediaExportDefines.h"
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <iostream>
#include <fstream>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <nsNetUtil.h>

class sbMediaExportTaskWriter : public nsISupports
{
public:
  sbMediaExportTaskWriter();
  virtual ~sbMediaExportTaskWriter();

  NS_DECL_ISUPPORTS

  //
  // \brief Initialize a task writer. This method will setup the necessary
  //        resources for writing the media export data to the task file.
  //
  nsresult Init();

  //
  // \brief This method should be called when all the export data has been
  //        written out.
  //
  nsresult Finish();

  //
  // \brief Write the header for the "added media lists" exported data. 
  //
  nsresult WriteAddedMediaListsHeader();

  //
  // \brief Write the header for the "removed media lists" exported data.
  //
  nsresult WriteRemovedMediaListsHeader();

  //
  // \brief Write the header for the "updated smart playlist" exported data.
  // \param aMediaList The media list that has been updated.
  //
  nsresult WriteUpdatedSmartPlaylistHeader(sbIMediaList *aMediaList);

  //
  // \brief Write the header for the "added media items" exported data.
  // \param aMediaList The parent media list of the added media items.
  // \param aIsMainLibrary Whether or not this list is the main library.
  //
  nsresult WriteAddedMediaItemsListHeader(sbIMediaList *aMediaList,
                                          PRBool aIsMainLibrary = PR_FALSE);

  //
  // \brief Write the header for the "updated media items" exported data.
  //
  nsresult WriteUpdatedMediaItemsListHeader();

  //
  // \brief Write a added track out to the exported task file.
  // \param aMediaItem The media item to write out to disk.
  //
  nsresult WriteAddedTrack(sbIMediaItem *aMediaItem);

  //
  // \brief Write an updated track out to the exported task file.
  // \param aMediaItem The media item to write out to disk.
  //
  nsresult WriteUpdatedTrack(sbIMediaItem *aMediaItem);

  //
  // \brief Write a media list out to the exported task file.
  // \param aMediaList The media list to write out to the task file.
  //
  nsresult WriteMediaListName(sbIMediaList *aMediaList);

  //
  // \brief Write a string out to the exported task file, after URL-escaping it.
  // \param aString The string to write out to the task file.
  //
  nsresult WriteEscapedString(const nsAString & aString);

private:
  nsString       mTaskFilepath;
  PRUint32       mCurOutputIndex;
  std::ofstream  mOutputStream;
  nsCOMPtr<nsINetUtil> mNetUtil;
};

#endif  // sbMediaExportTaskWriter_h_

