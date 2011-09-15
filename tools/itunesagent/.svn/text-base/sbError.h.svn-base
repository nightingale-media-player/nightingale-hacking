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
  explicit sbError(std::string const & aMessage = std::string()) :
    mMessage(aMessage),
    mChecked(false) {
  }
  explicit sbError(char const * aMessage) :
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
  explicit sbError(bool aChecked) : mChecked(aChecked) {}

  /**
   * Used to create misc. sbError instances.
   */
  explicit sbError(std::string const & aMessage, bool aChecked) :
    mMessage(aMessage) ,
    mChecked(aChecked) {
  }
  explicit sbError(const char *aMessage, bool aChecked) :
    mMessage(aMessage) ,
    mChecked(aChecked) {
  }
  
  /**
   * Detect unhandled errors
   */
  ~sbError() {
    if (!mChecked) {
      printf("\n UNCHECKED MESSAGE: %s\n", mMessage.c_str());
    }
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
   * Compare two error instances by their messages.
   */
  bool operator == (sbError const & aError) const {
    mChecked = true;
    return mMessage == aError.mMessage;
  }

  /**
   * Ensure inequality between two errors by their messages.
   */
  bool operator != (sbError const & aError) const {
    mChecked = true;
    return !operator == (aError);
  }
  
  /**
   * Mark it checked
   */
  void Checked() {
    mChecked = true;
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

