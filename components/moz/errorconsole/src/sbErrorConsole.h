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

#ifndef SBERRORCONSOLE_H_
#define SBERRORCONSOLE_H_

#include <nsStringAPI.h>

// Mozilla interfaces
#include <nsIScriptError.h>

// Mozilla includes
#include <nsComponentManagerUtils.h>

// Songbird includes
#include <sbThreadUtils.h>

/**
 * Class that helps with the use of the error console service. This may be
 * called from any thread, main thread code handled internally
 * Usage: sbErrorConsole::Error("Category", "Message", "Source", 123);
 *        sbErrorConsole::Warning("Category", "Message", "Source", 123);
 */
class sbErrorConsole : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  /**
   * Parameters used for the thread dispatch function LogThread
   */
  struct ErrorParams
  {
    ErrorParams(PRUint32 aFlags,
                nsAString const & aSource,
                PRUint32 aLine,
                nsAString const & aMessage,
                nsACString const & aCategory) : mFlags(aFlags),
                                                mSource(aSource),
                                                mLine(aLine),
                                                mMessage(aMessage),
                                                mCategory(aCategory) {}
    PRUint32 mFlags;
    nsString mSource;
    PRUint32 mLine;
    nsString mMessage;
    nsCString mCategory;
  };
  static void Error(char const * aCategory,
                    nsAString const & aMessage,
                    nsAString const & aSource = nsString(),
                    PRUint32 aLine = 0);
  static void Warning(char const * aCategory,
                      nsAString const & aMessage,
                      nsAString const & aSource = nsString(),
                      PRUint32 aLine = 0);
  static void Message(char const * aFmt, ...);
private:
  enum { infoMessageFlag = ~0u };
  nsresult Log(nsACString const & aCategory,
               PRUint32 aFlags,
               nsAString const & aMessage,
               nsAString const & aSource,
               PRUint32 aLine);
  nsresult LogThread(ErrorParams aParameters);
};

#endif
