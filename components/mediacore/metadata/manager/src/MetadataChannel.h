/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file MetadataChannel.h
* \brief 
*/

#ifndef __METADATA_CHANNEL_H__
#define __METADATA_CHANNEL_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <string/nsString.h>
#include <xpcom/nsMemory.h>
#include <nsCOMPtr.h>
#include "sbIMetadataChannel.h"
#include <map>

// DEFINES ====================================================================
#define SONGBIRD_METADATACHANNEL_CONTRACTID               \
  "@songbird.org/Songbird/MetadataChannel;1"
#define SONGBIRD_METADATACHANNEL_CLASSNAME                \
  "Songbird Metadata Channel Helper"
#define SONGBIRD_METADATACHANNEL_CID                      \
{ /* 37595d88-f49e-4309-89a1-376749a4b285 */              \
  0x37595d88,                                             \
  0xf49e,                                                 \
  0x4309,                                                 \
  {0x89, 0xa1, 0x37, 0x67, 0x49, 0xa4, 0xb2, 0x85}        \
}
// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataChannel : public sbIMetadataChannel
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATACHANNEL
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  sbMetadataChannel();
  virtual ~sbMetadataChannel();

  // Internal constants
  enum sbBufferConstants
  {
    SIZE_SHIFT = 16, // 64k blocks (what things seem to prefer)
    BLOCK_SIZE = ( 1 << SIZE_SHIFT ),
    BLOCK_MASK = BLOCK_SIZE - 1
  };

  // Helper functions
  inline PRUint64 IDX( PRUint64 i ) { return ( i >> SIZE_SHIFT ); }
  inline PRUint64 POS( PRUint64 i ) { return ( i & (PRUint64)BLOCK_MASK ); }
  inline char *   BUF( PRUint64 i ) { return m_Blocks[ IDX(i) ].buf + POS(i); }

  // Self allocating memory block. Works well with the map [] dereference.
  struct sbBufferBlock {
    char *buf;
    sbBufferBlock() { buf = (char *)nsMemory::Alloc( BLOCK_SIZE ); }
    sbBufferBlock( const sbBufferBlock &T ) { buf = T.buf; const_cast<sbBufferBlock &>(T).buf = nsnull; }
    ~sbBufferBlock() { if ( buf ) nsMemory::Free( buf ); }
  };
  class blockmap_t : public std::map<PRUint64, sbBufferBlock> {};

  nsCOMPtr<nsIChannel> m_Channel;
  nsCOMPtr<sbIMetadataHandler> m_Handler;
  PRUint64 m_Pos;
  PRUint64 m_Buf;
  blockmap_t m_Blocks;
  PRBool   m_Completed;
};

#endif // __METADATA_CHANNEL_H__

