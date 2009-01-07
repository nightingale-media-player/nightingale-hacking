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

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

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
    CFStringTransform( (CFMutableStringRef)str, 
                       NULL, 
                       kCFStringTransformStripCombiningMarks, 
                       false);
  }
  
  if(aTransformFlags & sbIStringTransform::TRANSFORM_IGNORE_SYMBOLS) {
    NSCharacterSet *symbols = [NSCharacterSet symbolCharacterSet];
    
    for(unsigned int current = 0; current < [str length]; ++current) {
      unichar c = [str characterAtIndex:current];
      if([symbols characterIsMember:c]) {
        NSRange r = NSMakeRange(current, 1);
        [str replaceCharactersInRange:r withString:@""];
      }
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
