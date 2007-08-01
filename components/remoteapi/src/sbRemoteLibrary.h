/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_REMOTE_LIBRARY_H__
#define __SB_REMOTE_LIBRARY_H__

#include "sbRemoteAPI.h"
#include "sbRemoteLibraryBase.h"
#include <sbIRemoteLibrary.h>

#include <nsIClassInfo.h>

#define SONGBIRD_REMOTELIBRARY_CONTRACTID               \
  "@songbirdnest.com/remoteapi/remotelibrary;1"
#define SONGBIRD_REMOTELIBRARY_CLASSNAME                \
  "Songbird Remote Library"
#define SONGBIRD_REMOTELIBRARY_CID                      \
{ /* 94b825ed-7308-473f-9ed3-2fd97463fe3c */            \
  0x94b825ed,                                            \
  0x7308,                                                \
  0x473f,                                                \
  {0x9e, 0xd3, 0x2f, 0xd9, 0x74, 0x63, 0xfe, 0x3c}       \
}

class sbRemoteLibrary : public sbRemoteLibraryBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  sbRemoteLibrary();

protected:
  virtual ~sbRemoteLibrary();

  // on connection to a library, set the internal remote medialist
  virtual nsresult InitInternalMediaList();
};

#endif // __SB_REMOTE_LIBRARY_H__
