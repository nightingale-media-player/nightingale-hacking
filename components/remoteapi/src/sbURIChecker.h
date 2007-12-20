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

#ifndef __SB_URICHECKER_H__
#define __SB_URICHECKER_H__

#include <nsStringGlue.h>

class nsIURI;

/**
 * This splits out the domain/path checking against a given URI to see if
 * untrusted content is allowed to pretend to be from the given domain/path.
 * @see sbRemotePlayer::SetSiteScope()
 */
class sbURIChecker {
public:
  // Check the domain and path against the given URI
  // the domain and path will be returned fixed up
  // domain and path may be empty, in which case it will be returned
  // with the host and path of the URI
  static nsresult CheckURI(/* inout */ nsACString& aDomain,
                           /* inout */ nsACString& aPath,
                           nsIURI* aURI);

  static nsresult FixupDomain( const nsACString& aDomain,
                               nsACString& _retval );
  static nsresult FixupPath( nsIURI* aURI,
                             nsACString& _retval );
  // calls the other version of FixupPath with a generated dummy URI
  static nsresult FixupPath( const nsACString& aPath,
                             nsACString& _retval );
  // validates aPath against aSiteURI, setting aPath if empty
  static nsresult CheckPath( /* inout */ nsACString &aPath,
                             nsIURI *aSiteURI );
  // validates aDomain against aSiteURI, setting aDomain if empty
  static nsresult CheckDomain( /* inout */ nsACString &aDomain,\
                               nsIURI *aSiteURI );
};

#endif // __SB_URICHECKER_H__
