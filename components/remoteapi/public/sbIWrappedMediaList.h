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

/**
 * \file sbIWrappedMediaList.h
 */

#ifndef __SB_IWRAPPEDMEDIALIST_H__
#define __SB_IWRAPPEDMEDIALIST_H__

#include "sbIWrappedMediaItem.h"

#include <nsISupports.h>
#include <nsCOMPtr.h>

class sbIMediaList;

#define SB_IWRAPPEDMEDIALIST_IID \
{ 0x73cc4261, 0x1d0f, 0x48f3, { 0x9b, 0x7b, 0x6a, 0x5b, 0xc8, 0x38, 0x26, 0x14 } }

class sbIWrappedMediaList : public sbIWrappedMediaItem
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(SB_IWRAPPEDMEDIALIST_IID)

  virtual already_AddRefed<sbIMediaList> GetMediaList()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbIWrappedMediaList,
                              SB_IWRAPPEDMEDIALIST_IID)

#endif // __SB_IWRAPPEDMEDIALIST_H__

