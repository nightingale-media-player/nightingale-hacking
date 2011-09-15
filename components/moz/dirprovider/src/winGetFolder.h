/*
  Originally from /xpcom/io/SpecialSystemDirectory.cpp
  (with changeset http://hg.mozilla.org/mozilla-central/rev/09f7d5c32f3a
  removed because we don't have to handle this case)
*/
 
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Doug Turner <dougt@netscape.com>
 *   IBM Corp.
 *   Fredrik Holmqvist <thesuckiestemail@yahoo.se>
 *   Jungshik Shin <jshin@i18nl10n.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

static nsresult GetWindowsFolder(int folder, nsILocalFile** aFile)
{
#ifdef WINCE
#define SHGetSpecialFolderPathW SHGetSpecialFolderPath
#endif

    WCHAR path[MAX_PATH + 2];
    HRESULT result = SHGetSpecialFolderPathW(NULL, path, folder, true);

    if (!SUCCEEDED(result))
        return NS_ERROR_FAILURE;

    // Append the trailing slash
    int len = wcslen(path);
    if (len > 1 && path[len - 1] != L'\\')
    {
        path[len]   = L'\\';
        path[++len] = L'\0';
    }

    return NS_NewLocalFile(nsDependentString(path, len), PR_TRUE, aFile);
}
