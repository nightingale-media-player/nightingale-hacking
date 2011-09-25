/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbStringMap.h"

NS_IMPL_ISUPPORTS2(sbStringMap, 
                   sbIStringMap,
                   sbIMutableStringMap)

sbStringMap::sbStringMap()
{
  mMap.Init();
}

sbStringMap::~sbStringMap()
{
}

/* AString Find (in AString key); */
NS_IMETHODIMP sbStringMap::Get(const nsAString & aKey, nsAString & aValue)
{
  nsString value;
  PRBool const found = mMap.Get(aKey, &value);
  if (!found) {
    aValue.SetIsVoid(PR_TRUE);
  }
  else {
    aValue = value;
  }
  return NS_OK;
}

// sbIStringMapWritable implementation
/* void Set (in AString key, in AString value); */
NS_IMETHODIMP sbStringMap::Set(const nsAString & key, const nsAString & value)
{
  PRBool const success = mMap.Put(nsString(key), nsString(value));
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/* void Clear (); */
NS_IMETHODIMP sbStringMap::Clear()
{
  mMap.Clear();
  return NS_OK;
}
