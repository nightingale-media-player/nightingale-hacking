/* vim: set sw=2 :miv */
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

#ifndef SBWPDCAPABILITIESBUILDER_H_
#define SBWPDCAPABILITIESBUILDER_H_

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include "sbWPDCommon.h"

class sbIDeviceCapabilities;
struct IPortableDeviceCapabilities;

/**
 * \brief This class groups the functions related to the function of building 
 * the device capabilities There's a number of assumptions in this 
 * implementation. The biggest is that all formats found under the capture
 * categories' content types are a subset of the same formats found under the 
 * rendering categories' content types. I think it would be odd for a device to
 * be able create content it could not render. This assumption is used because 
 * the sbDeviceCapabilities can only call addFormats for a content once. At 
 * some point either this class can add additional logic to overcome this 
 * limitation or the capabilities implementation can be modified to allow this.
 */
class sbWPDCapabilitiesBuilder
{
public:
  sbWPDCapabilitiesBuilder(IPortableDeviceCapabilities * wpdCapabilities);
	~sbWPDCapabilitiesBuilder();
	nsresult Create(sbIDeviceCapabilities ** capabilities);
  /**
   * Sets the list of formats for a given content type
   */
  nsresult SetSupportedFormats(GUID contentType);
  /**
   * Sets the list of content types for rendering function types
   */
  nsresult SetContentTypes();
  /**
   * This function sets the content types for the various supported capture functional
   * categories.
   */
  nsresult SetContentCaptureTypes();
  /**
   * Helper function that sets the list of event types we support
   */
  nsresult SetEventTypes();
	private:
	  nsRefPtr<IPortableDeviceCapabilities> mWPDCapabilities;
	  nsCOMPtr<sbIDeviceCapabilities> mSBCapabilities;
};

#endif /*SBWPDCAPABILITIESBUILDER_H_*/
