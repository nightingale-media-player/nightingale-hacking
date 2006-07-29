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
* \file MetadataHandlerOGG.h
* \brief 
*/

#ifndef __METADATA_HANDLER_OGG_H__
#define __METADATA_HANDLER_OGG_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <necko/nsIChannel.h>
#include <necko/nsIResumableChannel.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsEscape.h>
#include <string/nsReadableUtils.h>
#include <string/nsString.h>

#include "sbIMetadataHandler.h"
#include "sbIMetadataValues.h"
#include "sbIMetadataChannel.h"

// DEFINES ====================================================================
#define SONGBIRD_METADATAHANDLEROGG_CONTRACTID            \
  "@songbirdnest.com/Songbird/MetadataHandler/OGG;1"
#define SONGBIRD_METADATAHANDLEROGG_CLASSNAME             \
  "Songbird OGG Metadata Handler Interface"
#define SONGBIRD_METADATAHANDLEROGG_CID                   \
{ /* 6370485c-93d6-4de7-bacc-72ab7498f3bb */              \
  0x6370485c,                                             \
  0x93d6,                                                 \
  0x4de7,                                                 \
  {0xba, 0xcc, 0x72, 0xab, 0x74, 0x98, 0xf3, 0xbb}        \
}
// CLASSES ====================================================================
class sbMetadataHandlerOGG : public sbIMetadataHandler
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER

  sbMetadataHandlerOGG();
  virtual ~sbMetadataHandlerOGG();

protected:
  struct sbOGGHeader
  {
    PRInt8 stream_structure_version;
    PRInt8 header_type_flag;
    PRInt64 absolute_granule_position;
    PRInt32 stream_serial_number;
    PRInt32 page_sequence_no;
    PRInt32 page_checksum;
    PRInt32 m_Size;

    sbOGGHeader() : 
      stream_structure_version( 0 ),
      header_type_flag( 0 ),
      absolute_granule_position( 0 ),
      stream_serial_number( 0 ),
      page_sequence_no( 0 ),
      page_checksum( 0 ),
      m_Size( 0 )
    {
    }
  };

  void ParseChannel();
  sbOGGHeader ParseHeader();
  nsString ReadIntString();

  nsCOMPtr<sbIMetadataValues>   m_Values;
  nsCOMPtr<sbIMetadataChannel>  m_ChannelHandler;
  nsCOMPtr<nsIChannel>          m_Channel;
  PRBool                        m_Completed;
};

#endif // __METADATA_HANDLER_OGG_H__

