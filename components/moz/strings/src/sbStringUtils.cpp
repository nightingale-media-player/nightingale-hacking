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

#include "sbStringUtils.h"

#include <prprf.h>
#include <nsDataHashtable.h>
#include <nsIStringEnumerator.h>

PRInt32
nsString_FindCharInSet(const nsAString& aString,
                       const char *aPattern,
                       PRInt32 aOffset)
{
  const PRUnichar *begin, *end;
  aString.BeginReading(&begin, &end);
  for (const PRUnichar *current = begin + aOffset; current < end; ++current)
  {
    for (const char *pattern = aPattern; *pattern; ++pattern)
    {
      if (NS_UNLIKELY(*current == PRUnichar(*pattern)))
      {
        return current - begin;
      }
    }
  }
  return -1;
}

void
AppendInt(nsAString& str, PRUint64 val)
{
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%llu", val);
  str.Append(NS_ConvertASCIItoUTF16(buf));
}

PRUint64
ToInteger64(const nsAString& str, nsresult* rv)
{
  PRUint64 result;
  NS_LossyConvertUTF16toASCII narrow(str);
  PRInt32 converted = PR_sscanf(narrow.get(), "%llu", &result);
  if (converted != 1) {
    if (rv) {
      *rv = NS_ERROR_INVALID_ARG;
    }
    return 0;
  }

#ifdef DEBUG
  nsString check;
  AppendInt(check, result);
  NS_ASSERTION(check.Equals(str), "Conversion failed");
#endif

  if (rv) {
    *rv = NS_OK;
  }
  return result;
}

nsresult
SB_StringEnumeratorEquals(nsIStringEnumerator* aLeft,
                          nsIStringEnumerator* aRight,
                          PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aLeft);
  NS_ENSURE_ARG_POINTER(aRight);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsDataHashtable<nsStringHashKey, PRUint32> leftValues;
  PRBool success = leftValues.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  PRBool hasMore;
  while (NS_SUCCEEDED(aLeft->HasMore(&hasMore)) && hasMore) {
    nsString value;
    rv = aLeft->GetNext(value);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 count = 1;
    if (leftValues.Get(value, &count)) {
      count++;
    }

    PRBool success = leftValues.Put(value, count);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  while (NS_SUCCEEDED(aRight->HasMore(&hasMore)) && hasMore) {
    nsString value;
    rv = aRight->GetNext(value);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 count;
    if (!leftValues.Get(value, &count)) {
      *_retval = PR_FALSE;
      return NS_OK;
    }

    count--;
    if (count) {
      PRBool success = leftValues.Put(value, count);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      leftValues.Remove(value);
    }
  }

  if (leftValues.Count() == 0) {
    *_retval = PR_TRUE;
  }
  else {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

void nsString_ReplaceChar(/* inout */ nsAString& aString,
                          const nsAString& aOldChars,
                          const PRUnichar aNewChar)
{
  PRUint32 length = aString.Length();
  for (PRUint32 index = 0; index < length; index++) {
    PRUnichar currentChar = aString.CharAt(index);
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar);
    if (oldCharsIndex > -1)
      aString.Replace(index, 1, aNewChar);
  }
}

PRBool IsLikelyUTF8(const nsACString& aString)
{
  if (aString.IsEmpty()) {
    return PR_TRUE;
  }

  // number of bytes following given this prefix byte
  // -1 = invalid prefix, is a continuation; -2 = invalid byte anywhere
  const PRInt32 prefix_table[] = {
  // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 0
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 1
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 2
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 3
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 4
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 5
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 6
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 7
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 8
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 9
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // A
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // B
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // C
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // D
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, // E
     3,  3,  3,  3,  3, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2  // F
  };

  PRInt32 bytesRemaining = 0; // for multi-byte sequences
  const nsACString::char_type *begin, *end;
  aString.BeginReading(&begin, &end);

  for (; begin != end; ++begin) {
    PRInt32 next = prefix_table[*begin];
    if (bytesRemaining) {
      if (next != -1) {
        // expected more bytes but didn't get a continuation
        return PR_FALSE;
      }
      --bytesRemaining;
      continue;
    }

    // expecting the next byte to be a lead byte
    if (next < 0) {
      // but isn't
      return PR_FALSE;
    }

    bytesRemaining = next;
  }
  return PR_TRUE;
}
