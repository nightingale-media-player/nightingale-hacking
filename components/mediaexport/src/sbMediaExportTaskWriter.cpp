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

#include "sbMediaExportTaskWriter.h"

#include <nsIFile.h>
#include <nsDirectoryServiceUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <sbStandardProperties.h>


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
  [added-mediaitems:TEST PLAYLIST]
  0=file:///path/to/file1.mp3
  1=file:///path/to/file2.mp3
  2=file:///path/to/file3.mp3

*/


NS_IMPL_ISUPPORTS0(sbMediaExportTaskWriter)

sbMediaExportTaskWriter::sbMediaExportTaskWriter()
{
}

sbMediaExportTaskWriter::~sbMediaExportTaskWriter()
{
}

nsresult
sbMediaExportTaskWriter::Init()
{
  LOG(("%s: Setting up a task writer instance", __FUNCTION__));
  
  // First, setup the actual export data file to write to.
  nsresult rv;
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

  LOG(("%s: Creating task file at '%s'", 
        __FUNCTION__, NS_ConvertUTF16toUTF8(mTaskFilepath).get()));
  
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
  LOG(("%s: Done writing task file at '%s'", 
        __FUNCTION__, mTaskFilepath.get()));
  
  mOutputStream.close();
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteAddedMediaListsHeader()
{
  LOG(("%s Writing header '%s'",
        __FUNCTION__, TASKFILE_ADDEDMEDIALISTS_HEADER));
  
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
  LOG(("%s Writing header '%s'",
        __FUNCTION__, TASKFILE_REMOVEDMEDIALISTS_HEADER));
  
  mOutputStream << "["
                << TASKFILE_REMOVEDMEDIALISTS_HEADER
                << "]"
                << std::endl;
  
  // Reset the output index
  mCurOutputIndex = 0;
  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteAddedMediaItemsListHeader(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  nsString mediaListName;
  rv = aMediaList->GetName(mediaListName);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("%s: Writing header '%s' for medialist name '%s'",
        __FUNCTION__, 
        TASKFILE_ADDEDMEDIAITEMS_HEADER,
        NS_ConvertUTF16toUTF8(mediaListName).get()));

  // Header format looks like this
  // [added-mediaitems:Playlist Name]
  mOutputStream << "[" 
                << TASKFILE_ADDEDMEDIAITEMS_HEADER
                << ":"
                << NS_ConvertUTF16toUTF8(mediaListName).get()
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

  nsString itemContentURL;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                               itemContentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("%s: Writing added track '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(itemContentURL).get()));

  mOutputStream << mCurOutputIndex++ 
                << "="
                << NS_ConvertUTF16toUTF8(itemContentURL).get()
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

  LOG(("%s: Writing media list name '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(listName).get()));

  mOutputStream << mCurOutputIndex++
                << "="
                << NS_ConvertUTF16toUTF8(listName).get()
                << std::endl;

  return NS_OK;
}

nsresult
sbMediaExportTaskWriter::WriteString(const nsAString & aString)
{
  LOG(("%s: Writing string '%s'",
        __FUNCTION__, NS_ConvertUTF16toUTF8(aString).get()));

  mOutputStream << mCurOutputIndex++
                << "="
                << NS_ConvertUTF16toUTF8(aString).get()
                << std::endl;

  return NS_OK;
}

