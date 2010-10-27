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

#include "sbErrorConsole.h"

// Mozilla includes
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>

// Mozilla interfaces
#include <nsIConsoleService.h>
#include <nsIScriptError.h>


NS_IMPL_THREADSAFE_ISUPPORTS0(sbErrorConsole)

void sbErrorConsole::Error(char const * aCategory,
                           nsAString const & aMessage,
                           nsAString const & aSource,
                           PRUint32 aLine)
{
  nsRefPtr<sbErrorConsole> errorConsole = new sbErrorConsole();
  if (errorConsole) {
    errorConsole->Log(nsDependentCString(aCategory),
                      nsIScriptError::errorFlag,
                      aMessage,
                      aSource,
                      aLine);
  }
}
void sbErrorConsole::Warning(char const * aCategory,
                             nsAString const & aMessage,
                             nsAString const & aSource,
                             PRUint32 aLine)
{
  nsRefPtr<sbErrorConsole> errorConsole = new sbErrorConsole();
  if (errorConsole) {
    errorConsole->Log(nsDependentCString(aCategory),
                      nsIScriptError::warningFlag,
                      aMessage,
                      aSource,
                      aLine);
  }
}

nsresult
sbErrorConsole::Log(nsACString const & aCategory,
                    PRUint32 aFlags,
                    nsAString const & aMessage,
                    nsAString const & aSource,
                    PRUint32 aLine)
{
  nsresult rv;

  ErrorParams params(aFlags,
                     aSource,
                     aLine,
                     aMessage,
                     aCategory);

  if (!NS_IsMainThread()) {
    // If this fails, not much we can do
    rv = sbInvokeOnMainThread1(*this,
                               &sbErrorConsole::LogThread,
                               NS_ERROR_FAILURE,
                               params);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = LogThread(params);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbErrorConsole::LogThread(ErrorParams aParameters)
{
  nsresult rv;

  nsCOMPtr<nsIConsoleService> consoleService =
    do_GetService("@mozilla.org/consoleservice;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<nsIScriptError> scriptError =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
  if (!scriptError) {
    return NS_ERROR_FAILURE;
  }

  rv = scriptError->Init(aParameters.mMessage.get(),
                         aParameters.mSource.get(),
                         nsString().get(), // no source line
                         aParameters.mLine,
                         0, // No column number
                         aParameters.mFlags,
                         aParameters.mCategory.BeginReading());
  NS_ENSURE_SUCCESS(rv,rv);

  rv = consoleService->LogMessage(scriptError);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}
