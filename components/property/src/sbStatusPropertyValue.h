/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef SBSTATUSPROPERTYVALUE_H_
#define SBSTATUSPROPERTYVALUE_H_

// Mozilla includes
#include <nsStringAPI.h>

// Nightingale includes
#include <sbStringUtils.h>


/**
 * Helper class to work with the status property
 */
class sbStatusPropertyValue
{
public:
  enum Mode {
    eNone        = 0,
    eRipping     = 1,
    eComplete    = 2,
    eFailed      = 3
  };
  /**
   * Default initializer
   */
  sbStatusPropertyValue() :
    mMode(eNone),
    mCurrent(0)
  {
  }
  /**
   * Initializes the value from a string
   */
  sbStatusPropertyValue(const nsAString& aValue) :
    mMode(eNone),
    mCurrent(0)
  {
    PRInt32 const pipe  = aValue.FindChar('|');

    if (pipe >= 1) {
      nsDependentSubstring const & modeString = Substring(aValue, 0, pipe);
      nsresult rv;
      PRInt32 const mode = modeString.ToInteger(&rv);
      NS_ENSURE_SUCCESS(rv, /* void */);

      switch (mode) {
        case 0:
          mMode = eNone;
          break;
        case 1:
          mMode = eRipping;
          break;
        case 2:
          mMode = eComplete;
          break;
        case 3:
          mMode = eFailed;
          break;
        default:
          mMode = eNone;
          NS_WARNING("Invalid mode value for status property");
          break;
      }

      PRInt32 const currentValue = Substring(aValue, pipe + 1).ToInteger(&rv);
      if (NS_SUCCEEDED(rv)) {
        mCurrent = currentValue;
      }
    }
  }
  /**
   * Returns the mode of the value
   */
  Mode GetMode() const
  {
    return mMode;
  }
  /**
   * Sets the mode for the value
   */
  void SetMode(Mode aMode)
  {
    mMode = aMode;
  }
  /**
   * Returns the current progress value 0 - 100
   */
  PRUint32 GetCurrent() const
  {
    return mCurrent;
  }
  /**
   * Sets the current progress value 0 - 100
   */
  void SetCurrent(PRUint32 aCurrent)
  {
    mCurrent      = aCurrent;
  }
  /**
   * Returns the property as a string
   */
  nsString GetValue() const
  {
    nsString value;
    value.AppendInt(GetMode());
    switch (GetMode()) {
      case eComplete:
      case eFailed:
        value.AppendLiteral("|100");
        break;
      default:
        value.AppendLiteral("|");
        AppendInt(value, GetCurrent());
        break;
    }
    return value;
  }

private:
  Mode mMode;
  PRUint32 mCurrent;
};


#endif /* SBSTATUSPROPERTYVALUE_H_ */
