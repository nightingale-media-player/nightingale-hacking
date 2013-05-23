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

#include <nscore.h>
#include <prlink.h>
#include <nsILocalFile.h>
#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsIComponentManager.h>
#include <nsIModule.h>
#include <nsXPCOM.h>
#include <sbLibraryLoaderUtils.h>
#include <mozilla-config.h>
#include <mozilla/Module.h>

#include <nsIEnvironment.h>

#ifdef DEBUG
static char kRealComponent[] = "sbGStreamerMediacore_d" MOZ_DLL_SUFFIX;
#else
static char kRealComponent[] = "sbGStreamerMediacore" MOZ_DLL_SUFFIX;
#endif

extern "C" NS_EXPORT nsresult
NSGetModule(nsIComponentManager* aCompMgr,
            nsIFile* aLocation,
            nsIModule* *aResult)
{
  nsresult rv;
  PRBool systemGst;

  // aLocation starts off pointing to this component.
  nsCOMPtr<nsIFile> parent;
  rv = aLocation->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> libDir;
  rv = parent->Clone(getter_AddRefs(libDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = libDir->SetNativeLeafName(NS_LITERAL_CSTRING("lib"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Always bundled on OSX, Windows. Default to using the bundled version
  // elsewhere, unless SB_GST_SYSTEM is set.
#if defined(XP_MACOSX) || defined(XP_WIN)
  systemGst = PR_FALSE;
#else
  nsCOMPtr<nsIEnvironment> envSvc =
    do_GetService("@mozilla.org/process/environment;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = envSvc->Exists(NS_LITERAL_STRING("SB_GST_SYSTEM"), &systemGst);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  if (!systemGst) {
    // Load the libraries in this lib dir from the gst_libs.txt list,
    // when we're running against the bundled version of gstreamer.
    nsCOMPtr<nsIFile> manifest;
    rv = parent->Clone(getter_AddRefs(manifest));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = manifest->AppendNative(NS_LITERAL_CSTRING("gst_libs.txt"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SB_LoadLibraries(manifest);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = libDir->AppendNative(NS_LITERAL_CSTRING(kRealComponent));
  NS_ENSURE_SUCCESS(rv, rv);

  // load the actual component
  nsCOMPtr<nsILocalFile> libDirLocal = do_QueryInterface(libDir, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRLibrary* lib;
  rv = libDirLocal->Load(&lib);
  NS_ENSURE_SUCCESS(rv, rv);

  const mozilla::Module *module = *(mozilla::Module const *const *)
                                  PR_FindSymbol(lib, "NSModule");
  if (module)
    return NS_OK;
  return NULL;
}

/*

This file was derived from:
http://developer.mozilla.org/en/docs/Using_Dependent_Libraries_In_Extension_Components
Copyright (c) 2005 Benjamin Smedberg <benjamin@smedbergs.us>
It had the following license:

The MIT License

Copyright (c) 2005 Benjamin Smedberg <benjamin@smedbergs.us>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
