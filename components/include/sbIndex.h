/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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


#ifndef SBINDEX_H_
#define SBINDEX_H_

#include <vector>
/**
 * This class provides an index into a vector via iterators
 */
template <class KeyT, class IterT, class CompareT>
class sbIndex
{
public:
  typedef std::vector<IterT> Indexes;
  typedef typename Indexes::const_iterator const_iterator;
  typedef typename Indexes::iterator iterator;
  /**
   * Constructs and builds the index, but is not sorted
   */
  sbIndex(IterT aBegin,
          IterT aEnd) :
            mIndexes(aEnd - aBegin),
            mEnd(aEnd),
            mSorted(false)
  {
    while (aBegin != aEnd) {
      mIndexes.push_back(aBegin++);
    }
  }
  sbIndex() : mSorted(false) {}
  void Build(IterT aBegin,
             IterT aEnd)
  {
    mIndexes.reserve(aEnd - aBegin);
    while (aBegin != aEnd) {
      mIndexes.push_back(aBegin++);
    }
    sort();
  }
  void sort()
  {
    std::sort(mIndexes.begin(), mIndexes.end(), CompareT());
    mSorted = true;
  }
  const_iterator find(KeyT const & aData) const
  {
    NS_ASSERTION(mSorted || mIndexes.empty(),
                 "sbIndex::find called without a call to sort");
     const_iterator const iter = std::lower_bound(mIndexes.begin(),
                                                  mIndexes.end(),
                                                  aData,
                                                  mCompareIter);
    if (iter != mIndexes.end() && !mCompareIter(aData, *iter)) {
      return iter;
    }
    return mIndexes.end();
  }
  iterator find(KeyT const & aData)
  {
    NS_ASSERTION(mSorted || mIndexes.empty(),
                 "sbIndex::find called without a call to sort");
    iterator const iter = std::lower_bound(mIndexes.begin(),
                                           mIndexes.end(),
                                           aData,
                                           mCompareIter);
    if (iter != mIndexes.end() && !mCompareIter(aData, *iter)) {
      return iter;
    }
    return mIndexes.end();
  }
  const_iterator begin() const
  {
    return mIndexes.begin();
  }
  const_iterator end() const
  {
    return mIndexes.end();
  }
  iterator begin()
  {
    return mIndexes.begin();
  }
  iterator end()
  {
    return mIndexes.end();
  }
private:
  Indexes mIndexes;
  IterT mEnd;
  bool mSorted;
  CompareT mCompareIter;
};

#endif /* SBINDEXEDVECTOR_H_ */
