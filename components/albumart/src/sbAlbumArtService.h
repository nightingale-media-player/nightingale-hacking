/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef __SB_ALBUMARTSERVICE_H__
#define __SB_ALBUMARTSERVICE_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtService.h
 * \brief Songbird Album Art Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird album art service imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIAlbumArtService.h>
#include <sbMemoryUtils.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIFileStreams.h>
#include <nsIIOService.h>
#include <nsIMIMEService.h>
#include <nsStringGlue.h>
#include <nsTArray.h>


//------------------------------------------------------------------------------
//
// Songbird album art service defs.
//
//------------------------------------------------------------------------------

//
// Songbird album art service component defs.
//

#define SB_ALBUMARTSERVICE_CLASSNAME "sbAlbumArtService"
#define SB_ALBUMARTSERVICE_CID                                                 \
  /* {8DF4920B-89AC-49A3-8552-D74D313C62B6} */                                 \
  {                                                                            \
    0x8DF4920B,                                                                \
    0x89AC,                                                                    \
    0x49A3,                                                                    \
    { 0x85, 0x52, 0xD7, 0x4D, 0x31, 0x3C, 0x62, 0xB6 }                         \
  }


//------------------------------------------------------------------------------
//
// Songbird album art service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the album art service component.
 */

class sbAlbumArtService : public sbIAlbumArtService
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTSERVICE


  //
  // Public services.
  //

  sbAlbumArtService();

  virtual ~sbAlbumArtService();

  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mIOService                 I/O service.
  // mMIMEService               MIME service.
  // mAlbumArtCacheDir          Album art cache directory.
  // mValidExtensionList        List of valid album art file extensions.
  //

  nsCOMPtr<nsIIOService>        mIOService;
  nsCOMPtr<nsIMIMEService>      mMIMEService;
  nsCOMPtr<nsIFile>             mAlbumArtCacheDir;
  nsTArray<nsCString>           mValidExtensionList;


  //
  // Internal services.
  //

  nsresult GetCacheFileBaseName(const PRUint8* aData,
                                PRUint32       aDataLen,
                                nsAString&     aFileBaseName);

  nsresult GetCacheFileExtension(const nsACString& aMimeType,
                                 nsAString&        aFileExtension);
};


//
// Auto-disposal class wrappers.
//
//   sbAutoFileOutputStream     Wrapper to auto close an nsIFileOutputStream.
//

SB_AUTO_CLASS(sbAutoFileOutputStream,
              nsCOMPtr<nsIFileOutputStream>,
              mValue,
              mValue->Close(),
              mValue = nsnull);


#endif // __SB_ALBUMARTSERVICE_H__

