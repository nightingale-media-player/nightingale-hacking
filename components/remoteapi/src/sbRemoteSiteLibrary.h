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

#ifndef __SB_REMOTE_SITELIBRARY_H__
#define __SB_REMOTE_SITELIBRARY_H__

#include "sbRemoteAPI.h"
#include "sbRemoteLibraryBase.h"
#include <sbIRemoteLibrary.h>

#include <nsCOMPtr.h>

class nsIFile;
class nsIURI;

#define SONGBIRD_REMOTESITELIBRARY_CONTRACTID           \
  "@songbirdnest.com/remoteapi/remotesitelibrary;1"
#define SONGBIRD_REMOTESITELIBRARY_CLASSNAME            \
  "Songbird Remote Site Library"
#define SONGBIRD_REMOTESITELIBRARY_CID                  \
{ /* 76e695a5-a9dc-435e-a06a-a9bec0c39afa */            \
  0x76e695a5,                                           \
  0xa9dc,                                               \
  0x435e,                                               \
  {0xa0, 0x6a, 0xa9, 0xbe, 0xc0, 0xc3, 0x9a, 0xfa}      \
}

class sbRemoteSiteLibrary : public sbRemoteLibraryBase,
                            public sbIRemoteSiteLibrary
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIREMOTESITELIBRARY
  NS_FORWARD_SBIREMOTELIBRARY(sbRemoteLibraryBase::)
  SB_DECL_SECURITYCHECKEDCOMP_INIT

  sbRemoteSiteLibrary();

protected:
  virtual ~sbRemoteSiteLibrary();
  virtual nsresult InitInternalMediaList();

  // fetches the URI from the security mixin
  already_AddRefed<nsIURI> GetURI();
  // validates aPath against aSiteURI, setting aPath if empty
  nsresult CheckPath( nsACString &aPath, nsIURI *aSiteURI );
  // validates aDomain against aSiteURI, setting aDomain if empty
  nsresult CheckDomain( nsACString &aDomain, nsIURI *aSiteURI );
  // builds a path to the db file for the passed in domain and path
  already_AddRefed<nsIFile> GetSiteLibraryFile( const nsACString &aDomain,
                                                const nsACString &aPath );

  nsRefPtr<sbRemoteSiteMediaList> mRemSiteMediaList;

  // Only set in debug builds - used for validating library creation
#ifdef DEBUG
  nsString mFilename;
#endif
};

#endif // __SB_REMOTE_SITELIBRARY_H__
