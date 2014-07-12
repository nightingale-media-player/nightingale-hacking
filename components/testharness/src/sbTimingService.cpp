/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbTimingService.h"
#include "sbTestHarnessCID.h"

#include <nsICategoryManager.h>
#include <nsIDateTimeFormat.h>
#include <nsILocale.h>
#include <nsILocaleService.h>
#include <nsIObserverService.h>

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsDateTimeFormatCID.h>
#include <nsILocalFile.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>

#include <mozilla/Mutex.h>

#include <sbStringUtils.h>

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_APPSTARTUP_TOPIC              "app-startup"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTimingServiceTimer, 
                              sbITimingServiceTimer)

sbTimingServiceTimer::sbTimingServiceTimer() 
: mTimerLock("sbTimingServiceTimer::mTimerLock")
, mTimerStartTime(0)
, mTimerStopTime(0)
, mTimerTotalTime(0)
{
  MOZ_COUNT_CTOR(sbTimingService);
}

sbTimingServiceTimer::~sbTimingServiceTimer()
{
  MOZ_COUNT_DTOR(sbTimingService);
}

nsresult 
sbTimingServiceTimer::Init(const nsAString &aTimerName)
{
  mozilla::MutexAutoLock lock(mTimerLock);
  mTimerName = aTimerName;

  mTimerStartTime = PR_Now();

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetName(nsAString & aName)
{
  mozilla::MutexAutoLock lock(mTimerLock);
  aName = mTimerName;
  
  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetStartTime(PRInt64 *aStartTime)
{
  NS_ENSURE_ARG_POINTER(aStartTime);

  mozilla::MutexAutoLock lock(mTimerLock);
  *aStartTime = mTimerStartTime;

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetStopTime(PRInt64 *aStopTime)
{
  NS_ENSURE_ARG_POINTER(aStopTime);

  mozilla::MutexAutoLock lock(mTimerLock);
  *aStopTime = mTimerStopTime;

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetTotalTime(PRInt64 *aTotalTime)
{
  NS_ENSURE_ARG_POINTER(aTotalTime);

  mozilla::MutexAutoLock lock(mTimerLock);
  *aTotalTime = mTimerTotalTime;

  return NS_OK;
}


NS_IMPL_THREADSAFE_ISUPPORTS1(sbTimingService, 
                              sbITimingService)

sbTimingService::sbTimingService()
: mLoggingLock("sbTimingService::mLoggingLock")
, mLoggingEnabled(PR_TRUE)
, mTimersLock("sbTimingService::mTimersLock")
, mResultsLock("sbTimingService::mResultsLock")
{
  MOZ_COUNT_CTOR(sbTimingService);
}

sbTimingService::~sbTimingService()
{
  MOZ_COUNT_DTOR(sbTimingService);

  mTimers.Clear();
  mResults.Clear();
}

static nsresult 
CheckEnvironmentVariable(nsIFile ** aFile)
{
  nsresult rv;

  char const * const fileName = getenv("SB_TIMING_SERVICE_LOG");
  if (!fileName)
    return NS_OK;
  
  nsCOMPtr<nsILocalFile> file =
      do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = file->InitWithNativePath(nsDependentCString(fileName));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(file, aFile);
}

NS_METHOD
sbTimingService::Init()
{
  PRBool success = mTimers.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mResults.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CheckEnvironmentVariable(getter_AddRefs(mLogFile));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "CheckEnvironmentVariable failed");

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::GetEnabled(PRBool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);

  mozilla::MutexAutoLock lock(mLoggingLock);
  *aEnabled = mLoggingEnabled;

  return NS_OK;
}
NS_IMETHODIMP 
sbTimingService::SetEnabled(PRBool aEnabled)
{
  mozilla::MutexAutoLock lock(mLoggingLock);
  mLoggingEnabled = aEnabled;

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::GetLogFile(nsIFile * *aLogFile)
{
  NS_ENSURE_ARG_POINTER(aLogFile);

  mozilla::MutexAutoLock lock(mLoggingLock);
  NS_ADDREF(*aLogFile = mLogFile);

  return NS_OK;
}
NS_IMETHODIMP 
sbTimingService::SetLogFile(nsIFile * aLogFile)
{
  NS_ENSURE_ARG_POINTER(aLogFile);

  mozilla::MutexAutoLock lock(mLoggingLock);
  mLogFile = aLogFile;

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::StartPerfTimer(const nsAString & aTimerName)
{
  nsRefPtr<sbTimingServiceTimer> timer = new sbTimingServiceTimer();
  NS_ENSURE_TRUE(timer, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = timer->Init(aTimerName);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MutexAutoLock lockTimers(mTimersLock);

  if(mTimers.Get(aTimerName, nsnull)) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  PRBool success = mTimers.Put(aTimerName, timer);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::StopPerfTimer(const nsAString & aTimerName, PRInt64 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRTime stopTime = PR_Now();
  nsCOMPtr<sbITimingServiceTimer> timer;

  {
    mozilla::MutexAutoLock lockTimers(mTimersLock);

    if(!mTimers.Get(aTimerName, getter_AddRefs(timer))) {
      return NS_ERROR_NOT_INITIALIZED;
    }

    mTimers.Remove(aTimerName);
  }

  sbTimingServiceTimer *rawTimer = 
    reinterpret_cast<sbTimingServiceTimer *>(timer.get());

  rawTimer->mTimerStopTime = stopTime;
  rawTimer->mTimerTotalTime = stopTime - rawTimer->mTimerStartTime;
  *_retval = rawTimer->mTimerTotalTime;

  {
    mozilla::MutexAutoLock lockResults(mResultsLock);
    PRUint32 resultCount = mResults.Count();
    
    PRBool success = mResults.Put(resultCount, timer);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbTimingService::Observe(nsISupports* aSubject,
                         const char* aTopic,
                         const PRUnichar* aData)
{
  nsresult rv;

  mozilla::MutexAutoLock lock(mLoggingLock);

  if(strcmp(aTopic, "profile-before-change") == 0) {

     if(mLoggingEnabled) {
      // Logging output is enabled; format and output to the console.
      nsCString output;
      rv = FormatResultsToString(output);
      NS_ENSURE_SUCCESS(rv, rv);

      printf("%s", output.BeginReading());

      // If we also have a log file, output to the logfile.
      if(mLogFile) {
        nsCOMPtr<nsIOutputStream> outputStream;
        rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                         mLogFile,
                                         PR_APPEND | PR_CREATE_FILE | PR_WRONLY);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 bytesOut = 0;
        rv = outputStream->Write(output.BeginReading(), 
                                 output.Length(), 
                                 &bytesOut);
        
        // Close it off regardless of the error
        nsresult rvclose = outputStream->Close();
        
        // Handle write errors before the close error
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(bytesOut == output.Length(), NS_ERROR_UNEXPECTED);
        
        // Now handle any error from close
        NS_ENSURE_SUCCESS(rvclose, rvclose);
      }
    }
  }

  return NS_OK;
}

nsresult sbTimingService::FormatResultsToString(nsACString &aOutput)
{
  mozilla::MutexAutoLock lockResults(mResultsLock);
  PRUint32 resultCount = mResults.Count();

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsILocaleService> localeService =
    do_GetService("@mozilla.org/intl/nslocaleservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocale> locale;
  rv = localeService->GetApplicationLocale(getter_AddRefs(locale));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDateTimeFormat> dateTimeFormat = 
    do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv);

  PRTime startTime = 0;
  PRTime stopTime = 0;
  PRTime totalTime = 0;

  nsCOMPtr<sbITimingServiceTimer> timer;

  nsString out, output, timerName;

  output.AppendLiteral("\n\n\t\tsbTimingService Results\n\n");

  for(PRUint32 current = 0; current < resultCount; ++current) {
    PRBool success = mResults.Get(current, getter_AddRefs(timer));
    NS_ENSURE_TRUE(success, NS_ERROR_NOT_AVAILABLE);

    rv = timer->GetStartTime(&startTime);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = timer->GetStopTime(&stopTime);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = timer->GetTotalTime(&totalTime);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = timer->GetName(timerName);
    NS_ENSURE_SUCCESS(rv, rv);

    output.Append(timerName);
    output.AppendLiteral("\t");

    PRExplodedTime explodedTime = {0};
    PR_ExplodeTime(startTime, PR_LocalTimeParameters, &explodedTime);

    rv = dateTimeFormat->FormatPRExplodedTime(locale,
                                              kDateFormatNone,
                                              kTimeFormatSeconds,
                                              &explodedTime,
                                              out);
    NS_ENSURE_SUCCESS(rv, rv);

    output.Append(out);
    output.AppendLiteral(" ");
    output.AppendInt(static_cast<PRInt32>((startTime % PR_USEC_PER_SEC) / PR_USEC_PER_MSEC ));
    output.AppendLiteral("ms\t");

    PR_ExplodeTime(stopTime, PR_LocalTimeParameters, &explodedTime);

    rv = dateTimeFormat->FormatPRExplodedTime(locale,
                                              kDateFormatNone,
                                              kTimeFormatSeconds,
                                              &explodedTime,
                                              out);
    NS_ENSURE_SUCCESS(rv, rv);

    output.Append(out);
    output.AppendLiteral(" ");
    output.AppendInt(static_cast<PRInt32>((stopTime % PR_USEC_PER_SEC) / PR_USEC_PER_MSEC ));
    output.AppendLiteral("ms\t");

    AppendInt(output, totalTime / PR_USEC_PER_MSEC);
    output.AppendLiteral("ms\n");
  }

  output.AppendLiteral("\n\n");
  aOutput.Assign(NS_ConvertUTF16toUTF8(output));

  return NS_OK;
}
