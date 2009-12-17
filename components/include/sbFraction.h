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

#ifndef SBFRACTION_H_
#define SBFRACTION_H_

#include <nsStringAPI.h>

/**
 * This is a simple class to represent fractions. Some media attributes are
 * best represented as fractions rather than floating point values which can
 * introduce rounding errors. Supports comparisons but no math operations
 */
class sbFraction
{
public:
  /**
   * Default constructor. Initializes to 0/1 or 0
   */
  sbFraction() : mNumerator(0),
                 mDenominator(1) {}

  /**
   * Initialize the fraction given a numerator and optional denominator. If
   * the denominator is not specified it defaults to one and allows this to
   * serve as a whole number constructor
   */
  sbFraction(PRUint32 aNumerator, PRUint32 aDenominator = 1) :
    mNumerator(aNumerator),
    mDenominator(aDenominator) {}

  /**
   * Initializes the fraction given a whole number, numerator, and denominator
   */
  sbFraction(PRUint32 aWholeNumber,
             PRUint32 aNumerator,
             PRUint32 aDenominator) :
               mNumerator(aNumerator + aDenominator * aWholeNumber),
               mDenominator(aDenominator) {}

  /**
   * Compares this fraction with another for equality
   *
   * \param aOther The other fraction to compare
   * \return true if the two fraction are equal
   */
  bool IsEqual(sbFraction const & aOther) const
  {
    return mNumerator == aOther.mNumerator &&
           mDenominator == aOther.mDenominator;
  }

  /**
   * Compares if this fraction is less than the "other"
   *
   * NOTE: This is not exact as it uses floating point
   * \param aOther The secondary fraction to compare
   * \return true if this fraction is less than aOther
   */
  bool IsLessThan(sbFraction const & aOther) const
  {
    return static_cast<double>(*this) < static_cast<double>(aOther);
  }

  /**
   * Returns a floating point representation of the fraction
   */
  operator double() const
  {
    return static_cast<double>(mNumerator) / static_cast<double>(mDenominator);
  }

  /**
   * Returns the numerator portion of the fraction given an improper fraction
   * representation
   */
  PRUint32 Numerator() const
  {
    return mNumerator;
  }

  /**
   * Returns the denominator portion of the fraction
   */
  PRUint32 Denominator() const
  {
    return mDenominator;
  }

  void GetProperFraction(PRUint32& aWhole,
                         PRUint32& aNumerator,
                         PRUint32& aDenominator) const
  {
    aWhole = mNumerator / mDenominator;
    aNumerator = mNumerator - (aWhole * aDenominator);
    aDenominator = mDenominator;
  }
private:
  PRUint32 mNumerator;
  PRUint32 mDenominator;
};

/**
 * equality operator
 */
inline
bool operator ==(sbFraction const & aLeft, sbFraction const & aRight)
{
  return aLeft.IsEqual(aRight);
}

/**
 * Inequality operator
 */
inline
bool operator !=(sbFraction const & aLeft, sbFraction const & aRight)
{
  return !aLeft.IsEqual(aRight);
}

/**
 * Less than comparison
 * NOTE: This is not exact as it uses floating point
 */
inline
bool operator <(sbFraction const & aLeft, sbFraction const & aRight)
{
  return aLeft.IsLessThan(aRight);
}

/**
 * Greater than comparison
 * NOTE: This is not exact as it uses floating point
 */
inline
bool operator >(sbFraction const & aLeft, sbFraction const & aRight)
{
  return aLeft != aRight && !aLeft.IsLessThan(aRight);
}

/**
 * Greater than or equal to comparison
 * NOTE: This is not exact as it uses floating point
 */
inline
bool operator >=(sbFraction const & aLeft, sbFraction const & aRight)
{
  return aLeft == aRight || aLeft > aRight;
}

/**
 * Less than or equal to comparison
 * NOTE: This is not exact as it uses floating point
 */
inline
bool operator <=(sbFraction const & aLeft, sbFraction const & aRight)
{
  return aLeft == aRight || aLeft < aRight;
}

/**
 * Converts a fraction to a string in proper fraction format ( 1 2/3)
 */
inline
nsString sbFractionToString(sbFraction const & aFraction)
{
  nsString result;

  result.AppendInt(aFraction.Numerator(), 10);
  if (aFraction.Denominator() > 1) {
    result.Append(NS_LITERAL_STRING("/"));
    result.AppendInt(aFraction.Denominator(), 10);
  }

  return result;
}

inline
nsresult sbFractionParsePart(nsAString const & aString,
                             PRInt32 aStart,
                             PRInt32 aLength,
                             PRUint32 * aValue)
{
  nsresult rv;

  nsDependentSubstring part(aString, aStart, aLength);
  PRInt32 temp = part.ToInteger(&rv, 10);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(temp >= 0, NS_ERROR_FAILURE);

  *aValue = temp;

  return NS_OK;
}

/**
 * Parses a string for a fraction
 */
inline
nsresult sbFractionFromString(nsAString const & aString,
                              sbFraction & aFraction)
{
  nsresult rv;

  PRUint32 whole = 0;
  PRUint32 numerator = 0;
  PRUint32 denominator = 1;

  PRInt32 const space = aString.Find(NS_LITERAL_STRING(" "));
  PRInt32 const slash = aString.Find(NS_LITERAL_STRING("/"));

  // Whole number
  if (space == -1) {
    numerator = aString.ToInteger(&rv, 10);
    aFraction = sbFraction(numerator);
    return NS_OK;
  }
  // Bad format, no slash
  if (slash == -1) {
    return NS_ERROR_FAILURE;
  }

  rv = sbFractionParsePart(aString, 0, space, &whole);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbFractionParsePart(aString, space + 1, slash - space - 1, &numerator);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbFractionParsePart(aString,
                           slash + 1,
                           aString.Length() - slash - 1,
                           &denominator);
  NS_ENSURE_SUCCESS(rv, rv);

  aFraction = sbFraction(whole, numerator, denominator);

  return NS_OK;
}
#endif /* SBFRACTION_H_ */
