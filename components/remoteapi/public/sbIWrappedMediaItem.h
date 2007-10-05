/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/**
 * \file sbIWrappedMediaItem.h
 */

#ifndef __SB_IWRAPPEDMEDIAITEM_H__
#define __SB_IWRAPPEDMEDIAITEM_H__

#include <nsISupports.h>
#include <nsCOMPtr.h>

class sbIMediaItem;

#define SB_IWRAPPEDMEDIAITEM_IID \
{ 0x14a50b3c, 0x4e4e, 0x422b, { 0xbe, 0x86, 0x98, 0xb5, 0x88, 0xbf, 0xcb, 0xc6 } }

/*
Class: MediaItem
A <MediaItem> represents a partial or whole piece of media.
*/

class sbIWrappedMediaItem : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(SB_IWRAPPEDMEDIAITEM_IID)

  virtual already_AddRefed<sbIMediaItem> GetMediaItem()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbIWrappedMediaItem,
                              SB_IWRAPPEDMEDIAITEM_IID)

#endif // __SB_IWRAPPEDMEDIAITEM_H__

