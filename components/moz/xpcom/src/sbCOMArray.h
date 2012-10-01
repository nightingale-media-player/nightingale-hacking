/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is a COM aware array class.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alec Flett <alecf@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef sbCOMArray_h__
#define sbCOMArray_h__

#include "nsVoidArray.h"
#include "nsISupports.h"

// See below for the definition of sbCOMArray<T>

// a class that's nsISupports-specific, so that we can contain the
// work of this class in the XPCOM dll
class sbCOMArray_base
{
    friend class sbArray;
protected:
    sbCOMArray_base() {}
    sbCOMArray_base(PRInt32 aCount) : mArray(aCount) {}
    sbCOMArray_base(const sbCOMArray_base& other);
    ~sbCOMArray_base();

    PRInt32 IndexOf(nsISupports* aObject) const {
        return mArray.IndexOf(aObject);
    }

    PRInt32 IndexOfObject(nsISupports* aObject) const;

    bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateForwards(aFunc, aData);
    }
    
    bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateBackwards(aFunc, aData);
    }
    
    void Sort(nsVoidArrayComparatorFunc aFunc, void* aData) {
        mArray.Sort(aFunc, aData);
    }
    
    // any method which is not a direct forward to mArray should
    // avoid inline bodies, so that the compiler doesn't inline them
    // all over the place
    void Clear();
    bool InsertObjectAt(nsISupports* aObject, PRInt32 aIndex);
    bool InsertObjectsAt(const sbCOMArray_base& aObjects, PRInt32 aIndex);
    bool ReplaceObjectAt(nsISupports* aObject, PRInt32 aIndex);
    bool AppendObject(nsISupports *aObject) {
        return InsertObjectAt(aObject, Count());
    }
    bool AppendObjects(const sbCOMArray_base& aObjects) {
        return InsertObjectsAt(aObjects, Count());
    }
    bool RemoveObject(nsISupports *aObject);
    bool RemoveObjectAt(PRInt32 aIndex);

public:
    // override nsVoidArray stuff so that they can be accessed by
    // consumers of nsCOMArray
    PRInt32 Count() const {
        return mArray.Count();
    }

    nsISupports* ObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.FastElementAt(aIndex));
    }
    
    nsISupports* SafeObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.SafeElementAt(aIndex));
    }

    nsISupports* operator[](PRInt32 aIndex) const {
        return ObjectAt(aIndex);
    }

    // Ensures there is enough space to store a total of aCapacity objects.
    // This method never deletes any objects.
    bool SetCapacity(PRUint32 aCapacity) {
      return aCapacity > 0 ? mArray.SizeTo(static_cast<PRInt32>(aCapacity))
                           : PR_TRUE;
    }

private:
    
    // the actual storage
    nsVoidArray mArray;

    // don't implement these, defaults will muck with refcounts!
    sbCOMArray_base& operator=(const sbCOMArray_base& other);
};

#endif
