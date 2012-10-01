/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef __SBSTRINGUTILS_H__
#define __SBSTRINGUTILS_H__

#include <nsStringAPI.h>
#include <nsTArray.h>
#include <prprf.h>
#include <sbMemoryUtils.h>

class nsIStringEnumerator;

/**
 * Class used to create strings from other data types.
 */
class sbAutoString : public nsAutoString
{
public:
  sbAutoString(const char* aValue)
  {
    AssignLiteral(aValue);
  }

  sbAutoString(int aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%d", aValue);
    AssignLiteral(valueStr);
  }

  sbAutoString(PRUint32 aValue,
               bool aHex = PR_FALSE,
               bool aHexPrefix = PR_TRUE)
  {
    char valueStr[64];

    if(!aHex) {
      PR_snprintf(valueStr, sizeof(valueStr), "%lu", long(aValue));
    }
    else {
      if(aHexPrefix) {
        PR_snprintf(valueStr, sizeof(valueStr), "0x%lx", long(aValue));
      }
      else {
        PR_snprintf(valueStr, sizeof(valueStr), "%lx", long(aValue));
      }
    }

    AssignLiteral(valueStr);
  }

  sbAutoString(PRInt64 aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%lld", aValue);
    AssignLiteral(valueStr);
  }

  sbAutoString(PRUint64 aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%llu", aValue);
    AssignLiteral(valueStr);
  }

  sbAutoString(nsID* aValue)
  {
    char idString[NSID_LENGTH];
    aValue->ToProvidedString(idString);
    AssignLiteral(idString);
  }
};

/**
 * Class used to create strings from other data types.
 */
class sbCAutoString : public nsCAutoString
{
public:
  sbCAutoString(const char* aValue)
  {
    AssignLiteral(aValue);
  }

  sbCAutoString(int aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%d", aValue);
    AssignLiteral(valueStr);
  }

  sbCAutoString(PRUint32 aValue,
                bool aHex = PR_FALSE,
                bool aHexPrefix = PR_TRUE)
  {
    char valueStr[64];

    if(!aHex) {
      PR_snprintf(valueStr, sizeof(valueStr), "%lu", long(aValue));
    }
    else {
      if(aHexPrefix) {
        PR_snprintf(valueStr, sizeof(valueStr), "0x%lx", long(aValue));
      }
      else {
        PR_snprintf(valueStr, sizeof(valueStr), "%lx", long(aValue));
      }
    }

    AssignLiteral(valueStr);
  }

  sbCAutoString(PRInt64 aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%lld", aValue);
    AssignLiteral(valueStr);
  }

  sbCAutoString(PRUint64 aValue)
  {
    char valueStr[64];

    PR_snprintf(valueStr, sizeof(valueStr), "%llu", aValue);
    AssignLiteral(valueStr);
  }

  sbCAutoString(nsID* aValue)
  {
    char idString[NSID_LENGTH];
    aValue->ToProvidedString(idString);
    AssignLiteral(idString);
  }
};

/**
 * Class used to pass void strings to functions.
 *
 * E.g., SomeFunction(SBVoidString());
 */
class SBVoidString : public nsString
{
public:
  SBVoidString()
  {
    SetIsVoid(PR_TRUE);
  }
};

/**
 * Class used to pass void strings to functions.
 *
 * E.g., SomeFunction(SBVoidCString());
 */
class SBVoidCString : public nsCString
{
public:
  SBVoidCString()
  {
    SetIsVoid(PR_TRUE);
  }
};
/// @see nsString::FindCharInSet
PRInt32 nsString_FindCharInSet(const nsAString& aString,
                               const char *aPattern,
                               PRInt32 aOffset = 0);

void AppendInt(nsAString& str, PRUint64 val);

PRInt64 nsString_ToInt64(const nsAString& str, nsresult* rv = nsnull);

PRUint64 nsString_ToUint64(const nsAString& str, nsresult* rv = nsnull);

/**
 * Trim whitespace from the beginning and end of a string; then compress
 * remaining runs of whitespace characters to a single space.
 *
 * \param aString               String to compress.
 * \param aLeading              Whether to compress the leading whitespace.
 * \param aTrailing             Whether to compress the trailing whitespace.
 *
 * @see CompressWhitespace in nsStringAPI.
 */
void SB_CompressWhitespace(nsAString& aString,
                           bool aLeading = PR_TRUE,
                           bool aTrailing = PR_TRUE);

nsresult SB_StringEnumeratorEquals(nsIStringEnumerator* aLeft,
                                   nsIStringEnumerator* aRight,
                                   bool* _retval);

/**
 * Searches a string for any occurences of any character of a set and replace
 * them with a replacement character.  Modifies the string in-place.
 * @see nsString_internal::ReplaceChar
 */
void nsString_ReplaceChar(/* inout */ nsAString& aString,
                          const nsAString& aOldChars,
                          const PRUnichar aNewChar);

void
nsCString_ReplaceChars(nsACString& aOldString,
                       const nsACString& aOldChars,
                       const char aNewChar);

/**
 * Searches a string for any occurences of the substring and replaces
 * it with the replacement string. Modifies the string in-place.
 * @see nsString_internal::ReplaceSubstring
 */
void nsString_ReplaceSubstring(/* inout */ nsAString &aString,
                               const nsAString &aOldString,
                               const nsAString &aNewString);

/**
 * Return true if the given string is possibly UTF8
 * (i.e. it errs on the side of returning true)
 *
 * Note that it assumes all 7-bit encodings are utf8, and doesn't check for
 * invalid characters (e.g. 0xFFFE, surrogates).  This is a weaker check than
 * the nsReadableUtils version.
 */
bool IsLikelyUTF8(const nsACString& aString);

/**
 * Returns true if the given string is UTF8
 *
 * Note that this actually decodes the UTF8 string so it is significantly
 * slower than IsLikelyUTF8.
 */
bool IsUTF8(const nsACString& aString);

/**
 * Split the string specified by aString into sub-strings using the delimiter
 * specified by aDelimiter and place the sub-strings in the array specified by
 * aStringArray.
 *
 * \param aString               String to split.
 * \param aDelimiter            Sub-string delimiter.
 * \param aSubStringArray       Array of sub-strings.
 */
void nsString_Split(const nsAString&    aString,
                    const nsAString&    aDelimiter,
                    nsTArray<nsString>& aSubStringArray);

/**
 * Split the string specified by aString into sub-strings using the delimiter
 * specified by aDelimiter and place the sub-strings in the array specified by
 * aStringArray.
 *
 * \param aString               String to split.
 * \param aDelimiter            Sub-string delimiter.
 * \param aSubStringArray       Array of sub-strings.
 */
void nsCString_Split(const nsACString&    aString,
                     const nsACString&    aDelimiter,
                     nsTArray<nsCString>& aSubStringArray);

/**
 * Creates an ISO 8610 formatted time string from the aTime passed in
 * \param aTime The time to create the formatted string
 * \return The time as an ISO 8601 formatted string
 */
nsString SB_FormatISO8601TimeString(PRTime aTime);

/**
 * Parse the ISO 8601 formatted time string specified by aISO8601TimeString and
 * return the time in aTime.
 *
 * \param aISO8601TimeString    ISO 8601 formatted time string to parse.
 * \param aTime                 Returned time.
 */
nsresult SB_ParseISO8601TimeString(const nsAString& aISO8601TimeString,
                                   PRTime*          aTime);

/*
 * Songbird string bundle URL.
 */
#define SB_STRING_BUNDLE_URL "chrome://songbird/locale/songbird.properties"

/**
 * Get and return in aString the localized string with the key specified by aKey
 * using the string bundle specified by aStringBundle.  If the string cannot be
 * found, return the default string specified by aDefault; if aDefault is not
 * specified, return aKey.
 *
 * If aStringBundle is not specified, use the main Songbird string bundle.
 *
 * \param aString               Returned localized string.
 * \param aKey                  Localized string key.
 * \param aDefault              Optional default string.
 * \param aStringBundle         Optional string bundle.
 */
nsresult SBGetLocalizedString(nsAString&             aString,
                              const nsAString&       aKey,
                              const nsAString&       aDefault,
                              class nsIStringBundle* aStringBundle = nsnull);

nsresult SBGetLocalizedString(nsAString&       aString,
                              const nsAString& aKey);

nsresult SBGetLocalizedString(nsAString&             aString,
                              const char*            aKey,
                              const char*            aDefault = nsnull,
                              class nsIStringBundle* aStringBundle = nsnull);

nsresult SBGetLocalizedFormattedString(nsAString&                aString,
                                       const nsAString&          aKey,
                                       const nsTArray<nsString>& aParams,
                                       const nsAString&          aDefault,
                                       class nsIStringBundle*    aStringBundle);

/**
 * Class used to create localized strings.
 *
 * E.g., SomeFunction(SBLocalizedString("string_key"));
 */
class SBLocalizedString : public nsString
{
public:
  SBLocalizedString(const nsAString&       aKey,
                    const nsAString&       aDefault,
                    class nsIStringBundle* aStringBundle = nsnull)
  {
    nsString stringValue;
    SBGetLocalizedString(stringValue, aKey, aDefault, aStringBundle);
    Assign(stringValue);
  }

  SBLocalizedString(const nsAString& aKey)
  {
    nsString stringValue;
    SBGetLocalizedString(stringValue, aKey);
    Assign(stringValue);
  }

  SBLocalizedString(const char*            aKey,
                    const char*            aDefault = nsnull,
                    class nsIStringBundle* aStringBundle = nsnull)
  {
    nsString stringValue;
    SBGetLocalizedString(stringValue, aKey, aDefault, aStringBundle);
    Assign(stringValue);
  }

  SBLocalizedString(const char*               aKey,
                    const nsTArray<nsString>& aParams,
                    const char*               aDefault = nsnull,
                    class nsIStringBundle*    aStringBundle = nsnull)
  {
    // Get the key and default arguments.
    nsString key;
    key.AssignLiteral(aKey);
    nsString _default;
    if (aDefault)
      _default.AssignLiteral(aDefault);
    else
      _default.SetIsVoid(PR_TRUE);

    // Set string value.
    nsString stringValue;
    SBGetLocalizedFormattedString(stringValue,
                                  key,
                                  aParams,
                                  _default,
                                  aStringBundle);
    Assign(stringValue);
  }

  SBLocalizedString(const nsAString&          aKey,
                    const nsTArray<nsString>& aParams,
                    const nsAString&          aDefault,
                    class nsIStringBundle*    aStringBundle = nsnull)
  {
    nsString stringValue;
    SBGetLocalizedFormattedString(stringValue,
                                  aKey,
                                  aParams,
                                  aDefault,
                                  aStringBundle);
    Assign(stringValue);
  }
};

template <class T>
inline
T const & sbAppendStringArrayDefaultExtractor(T const & aArrayItem)
{
  return aArrayItem;
}

template <class S, class Sep, class T, class E>
inline
S sbAppendStringArray(S & aTarget,
                      Sep const & aSeparator,
                      T const & aStringArray,
                      E aExtractor)
{
  // Save off the lengths
  PRUint32 const separatorLength = aSeparator.Length();
  PRUint32 const stringLength = aTarget.Length();
  PRUint32 const length = aStringArray.Length();

  // Calc the final length from the original string + length of all the
  // separators
  PRUint32 finalLength = stringLength + (separatorLength * (aStringArray.Length() - 1));
  for (PRUint32 index = 0; index < length; ++index) {
    finalLength += aStringArray[index].Length();
  }
  typename S::char_type const * const separator = aSeparator.BeginReading();
  // Set the writePosition to the end of the string
  typename S::char_type * writePosition;
  typename S::char_type * endPosition;
  aTarget.BeginWriting(&writePosition, &endPosition, finalLength);
  if (!writePosition)
    return aTarget;
  writePosition += stringLength;
  // Now append
  bool const isSeparatorEmpty = aSeparator.IsEmpty();
  for (PRUint32 index = 0; index < length; ++index) {
    if (index != 0 && !isSeparatorEmpty) {
      memcpy(writePosition, separator, separatorLength * sizeof(typename S::char_type));
      writePosition += separatorLength;
    }
    S const & value = aExtractor(aStringArray[index]);
    memcpy(writePosition,
           value.BeginReading(),
           value.Length() * sizeof(typename S::char_type));
    writePosition += value.Length();
  }
  return aTarget;
}

template <class S, class Sep, class T>
inline
S sbAppendStringArray(S & aTarget,
                      Sep const & aSeparator,
                      T const & aStringArray)
{
  return sbAppendStringArray(aTarget, aSeparator, aStringArray, sbAppendStringArrayDefaultExtractor<S>);
}


/**
 * Append the strings in the string enumerator specified by aEnumerator to the
 * string array specified by aStringArray.
 *
 * \param aStringArray          String array to which to append.
 * \param aEnumerator           Source string enumerator.
 *
 * Example:
 *
 *   nsTArray<nsString> stringArray;
 *   nsCOMPtr<nsIStringEnumerator> stringEnumerator = GetStringEnumerator();
 *   sbAppendStringEnumerator(stringArray, stringEnumerator);
 */
template <class StringType, class EnumeratorType>
inline nsresult
sbAppendStringEnumerator(StringType&     aStringArray,
                         EnumeratorType* aEnumerator)
{
  NS_ENSURE_ARG_POINTER(aEnumerator);

  // Append all elements from the enumerator.
  nsresult rv;
  while (1) {
    // Check if the enumerator has any more elements.
    bool hasMore;
    rv = aEnumerator->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMore)
      break;

    // Append the enumerator element to the array.
    typename StringType::elem_type stringElement;
    rv = aEnumerator->GetNext(stringElement);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(aStringArray.AppendElement(stringElement),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

//
// Auto-disposal class wrappers.
//
//   sbAutoSmprintf             Wrapper to auto-free strings created by
//                              smprintf.
//

SB_AUTO_NULL_CLASS(sbAutoSmprintf, char*, PR_smprintf_free(mValue));

/**
 * This method escapes strings for use in HTML/XML.documents
 * copied from nsEscape.cpp version of nsEscapeHTML2
 */
template <class T>
T sbEscapeXML(T const & aSrc)
{
  T result;
  typename T::char_type const * src;
  typename T::char_type const * srcEnd;
  aSrc.BeginReading(&src, &srcEnd);

  typename T::char_type * destStart;
  typename T::char_type * destEnd;
  result.BeginWriting(&destStart, &destEnd, (aSrc.Length() * 6 + 1));
  typename T::char_type * dest = destStart;

  while (src != srcEnd) {
    const char c = *src++;
    // Escape the character if needed
    switch (c) {
      case '<':
        *dest++ = '&';
        *dest++ = 'l';
        *dest++ = 't';
        *dest++ = ';';
      break;
      case '>':
        *dest++ = '&';
        *dest++ = 'g';
        *dest++ = 't';
        *dest++ = ';';
      break;
      case '&':
        *dest++ = '&';
        *dest++ = 'a';
        *dest++ = 'm';
        *dest++ = 'p';
        *dest++ = ';';
      break;
      case '"':
        *dest++ = '&';
        *dest++ = 'q';
        *dest++ = 'u';
        *dest++ = 'o';
        *dest++ = 't';
        *dest++ = ';';
      break;
      case '\'':
        *dest++ = '&';
        *dest++ = '#';
        *dest++ = '3';
        *dest++ = '9';
        *dest++ = ';';
      break;
      default:
        *dest++ = c;
      break;
    }
  }

  result.SetLength(dest - destStart);
  return result;
}

#endif /* __SBSTRINGUTILS_H__ */
