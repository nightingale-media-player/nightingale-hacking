/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
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

#include "sbIMetadataHandler.h"
#include "sbIMetadataValues.h"
#include "sbIMetadataChannel.h"

// DEFINES ====================================================================
#define SONGBIRD_METADATAHANDLEROGG_CONTRACTID  "@songbird.org/Songbird/MetadataHandler/OGG;1"
#define SONGBIRD_METADATAHANDLEROGG_CLASSNAME   "Songbird OGG Metadata Handler Interface"

// {1DB86685-8965-400f-99A0-F2A18C38C605}
#define SONGBIRD_METADATAHANDLEROGG_CID { 0x1db86685, 0x8965, 0x400f, { 0x99, 0xa0, 0xf2, 0xa1, 0x8c, 0x38, 0xc6, 0x5 } }

// FUNCTIONS ==================================================================

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

