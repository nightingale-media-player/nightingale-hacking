/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef SBCALLBACK_H_
#define SBCALLBACK_H_

#include <assert.h>

/**
 * Base class for the callback. This is the callback class that you
 * will use to invoke the callback from.
 */

template <class Result,class Message>
class sbCallbackBase
{
public:
  virtual
  sbCallbackBase<Result,Message>* Clone(void) const = 0;
  virtual
  Result Send(Message i_Message) const = 0;
  Result operator()(Message i_Message) const { return Send(i_Message); }
};

/**
 * Class used to create the callback. This is then passed as a sbCallbackBase.
 * Receiver is the class that will receive the callback, result is the return 
 * type of the message. Message is the message passed to the callback.
 */
template <class Receiver,class Result,class Message>
class sbCallback : public sbCallbackBase<Result,Message>
{
public:
  typedef Result (Receiver::*ReceiverFunction)(Message);

  sbCallback(Receiver* const i_pReceiver = 0,
             ReceiverFunction i_ReceiverFunction = 0);
  virtual
  sbCallbackBase<Result,Message>* Clone(void) const;
  virtual
  Result Send(Message i_Message) const;
private:
  Receiver*     m_pReceiver;
  ReceiverFunction  m_ReceiverFunction;
};

/**
 * Initializes the callback from a pointer to the class and it's method
 */
template <class Receiver,class Result,class Message>
inline
sbCallback<Receiver,Result,Message>::sbCallback(
  Receiver* const i_pReceiver,
  ReceiverFunction i_ReceiverFunction):
    m_pReceiver(i_pReceiver),
    m_ReceiverFunction(i_ReceiverFunction) {
}

/**
 * Creates a copy of self, caller own's pointer
 */

template <class Receiver, class Result, class Message>
inline
sbCallbackBase<Result,Message>* 
sbCallback<Receiver,Result,Message>::Clone(void) const {
  return new sbCallback<Receiver,Result,Message>(
    m_pReceiver,
    m_ReceiverFunction);
}

/**
 * Invokes the callback with the given message
 */

template <class Receiver,class Result,class Message>
Result sbCallback<Receiver,Result,Message>::Send(
  Message i_Message) const {
  assert(m_pReceiver != 0 && m_ReceiverFunction != 0);
  if (m_pReceiver != 0 && m_ReceiverFunction != 0) {
    return (m_pReceiver->*m_ReceiverFunction)(i_Message);
  }
  else {
    return Result();
  }
}

#endif /* SBCALLBACK_H_ */
