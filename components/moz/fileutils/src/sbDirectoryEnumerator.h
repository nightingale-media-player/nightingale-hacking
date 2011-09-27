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

#include <mozilla/Mutex.h>

#ifndef __SB_DIRECTORY_ENUMERATOR_H__
#define __SB_DIRECTORY_ENUMERATOR_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird directory enumerator defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDirectoryEnumerator.h
 * \brief Songbird Directory Enumerator Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird directory enumerator imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIDirectoryEnumerator.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsISimpleEnumerator.h>


//------------------------------------------------------------------------------
//
// Songbird directory enumerator definitions.
//
//------------------------------------------------------------------------------

//
// Songbird directory enumerator XPCOM component definitions.
//

#define SB_DIRECTORYENUMERATOR_CLASSNAME "sbDirectoryEnumerator"
#define SB_DIRECTORYENUMERATOR_DESCRIPTION "Songbird Directory Enumerator"
#define SB_DIRECTORYENUMERATOR_CID                                             \
{                                                                              \
  0x7065ab15,                                                                  \
  0x8b5f,                                                                      \
  0x4e02,                                                                      \
  { 0x96, 0x5e, 0xcf, 0x49, 0x9b, 0xd9, 0xca, 0x3a }                           \
}

//------------------------------------------------------------------------------
//
// Songbird directory enumerator classes.
//
//------------------------------------------------------------------------------

/**
 * This class enumerates directories.
 */

class sbDirectoryEnumerator : public sbIDirectoryEnumerator
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDIRECTORYENUMERATOR


  //
  // Public services.
  //

  sbDirectoryEnumerator();

  virtual ~sbDirectoryEnumerator();

  nsresult Initialize();

  void Finalize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:
  //
  // mIsInitialized             If true, directory enumerator has been
  //                            initialized.
  // mEnumeratorLock            Lock for the enumerator.
  // mEntriesEnumStack          Stack of directory entry enumerators.
  // mNextFile                  Next file in enumeration.
  // mMaxDepth                  Maximum depth of enumeration.
  // mDirectoriesOnly           If true, enumerate only directories.
  // mFilesOnly                 If true, enumerate only files.
  //

  PRBool                        				mIsInitialized;
  mozilla::Mutex*                       		mEnumeratorLock;
  nsCOMArray<nsISimpleEnumerator>
												mEntriesEnumStack;
  nsCOMPtr<nsIFile>             				mNextFile;
  PRUint32                      				mMaxDepth;
  PRBool                        				mDirectoriesOnly;
  PRBool                        				mFilesOnly;


  //
  // Public services.
  //

  nsresult ScanForNextFile();
};


#endif /* __SB_DIRECTORY_ENUMERATOR_H__ */

