/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird directory enumerator.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDirectoryEnumerator.cpp
 * \brief Songbird Directory Enumerator Source.
 */

//------------------------------------------------------------------------------
//
// Songbird directory enumerator imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDirectoryEnumerator.h"

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <prerr.h>
#include <prerror.h>

//------------------------------------------------------------------------------
//
// Songbird directory enumerator nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDirectoryEnumerator, sbIDirectoryEnumerator)


//------------------------------------------------------------------------------
//
// Songbird directory enumerator sbIDirectoryEnumerator implementation.
//
//------------------------------------------------------------------------------

/**
 * Enumerates a given directory specified by an nsIFile object.
 * This is a class to get around bug 24478 in the least invasive manner.
 * At some point this needs to be cleaned up. Latest version of XULRunner this
 * shouldn't be an issue. If we need a proper fix before then using the
 * NSPR routines directly should be fine.
 */
class sbDirectoryEnumeratorHelper : public nsISimpleEnumerator
{
public:
  /**
   * Initializes simple data members
   */
  sbDirectoryEnumeratorHelper();

  /**
   * Closes the directory if opened
   */
  virtual ~sbDirectoryEnumeratorHelper();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  /**
   * Initializes the directory pointer
   * \param aDirectory the directory to enumerate
   */
  nsresult Init(nsIFile * aDirectory);
private:
  PRDir * mDir;
  nsCOMPtr<nsIFile> mDirectory;
  PRDirEntry * mEntry;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDirectoryEnumeratorHelper, nsISimpleEnumerator)

sbDirectoryEnumeratorHelper::sbDirectoryEnumeratorHelper() :
  mDir(nsnull),
  mEntry(nsnull)
{
}

sbDirectoryEnumeratorHelper::~sbDirectoryEnumeratorHelper()
{
  if (mDir) {
    PR_CloseDir(mDir);
  }
}
nsresult sbDirectoryEnumeratorHelper::Init(nsIFile * aDirectory)
{
  NS_ENSURE_ARG_POINTER(aDirectory);

  nsresult rv;

  mDirectory = aDirectory;

  nsString path;
  rv = aDirectory->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  mDir = PR_OpenDir(NS_ConvertUTF16toUTF8(path).BeginReading());
  NS_ENSURE_TRUE(mDir, NS_ERROR_FILE_NOT_FOUND);

  // Preload the first entry, so HasMoreElements knows there is an entry
  // waiting or not.
  mEntry = PR_ReadDir(mDir, PR_SKIP_BOTH);
  if (!mEntry) {
    NS_ENSURE_TRUE(PR_GetError() == PR_NO_MORE_FILES_ERROR,
                   NS_ERROR_UNEXPECTED);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryEnumeratorHelper::HasMoreElements(PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = mEntry ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryEnumeratorHelper::GetNext(nsISupports ** aEntry NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aEntry);

  if (!mEntry) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv;

  nsCOMPtr<nsILocalFile> file =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);

  nsString path;
  rv = mDirectory->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->InitWithPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendNative(nsCString(mEntry->name));
  NS_ENSURE_SUCCESS(rv, rv);

  *aEntry = file.get();
  file.forget();

  // Prefetch the next entry if any, skipping . and ..
  mEntry = PR_ReadDir(mDir, PR_SKIP_BOTH);
  if (!mEntry) {
    NS_ENSURE_TRUE(PR_GetError() == PR_NO_MORE_FILES_ERROR,
                   NS_ERROR_UNEXPECTED);
  }

  return NS_OK;
}

/**
 * \brief Enumerate the directory specified by aDirectory.
 *
 * \param aDirectory          Directory to enumerate.
 */

NS_IMETHODIMP
sbDirectoryEnumerator::Enumerate(nsIFile* aDirectory)
{
  // Validate arguments and state.
  NS_ENSURE_ARG_POINTER(aDirectory);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Operate under the enumerator lock.
  nsAutoLock autoLock(mEnumeratorLock);

  // Ensure directory exists and is a directory.
  PRBool exists;
  PRBool isDirectory;
  rv = aDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(exists, NS_ERROR_FILE_NOT_FOUND);
  rv = aDirectory->IsDirectory(&isDirectory);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(isDirectory, NS_ERROR_FILE_NOT_DIRECTORY);

  // Get the entries in the directory.  If file not found is returned, the
  // directory is empty.
  nsRefPtr<sbDirectoryEnumeratorHelper> dirEnumerator =
    new sbDirectoryEnumeratorHelper();
  NS_ENSURE_TRUE(dirEnumerator, NS_ERROR_OUT_OF_MEMORY);
  rv = dirEnumerator->Init(aDirectory);

  // Initialize the entries enumeration stack.
  mEntriesEnumStack.Clear();

  if (rv != NS_ERROR_FILE_NOT_FOUND) {
    NS_ENSURE_SUCCESS(rv, rv);
    success = mEntriesEnumStack.AppendObject(dirEnumerator);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Scan for the next file.
  rv = ScanForNextFile();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Return true if more elements are available to enumerate.
 *
 * \return True if more elements are available.
 */

NS_IMETHODIMP
sbDirectoryEnumerator::HasMoreElements(PRBool* aHasMoreElements)
{
  // Validate arguments and state.
  NS_ENSURE_ARG_POINTER(aHasMoreElements);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");

  // Operate under the enumerator lock.
  nsAutoLock autoLock(mEnumeratorLock);

  // Return results.
  *aHasMoreElements = (mEntriesEnumStack.Count() > 0);

  return NS_OK;
}


/**
 * \brief Return the next file in the enumeration.
 *
 * \return Next file in enumeration.
 */

NS_IMETHODIMP
sbDirectoryEnumerator::GetNext(nsIFile** aFile)
{
  // Validate arguments and state.
  NS_ENSURE_ARG_POINTER(aFile);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");

  // Function variables.
  nsresult rv;

  // Operate under the enumerator lock.
  nsAutoLock autoLock(mEnumeratorLock);

  // Return the next file.  Return error if no next file.
  if (mNextFile)
    mNextFile.forget(aFile);
  else
    return NS_ERROR_NOT_AVAILABLE;

  // Scan for the next file.
  rv = ScanForNextFile();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//
// Getters/setters.
//

/**
 * \brief Maximum depth to which to enumerate.  A depth of 0 will enumerate to
 *        an unlimited depth.  A depth of 1 will enumerate only the base
 *        directory (same as getDirectoryEntries).
 */

NS_IMETHODIMP
sbDirectoryEnumerator::GetMaxDepth(PRUint32* aMaxDepth)
{
  NS_ENSURE_ARG_POINTER(aMaxDepth);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  *aMaxDepth = mMaxDepth;
  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryEnumerator::SetMaxDepth(PRUint32 aMaxDepth)
{
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  mMaxDepth = aMaxDepth;
  return NS_OK;
}


/**
 * \brief If true, only return directories in getNext.
 */

NS_IMETHODIMP
sbDirectoryEnumerator::GetDirectoriesOnly(PRBool* aDirectoriesOnly)
{
  NS_ENSURE_ARG_POINTER(aDirectoriesOnly);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  *aDirectoriesOnly = mDirectoriesOnly;
  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryEnumerator::SetDirectoriesOnly(PRBool aDirectoriesOnly)
{
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  mDirectoriesOnly = aDirectoriesOnly;
  return NS_OK;
}


/**
 * \brief If true, only return files in getNext.
 */

NS_IMETHODIMP
sbDirectoryEnumerator::GetFilesOnly(PRBool* aFilesOnly)
{
  NS_ENSURE_ARG_POINTER(aFilesOnly);
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  *aFilesOnly = mFilesOnly;
  return NS_OK;
}

NS_IMETHODIMP
sbDirectoryEnumerator::SetFilesOnly(PRBool aFilesOnly)
{
  NS_PRECONDITION(mIsInitialized, "Directory enumerator not initialized");
  nsAutoLock autoLock(mEnumeratorLock);
  mFilesOnly = aFilesOnly;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird directory enumerator public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird directory enumerator object.
 */

sbDirectoryEnumerator::sbDirectoryEnumerator() :
  mIsInitialized(PR_FALSE),
  mEnumeratorLock(nsnull),
  mMaxDepth(0),
  mDirectoriesOnly(PR_FALSE),
  mFilesOnly(PR_FALSE)
{
}

/**
 * Destroy a Songbird directory enumerator object.
 */

sbDirectoryEnumerator::~sbDirectoryEnumerator()
{
  // Finalize the Songbird directory enumerator.
  Finalize();
}


/**
 * Initialize the Songbird directory enumerator.
 */

nsresult
sbDirectoryEnumerator::Initialize()
{
  // Do nothing if already initialized.
  if (mIsInitialized)
    return NS_OK;

  // Create the directory enumerator lock.
  mEnumeratorLock =
    nsAutoLock::NewLock("sbDirectoryEnumerator.mEnumeratorLock");
  NS_ENSURE_TRUE(mEnumeratorLock, NS_ERROR_OUT_OF_MEMORY);

  // Indicate that the directory enumerator has been initialized.
  mIsInitialized = PR_TRUE;

  return NS_OK;
}


/**
 * Finalize the Songbird directory enumerator.
 */

void
sbDirectoryEnumerator::Finalize()
{
  // Indicate that the directory enumerator is no longer initialized.
  mIsInitialized = PR_FALSE;

  // Dispose of the directory enumerator lock.
  if (mEnumeratorLock)
    nsAutoLock::DestroyLock(mEnumeratorLock);
  mEnumeratorLock = nsnull;

  // Clear the directory entries enumeration stack.
  mEntriesEnumStack.Clear();

  // Release object references.
  mNextFile = nsnull;
}


//------------------------------------------------------------------------------
//
// Songbird directory enumerator private services.
//
//------------------------------------------------------------------------------

/**
 * ScanForNextFile
 *
 *   This function scans for the next file in the enumeration and updates the
 * enumerator appropriately.
 */

nsresult
sbDirectoryEnumerator::ScanForNextFile()
{
  PRBool   success;
  nsresult rv;

  // Scan for the next file.
  while (!mNextFile && (mEntriesEnumStack.Count() > 0)) {
    // Get the enum at the top of the stack.
    nsCOMPtr<nsISimpleEnumerator>
      entriesEnum = mEntriesEnumStack.ObjectAt(mEntriesEnumStack.Count() - 1);

    // If there are no more entries, pop the stack and continue.
    PRBool hasMoreElements;
    rv = entriesEnum->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements) {
      mEntriesEnumStack.RemoveObjectAt(mEntriesEnumStack.Count() - 1);
      continue;
    }

    // Get the next file.
    nsCOMPtr<nsISupports> fileEntry;
    rv = entriesEnum->GetNext(getter_AddRefs(fileEntry));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> file = do_QueryInterface(fileEntry, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip file if it doesn't match enumerator criteria.
    PRBool skipFile = PR_FALSE;
    if (mDirectoriesOnly) {
      PRBool isDirectory;
      rv = file->IsDirectory(&isDirectory);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!isDirectory)
        skipFile = PR_TRUE;
    }
    if (mFilesOnly) {
      PRBool isFile;
      rv = file->IsFile(&isFile);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!isFile)
        skipFile = PR_TRUE;
    }
    if (!skipFile)
      mNextFile = file;

    // If the current file is a directory, and the maximum depth has not been
    // reached, enumerate the directory entries.
    PRBool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 depth = mEntriesEnumStack.Count();
    if (isDirectory && !((mMaxDepth > 0) && (depth >= mMaxDepth))) {
      // Get the directory entries.  If file not found is returned, the
      // directory is empty.
      nsRefPtr<sbDirectoryEnumeratorHelper> dirEnumeratorAdapter =
        new sbDirectoryEnumeratorHelper();
      NS_ENSURE_TRUE(dirEnumeratorAdapter, NS_ERROR_OUT_OF_MEMORY);
      rv = dirEnumeratorAdapter->Init(file);
      nsCOMPtr<nsISimpleEnumerator> dirEntryEnumerator;
      if (rv != NS_ERROR_FILE_NOT_FOUND) {
        NS_ENSURE_SUCCESS(rv, rv);
        dirEntryEnumerator = do_QueryInterface(dirEnumeratorAdapter, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      if (dirEntryEnumerator) {
        success = mEntriesEnumStack.AppendObject(dirEntryEnumerator);
        NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
      }
    }
  }

  return NS_OK;
}
