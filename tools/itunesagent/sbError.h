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

#ifndef SBERROR_H_
#define SBERROR_H_

#include <assert.h>
#include <string>


/**
 * Simple error class returned from API's
 */
class sbError
{
public:
  /**
   * Initialize the error object with a message and the processor
   */
  sbError(std::string const & aMessage = std::string()) :
    mMessage(aMessage),
    mChecked(false) {
  }

  /**
   * Copies the error, marking the source as checked 
   */
  sbError(sbError const & aError) : mMessage(aError.mMessage),
                                    mChecked(aError.mChecked) {
    aError.mChecked = true;
  }

  /**
   * Assigns an error, marking the source as checked
   */
  sbError & operator =(sbError const & aError) {
    mMessage = aError.mMessage;
    mChecked = aError.mChecked;
    aError.mChecked = true;
    
    return *this;
  }

  /**
   * Used to create the sbNoError instance
   */
  sbError(bool aChecked) : mChecked(aChecked) {}
  
  /**
   * Detect unhandled errors
   */
  ~sbError() {
    assert(mChecked);
  }
  
  /**
   * Returns true if an error occurred
   */
  operator bool() const {
    mChecked = true;
    return !mMessage.empty();
  }
  
  /**
   * Returns the message associated with the error
   */
  std::string const & Message() const {
    mChecked = true;
    return mMessage;
  }

private:
  std::string mMessage;
  mutable bool mChecked;
};


/**
 * Used for returns when there is no error
 */
extern sbError const sbNoError;

#endif /* SBERROR_H_ */

