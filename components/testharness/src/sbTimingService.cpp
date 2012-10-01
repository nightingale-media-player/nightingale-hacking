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

#include <sbStringUtils.h>

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_APPSTARTUP_TOPIC              "app-startup"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTimingServiceTimer, 
                              sbITimingServiceTimer)

sbTimingServiceTimer::sbTimingServiceTimer() 
: mTimerLock(nsnull)
, mTimerStartTime(0)
, mTimerStopTime(0)
, mTimerTotalTime(0)
{
  MOZ_COUNT_CTOR(sbTimingService);
}

sbTimingServiceTimer::~sbTimingServiceTimer()
{
  MOZ_COUNT_DTOR(sbTimingService);

  if(mTimerLock) {
    nsAutoLock::DestroyLock(mTimerLock);
  }
}

nsresult 
sbTimingServiceTimer::Init(const nsAString &aTimerName)
{
  mTimerLock = nsAutoLock::NewLock("sbTimingServiceTimer::mTimerLock");
  NS_ENSURE_TRUE(mTimerLock, NS_ERROR_OUT_OF_MEMORY);

  nsAutoLock lock(mTimerLock);
  mTimerName = aTimerName;

  mTimerStartTime = PR_Now();

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetName(nsAString & aName)
{
  nsAutoLock lock(mTimerLock);
  aName = mTimerName;
  
  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetStartTime(PRInt64 *aStartTime)
{
  NS_ENSURE_ARG_POINTER(aStartTime);

  nsAutoLock lock(mTimerLock);
  *aStartTime = mTimerStartTime;

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetStopTime(PRInt64 *aStopTime)
{
  NS_ENSURE_ARG_POINTER(aStopTime);

  nsAutoLock lock(mTimerLock);
  *aStopTime = mTimerStopTime;

  return NS_OK;
}

NS_IMETHODIMP sbTimingServiceTimer::GetTotalTime(PRInt64 *aTotalTime)
{
  NS_ENSURE_ARG_POINTER(aTotalTime);

  nsAutoLock lock(mTimerLock);
  *aTotalTime = mTimerTotalTime;

  return NS_OK;
}


NS_IMPL_THREADSAFE_ISUPPORTS1(sbTimingService, 
                              sbITimingService)

sbTimingService::sbTimingService()
: mLoggingLock(nsnull)
, mLoggingEnabled(PR_TRUE)
, mTimersLock(nsnull)
, mResultsLock(nsnull)
{
  MOZ_COUNT_CTOR(sbTimingService);
}

sbTimingService::~sbTimingService()
{
  MOZ_COUNT_DTOR(sbTimingService);

  if(mLoggingLock) {
    nsAutoLock::DestroyLock(mLoggingLock);
  }
  if(mTimersLock) {
    nsAutoLock::DestroyLock(mTimersLock);
  }
  if(mResultsLock) {
    nsAutoLock::DestroyLock(mResultsLock);
  }

  mTimers.Clear();
  mResults.Clear();
}

/*static*/ NS_METHOD 
sbTimingService::RegisterSelf(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* aLoaderStr,
                              const char* aType,
                              const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                         SB_TIMINGSERVICE_DESCRIPTION,
                                         "service,"
                                         SB_TIMINGSERVICE_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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
sbTimingService::Init() {
  mLoggingLock = nsAutoLock::NewLock("sbTimingService::mLoggingLock");
  NS_ENSURE_TRUE(mLoggingLock, NS_ERROR_OUT_OF_MEMORY);

  mTimersLock = nsAutoLock::NewLock("sbTimingService::mTimersLock");
  NS_ENSURE_TRUE(mTimersLock, NS_ERROR_OUT_OF_MEMORY);

  mResultsLock = nsAutoLock::NewLock("sbTimingService::mResultsLock");
  NS_ENSURE_TRUE(mResultsLock, NS_ERROR_OUT_OF_MEMORY);

  bool success = mTimers.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mResults.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CheckEnvironmentVariable(getter_AddRefs(mLogFile));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "CheckEnvironmentVariable failed");  

  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "profile-before-change", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::GetEnabled(bool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);

  nsAutoLock lock(mLoggingLock);
  *aEnabled = mLoggingEnabled;

  return NS_OK;
}
NS_IMETHODIMP 
sbTimingService::SetEnabled(bool aEnabled)
{
  nsAutoLock lock(mLoggingLock);
  mLoggingEnabled = aEnabled;

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::GetLogFile(nsIFile * *aLogFile)
{
  NS_ENSURE_ARG_POINTER(aLogFile);

  nsAutoLock lock(mLoggingLock);
  NS_ADDREF(*aLogFile = mLogFile);

  return NS_OK;
}
NS_IMETHODIMP 
sbTimingService::SetLogFile(nsIFile * aLogFile)
{
  NS_ENSURE_ARG_POINTER(aLogFile);

  nsAutoLock lock(mLoggingLock);
  mLogFile = aLogFile;

  return NS_OK;
}

NS_IMETHODIMP 
sbTimingService::StartPerfTimer(const nsAString & aTimerName)
{
  nsRefPtr<sbTimingServiceTimer> timer;
  NS_NEWXPCOM(timer, sbTimingServiceTimer);
  NS_ENSURE_TRUE(timer, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = timer->Init(aTimerName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lockTimers(mTimersLock);

  if(mTimers.Get(aTimerName, nsnull)) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  bool success = mTimers.Put(aTimerName, timer);
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
    nsAutoLock lockTimers(mTimersLock);

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
    nsAutoLock lockResults(mResultsLock);
    PRUint32 resultCount = mResults.Count();
    
    bool success = mResults.Put(resultCount, timer);
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
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

  nsAutoLock lock(mLoggingLock);

  if(strcmp(aTopic, "profile-before-change") == 0) {
    
    nsCOMPtr<nsIObserverService> observerService = 
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "profile-before-change");
    NS_ENSURE_SUCCESS(rv, rv);

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
  nsAutoLock lockResults(mResultsLock);
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
    bool success = mResults.Get(current, getter_AddRefs(timer));
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
