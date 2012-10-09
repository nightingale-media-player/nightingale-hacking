/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef __SB_WEAKREFERENCE_H__
#define __SB_WEAKREFERENCE_H__

#include <nsIWeakReference.h>
#include <nsIWeakReferenceUtils.h>
#include <mozilla/Mutex.h>

class sbWeakReference;

// Set IMETHOD_VISIBILITY to empty so that the class-level NS_COM declaration
// controls member method visibility.
#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_COM_GLUE

class NS_COM_GLUE sbSupportsWeakReference : public nsISupportsWeakReference
{
public:
  sbSupportsWeakReference() 
    : mProxy(nsnull)
    , mProxyLock("sbSupportsWeakReference::mProxyLock") 
  {}

  NS_DECL_NSISUPPORTSWEAKREFERENCE

protected:
  inline ~sbSupportsWeakReference();

private:
  friend class sbWeakReference;

  void NoticeProxyDestruction() {
    mozilla::Mutex lock(mProxyLock);
    // ...called (only) by an |nsWeakReference| from _its_ dtor.
    mProxy = nsnull;
  }

  sbWeakReference* mProxy;
  // Lock to protect mProxy.
  mozilla::Mutex mProxyLock;

protected:
  void ClearWeakReferences();
  PRBool HasWeakReferences() const {
    mozilla::Mutex lock(mProxyLock);

    return mProxy != 0; 
  }
};

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN

inline
sbSupportsWeakReference::~sbSupportsWeakReference() {
  ClearWeakReferences();
  
  if (mProxyLock) {
    delete mProxyLock;
  }
  mProxyLock = NULL;
}

#endif // __SB_WEAKREFERENCE_H__
