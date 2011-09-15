/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef SBDOMEVENTLISTENERSCALLBACK_H__
#define SBDOMEVENTLISTENERSCALLBACK_H__

#include <nsIDOMEventListener.h>

/**
 * this class holds the base implementation for the base nsISupports
 * methods.
 */
class sbDOMEventListenerCallbackObjBase : public nsIDOMEventListener
{
public:

  NS_DECL_ISUPPORTS

protected:
  virtual ~sbDOMEventListenerCallbackObjBase() {}
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDOMEventListenerCallbackObjBase,
                              nsIDOMEventListener);

/**
 * This class is used to dispatch dom events to a C++ class method
 * To use this the template function sbDOMEventListenerCallback is the entry
 * point
 * usage:
 * nsCOMPtr<nsIDOMEventListener> listener =
 *   sbDOMEventListener(this, &class::method, arg);
 *
 * The method takes the following parameters:
 *   The nsIDOMEventListener pointer of the listener dispatching the event
 *   The context object passed to the sbDOMEventListenerCallback
 *   The nsIDOMEvent object of the DOM event
 */
template <class O, class F, class T>
class sbDOMEventListenerCallbackObj : public sbDOMEventListenerCallbackObjBase
{
public:
  NS_DECL_NSIDOMEVENTLISTENER

  sbDOMEventListenerCallbackObj(O * aObject, F aFunc, T aContext);
private:
  nsRefPtr<O> mObject;
  T mContext;
  F mFunction;
};

template <class O, class F, class T>
inline
sbDOMEventListenerCallbackObj<O, F, T>::sbDOMEventListenerCallbackObj(
                                                                   O * aObject,
                                                                   F aFunction,
                                                                   T aContext) :
  mObject(aObject),
  mContext(aContext),
  mFunction(aFunction)
{
}

template <class O, class F, class T>
NS_IMETHODIMP
sbDOMEventListenerCallbackObj<O, F, T>::HandleEvent(nsIDOMEvent *event)
{
  // Function variables.
  nsresult rv;

  // Dispatch to the on response function.
  rv = (mObject.get()->*mFunction)(static_cast<nsIDOMEventListener *>(this),
                                   mContext,
                                   event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * This function creates the dom event listener callback object that is then
 * passed to the dom target.
 */
template <class O, class F, class T>
inline
sbDOMEventListenerCallbackObj<O, F, T> * sbDOMEventListenerCallback(O * aObject,
                                                                    F aFunc,
                                                                    T aContext)
{
  return new sbDOMEventListenerCallbackObj<O, F, T>(aObject, aFunc, aContext);
}
#endif /* __SB_VARIANT_UTILS_H__ */

