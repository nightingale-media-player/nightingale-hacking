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
* \file MetadataHandlerMP4.h
* \brief 
*/

#ifndef __METADATA_HANDLER_MP4_H__
#define __METADATA_HANDLER_MP4_H__

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

#include "quicktime.h"
#include "private.h"
#include "funcprotos.h"

// DEFINES ====================================================================
#define SONGBIRD_METADATAHANDLERMP4_CONTRACTID            \
  "@songbird.org/Songbird/MetadataHandler/MP4;1"
#define SONGBIRD_METADATAHANDLERMP4_CLASSNAME             \
  "Songbird MP4 Metadata Handler Interface"
#define SONGBIRD_METADATAHANDLERMP4_CID                   \
{ /* 6f289ead-9ec8-4230-ad6b-fd8873987fee */              \
  0x6f289ead,                                             \
  0x9ec8,                                                 \
  0x4230,                                                 \
  {0xad, 0x6b, 0xfd, 0x88, 0x73, 0x98, 0x7f, 0xee}        \
}
// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataHandlerMP4 : public sbIMetadataHandler
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER

  sbMetadataHandlerMP4();
  virtual ~sbMetadataHandlerMP4();

public:
  void callback( const char *atom_path, const char *value_string );

protected:
  nsCOMPtr<sbIMetadataValues> m_Values;
  nsCOMPtr<sbIMetadataChannel> m_ChannelHandler;
  nsCOMPtr<nsIChannel> m_Channel;
  PRBool               m_Completed;
};

#endif // __METADATA_HANDLER_MP4_H__

