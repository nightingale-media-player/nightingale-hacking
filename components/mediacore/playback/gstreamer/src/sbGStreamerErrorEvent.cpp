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

#include "sbGStreamerErrorEvent.h"

#include <nsThreadUtils.h>

#include <gst/gst.h>

NS_IMPL_ISUPPORTS1(sbGStreamerErrorEvent, sbIGStreamerEvent)

sbGStreamerErrorEvent::sbGStreamerErrorEvent(GError *error) :
    mCode(error->code),
    mDomain(error->domain),
    mMessage(error->message)
{
}

NS_IMETHODIMP sbGStreamerErrorEvent::GetType(PRUint32 *aType)
{
  int base;

  if (mDomain == GST_CORE_ERROR) {
    base = EVENT_ERROR_CORE_BASE;
  }
  else if (mDomain == GST_LIBRARY_ERROR) {
    base = EVENT_ERROR_LIBRARY_BASE;
  }
  else if (mDomain == GST_RESOURCE_ERROR) {
    base = EVENT_ERROR_RESOURCE_BASE;
  }
  else if (mDomain == GST_STREAM_ERROR) {
    base = EVENT_ERROR_STREAM_BASE;
  }
  else {
    // Unknown error code.
    return NS_ERROR_FAILURE;
  }

  // We take advantage of the fact that the GStreamer error codes
  // are sequentially numbered from 1, and map them this way.
  *aType = base + mCode;

  return NS_OK;
}

NS_IMETHODIMP sbGStreamerErrorEvent::GetMessage(nsAString & aMessage)
{
  // Convert and write to argument string.
  CopyUTF8toUTF16(nsDependentCString(mMessage), aMessage);

  return NS_OK;
}

