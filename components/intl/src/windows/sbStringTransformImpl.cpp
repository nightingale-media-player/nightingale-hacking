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

#include <windows.h>

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
sbStringTransformImpl::MakeFlags(PRUint32 aFlags)
{
  DWORD actualFlags = 0;

  if(aFlags & sbIStringTransform::TRANSFORM_LOWERCASE) {
    actualFlags |= LCMAP_LOWERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {
    actualFlags |= LCMAP_UPPERCASE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE) {
    actualFlags |= NORM_IGNORENONSPACE;
  }

  if(aFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    actualFlags |= NORM_IGNORESYMBOLS;
  }

  return actualFlags;
}

NS_IMETHODIMP 
sbStringTransformImpl::NormalizeString(const nsAString & aCharset, 
                                       PRUint32 aTransformFlags, 
                                       const nsAString & aInput, 
                                       nsAString & _retval)
{
  DWORD dwFlags = MakeFlags(aTransformFlags);

  LPWSTR wszJunk = {0};
  int requiredBufferSize = ::LCMapStringW(LOCALE_USER_DEFAULT,
                                          dwFlags,
                                          aInput.BeginReading(),
                                          aInput.Length(),
                                          wszJunk,
                                          0);

  int convertedChars = ::LCMapStringW(LOCALE_USER_DEFAULT, 
                                      dwFlags, 
                                      aInput.BeginReading(), 
                                      aInput.Length(), 
                                      _retval.BeginWriting(requiredBufferSize),
                                      requiredBufferSize);
  
  NS_ENSURE_TRUE(convertedChars == requiredBufferSize, 
                 NS_ERROR_CANNOT_CONVERT_DATA);

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
