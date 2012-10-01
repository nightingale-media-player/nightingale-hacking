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

#include "sbSelectionListUtils.h"

#include <nsTArray.h>
#include <nsIObjectOutputStream.h>

PLDHashOperator PR_CALLBACK
SB_SerializeSelectionListCallback(nsStringHashKey::KeyType aKey,
                                  nsString aEntry,
                                  void* aUserData)
{
  NS_ASSERTION(aUserData, "Null userData!");

  nsIObjectOutputStream* stream =
    static_cast<nsIObjectOutputStream*>(aUserData);
  NS_ASSERTION(stream, "Could not cast user data");

  nsresult rv;
  rv = stream->WriteWStringZ(aKey.BeginReading());
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = stream->WriteWStringZ(aEntry.BeginReading());
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

PLDHashOperator PR_CALLBACK
SB_CopySelectionListCallback(nsStringHashKey::KeyType aKey,
                             nsString aEntry,
                             void* aUserData)
{
  NS_ASSERTION(aUserData, "Null userData!");

  sbSelectionList* list =
    static_cast<sbSelectionList*>(aUserData);
  NS_ASSERTION(list, "Could not cast user data");

  bool success = list->Put(aKey, aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

PLDHashOperator PR_CALLBACK
SB_SelectionListGuidsToTArrayCallback(nsStringHashKey::KeyType aKey,
                                      nsString aEntry,
                                      void* aUserData)
{
  NS_ASSERTION(aUserData, "Null userData!");

  nsTArray<nsString>* list = static_cast<nsTArray<nsString>*>(aUserData);
  NS_ASSERTION(list, "Could not cast user data");

  nsString* appended = list->AppendElement(aEntry);
  NS_ENSURE_TRUE(appended, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}
