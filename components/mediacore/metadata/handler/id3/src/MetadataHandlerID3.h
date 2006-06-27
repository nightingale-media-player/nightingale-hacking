/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/

/**
* \file MetadataHandlerID3.h
* \brief 
*/

#ifndef __METADATA_HANDLER_ID3_H__
#define __METADATA_HANDLER_ID3_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <necko/nsIChannel.h>
#include <necko/nsIResumableChannel.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>

#include "sbIMetadataHandler.h"
#include "sbIMetadataValues.h"
#include "sbIMetadataChannel.h"

#include <id3/tag.h>

// DEFINES ====================================================================
#define SONGBIRD_METADATAHANDLERID3_CONTRACTID            \
  "@songbirdnest.com/Songbird/MetadataHandler/ID3;1"
#define SONGBIRD_METADATAHANDLERID3_CLASSNAME             \
  "Songbird ID3 Metadata Handler Interface"
#define SONGBIRD_METADATAHANDLERID3_CID                   \
{ /* a8fc38c2-a309-4f45-b76b-cbb0fa935637 */              \
  0xa8fc38c2,                                             \
  0xa309,                                                 \
  0x4f45,                                                 \
  {0xb7, 0x6b, 0xcb, 0xb0, 0xfa, 0x93, 0x56, 0x37}        \
}

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataHandlerID3 : public sbIMetadataHandler
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER

  sbMetadataHandlerID3();
  virtual ~sbMetadataHandlerID3();

protected:
  PRInt32 ReadTag(ID3_Tag &tag);
  PRInt32 ReadFrame(ID3_Frame *frame);
  PRInt32 ReadFields(ID3_Field *field);
  void CalculateBitrate(const char *buffer, PRUint32 length, PRUint64 file_size);

  nsCOMPtr<sbIMetadataValues> m_Values;
  nsCOMPtr<sbIMetadataChannel> m_ChannelHandler;
  nsCOMPtr<nsIChannel> m_Channel;
  ID3_Tag              m_ID3Tag;
  PRBool               m_Completed;
};

#endif // __METADATA_HANDLER_ID3_H__

