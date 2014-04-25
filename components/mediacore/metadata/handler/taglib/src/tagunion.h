/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#ifndef TAGLIB_TAGUNION_H
#define TAGLIB_TAGUNION_H

#include "tag.h"

#ifndef DO_NOT_DOCUMENT

namespace TagLib {

  /*!
   * \internal
   */

  class TagUnion : public Tag
  {
  public:

    enum AccessType { Read, Write };

    /*!
     * Creates a TagLib::Tag that is the union of \a first, \a second, and
     * \a third.  The TagUnion takes ownership of these tags and will handle
     * their deletion.
     */
    TagUnion(Tag *first = 0, Tag *second = 0, Tag *third = 0);

    virtual ~TagUnion() {};

    Tag *operator[](int index) const;
    Tag *tag(int index) const;

    virtual Tag *tag(int index) = 0;

    void set(int index, Tag *tag);

    virtual String title() const = 0;
    virtual String artist() const = 0;
    virtual String album() const = 0;
    virtual String comment() const = 0;
    virtual String genre() const = 0;
    virtual uint year() const = 0;
    virtual uint track() const = 0;

    virtual void setTitle(const String &s) = 0;
    virtual void setArtist(const String &s) = 0;
    virtual void setAlbum(const String &s) = 0;
    virtual void setComment(const String &s) = 0;
    virtual void setGenre(const String &s) = 0;
    virtual void setYear(uint i) = 0;
    virtual void setTrack(uint i) = 0;
    virtual bool isEmpty() const = 0;

    template <class T> T *access(int index, bool create)
    {
      if(!create || tag(index))
        return static_cast<T *>(tag(index));

      set(index, new T);
      return static_cast<T *>(tag(index));
    }

  private:
    TagUnion(const Tag &);
    TagUnion &operator=(const Tag &);

    class TagUnionPrivate;
    TagUnionPrivate *d;
  };
}

#endif
#endif
