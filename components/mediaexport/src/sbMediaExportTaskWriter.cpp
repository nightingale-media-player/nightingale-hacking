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

#include "sbMediaExportTaskWriter.h"

#include <nsIFile.h>
#include <nsIURI.h>
#include <nsIFileURL.h>
#include <nsDirectoryServiceUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <sbStandardProperties.h>
#include <sbDebugUtils.h>


/*
  This class helps create the exported media task file.
  The current structure of the file looks like:

  [schema-version:1]
  [added-medialists]
  0=Added Playlist 1
  1=Added Playlist 2
  [removed-medialists]
  0=Removed Playlist A
  1=Removed Playlist B
  [updated-smartplaylist:SMART PLAYLIST]
  0=/path/to/file1.mp3
  1=/path/to/file2.mp3
  [added-mediaitems:TEST PLAYLIST]
  0=/path/to/file1.mp3
  1=/path/to/file2.mp3
  2=/path/to/file3.mp3

  All playlist names and filenames are URL-escaped, and after unescaping will
  be in UTF-8.

*/


NS_IMPL_ISUPPORTS0(sbMediaExportTaskWriter)

sbMediaExportTaskWriter::sbMediaExportTaskWriter()
{
  SB_PRLOG_SETUP(sbMediaExportTaskWriter);
}

sbMediaExportTaskWriter::~sbMediaExportTaskWriter()
{
}

nsresult
sbMediaExportTaskWriter::Init()
{
  LOG("%s: Setting up a task writer instance", __FUNCTION__);

  // Create an nsINetUtil instance to do URL-escaping.
  nsresult rv;
  mNetUtil = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // First, setup the actual export data file to write to.
  nsCOMPtr<nsIFile> taskFile;
  rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR, 
                              getter_AddRefs(taskFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Determine if a task file already exists.
  rv = taskFile->Append(NS_LITERAL_STRING(TASKFILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = taskFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  // If an existing task file already exists. Find a unique extension.
  // NOTE: Consider using nsIFile::CreateUnique
  PRUint32 curNumeralExtension = 0;
  while (exists) {
    nsCString leafName(TASKFILE_NAME);
    leafName.AppendInt(++curNumeralExtension);

    rv = taskFile->SetNativeLeafName(leafName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = taskFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = taskFile->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = taskFile->GetPath(mTaskFilepath);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Creating task file at '%s'", 
        __FUNCTION__, NS_ConvertUTF16toUTF8(mTaskFilepath).get());
  
  // Init the output stream based on the file created above.
#if defined(XP_WIN) 
  mOutputStream.open(mTaskFilepath.get());
#else
  mOutputStream.open(NS_ConvertUTF16toUTF8(mTaskFilepath).get());
#endif
  // Write out the current schema version to disk
  mOutputStream << "["
                << TASKFILE_SCHEMAVERSION_HEADER
                << ":"
                << TASKFILE_SCHEMAVERSION
                << "]"
                << std::endl;

  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::Finish()
{
  LOG("%s: Done writing task file at '%s'", 
        __FUNCTION__, mTaskFilepath.get());
  
  mOutputStream.close();
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteAddedMediaListsHeader()
{
  LOG("%s Writing header '%s'",
        __FUNCTION__, TASKFILE_ADDEDMEDIALISTS_HEADER);
  
  mOutputStream << "["
                << TASKFILE_ADDEDMEDIALISTS_HEADER
                << "]"
                << std::endl;
  
  // Reset the output index
  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteRemovedMediaListsHeader()
{
  LOG("%s Writing header '%s'",
        __FUNCTION__, TASKFILE_REMOVEDMEDIALISTS_HEADER);
  
  mOutputStream << "["
                << TASKFILE_REMOVEDMEDIALISTS_HEADER
                << "]"
                << std::endl;
  
  // Reset the output index
  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteUpdatedSmartPlaylistHeader(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  LOG("%s Writing header '%s'",
        __FUNCTION__, TASKFILE_UPDATEDSMARTPLAYLIST_HEADER);

  nsresult rv;

  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString escaped;
  rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(mediaListName),
                              nsINetUtil::ESCAPE_URL_PATH,
                              escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  mOutputStream << "["
                << TASKFILE_UPDATEDSMARTPLAYLIST_HEADER
                << ":"
                << escaped.get()
                << "]"
                << std::endl;

  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteAddedMediaItemsListHeader(sbIMediaList *aMediaList, PRBool aIsMainLibrary)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString escaped;
  if (aIsMainLibrary) {
    // If you use this as your playlist name, you get what you deserve.
    escaped.AssignLiteral(SONGBIRD_MAIN_LIBRARY_NAME);
  } else {
    rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(mediaListName),
            nsINetUtil::ESCAPE_URL_PATH, escaped);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  LOG("%s: Writing header '%s' for medialist name '%s'",
        __FUNCTION__, 
        TASKFILE_ADDEDMEDIAITEMS_HEADER, escaped.get());

  // Header format looks like this
  // [added-mediaitems:Playlist Name]
  mOutputStream << "[" 
                << TASKFILE_ADDEDMEDIAITEMS_HEADER
                << ":"
                << escaped.get()
                << "]"
                << std::endl;

  // Reset the output index
  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteUpdatedMediaItemsListHeader()
{
  LOG("%s: Writing header '%s' for updated items",
        __FUNCTION__,
        TASKFILE_UPDATEDMEDIAITEMS_HEADER);

  // Header format looks like this
  // [updated-mediaitems]
  mOutputStream << "["
                << TASKFILE_UPDATEDMEDIAITEMS_HEADER
                << "]"
                << std::endl;

  // Reset the output index
  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteAddedTrack(sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Get the path of mediaitem and write that info to disk
  nsCOMPtr<nsIURI> contentUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> contentFileURL = do_QueryInterface(contentUri, &rv);
  if (NS_FAILED(rv) || !contentFileURL) {
    // If this is not a local resource, just warn and return.
    NS_WARNING("WARNING: Tried to write a remote mediaitem resource!");
    return NS_OK;
  }

  nsCOMPtr<nsIFile> contentFile;
  rv = contentFileURL->GetFile(getter_AddRefs(contentFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemContentPath;
  rv = contentFile->GetPath(itemContentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = contentFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(exists, NS_ERROR_FILE_NOT_FOUND);

  nsCString escaped;
  rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(itemContentPath),
          nsINetUtil::ESCAPE_URL_PATH, escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString guid;
  rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  LOG("%s: Writing added track '%s'",
        __FUNCTION__, escaped.get());

  mOutputStream << NS_LossyConvertUTF16toASCII(guid).get()
                << "="
                << escaped.get()
                << std::endl;

  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteUpdatedTrack(sbIMediaItem *aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Get the itunes id of the media item
  nsString iTunesID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ITUNES_GUID),
                               iTunesID);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(!iTunesID.IsEmpty(), NS_ERROR_FAILURE);

  // Get the path of mediaitem and write that info to disk
  nsCOMPtr<nsIURI> contentUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> contentFileURL = do_QueryInterface(contentUri, &rv);
  if (NS_FAILED(rv) || !contentFileURL) {
    // If this is not a local resource, just warn and return.
    NS_WARNING("WARNING: Tried to write a remote mediaitem resource!");
    return NS_OK;
  }

  nsCOMPtr<nsIFile> contentFile;
  rv = contentFileURL->GetFile(getter_AddRefs(contentFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemContentPath;
  rv = contentFile->GetPath(itemContentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = contentFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(exists, NS_ERROR_FILE_NOT_FOUND);

  nsCString escaped;
  rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(itemContentPath),
          nsINetUtil::ESCAPE_URL_PATH, escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Writing updated track '%s' -> '%s'",
        __FUNCTION__,
        NS_LossyConvertUTF16toASCII(iTunesID).get(),
        escaped.get());

  mOutputStream << NS_LossyConvertUTF16toASCII(iTunesID).get()
                << "="
                << escaped.get()
                << std::endl;

  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteMediaListName(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsString listName;
  rv = aMediaList->GetName(listName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString escaped;
  rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(listName),
          nsINetUtil::ESCAPE_URL_PATH, escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Writing media list name '%s'",
        __FUNCTION__, escaped.get());

  mOutputStream << mCurOutputIndex++
                << "="
                << escaped.get()
                << std::endl;

  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteEscapedString(const nsAString & aString)
{
  nsCString escaped;
  nsresult rv = mNetUtil->EscapeString(NS_ConvertUTF16toUTF8(aString),
          nsINetUtil::ESCAPE_URL_PATH, escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("%s: Writing string '%s'",
        __FUNCTION__, escaped.get());

  mOutputStream << mCurOutputIndex++
                << "="
                << escaped.get()
                << std::endl;

  return NS_OK;
}

