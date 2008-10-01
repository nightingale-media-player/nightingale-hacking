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

#ifndef _SB_GSTREAMERMEDIACOREUTILS_H_
#define _SB_GSTREAMERMEDIACOREUTILS_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsStringGlue.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMWindow.h>
#include <nsITimer.h>
#include <nsComponentManagerUtils.h>
#include <nsIStringBundle.h>

#include <sbIPropertyArray.h>

#include <gst/gst.h>

GstTagList *ConvertPropertyArrayToTagList(sbIPropertyArray *properties);

nsresult ConvertTagListToPropertyArray(GstTagList *taglist, 
        sbIPropertyArray **aPropertyArray);

#endif // _SB_GSTREAMERMEDIACOREUTILS_H_

