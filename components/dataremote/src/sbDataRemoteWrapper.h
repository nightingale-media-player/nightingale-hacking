/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef nsDataRemoteWrapper_h__
#define nsDataRemoteWrapper_h__

#include <nsIClassInfo.h>
#include <nsIObserver.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <sbIDataRemote.h>
#include <sbPIDataRemote2.h>
#include <nsComponentManagerUtils.h>


#define SB_DATAREMOTEWRAPPER_CLASSNAME \
  "Songbird Data Remote Wrapper Instance"

// {1cb891b0-e9c5-11dd-ba2f-0800200c9a66}
#define SB_DATAREMOTEWRAPPER_CID \
{ 0x1cb891b0, 0xe9c5, 0x11dd, \
  { 0xba, 0x2f, 0xd1, 0x08, 0x02, 0x00, 0xc9, 0x66 } }

#define SB_DATAREMOTEWRAPPER_CONTRACTID \
  "@songbirdnest.com/Songbird/DataRemote;1"

#define NS_FORWARD_SOME_SBIDATAREMOTE_METHODS(_to) \
  NS_SCRIPTABLE NS_IMETHOD Init(const nsAString & aKey, const nsAString & aRoot) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(aKey, aRoot); } \
  NS_SCRIPTABLE NS_IMETHOD BindProperty(nsIDOMElement *aElement, const nsAString & aProperty, bool aIsBool, bool aIsNot, const nsAString & aEvalString) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindProperty(aElement, aProperty, aIsBool, aIsNot, aEvalString); } \
  NS_SCRIPTABLE NS_IMETHOD BindAttribute(nsIDOMElement *aElement, const nsAString & aProperty, bool aIsBool, bool aIsNot, const nsAString & aEvalString) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindAttribute(aElement, aProperty, aIsBool, aIsNot, aEvalString); } \
  NS_SCRIPTABLE NS_IMETHOD DeleteBranch() { return !_to ? NS_ERROR_NULL_POINTER : _to->DeleteBranch(); }


/******************************************************************************
 * \class sbDataRemoteWrapper
 * \brief An adaptor for DataRemote that allows getters/setters to be used
 *        from an unprivileged web page context.  This is a hack.
 *
 * The DataRemote ContractID "@songbirdnest.com/Songbird/DataRemote;1"
 * now maps to this wrapper class, which delegates to the original 
 * implementation via the new sbPIDataRemote2 interface.  This bypasses
 * Mozilla Bug 304048, which prevents our Remote Web Page API from accessing
 * properties on a JavaScript XPCOM component.  This is a hack, but
 * it is the safest, simplest way to improve performance until the
 * DataRemote system is completely ripped out.
 *
 * \sa sbIDataRemote.idl
 *     https://bugzilla.mozilla.org/show_bug.cgi?id=304048 
 *          xpconnect getters/setters don't have principals until after they 
 *          pass or fail their security check
 *     http://bugzilla.songbirdnest.com/show_bug.cgi?id=8703 
 *         "There is no data, there are only prefs"
 *     http://bugzilla.songbirdnest.com/show_bug.cgi?id=10806
 *         "Memory leak during playback"
 * 
 *****************************************************************************/
class sbDataRemoteWrapper : public sbIDataRemote,
                            public nsIClassInfo
{
public:
    sbDataRemoteWrapper();
    nsresult InitWrapper();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICLASSINFO
    NS_DECL_NSIOBSERVER
    NS_FORWARD_SOME_SBIDATAREMOTE_METHODS(mInnerDataRemote)

    NS_SCRIPTABLE NS_IMETHOD Unbind(void);
    NS_SCRIPTABLE NS_IMETHOD BindObserver(nsIObserver *aObserver, bool aSuppressFirst);
    NS_SCRIPTABLE NS_IMETHOD BindRemoteObserver(sbIRemoteObserver *aObserver, bool aSuppressFirst); 
    NS_SCRIPTABLE NS_IMETHOD GetStringValue(nsAString & aStringValue); 
    NS_SCRIPTABLE NS_IMETHOD SetStringValue(const nsAString & aStringValue); 
    NS_SCRIPTABLE NS_IMETHOD GetBoolValue(bool *aBoolValue); 
    NS_SCRIPTABLE NS_IMETHOD SetBoolValue(bool aBoolValue); 
    NS_SCRIPTABLE NS_IMETHOD GetIntValue(PRInt64 *aIntValue); 
    NS_SCRIPTABLE NS_IMETHOD SetIntValue(PRInt64 aIntValue);

private:
    ~sbDataRemoteWrapper();
    nsCOMPtr<sbPIDataRemote2> mInnerDataRemote;
    nsCOMPtr<nsIObserver> mObserver;
};

#endif
