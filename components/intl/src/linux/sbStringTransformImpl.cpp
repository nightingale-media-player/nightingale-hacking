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

#include <glib.h>
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

NS_IMETHODIMP 
sbStringTransformImpl::NormalizeString(const nsAString & aCharset, 
                                       PRUint32 aTransformFlags, 
                                       const nsAString & aInput, 
                                       nsAString & _retval)
{
  nsCString str;
  CopyUTF16toUTF8(aInput, str);

  if(aTransformFlags & sbIStringTransform::TRANSFORM_LOWERCASE) {
    gchar* lowercaseStr = g_utf8_strdown(str.BeginReading(), str.Length());
    NS_ENSURE_TRUE(lowercaseStr, NS_ERROR_OUT_OF_MEMORY);
    str.Assign(lowercaseStr);
    g_free(lowercaseStr);
  }

  if(aTransformFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {
    gchar* uppercaseStr = g_utf8_strup(str.BeginReading(), str.Length());
    NS_ENSURE_TRUE(uppercaseStr, NS_ERROR_OUT_OF_MEMORY);
    str.Assign(uppercaseStr);
    g_free(uppercaseStr);
  }

  if(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE) {
    nsString workingStr;

    PRBool leadingOnly = aTransformFlags & 
                         sbIStringTransform::TRANSFORM_IGNORE_LEADING;
    PRBool bypassTest = PR_FALSE;

    gchar* nonspaceStr = g_utf8_normalize(str.BeginReading(), 
                                          str.Length(), 
                                          G_NORMALIZE_ALL);
    NS_ENSURE_TRUE(nonspaceStr, NS_ERROR_OUT_OF_MEMORY);

    glong strLen = g_utf8_strlen(nonspaceStr, -1);
    
    for(glong currentChar = 0; currentChar < strLen; ++currentChar) {

      gchar* offset = g_utf8_offset_to_pointer(nonspaceStr, currentChar);
      gunichar unichar = g_utf8_get_char(offset);
      GUnicodeType unicharType = g_unichar_type(unichar);

      if(bypassTest ||
         (unicharType != G_UNICODE_NON_SPACING_MARK && 
          unicharType != G_UNICODE_COMBINING_MARK &&
          unicharType != G_UNICODE_ENCLOSING_MARK)) {
        workingStr += unichar;
        if(leadingOnly)
          bypassTest = PR_TRUE;
      }
    }

    g_free(nonspaceStr);
    CopyUTF16toUTF8(workingStr, str);
  }

  if(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    nsString workingStr;

    PRBool leadingOnly = aTransformFlags & 
                         sbIStringTransform::TRANSFORM_IGNORE_LEADING;
    PRBool bypassTest = PR_FALSE;

    gchar* nosymbolsStr = g_utf8_normalize(str.BeginReading(), 
                                           str.Length(), 
                                           G_NORMALIZE_ALL);
    NS_ENSURE_TRUE(nosymbolsStr, NS_ERROR_OUT_OF_MEMORY);

    glong strLen = g_utf8_strlen(nosymbolsStr, -1);
    
    for(glong currentChar = 0; currentChar < strLen; ++currentChar) {
      gchar* offset = g_utf8_offset_to_pointer(nosymbolsStr, currentChar);
      gunichar unichar = g_utf8_get_char(offset);
      GUnicodeType unicharType = g_unichar_type(unichar);

      if (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS) {
        PRInt32 numberLength;
        SB_ExtractLeadingNumber((const gchar *)offset, NULL, NULL, &numberLength);
        if (numberLength > 0) {
          for (glong copychar=0;copychar < numberLength;copychar++) {
            gchar* copyoffset = g_utf8_offset_to_pointer(nosymbolsStr, currentChar+copychar);
            gunichar unichar = g_utf8_get_char(copyoffset);
            workingStr += unichar;
          }
          currentChar += numberLength-1;
          if(leadingOnly)
            bypassTest = PR_TRUE;
          continue;
        }
      }

      if(bypassTest ||
         (unicharType != G_UNICODE_CURRENCY_SYMBOL &&
          unicharType != G_UNICODE_MODIFIER_SYMBOL &&
          unicharType != G_UNICODE_MATH_SYMBOL &&
          unicharType != G_UNICODE_OTHER_SYMBOL)) {
        workingStr += unichar;
        if(leadingOnly)
          bypassTest = PR_TRUE;
      }
    }

    g_free(nosymbolsStr);
    CopyUTF16toUTF8(workingStr, str); 
  }

  if((aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM) ||
     (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE)) {
    nsString workingStr;

    PRBool leadingOnly = aTransformFlags & 
                         sbIStringTransform::TRANSFORM_IGNORE_LEADING;
    PRBool bypassTest = PR_FALSE;

    gchar* nosymbolsStr = g_utf8_normalize(str.BeginReading(), 
                                           str.Length(), 
                                           G_NORMALIZE_ALL);
    NS_ENSURE_TRUE(nosymbolsStr, NS_ERROR_OUT_OF_MEMORY);

    glong strLen = g_utf8_strlen(nosymbolsStr, -1);
    
    for(glong currentChar = 0; currentChar < strLen; ++currentChar) {

      gchar* offset = g_utf8_offset_to_pointer(nosymbolsStr, currentChar);
      gunichar unichar = g_utf8_get_char(offset);
      GUnicodeType unicharType = g_unichar_type(unichar);

      if (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS) {
        PRInt32 numberLength;
        SB_ExtractLeadingNumber((const gchar *)offset, NULL, NULL, &numberLength);
        if (numberLength > 0) {
          for (glong copychar=0;copychar < numberLength;copychar++) {
            gchar* copyoffset = g_utf8_offset_to_pointer(nosymbolsStr, currentChar+copychar);
            gunichar unichar = g_utf8_get_char(copyoffset);
            workingStr += unichar;
          }
          currentChar += numberLength-1;
          if(leadingOnly)
            bypassTest = PR_TRUE;
          continue;
        }
      }

      if(bypassTest ||
         (unicharType == G_UNICODE_LOWERCASE_LETTER ||
          unicharType == G_UNICODE_MODIFIER_LETTER ||
          unicharType == G_UNICODE_OTHER_LETTER ||
          unicharType == G_UNICODE_TITLECASE_LETTER ||
          unicharType == G_UNICODE_UPPERCASE_LETTER ||
          unicharType == G_UNICODE_DECIMAL_NUMBER ||
          unicharType == G_UNICODE_LETTER_NUMBER ||
          unicharType == G_UNICODE_OTHER_NUMBER) ||
          (!(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE) && 
            unichar == ' ')) {
        workingStr += unichar;
        if(leadingOnly)
          bypassTest = PR_TRUE;
      }
    }

    g_free(nosymbolsStr);
    CopyUTF16toUTF8(workingStr, str);
  }

  CopyUTF8toUTF16(str, _retval);

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
