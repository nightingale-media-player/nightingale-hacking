/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbStringUtils.h"

#include <prprf.h>
#include <nsDataHashtable.h>
#include <nsIStringBundle.h>
#include <nsIStringEnumerator.h>
#include <nsICharsetConverterManager.h>
#include <nsIUnicodeDecoder.h>
#include <nsServiceManagerUtils.h>
#include <sbIStringBundleService.h>
#include <sbMemoryUtils.h>

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

PRInt64
nsString_ToInt64(const nsAString& str, nsresult* rv)
{
  PRInt64 result;
  NS_LossyConvertUTF16toASCII narrow(str);
  PRInt32 converted = PR_sscanf(narrow.get(), "%lld", &result);
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

PRUint64
nsString_ToUint64(const nsAString& str, nsresult* rv)
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

/**
 * This is originated from CompressWhitespace in nsStringAPI, with
 * modification. Make sure to update it when the upstream is changed.
 */
void
SB_CompressWhitespace(nsAString& aString, PRBool aLeading, PRBool aTrailing)
{
  PRUnichar *start;
  PRUint32 len = NS_StringGetMutableData(aString, PR_UINT32_MAX, &start);
  PRUnichar *end = start + len;
  PRUnichar *from = start, *to = start;

  while (from < end && NS_IsAsciiWhitespace(*from))
    from++;

  if (!aLeading)
    to = from;

  while (from < end) {
    PRUnichar theChar = *from++;
    if (NS_IsAsciiWhitespace(theChar)) {
      // We found a whitespace char, so skip over any more
      while (from < end && NS_IsAsciiWhitespace(*from))
        from++;

      // Turn all whitespace into spaces
      theChar = ' ';
    }

    if (from == end && theChar == ' ') {
      to = from;
    } else {
      *to++ = theChar;
    }
  }

  // Drop any trailing space
  if (aTrailing) {
    while (to > start && to[-1] == ' ')
      to--;
  }

  // Re-terminate the string
  *to = '\0';

  // Set the new length
  aString.SetLength(to - start);
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

void 
nsCString_ReplaceChars(nsACString& aOldString,
                       const nsACString& aOldChars,
                       const char aNewChar)
{
  PRUint32 length = aOldString.Length();
  for (PRUint32 index = 0; index < length; index++) {
    char currentChar = aOldString.CharAt(index);
    PRInt32 oldCharsIndex = aOldChars.FindChar(currentChar);
    if (oldCharsIndex > -1)
      aOldString.Replace(index, 1, aNewChar);
  }
}

void nsString_ReplaceSubstring(/* inout */ nsAString &aString,
                               const nsAString &aOldString,
                               const nsAString &aNewString)
{
  if (aOldString.Length() == 0) {
    return;
  }

  PRUint32 i = 0;
  while (i < aString.Length())
  {
    PRInt32 r = aString.Find(aOldString, i);
    if (r == -1)
      break;

    aString.Replace(r, aOldString.Length(), aNewString);
    i += r + aNewString.Length();
  }

  return;
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

PRBool IsUTF8(const nsACString& aString)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> converterManager =
      do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = converterManager->GetUnicodeDecoderRaw("UTF-8", getter_AddRefs(decoder));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRInt32 dataLen = aString.Length();
  PRInt32 size;
  rv = decoder->GetMaxLength(aString.BeginReading(), dataLen, &size);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRUnichar *wstr = reinterpret_cast< PRUnichar * >( NS_Alloc( (size + 1) * sizeof( PRUnichar ) ) );
  rv = decoder->Convert(aString.BeginReading(), &dataLen, wstr, &size);
  NS_Free(wstr);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return PR_TRUE;
}

void
nsString_Split(const nsAString&    aString,
               const nsAString&    aDelimiter,
               nsTArray<nsString>& aSubStringArray)
{
  // Clear out sub-string array.
  aSubStringArray.Clear();

  // Get the delimiter length.  Just put the entire string in the array if the
  // delimiter is empty.
  PRUint32 delimiterLength = aDelimiter.Length();
  if (delimiterLength == 0) {
    aSubStringArray.AppendElement(aString);
    return;
  }

  // Split string into sub-strings.
  PRInt32 stringLength = aString.Length();
  PRInt32 currentOffset = 0;
  PRInt32 delimiterIndex = 0;
  do {
    // Find the index of the next delimiter.  If delimiter cannot be found, set
    // the index to the end of the string.
    delimiterIndex = aString.Find(aDelimiter, currentOffset);
    if (delimiterIndex < 0)
      delimiterIndex = stringLength;

    // Add the next sub-string to the array.
    PRUint32 subStringLength = delimiterIndex - currentOffset;
    if (subStringLength > 0) {
      nsDependentSubstring subString(aString, currentOffset, subStringLength);
      aSubStringArray.AppendElement(subString);
    } else {
      aSubStringArray.AppendElement(NS_LITERAL_STRING(""));
    }

    // Advance to the next sub-string.
    currentOffset = delimiterIndex + delimiterLength;
  } while (delimiterIndex < stringLength);
}

void
nsCString_Split(const nsACString&    aString,
                const nsACString&    aDelimiter,
                nsTArray<nsCString>& aSubStringArray)
{
  // Clear out sub-string array.
  aSubStringArray.Clear();

  // Get the delimiter length.  Just put the entire string in the array if the
  // delimiter is empty.
  PRUint32 delimiterLength = aDelimiter.Length();
  if (delimiterLength == 0) {
    aSubStringArray.AppendElement(aString);
    return;
  }

  // Split string into sub-strings.
  PRInt32 stringLength = aString.Length();
  PRInt32 currentOffset = 0;
  PRInt32 delimiterIndex = 0;
  do {
    // Find the index of the next delimiter.  If delimiter cannot be found, set
    // the index to the end of the string.
    delimiterIndex = aString.Find(aDelimiter, currentOffset);
    if (delimiterIndex < 0)
      delimiterIndex = stringLength;

    // Add the next sub-string to the array.
    PRUint32 subStringLength = delimiterIndex - currentOffset;
    if (subStringLength > 0) {
      nsDependentCSubstring subString(aString, currentOffset, subStringLength);
      aSubStringArray.AppendElement(subString);
    } else {
      aSubStringArray.AppendElement(NS_LITERAL_CSTRING(""));
    }

    // Advance to the next sub-string.
    currentOffset = delimiterIndex + delimiterLength;
  } while (delimiterIndex < stringLength);
}

nsresult
SB_ParseISO8601TimeString(const nsAString& aISO8601TimeString,
                          PRTime*          aTime)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aTime);

  // Function variables.
  nsTArray<nsCString> components;
  PRStatus            status;

  // Convert the ISO 8601 string to "MM-DD-YYYY HH:MM:SS.SSSS TZ" format so that
  // PR_ParseTimeString can parse it.
  // E.g., Convert "1970-01-31T01:02:03.4567Z" to
  //               "01-31-1970 01:02:03.4567 GMT".
  //
  // TODO: support other ISO 8601 formats.

  // Split the ISO 8601 time string into separate time and date components.
  nsCAutoString
    iso8601TimeString = NS_LossyConvertUTF16toASCII(aISO8601TimeString);
  nsCString_Split(iso8601TimeString, NS_LITERAL_CSTRING("T"), components);
  NS_ENSURE_TRUE(components.Length() == 2, NS_ERROR_INVALID_ARG);
  nsCAutoString date = components[0];
  nsCAutoString time = components[1];

  // Split the date into year, month, and day components.
  nsCString_Split(date, NS_LITERAL_CSTRING("-"), components);
  NS_ENSURE_TRUE(components.Length() == 3, NS_ERROR_INVALID_ARG);
  nsCAutoString year = components[0];
  nsCAutoString month = components[1];
  nsCAutoString day = components[2];

  // Check for local or GMT timezone.
  nsCAutoString timezone;
  if (time[time.Length() - 1] == 'Z') {
    timezone.Assign(NS_LITERAL_CSTRING(" GMT"));
    time.SetLength(time.Length() - 1);
  }

  // Produce the format for PR_ParseTimeString.
  sbAutoSmprintf timeString = PR_smprintf("%s-%s-%s %s%s",
                                          month.get(),
                                          day.get(),
                                          year.get(),
                                          time.get(),
                                          timezone.get());

  // Parse the time string.
  status = PR_ParseTimeString(timeString, PR_FALSE, aTime);
  NS_ENSURE_TRUE(status == PR_SUCCESS, NS_ERROR_FAILURE);

  return NS_OK;
}

/**
 * Get and return in aString the localized string with the key specified by aKey
 * using the string bundle specified by aStringBundle.  If the string cannot be
 * found, return the default string specified by aDefault; if aDefault is void,
 * return aKey.
 *
 * If aStringBundle is not specified, use the main Songbird string bundle.
 *
 * \param aString               Returned localized string.
 * \param aKey                  Localized string key.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 */

nsresult
SBGetLocalizedString(nsAString&             aString,
                     const nsAString&       aKey,
                     const nsAString&       aDefault,
                     class nsIStringBundle* aStringBundle)
{
  nsresult rv;

  // Set default result.
  if (!aDefault.IsVoid())
    aString = aDefault;
  else
    aString = aKey;

  // Get the string bundle.
  nsCOMPtr<nsIStringBundle> stringBundle = aStringBundle;

  // If no string bundle was provided, get the default string bundle.
  if (!stringBundle) {
    nsCOMPtr<nsIStringBundleService> stringBundleService =
      do_GetService(SB_STRINGBUNDLESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stringBundleService->CreateBundle(SB_STRING_BUNDLE_URL,
                                           getter_AddRefs(stringBundle));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get the string from the bundle.
  nsAutoString stringValue;
  rv = stringBundle->GetStringFromName(aKey.BeginReading(),
                                       getter_Copies(stringValue));
  NS_ENSURE_SUCCESS(rv, rv);
  aString = stringValue;

  return NS_OK;
}

nsresult
SBGetLocalizedString(nsAString&       aString,
                     const nsAString& aKey)
{
  return SBGetLocalizedString(aString, aKey, SBVoidString());
}

nsresult
SBGetLocalizedString(nsAString&             aString,
                     const char*            aKey,
                     const char*            aDefault,
                     class nsIStringBundle* aStringBundle)
{
  nsAutoString key;
  if (aKey)
    key = NS_ConvertUTF8toUTF16(aKey);
  else
    key = SBVoidString();

  nsAutoString defaultString;
  if (aDefault)
    defaultString = NS_ConvertUTF8toUTF16(aDefault);
  else
    defaultString = SBVoidString();

  return SBGetLocalizedString(aString, key, defaultString, aStringBundle);
}

/**
 *   Get and return in aString the formatted localized string with the key
 * specified by aKey using the format parameters specified by aParams and the
 * string bundle specified by aStringBundle.  If the string cannot be found,
 * return the default string specified by aDefault; if aDefault is void, return
 * aKey.
 *   If aStringBundle is not specified, use the main Songbird string bundle.
 *
 * \param aString               Returned localized string.
 * \param aKey                  Localized string key.
 * \param aParams               Format params array.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 */

nsresult
SBGetLocalizedFormattedString(nsAString&                aString,
                              const nsAString&          aKey,
                              const nsTArray<nsString>& aParams,
                              const nsAString&          aDefault,
                              class nsIStringBundle*    aStringBundle)
{
  nsresult rv;

  // Set default result.
  if (!aDefault.IsVoid())
    aString = aDefault;
  else
    aString = aKey;

  // Get the string bundle.
  nsCOMPtr<nsIStringBundle> stringBundle = aStringBundle;

  // If no string bundle was provided, get the default string bundle.
  if (!stringBundle) {
    nsCOMPtr<nsIStringBundleService> stringBundleService =
      do_GetService(SB_STRINGBUNDLESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stringBundleService->CreateBundle(SB_STRING_BUNDLE_URL,
                                           getter_AddRefs(stringBundle));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Convert the parameter array to a new array and set the new array up for
  // auto-disposal.
  PRUint32 paramCount = aParams.Length();
  const PRUnichar** params = static_cast<const PRUnichar**>
                               (NS_Alloc(paramCount * sizeof(PRUnichar*)));
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSTypePtr<const PRUnichar*> autoParams(params);
  for (PRUint32 i = 0; i < paramCount; i++) {
    params[i] = aParams[i].get();
  }

  // Get the string from the bundle.
  nsAutoString stringValue;
  rv = stringBundle->FormatStringFromName(aKey.BeginReading(),
                                          params,
                                          paramCount,
                                          getter_Copies(stringValue));
  NS_ENSURE_SUCCESS(rv, rv);
  aString = stringValue;

  return NS_OK;
}

