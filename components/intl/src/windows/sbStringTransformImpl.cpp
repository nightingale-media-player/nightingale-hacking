/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbStringTransformImpl.h"

#include <nsMemory.h>
#include <nsStringGlue.h>

#include <prmem.h>
#include "sbLeadingNumbers.h"

sbStringTransformImpl::sbStringTransformImpl()
{
}

sbStringTransformImpl::~sbStringTransformImpl()
{
}

nsresult 
sbStringTransformImpl::Init() {
  return NS_OK;
}

unsigned long 
sbStringTransformImpl::MakeFlags(PRUint32 aFlags,
                                 nsTArray<WORD> aExcludeChars[NTYPES],
                                 nsTArray<WORD> aIncludeChars[NTYPES])
{
  DWORD actualFlags = 0;
  
  if(aFlags & sbIStringTransform::TRANSFORM_LOWERCASE) {
    actualFlags |= LCMAP_LOWERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {
    actualFlags |= LCMAP_UPPERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE) {
    aExcludeChars[C3].AppendElement(C3_DIACRITIC);
    aExcludeChars[C3].AppendElement(C3_NONSPACING);
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    aExcludeChars[C3].AppendElement(C3_LEXICAL);
    aExcludeChars[C3].AppendElement(C3_VOWELMARK);
  }

  if((aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM) ||
     (aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE)) {
    aExcludeChars[C3].AppendElement(C3_LEXICAL);
    aExcludeChars[C1].AppendElement(C1_PUNCT);
    aIncludeChars[C3].AppendElement(C3_ALPHA);
    aIncludeChars[C2].AppendElement(C2_EUROPENUMBER);
    aIncludeChars[C1].AppendElement(C1_DIGIT);
    aIncludeChars[C1].AppendElement(C1_ALPHA);
    if (aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE) {
      aExcludeChars[C1].AppendElement(C1_SPACE);
    }
  }

  return actualFlags;
}

NS_IMETHODIMP 
sbStringTransformImpl::NormalizeString(const nsAString & aCharset, 
                                       PRUint32 aTransformFlags, 
                                       const nsAString & aInput, 
                                       nsAString & _retval)
{
  nsString finalStr;
  nsString inStr(aInput);

  if(inStr.IsEmpty()) {
    _retval.Truncate();
    return NS_OK;
  }

  nsTArray<WORD> excludeChars[NTYPES];
  nsTArray<WORD> includeChars[NTYPES];
  DWORD dwFlags = MakeFlags(aTransformFlags, 
                            excludeChars,
                            includeChars);

  if(aTransformFlags & sbIStringTransform::TRANSFORM_LOWERCASE ||
     aTransformFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {

    WCHAR *wszJunk = {0};
    int requiredBufferSize = ::LCMapStringW(LOCALE_USER_DEFAULT,
                                            dwFlags,
                                            inStr.BeginReading(),
                                            inStr.Length(),
                                            wszJunk,
                                            0);

    nsString bufferStr;
    int convertedChars = 
      ::LCMapStringW(LOCALE_USER_DEFAULT, 
                     dwFlags, 
                     inStr.BeginReading(), 
                     inStr.Length(), 
                     bufferStr.BeginWriting(requiredBufferSize),
                     requiredBufferSize);

    NS_ENSURE_TRUE(convertedChars == requiredBufferSize, 
                   NS_ERROR_CANNOT_CONVERT_DATA);

    finalStr = bufferStr;
    inStr = bufferStr;
  }

  if(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE ||
     aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS ||
     aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM ||
     aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE) {
    PRBool leadingOnly = aTransformFlags & 
                         sbIStringTransform::TRANSFORM_IGNORE_LEADING;
    PRBool bypassTest = PR_FALSE;
    LPWSTR wszJunk = {0};
    int requiredBufferSize = ::FoldStringW(MAP_COMPOSITE, 
                                           inStr.BeginReading(), 
                                           inStr.Length(), 
                                           wszJunk, 
                                           0);

    nsString bufferStr;
    int convertedChars = 
      ::FoldStringW(MAP_COMPOSITE, 
                    inStr.BeginReading(),
                    inStr.Length(),
                    bufferStr.BeginWriting(requiredBufferSize),
                    requiredBufferSize);

    NS_ENSURE_TRUE(convertedChars == requiredBufferSize,
                   NS_ERROR_CANNOT_CONVERT_DATA);

    LPWORD ct1 = new WORD[requiredBufferSize];
    BOOL success = GetStringTypeW(CT_CTYPE1,
                                  (LPWSTR) bufferStr.BeginReading(), 
                                  bufferStr.Length(), 
                                  &ct1[0]);

    if(!success) {
      delete [] ct1;
      _retval.Truncate();
      return NS_ERROR_CANNOT_CONVERT_DATA;
    }

    LPWORD ct2 = new WORD[requiredBufferSize];
    success = GetStringTypeW(CT_CTYPE2,
                             (LPWSTR) bufferStr.BeginReading(), 
                             bufferStr.Length(), 
                             &ct2[0]);

    if(!success) {
     delete [] ct1;
     delete [] ct2;
     _retval.Truncate();
     return NS_ERROR_CANNOT_CONVERT_DATA;
    }

    LPWORD ct3 = new WORD[requiredBufferSize];
    success = GetStringTypeW(CT_CTYPE3,
                             (LPWSTR) bufferStr.BeginReading(), 
                             bufferStr.Length(), 
                             &ct3[0]);

    if(!success) {
     delete [] ct1;
     delete [] ct2;
     delete [] ct3;
     _retval.Truncate();
     return NS_ERROR_CANNOT_CONVERT_DATA;
    }

    LPWORD charTypes[NTYPES] = {ct1, ct2, ct3};

    for(int current = 0; current < requiredBufferSize; ++current) {
      PRBool validChar = PR_TRUE;
      PRInt32 skipChars = 0;

      if (!bypassTest) {
        if (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS) {
          PRInt32 numberLength;
          SB_ExtractLeadingNumber(bufferStr.BeginReading() + current, NULL, NULL, &numberLength);
          if (numberLength > 0) {
            finalStr.Append(bufferStr.BeginReading() + current, numberLength);
            current += numberLength-1;
            if (leadingOnly) {
              bypassTest = PR_TRUE;
            }
            continue;
          }
        }
        
        // first check if the char is excluded by any of its type flags
        for (int type = FIRSTTYPE; type <= LASTTYPE && validChar; type++) {
          PRUint32 excludeCharsLength = excludeChars[type].Length();
          for(PRUint32 invalid = 0; invalid < excludeCharsLength; ++invalid) {
            if(excludeChars[type][invalid] & charTypes[type][current]) {
              validChar = PR_FALSE;
              break;
            }
          }
        }
        // next, check if the char is in the included chars arrays. if all
        // arrays are empty, allow all chars instead of none
        PRBool found = PR_FALSE;
        PRBool testedAnything = PR_FALSE;
        for (int type = FIRSTTYPE; 
             type <= LASTTYPE && validChar && !found; 
             type++) {
          PRUint32 includeCharsLength = includeChars[type].Length();
          for(PRUint32 valid = 0; valid < includeCharsLength; ++valid) {
            testedAnything = PR_TRUE;
            if (includeChars[type][valid] & charTypes[type][current]) {
              found = PR_TRUE;
              break;
            }
          }
        }
        if (testedAnything && 
            !found) {
          validChar = PR_FALSE;    
        }
      }
            
      if(validChar) {
        if (leadingOnly) {
          bypassTest = PR_TRUE;
        }
        finalStr.Append(bufferStr.CharAt(current));
      }
      current += skipChars;
    }

    delete [] ct1;
    delete [] ct2;
    delete [] ct3;
  }

  _retval = finalStr;

  return NS_OK;
}

NS_IMETHODIMP 
sbStringTransformImpl::ConvertToCharset(const nsAString & aDestCharset, 
                                        const nsAString & aInput, 
                                        nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbStringTransformImpl::GuessCharset(const nsAString & aInput, 
                                    nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
