/* vim: set sw=2 : */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_TRANSCODEPROFILEPROPERTY_H__
#define __SB_TRANSCODEPROFILEPROPERTY_H__

#include <sbITranscodeProfile.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class nsIVariant;

class sbTranscodeProfileProperty : public sbITranscodeProfileProperty
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEPROFILEPROPERTY

  sbTranscodeProfileProperty();

// setters
  nsresult SetPropertyName(const nsAString & aPropertyName);
  nsresult SetValueMin(nsIVariant * aValueMin);
  nsresult SetValueMax(nsIVariant * aValueMax);
  nsresult SetHidden(const PRBool aHidden);
  nsresult SetMapping(const nsACString & aMapping);
  nsresult SetScale(const nsACString & aScale);

private:
  ~sbTranscodeProfileProperty();

protected:
  /* The name of the property */
  nsString mPropertyName;

  /* The minimum value this property may have, if any */
  nsCOMPtr<nsIVariant> mValueMin;

  /* The maximum value this property may have, if any */
  nsCOMPtr<nsIVariant> mValueMax;

  /* \brief The current value of this property (initially the default) */
  nsCOMPtr<nsIVariant> mValue;
  
  /**
   * \brief If true, this property should not be output (instead it should
   * be used for calcuating a different property)
   */
  PRBool mHidden;

  /**
   * string key for a mapping; if not empty, the value of this property should
   * be set from the other property given.
   */
  nsCString mMapping;

  /**
   * a scaling factor, expressed as a fraction; the output value of this
   * property should be multiplied by this fraction.
   */
  nsCString mScale;
};

#endif /* __SB_TRANSCODEPROFILEPROPERTY_H__ */
