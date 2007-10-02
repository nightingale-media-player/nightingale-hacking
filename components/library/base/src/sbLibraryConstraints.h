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

#ifndef __SB_LIBRARY_CONSTRAINTS_H__
#define __SB_LIBRARY_CONSTRAINTS_H__

#include <sbILibraryConstraints.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsIClassInfo.h>
#include <nsISerializable.h>

#define SONGBIRD_LIBRARYFILTER_DESCRIPTION                 \
  "Songbird Library Filter"
#define SONGBIRD_LIBRARYFILTER_CONTRACTID                  \
  "@songbirdnest.com/Songbird/Library/Filter;1"
#define SONGBIRD_LIBRARYFILTER_CLASSNAME                   \
  "Songbird Library Filter"
#define SONGBIRD_LIBRARYFILTER_CID                         \
{ /* 4aaf2da9-4ecb-4666-abca-a6326efe28bc */               \
  0x4aaf2da9,                                              \
  0x4ecb,                                                  \
  0x4666,                                                  \
  { 0xab, 0xca, 0xa6, 0x32, 0x6e, 0xfe, 0x28, 0xbc }       \
}

class sbLibraryFilter : public sbILibraryFilter,
                        public nsISerializable,
                        public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYFILTER
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  sbLibraryFilter();

private:
  PRBool mInitialized;
  nsString mProperty;
  nsTArray<nsString> mValues;
};

#define SONGBIRD_LIBRARYSEARCH_DESCRIPTION                 \
  "Songbird Library Search"
#define SONGBIRD_LIBRARYSEARCH_CONTRACTID                  \
  "@songbirdnest.com/Songbird/Library/Search;1"
#define SONGBIRD_LIBRARYSEARCH_CLASSNAME                   \
  "Songbird Library Search"
#define SONGBIRD_LIBRARYSEARCH_CID                         \
{ /* e83ec34f-dea0-4749-8d93-c18a9dc5f52c */               \
  0xe83ec34f,                                              \
  0xdea0,                                                  \
  0x4749,                                                  \
  { 0x8d, 0x93, 0xc1, 0x8a, 0x9d, 0xc5, 0xf5, 0x2c }       \
}

class sbLibrarySearch : public sbILibrarySearch,
                        public nsISerializable,
                        public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYSEARCH
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  sbLibrarySearch();

private:
  PRBool mInitialized;
  nsString mProperty;
  PRBool mIsAll;
  nsTArray<nsString> mValues;
};

#define SONGBIRD_LIBRARYSORT_DESCRIPTION                   \
  "Songbird Library Sort"
#define SONGBIRD_LIBRARYSORT_CONTRACTID                    \
  "@songbirdnest.com/Songbird/Library/Sort;1"
#define SONGBIRD_LIBRARYSORT_CLASSNAME                     \
  "Songbird Library Sort"
#define SONGBIRD_LIBRARYSORT_CID                           \
{ /* ac85b1e9-c3e1-456a-af0f-4161d36938df */               \
  0xac85b1e9,                                              \
  0xc3e1,                                                  \
  0x456a,                                                  \
  { 0xaf, 0x0f, 0x41, 0x61, 0xd3, 0x69, 0x38, 0xdf }       \
}

class sbLibrarySort : public sbILibrarySort,
                      public nsISerializable,
                      public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYSORT
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  sbLibrarySort();

private:
  PRBool mInitialized;
  nsString mProperty;
  PRBool mIsAscending;
};

#endif /* __SB_LIBRARY_CONSTRAINTS_H__ */

