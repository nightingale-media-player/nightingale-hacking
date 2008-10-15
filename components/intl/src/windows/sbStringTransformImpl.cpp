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

#include "sbStringTransformImpl.h"

#include <nsMemory.h>
#include <nsStringGlue.h>

#include <prmem.h>

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
sbStringTransformImpl::MakeFlags(PRUint32 aFlags, nsTArray<WORD> &aInvalidChars)
{
  DWORD actualFlags = 0;

  if(aFlags & sbIStringTransform::TRANSFORM_LOWERCASE) {
    actualFlags |= LCMAP_LOWERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {
    actualFlags |= LCMAP_UPPERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE) {
    aInvalidChars.AppendElement(C3_DIACRITIC);
    aInvalidChars.AppendElement(C3_NONSPACING);
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    aInvalidChars.AppendElement(C3_LEXICAL);
    aInvalidChars.AppendElement(C3_VOWELMARK);
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

  nsTArray<WORD> invalidChars;
  DWORD dwFlags = MakeFlags(aTransformFlags, invalidChars);

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
    int convertedChars = ::LCMapStringW(LOCALE_USER_DEFAULT, 
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
     aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    LPWSTR wszJunk = {0};
    int requiredBufferSize = ::FoldStringW(MAP_COMPOSITE, 
                                           inStr.BeginReading(), 
                                           inStr.Length(), 
                                           wszJunk, 
                                           0);

    nsString bufferStr;
    int convertedChars = ::FoldStringW(MAP_COMPOSITE, 
                                       inStr.BeginReading(),
                                       inStr.Length(),
                                       bufferStr.BeginWriting(requiredBufferSize),
                                       requiredBufferSize);

    NS_ENSURE_TRUE(convertedChars == requiredBufferSize,
                   NS_ERROR_CANNOT_CONVERT_DATA);

    LPWORD charTypes = new WORD[requiredBufferSize];
    BOOL success = GetStringTypeW(CT_CTYPE3, 
                                  (LPWSTR) bufferStr.BeginReading(), 
                                  bufferStr.Length(), 
                                  &charTypes[0]);

    if(!success) {
     delete [] charTypes;
     _retval.Truncate();
     return NS_ERROR_CANNOT_CONVERT_DATA;
    }


    PRUint32 invalidCharsLength = invalidChars.Length();

    for(int current = 0; current < requiredBufferSize; ++current) {
      PRBool validChar = PR_TRUE;
      for(PRUint32 invalid = 0; invalid < invalidCharsLength; ++invalid) {
        if(invalidChars[invalid] & charTypes[current]) {
          validChar = PR_FALSE;
          break;
        }
      }

      if(validChar) {
        finalStr.Append(bufferStr.CharAt(current));
      }
    }

    delete [] charTypes;
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
