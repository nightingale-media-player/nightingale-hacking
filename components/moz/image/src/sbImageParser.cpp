/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale image parser.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbImageParser.cpp
 * \brief Nightingale Image Parser Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale image parser imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbImageParser.h"

// Nightingale imports
#include <sbMemoryUtils.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <plbase64.h>

#include <nsIBinaryInputStream.h>
#include <nsIFileStreams.h>
#include <nsIIOService.h>

#ifdef IS_BIG_ENDIAN
#define LITTLE_TO_NATIVE16(x) (((((PRUint16) x) & 0xFF) << 8) | \
                               (((PRUint16) x) >> 8))
#define LITTLE_TO_NATIVE32(x) (((((PRUint32) x) & 0xFF) << 24) | \
                               (((((PRUint32) x) >> 8) & 0xFF) << 16) | \
                               (((((PRUint32) x) >> 16) & 0xFF) << 8) | \
                               (((PRUint32) x) >> 24))
#else
#define LITTLE_TO_NATIVE16(x) x
#define LITTLE_TO_NATIVE32(x) x
#endif

//------------------------------------------------------------------------------
//
// Nightingale image parser nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbImageParser,
                              sbIImageParser)


//------------------------------------------------------------------------------
//
// Nightingale image parser sbIImageParser implementation.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbImageParser::GetICOBySize(nsILocalFile*   aICOFile,
                            PRUint32        aSize,
                            nsAString&      _retval)
{
  nsresult rv;
  PRUint32 dataSize;
  PRUint8* data;
  
  nsCOMPtr<nsIFileInputStream> fileStream =
    do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Init(aICOFile, -1, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the binary data from the file into our buffer
  rv = fileStream->Available(&dataSize);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBinaryInputStream> imageDataStream =
    do_CreateInstance("@mozilla.org/binaryinputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = imageDataStream->SetInputStream(fileStream);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = imageDataStream->ReadByteArray(dataSize, &data);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we have a sane number of bytes in the buffer
  if (dataSize < 6)
    return NS_ERROR_UNEXPECTED;

  // Sanity check that the first 4 bytes of the .ico are right
  if (!(data[0] == 0 && data[1] == 0 && data[2] == 1 && data[3] == 0))
    return NS_ERROR_UNEXPECTED;

  // Get the # of icons present in the .ico file
  PRUint16 numIcons = 0;
  memcpy(&numIcons, data+4, 2);
  numIcons = LITTLE_TO_NATIVE16(numIcons);

  // Loop through the directory entries to find where our icons are
  for (unsigned int i=0; i<numIcons; i++) {
    PRUint8 width = 0;
    if (dataSize < (7 + (i*16)))
      return NS_ERROR_UNEXPECTED;
    memcpy(&width, data + 6 + (i*16), 1);
    if (width == aSize) {
      rv = GetIcon(i, data, dataSize, _retval);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (_retval.IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

/**
 * Return in iconString the base64 encoded string for the icon at the index
 * specified by imageIndex within the icon data specified by data.
 *
 * \param imageIndex            Index of icon to return.
 * \param data                  Icon image data.
 * \param iconString            Returned base64 encoded icon string.
 */

nsresult
sbImageParser::GetIcon(PRUint32   imageIndex,
                       PRUint8*   data,
                       PRUint32   dataSize,
                       nsAString& iconString)
{
  PRUint32 offset;
  PRUint32 size = 0;

  // get the size & offset
  if (dataSize < (6 + (imageIndex*16) + 12 + 4))
    return NS_ERROR_UNEXPECTED;
  memcpy(&size,  data + 6 + (imageIndex*16) + 8, 4);
  memcpy(&offset, data + 6 + (imageIndex*16) + 12, 4);
  size = LITTLE_TO_NATIVE32(size);
  offset = LITTLE_TO_NATIVE32(offset);

  // create a new ico buffer, with 1 image, and 1 directory entry
  // 22 bytes of ico header & directory entry
  PRUint8* buffer = static_cast<PRUint8*>(NS_Alloc(size + 22));
  NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoBuffer (buffer);

  // set type & # of images (1) in file
  buffer[0] = buffer[1] = buffer[3] = buffer[5] = 0;
  buffer[2] = buffer[4] = 1;

  // copy over directory entry
  memcpy(buffer+6, data + 6 + (imageIndex*16), 16);

  // munge the offset since we only have one image in this "file"
  buffer[18] = 22;
  buffer[19] = buffer[20] = buffer[21] = 0;

  // copy over icon data bytes
  if (dataSize < (offset + size))
    return NS_ERROR_UNEXPECTED;
  memcpy(buffer+22, data + offset, size);

  // base64 encode it into a data URI
  iconString.AssignLiteral("data:image/vnd.microsoft.icon;base64,");
  iconString.AppendLiteral(PL_Base64Encode((char*)buffer, size+22, NULL));

  return NS_OK;
}

sbImageParser::sbImageParser()
{
}

sbImageParser::~sbImageParser()
{
}
