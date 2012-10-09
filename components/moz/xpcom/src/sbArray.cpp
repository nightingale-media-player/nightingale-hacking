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
 * The Original Code is XPCOM Array implementation.
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

#include <mozilla/Mutex.h>
#include "sbArray.h"
#include "nsArrayEnumerator.h"
#include "nsWeakReference.h"

// used by IndexOf()
struct findIndexOfClosure
{
    nsISupports *targetElement;
    PRUint32 startIndex;
    PRUint32 resultIndex;
};

PR_STATIC_CALLBACK(bool) FindElementCallback(void* aElement, void* aClosure);

NS_INTERFACE_MAP_BEGIN(sbArray)
  NS_INTERFACE_MAP_ENTRY(nsIArray)
  NS_INTERFACE_MAP_ENTRY(nsIMutableArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMutableArray)
NS_INTERFACE_MAP_END

sbArray::sbArray()
{	
	: mLock("nsThreadsafeArray::mLock");
}
sbArray::sbArray(const sbCOMArray_base& aBaseArray)
    : mArray(aBaseArray)
{
	: mLock("nsThreadsafeArray::mLock");
}

sbArray::~sbArray()
{
    Clear();
}

NS_IMPL_THREADSAFE_ADDREF(sbArray)
NS_IMPL_THREADSAFE_RELEASE(sbArray)

NS_IMETHODIMP
sbArray::GetLength(PRUint32* aLength)
{
    MutexAutoLock lock(mLock);
    *aLength = mArray.Count();
    return NS_OK;
}

NS_IMETHODIMP
sbArray::QueryElementAt(PRUint32 aIndex,
                        const nsIID& aIID,
                        void ** aResult)
{
    MutexAutoLock lock(mLock);
    nsISupports * obj = mArray.SafeObjectAt(aIndex);
        if (!obj) return NS_ERROR_ILLEGAL_VALUE;

    // no need to worry about a leak here, because SafeObjectAt() 
    // doesn't addref its result
    return obj->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
sbArray::IndexOf(PRUint32 aStartIndex, nsISupports* aElement,
                 PRUint32* aResult)
{
    MutexAutoLock lock(mLock);

    // optimize for the common case by forwarding to mArray
    if (aStartIndex == 0) {
        *aResult = mArray.IndexOf(aElement);
        if (*aResult == PR_UINT32_MAX)
            return NS_ERROR_FAILURE;
        return NS_OK;
    }

    findIndexOfClosure closure = { aElement, aStartIndex, 0 };
    bool notFound = mArray.EnumerateForwards(FindElementCallback, &closure);
    if (notFound)
        return NS_ERROR_FAILURE;

    *aResult = closure.resultIndex;
    return NS_OK;
}

NS_IMETHODIMP
sbArray::Enumerate(nsISimpleEnumerator **aResult)
{
    return NS_NewArrayEnumerator(aResult, static_cast<nsIArray*>(this));
}

// nsIMutableArray implementation

NS_IMETHODIMP
sbArray::AppendElement(nsISupports* aElement, bool aWeak)
{
    bool result;
    if (aWeak) {
        nsCOMPtr<nsISupports> elementRef =
            getter_AddRefs(static_cast<nsISupports*>
                                      (NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "AppendElement: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
        { /* scope */
            MutexAutoLock lock(mLock);
            result = mArray.AppendObject(elementRef);
        }
    }

    else {
        // add the object directly
        MutexAutoLock lock(mLock);
        result = mArray.AppendObject(aElement);
    }
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbArray::RemoveElementAt(PRUint32 aIndex)
{
    MutexAutoLock lock(mLock);
    bool result = mArray.RemoveObjectAt(aIndex);
    return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbArray::InsertElementAt(nsISupports* aElement, PRUint32 aIndex, bool aWeak)
{
    nsCOMPtr<nsISupports> elementRef;
    if (aWeak) {
        elementRef =
            getter_AddRefs(static_cast<nsISupports*>
                                      (NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "InsertElementAt: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
    } else {
        elementRef = aElement;
    }
    { /* scope */
        MutexAutoLock lock(mLock);
        bool result = mArray.InsertObjectAt(elementRef, aIndex);
        return result ? NS_OK : NS_ERROR_FAILURE;
    }
}

NS_IMETHODIMP
sbArray::ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex, bool aWeak)
{
    nsCOMPtr<nsISupports> elementRef;
    if (aWeak) {
        elementRef =
            getter_AddRefs(static_cast<nsISupports*>
                                      (NS_GetWeakReference(aElement)));
        NS_ASSERTION(elementRef, "ReplaceElementAt: Trying to use weak references on an object that doesn't support it");
        if (!elementRef)
            return NS_ERROR_FAILURE;
    } else {
        elementRef = aElement;
    }
    { /* scope */
        MutexAutoLock lock(mLock);
        bool result = mArray.ReplaceObjectAt(elementRef, aIndex);
        return result ? NS_OK : NS_ERROR_FAILURE;
    }
}

NS_IMETHODIMP
sbArray::Clear()
{
    MutexAutoLock lock(mLock);
    mArray.Clear();
    return NS_OK;
}

//
// static helper routines
//
bool
FindElementCallback(void *aElement, void* aClosure)
{
    findIndexOfClosure* closure =
        static_cast<findIndexOfClosure*>(aClosure);

    nsISupports* element =
        static_cast<nsISupports*>(aElement);
    
    // don't start searching until we're past the startIndex
    if (closure->resultIndex >= closure->startIndex &&
        element == closure->targetElement) {
        return PR_FALSE;    // stop! We found it
    }
    closure->resultIndex++;

    return PR_TRUE;
}
