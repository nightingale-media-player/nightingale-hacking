/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
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
//=END SONGBIRD GPL
*/

#ifndef __QT_ATOM_READER_H__
#define __QT_ATOM_READER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// QuickTime atom reader services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
* \file  QTAtomReader.h
* \brief QuickTime Atom Reader Definitions.
*/

//------------------------------------------------------------------------------
//
// QuickTime atom reader imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIInputStream.h>
#include <nsIFile.h>
#include <nsIFileStreams.h>
#include <nsISeekableStream.h>
#include <nsStringGlue.h>
#include <nsTArray.h>


//------------------------------------------------------------------------------
//
// QuickTime atom reader classes.
//
//------------------------------------------------------------------------------

/**
 * This class may be used to read QuickTime atoms within a file.
 */

class QTAtomReader
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  QTAtomReader();

  virtual ~QTAtomReader();

  nsresult Open(nsIFile* aFile);

  void Close(void);

  nsresult GetFairPlayUserID(PRUint32* aUserID);

  nsresult GetFairPlayAccountName(nsAString& aAccountName);

  nsresult GetFairPlayUserName(nsAString& aUserName);

  nsresult GetIEKInfoUserIDs(nsTArray<PRUint32>& aUserIDList);

  nsresult AtomPathGet(const char*     aAtomPath,
                       void*     aAtom,
                       PRUint64* aStartOffset,
                       PRUint64* aEndOffset);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mFile                      File from which to read atoms.
  // mFileInputStream           Streams used for reading file.
  // mSeekableStream
  // mInputStream
  // mAtomHdrSize               Size of atom header.
  //

  nsCOMPtr<nsIFile>             mFile;
  nsCOMPtr<nsIFileInputStream>  mFileInputStream;
  nsCOMPtr<nsISeekableStream>   mSeekableStream;
  nsCOMPtr<nsIInputStream>      mInputStream;
  PRUint32                      mAtomHdrSize;

  nsresult AtomGet(PRUint32  aAtomType,
                   void*     aAtom,
                   PRUint64* aStartOffset,
                   PRUint64* aEndOffset);
};


/**
 * Auto-disposal class wrappers.
 *
 *   sbAutoQTAtomReader         Wrapper to auto-close and delete a QuickTime
 *                              atom reader.
 */

SB_AUTO_CLASS(sbAutoQTAtomReader,
              QTAtomReader*,
              mValue,
              { mValue->Close(); delete mValue; },
              mValue = nsnull);


#endif /* __QT_ATOM_READER_H__ */

