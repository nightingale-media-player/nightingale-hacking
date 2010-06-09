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

#ifndef SBMEDIALISTENUMERATORWRAPPER_H_
#define SBMEDIALISTENUMERATORWRAPPER_H_

// Mozilla includes
#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsISimpleEnumerator.h>

// Songbird includes
#include <sbIMediaListEnumeratorWrapper.h>

/**
 * \brief The enumerator wrapper is used to set downloadStatusTarget
 *        when adding / inserting items from one medialist to another
 *        medialist or library.
 */
class sbMediaListEnumeratorWrapper: public sbIMediaListEnumeratorWrapper,
                                    public nsISimpleEnumerator,
                                    public nsIClassInfo
{
  NS_DECL_ISUPPORTS;
  NS_DECL_NSICLASSINFO;
  NS_DECL_NSISIMPLEENUMERATOR;
  NS_DECL_SBIMEDIALISTENUMERATORWRAPPER;

  sbMediaListEnumeratorWrapper();

private:
  virtual ~sbMediaListEnumeratorWrapper();

  PRMonitor* mMonitor;
  nsCOMPtr<nsISimpleEnumerator> mEnumerator;
};

#endif
