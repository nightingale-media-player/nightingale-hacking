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
 * \file sbLeadingNumbers.h
 * \brief Leading Numbers Parsing
 */

#include <prtypes.h>
#include <math.h>

#ifdef XP_MACOSX
#include <Carbon/Carbon.h>
#else
#ifdef XP_UNIX
#include <glib.h>
#endif
#endif

#if defined(XP_MACOSX)
#define UTF16_CHARTYPE UniChar
#define NATIVE_CHAR_TYPE UniChar
#elif defined(XP_UNIX)
#define UTF16_CHARTYPE gunichar2
#define NATIVE_CHAR_TYPE gunichar
#elif defined(XP_WIN)
#define UTF16_CHARTYPE wchar_t
#define NATIVE_CHAR_TYPE wchar_t
#endif

#define CHARTYPE_OTHER 0
#define CHARTYPE_DIGIT 1
#define CHARTYPE_DECIMALPOINT 2
#define CHARTYPE_SIGN 3
#define CHARTYPE_EXPONENT 4

template<class CHARTYPE> inline PRInt32 SB_GetCharType(const CHARTYPE *p) {
  switch (*p) {
    case '.': 
    case ',': 
      return CHARTYPE_DECIMALPOINT;
    case '+':
    case '-': 
      return CHARTYPE_SIGN;
    case 'e':
    case 'E':
      return CHARTYPE_EXPONENT;
  }
  if (*p >= '0' && *p <= '9')
    return CHARTYPE_DIGIT;
  return CHARTYPE_OTHER;
}

template<class CHARTYPE> 
  inline void SB_ExtractLeadingNumber(const CHARTYPE *str,
                                      bool *hasLeadingNumber,
                                      PRFloat64 *leadingNumber,
                                      PRInt32 *numberLength) {

  // it would be nice to be able to do all of this with just sscanf, but 
  // unfortunately that function does not tell us where the parsed number ended,
  // and we need to know that in order to strip it from the string, so we have
  // to parse manually. also, we want to handle ',' as '.', which sscanf doesn't
  // do.

  bool gotDecimalPoint = PR_FALSE;
  bool gotExponent = PR_FALSE;
  bool gotSign = PR_FALSE;
  bool gotExponentSign = PR_FALSE;
  bool gotDigit = PR_FALSE;
  bool gotExponentDigit = PR_FALSE;
  bool abortParsing = PR_FALSE;
  PRFloat64 value = 0;
  PRInt32 expValue = 0;
  PRFloat64 decimalMul = 1;
  PRInt32 sign = 1;
  PRInt32 expSign = 1;
  
  const CHARTYPE *p = str;
  
  while (!abortParsing && *p) {
    switch (SB_GetCharType(p)) {
      case CHARTYPE_SIGN: 
        if (!gotExponent) {
          // if we already had a sign for this number, or if the number part has
          // already started (already had digits or a decimal point) we can't
          // accept a sign here, so abort parsing.
          if (gotSign || gotDigit || gotDecimalPoint) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a sign for the number part
          gotSign = PR_TRUE;
          switch (*p) {
            case '+': 
              sign = 1;
              break;
            case '-':
              sign = -1;
              break;
          }
        } else {
          // if we already had a sign for this exponent, or if the number part
          // of the exponent has already started (already had a digit in the
          // exponent) we can't accept a sign here, so abort parsing.
          if (gotExponentSign || gotExponentDigit) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a sign for the exponent part
          gotExponentSign = PR_TRUE;
          switch (*p) {
            case '+': 
              expSign = 1;
              break;
            case '-':
              expSign = -1;
              break;
          }
        }
        break;
      case CHARTYPE_DIGIT:
        // remember that the number part has started
        if (!gotExponent) {
          gotDigit = PR_TRUE;
          if (!gotDecimalPoint) {
            value *= 10;
            value += *p - '0';
          } else {
            decimalMul *= .1;
            value += (*p - '0') * decimalMul;
          }
        } else {
          gotExponentDigit = PR_TRUE;
          expValue *= 10;
          expValue += *p - '0';
        }
        break;
      case CHARTYPE_DECIMALPOINT:
        if (!gotExponent) {
          // if we already had a decimal point for this number, we can't have
          // another one, so abort parsing.
          if (gotDecimalPoint) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a decimal point for the number part
          gotDecimalPoint = PR_TRUE;
        } else {
          // decimal points cannot be part of an exponent, so abort parsing.
          abortParsing = PR_TRUE;
          break;
        }
        break;
      case CHARTYPE_EXPONENT:
        // if we already are in the exponent part, we cannot get another
        // exponent character, so abort parsing.
        if (gotExponent) {
          abortParsing = PR_TRUE;
          break;
        }
        // this is only an exponent character if the next character is either
        // a digit or a sign (it is safe to dereference p+1, since at worst
        // it will be a null terminator)
        switch (SB_GetCharType(p+1)) {
          case CHARTYPE_DIGIT:
          case CHARTYPE_SIGN:
            // remember that we got an exponent.
            gotExponent = PR_TRUE;
            break;
          default:
            // anything else means this is not an exponent, but just the letter
            // 'e' or 'E', so abort parsing.
            abortParsing = PR_TRUE;
            break;
        }
        break;
      case CHARTYPE_OTHER:
        // anything else is a character or symbol that isn't part of a valid
        // number, so abort parsing (this includes utf8 extended characters).
        abortParsing = PR_TRUE;
        break;
    }
    p++;
  }
  
  // if we stopped the parser on an invalid char, we need to back up one char,
  // otherwise the whole string was a number and p just points at the terminal
  // null char.
  if (abortParsing)
    p--;
  
  // p now points at the first character that isn't part of a valid number.
  // copy the string, without the number.
  if (numberLength) 
    *numberLength = p-str;
  
  // we may mistakenly think there is a number if we only got an exponent, or
  // just a sign, or just a decimal point, so in addition to checking that we
  // parsed at least one character, also make sure we did get digits
  if (p == str || 
      !gotDigit) {
    // no number found
    if (hasLeadingNumber)
      *hasLeadingNumber = PR_FALSE;
    if (leadingNumber)
      *leadingNumber = 0;
    if (numberLength) 
      *numberLength = 0;
  } else {
    
    // factor in the exponent
    if (expValue != 0) {
      PRFloat64 mul = pow((PRFloat64)10, (PRFloat64)(expValue * expSign));
      value *= mul;
    }
    
    // factor in the sign
    value *= sign;

    if (hasLeadingNumber)
      *hasLeadingNumber = PR_TRUE;
    if (leadingNumber)
      *leadingNumber = value;
  }
}

template<class CHARTYPE> inline PRInt32 SB_FindNextNumber(const CHARTYPE *aStr) { 
  if (!aStr) 
    return -1;
  
  const CHARTYPE *p = aStr;
  const CHARTYPE *beginning = NULL;
  while (*p) {
    PRInt32 c = SB_GetCharType(p);
    if (c == CHARTYPE_DIGIT) {
      if (!beginning)
        beginning = p;
      return beginning-aStr;
    }
    if (c == CHARTYPE_SIGN ||
        c == CHARTYPE_DECIMALPOINT) {
      if (!beginning) {
        beginning = p;
      }
    } else {
      beginning = NULL;
    }
    p++;
  }
  
  return -1;
}

