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

// Local includes
#include "sbClipboardHelper.h"

// Mozilla includes
#include <mozilla/ModuleUtils.h>

#define SB_CLIPBOARD_HELPER_CLASSNAME "sbClipboardHelper"
#define SB_CLIPBOARD_HELPER_CID \
{ 0x6063116b, 0x2b98, 0x44d5, \
  { 0x8f, 0x6e, 0x0a, 0x70, 0x43, 0xf0, 0x1f, 0xc3 } }

NS_GENERIC_FACTORY_CONSTRUCTOR(sbClipboardHelper)
NS_DEFINE_NAMED_CID(SB_CLIPBOARD_HELPER_CID);

static const mozilla::Module::CIDEntry kSongbirdMozClipboardCIDs[] = {
    { &kSB_CLIPBOARD_HELPER_CID, true, NULL, sbClipboardHelperConstructor },
    { NULL }
};

static const mozilla::Module kSongbirdMozClipboardModule = {
    mozilla::Module::kVersion,
    kSongbirdMozClipboardCIDs
};

NSMODULE_DEFN(SongbirdClipboardHelper) = &kSongbirdMozClipboardModule;
