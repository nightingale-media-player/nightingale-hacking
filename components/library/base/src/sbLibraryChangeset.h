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

/**
 * \file  sbLibraryChangeset.h
 * \brief sbLibraryChangeset Definition.
 */

#ifndef __SB_LIBRARYCHANGESET_H__
#define __SB_LIBRARYCHANGESET_H__

#include <sbILibraryChangeset.h>

#include <nsIArray.h>
#include <nsIClassInfo.h>
#include <sbIMediaItem.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#define SB_PROPERTY_CHANGE_IID \
{ 0x2cb3d75f, 0xfd37, 0x4ae0, { 0xa7, 0xf1, 0xe8, 0x5c, 0x7b, 0xb9, 0xd1, 0x7b } }

#define SB_LIBRARY_CHANGE_IID \
{ 0x681aebb6, 0xa22c, 0x491f, { 0x9b, 0xc1, 0xc0, 0xa6, 0x2, 0x1d, 0xee, 0xc5 } }

#define SB_LIBRARY_CHANGESET_IID \
{ 0x4597d14e, 0x4130, 0x4438, { 0xb7, 0xbd, 0x5d, 0x9c, 0x1b, 0x8c, 0xb7, 0x7b } }

struct PRLock;

class sbPropertyChange : public sbIPropertyChange,
                         public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICHANGEOPERATION
  NS_DECL_SBIPROPERTYCHANGE
  NS_DECL_NSICLASSINFO
  NS_DECLARE_STATIC_IID_ACCESSOR(SB_PROPERTY_CHANGE_IID)

  sbPropertyChange();

  nsresult Init();
  nsresult InitWithValues(PRUint32 aOperation,
                          const nsAString &aID,
                          const nsAString &aOldValue,
                          const nsAString &aNewValue);

  nsresult SetOperation(PRUint32 aOperation);

  nsresult SetID(const nsAString &aID);

  nsresult SetOldValue(const nsAString &aOldValue);
  nsresult SetNewValue(const nsAString &aNewValue);

private:
  ~sbPropertyChange();

protected:
  PRLock*  mOperationLock;
  PRUint32 mOperation;

  PRLock*  mIDLock;
  nsString mID;

  PRLock*  mOldValueLock;
  nsString mOldValue;

  PRLock*  mNewValueLock;
  nsString mNewValue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbPropertyChange,
                              SB_PROPERTY_CHANGE_IID)

class sbLibraryChange : public sbILibraryChange,
                        public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICHANGEOPERATION
  NS_DECL_SBILIBRARYCHANGE
  NS_DECL_NSICLASSINFO
  NS_DECLARE_STATIC_IID_ACCESSOR(SB_LIBRARY_CHANGE_IID)

  sbLibraryChange();

  nsresult Init();
  nsresult InitWithValues(PRUint32 aOperation,
                          PRUint64 aTimestamp,
                          sbIMediaItem *aSourceItem,
                          sbIMediaItem *aDestinationItem,
                          nsIArray *aProperties,
                          nsIArray *aListItems);

  nsresult SetOperation(PRUint32 aOperation);

  nsresult SetTimestamp(PRUint64 aTimestamp);
  nsresult SetItems(sbIMediaItem *aSourceItem,
                    sbIMediaItem *aDestinationItem);
  nsresult SetProperties(nsIArray *aProperties);
  nsresult SetListItems(nsIArray *aProperties);
  nsresult GetItemIsListLocked(PRBool *aItemIsList);

private:
  ~sbLibraryChange();

protected:
  PRLock*  mLock;
  PRUint32 mOperation;

  PRUint64 mTimestamp;

  nsCOMPtr<sbIMediaItem>  mSourceItem;
  nsCOMPtr<sbIMediaItem>  mDestinationItem;

  nsCOMPtr<nsIArray>  mProperties;
  nsCOMPtr<nsIArray>  mListItems;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbLibraryChange,
                              SB_LIBRARY_CHANGE_IID)

class sbLibraryChangeset : public sbILibraryChangeset,
                           public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBILIBRARYCHANGESET
  NS_DECLARE_STATIC_IID_ACCESSOR(SB_LIBRARY_CHANGESET_IID)

  sbLibraryChangeset();

  nsresult Init();
  nsresult InitWithValues(nsIArray *aSourceLists,
                          sbIMediaList *aDestinationList,
                          nsIArray *aChanges);

  nsresult SetSourceLists(nsIArray *aSourceLists);
  nsresult SetDestinationList(sbIMediaList *aDestinationList);

  nsresult SetChanges(nsIArray *aChanges);

private:
  ~sbLibraryChangeset();

protected:
  PRLock*                 mSourceListsLock;
  nsCOMPtr<nsIArray>      mSourceLists;

  PRLock*                 mDestinationListLock;
  nsCOMPtr<sbIMediaList>  mDestinationList;

  PRLock*                 mChangesLock;
  nsCOMPtr<nsIArray>      mChanges;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbLibraryChangeset,
                              SB_LIBRARY_CHANGESET_IID)

#endif /* __SB_LIBRARYCHANGESET_H__ */
