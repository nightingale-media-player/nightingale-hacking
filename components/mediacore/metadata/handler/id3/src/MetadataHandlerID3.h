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
* \file MetadataHandlerID3.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "sbIMetadataHandler.h"

#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>

#include <necko/nsIChannel.h>

#include <id3/tag.h>

// DEFINES ====================================================================
#define SONGBIRD_METADATAHANDLERID3_CONTRACTID  "@songbird.org/Songbird/MetadataHandler/ID3;1"
#define SONGBIRD_METADATAHANDLERID3_CLASSNAME   "Songbird ID3 Metadata Handler Interface"

// {D83C6CE1-DDCE-49ad-ABF3-430A5C223C3B}
#define SONGBIRD_METADATAHANDLERID3_CID { 0xd83c6ce1, 0xddce, 0x49ad, { 0xab, 0xf3, 0x43, 0xa, 0x5c, 0x22, 0x3c, 0x3b } }

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

  nsCOMPtr<nsIChannel> m_Channel;
  ID3_Tag              m_ID3Tag;

};