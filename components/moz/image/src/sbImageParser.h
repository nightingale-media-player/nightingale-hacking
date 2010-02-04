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

#ifndef __SB_IMAGEPARSER_H__
#define __SB_IMAGEPARSER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird image parser.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbImageParser.h
 * \brief Songbird Image Parser Definitions.
 */

// Songbird imports.
#include <sbIImageParser.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsIURI.h>


/**
 * This class implements the Songbird image parser interface.
 */

class sbImageParser : public sbIImageParser
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
  NS_DECL_SBIIMAGEPARSER


  //
  // Songbird image parser services.
  //

  sbImageParser();

  virtual ~sbImageParser();

  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  nsresult GetIcon(PRUint32 imageIndex,
                   PRUint8* data,
                   PRUint32 dataSize,
                   nsAString& iconString);

};


//
// Songbird image parser component defs.
//

// contract ID defined in sbIImageParser.idl
#define SONGBIRD_IMAGEPARSER_CLASSNAME "Songbird Image Parser"
#define SONGBIRD_IMAGEPARSER_CID \
  /* {39dd2e7c-1dd2-11b2-b8c2-b50253bd2e17} */ \
  /* {fa7ec5bd-7cab-4a63-a970-7bc4e83ee891} */ \
  { \
    0x39dd2e7c, \
    0x1dd2, \
    0x11b2, \
    {0xb8, 0xc2, 0xb5, 0x02, 0x53, 0xbd, 0x2e, 0x17 } \
  }

#endif // __SB_IMAGEPARSER_H__

