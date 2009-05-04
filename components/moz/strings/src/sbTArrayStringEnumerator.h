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

#ifndef __SBTARRAYSTRINGENUMERATOR_H__
#define __SBTARRAYSTRINGENUMERATOR_H__

#include <nsStringGlue.h>
#include <nsCOMPtr.h>
#include <nsTArray.h>
#include <nsIStringEnumerator.h>

/**
 * Adapter to allow an nsTArray<ns?String> to be used as an
 * nsIStringEnumerator
 */
class sbTArrayStringEnumerator : public nsIStringEnumerator
{
typedef nsTArray<nsString> sbStringArray;
typedef nsTArray<nsCString> sbCStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  static sbTArrayStringEnumerator * New(const sbStringArray & aStringArray);
  /**
   * Allocates sbTArrayStringEnumerator
   * XXX TODO This and the other methods should use a reference 
   * instead of a pointer
   */
  sbTArrayStringEnumerator(const sbStringArray* aStringArray);
  sbTArrayStringEnumerator(const sbCStringArray* aStringArray);

private:
  nsTArray<nsString> mStringArray;
  PRUint32 mNextIndex;
};

class sbTArrayCStringEnumerator : public nsIUTF8StringEnumerator
{
typedef nsTArray<nsString> sbStringArray;
typedef nsTArray<nsCString> sbCStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUTF8STRINGENUMERATOR

  sbTArrayCStringEnumerator(const sbStringArray* aStringArray);
  sbTArrayCStringEnumerator(const sbCStringArray* aStringArray);

private:
  nsTArray<nsCString> mCStringArray;
  nsCOMPtr<nsISupports> mOwner;
  PRUint32 mNextIndex;
};

#endif /* __SBTARRAYSTRINGENUMERATOR_H__ */

