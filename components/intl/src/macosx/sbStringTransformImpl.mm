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

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

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
  PRBool leadingOnly = 
    aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_LEADING;

  NSMutableString *str = [[NSMutableString alloc] initWithCharacters:aInput.BeginReading() 
                                                  length:aInput.Length()];
  
  if(aTransformFlags & sbIStringTransform::TRANSFORM_LOWERCASE) {
    NSString *lcaseStr = [str lowercaseString];
    str = [NSString stringWithString:lcaseStr];
  }
  
  if(aTransformFlags & sbIStringTransform::TRANSFORM_UPPERCASE) {
    NSString *ucaseStr = [str uppercaseString];
    str = [NSString stringWithString:ucaseStr];
  }
  
  if(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONSPACE) {
    if (leadingOnly) {
      NSString *strCopy = 
        [[NSMutableString alloc] initWithCharacters:aInput.BeginReading()
                                 length:aInput.Length()];
      // Perform the full transform on |strCpy| - then look for the first
      // similar character.
      CFStringTransform( (CFMutableStringRef)strCopy, 
                         NULL, 
                         kCFStringTransformStripCombiningMarks, 
                         false);
      
      // Find the first occurance of matching non-ignored characters. Then,
      // remove the 0 to i-th char from |str|.
      for (unsigned int i = 0; i < [str length]; i++) {
        if ([strCopy characterAtIndex:0] == [str characterAtIndex:i]) {
          [str replaceCharactersInRange:NSMakeRange(0, i)
               withString:@""];
          break;
        }
      }
      [strCopy release];
    }
    else {
      // Just transform the whole string
      CFStringTransform((CFMutableStringRef)str, 
                        NULL, 
                        kCFStringTransformStripCombiningMarks, 
                        false); 
    }
  }
  
  const PRUnichar *original = (const PRUnichar *)aInput.BeginReading();
  if ((aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) ||
      (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM) ||
      (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE)) {
    NSCharacterSet *symbols = [NSCharacterSet symbolCharacterSet];
    NSCharacterSet *alphaNumSet = [NSCharacterSet alphanumericCharacterSet];
    PRBool bypassSymbolFiltering = false;
    PRBool bypassNonalnumFiltering = false;
    
    for(int current = 0, optr = 0; current < [str length]; ++current, ++optr) {
      if (bypassSymbolFiltering &&
          bypassNonalnumFiltering) 
        break;
      
      unichar c = [str characterAtIndex:current];

      if (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS) {
        PRInt32 numberLength;
        SB_ExtractLeadingNumber(original + optr, NULL, NULL, &numberLength);
        if (numberLength > 0) {
          current += numberLength-1;
          optr += numberLength-1;
          if (leadingOnly) {
            break;
          }
          continue;
        }
      }
    
      if (!bypassSymbolFiltering && (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS)) {
        if([symbols characterIsMember:c]) {
          [str replaceCharactersInRange:NSMakeRange(current--, 1) withString:@""];
        } else {
          if (leadingOnly) {
            bypassSymbolFiltering = true;
          }
        }
      }

      if (!bypassNonalnumFiltering && 
          ((aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM) ||
           (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE))) {
        if(![alphaNumSet characterIsMember:c] &&
          ((c != ' ') || (aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE))) {
          [str replaceCharactersInRange:NSMakeRange(current--, 1) withString:@""];
        } else {
          if (leadingOnly) {
            bypassNonalnumFiltering = true;
          }
        }
      }
      
      // make sure we're still in sync
      NS_ASSERTION(original[optr] != 0, "error with optr position tracking");
      // but just in case...
      if (!original[optr])
        optr--;
    }
    
  }
  
  unichar *buf = (unichar *) malloc(sizeof(unichar) * [str length]);
  NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);
  
  [str getCharacters:buf];
   
  _retval.Assign(buf, [str length]);
  free(buf);
  [str release];
    
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
