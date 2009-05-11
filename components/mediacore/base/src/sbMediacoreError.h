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
 * \file  sbMediacoreError.h
 * \brief Songbird Mediacore Error Definition.
 */

#ifndef __SB_MEDIACOREERROR_H__
#define __SB_MEDIACOREERROR_H__

#if _MSC_VER
// This is really lame but we have to do it so that sbIMediacoreError
// gets the right method signature for GetMessage.
#pragma push_macro("GetMessage")
#undef GetMessage
#endif

#include <sbIMediacoreError.h>

#include <nsAutoLock.h>
#include <nsStringGlue.h>

class sbMediacoreError : public sbIMediacoreError
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREERROR

  sbMediacoreError();

  nsresult Init(PRUint32 aCode, 
                const nsAString &aMessage);

protected:
  virtual ~sbMediacoreError();

  PRLock *mLock;
  
  PRUint32 mCode;
  nsString mMessage;
};

#if _MSC_VER
#pragma pop_macro("GetMessage")
#endif

#endif /* __SB_MEDIACOREERROR_H__ */
