/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
#ifndef SBSTRINGMAP_H_
#define SBSTRINGMAP_H_

#include <nsDataHashtable.h>
#include <nsStringAPI.h>

#include <sbIStringMap.h>

/**
 * This class provides a very simple interface to store name value pairs of
 * strings. The main drive for this implementation was the lack of 
 * nsIPropertyBagWritable to provide any kind of clearing method. And to 
 * eliminate the overhead of the nsIVariant used there.
 */
class sbStringMap : public sbIMutableStringMap
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISTRINGMAP
  NS_DECL_SBIMUTABLESTRINGMAP

  /**
   * Initializes the hash
   */
  sbStringMap();

protected:
  /**
   * Just specified
   */
  virtual ~sbStringMap();

  nsDataHashtable<nsStringHashKey, nsString> mMap;
};

#endif /* SBSTRINGMAP_H_ */
