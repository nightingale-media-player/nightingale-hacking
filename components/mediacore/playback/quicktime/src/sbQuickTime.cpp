#include "sbQuickTime.h"
#include "sbQuickTimeUtils.h"

// Mozilla includes
#include <prlog.h>
#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsIBoxObject.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMXULElement.h>
#include <nsITimer.h>
#include <nsIWidget.h>
#include <nsRect.h>
#include <nsString.h>

// QuickTime includes
#include <Gestalt.h>
#include <Movies.h>

#ifdef PR_LOGGING
static PRLogModuleInfo* gQuickTimeLog = nsnull;
#define LOG(args)   PR_LOG(gQuickTimeLog, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gQuickTimeLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif
  
#define TIMER_INTERVAL 100

// QuickTime should only be initialized once per application run (*not*
// once for every component).
PRBool sbQuickTime::mInitialized = PR_FALSE;

NS_IMPL_ISUPPORTS3(sbQuickTime, sbIQuickTime,
                                nsIDOMEventListener,
                                nsIObserver)

sbQuickTime::sbQuickTime() :
  mController(nsnull),
  mMovie(nsnull),
  mControl(nsnull),
  mOffscreenGWorld(nsnull),
  mWindow(nsnull)
{
#ifdef PR_LOGGING
  if (!gQuickTimeLog)
    gQuickTimeLog = PR_NewLogModule("sbQuickTime");
#endif
  TRACE(("Constructor"));
  mStatus = qtStatusStopped;
}

sbQuickTime::~sbQuickTime()
{
  TRACE(("Destructor"));
  SetTargetElement(nsnull);
  
  if (mControl)
    DisposeControl(mControl);
  
  if (mMovie)
    DisposeMovie(mMovie);

  if (mOffscreenGWorld)
    DisposeGWorld(mOffscreenGWorld);
}

NS_IMETHODIMP
sbQuickTime::Initialize()
{
  TRACE(("Initialize"));
  PRBool minimumQuickTimePresent = IsMinimumQuickTimeVersionPresent();
  NS_ENSURE_TRUE(minimumQuickTimePresent, NS_ERROR_FAILURE);
  
  nsresult rv = InitializeQuickTime();
  NS_ENSURE_SUCCESS(rv, rv);
  
  Movie movie;
  rv = SB_CreateEmptyMovie(&movie);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return SetMovie(movie);
}

/**
 * Helper methods
 */

PRBool
sbQuickTime::IsMinimumQuickTimeVersionPresent()
{
  TRACE(("IsMinimumQuickTimeVersionPresent"));
  PRInt32 version;
  nsresult rv = GetQuickTimeVersion(&version);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  // We don't care about a min version at the moment.
  return PR_TRUE;
}

NS_IMETHODIMP
sbQuickTime::InitializeQuickTime()
{
  TRACE(("InitializeQuickTime"));
  if (!IsQuickTimeInitialized()) {
    OSErr err = EnterMovies();
    QT_ENSURE_NOERR(err);
    sbQuickTime::mInitialized = PR_TRUE;
  }
  
  return NS_OK;
}

PRBool
sbQuickTime::IsQuickTimeInitialized()
{
  TRACE(("IsQuickTimeInitialized"));
  return sbQuickTime::mInitialized;
}

NS_IMETHODIMP
sbQuickTime::CreateMovieControl()
{
  TRACE(("CreateMovieControl"));
  if (mControl) {
    NS_WARNING("CreateMovieControl called more than once!");
    return NS_OK;
  }

  // Must have a movie to create a controller...
  NS_ENSURE_STATE(mMovie);
  
  // Must also have a window
  NS_ENSURE_STATE(mWindow);
  
  Rect bounds = {0, 0, 1, 1};
  PRUint32 options = kMovieControlOptionHideController;
  OSErr err = ::CreateMovieControl(mWindow, &bounds, mMovie, options,
                                   &mControl);
  QT_ENSURE_NOERR(err);
  
  err = ::GetControlData(mControl, kControlEntireControl,
                         kMovieControlDataMovieController,
                         sizeof(MovieController), &mController, nsnull);
  QT_ENSURE_NOERR(err);
    
  ::MCDoAction(mController, mcActionSetUseBadge, (void*) PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::CreateOffscreenGWorld()
{
  TRACE(("CreateOffscreenGWorld"));
  if (mOffscreenGWorld) {
    NS_WARNING("CreateOffscreenGWorld called more than once!");
    return NS_OK;
  }
  
  Rect bounds = {0, 0, 1, 1};
  ::NewGWorld(&mOffscreenGWorld, 0, &bounds, nsnull, nsnull, 0);
  NS_ENSURE_TRUE(mOffscreenGWorld, NS_ERROR_FAILURE);
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetMovie(Movie aMovie)
{
  TRACE(("SetMovie"));
  NS_ENSURE_ARG(aMovie);
  
  if (aMovie == mMovie)
    return NS_OK;
  
  if (mController) {
    Point pt = {0, 0};
    ::MCSetMovie(mController, aMovie, nil, pt);
  }
  
  if (mMovie)
    ::DisposeMovie(mMovie);
  
  mMovie = aMovie;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetGWorld(GWorldPtr aGWorld)
{
  TRACE(("SetGWorld"));
  NS_ENSURE_ARG(aGWorld);
  NS_ENSURE_STATE(mMovie);
  
  if (mController)
    ::MCSetControllerPort(mController, aGWorld);
  
  ::SetMovieGWorld(mMovie, aGWorld, nsnull);
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::UpdateControlBounds()
{
  TRACE(("UpdateControlBounds"));
  NS_ENSURE_STATE(mTargetElement);

  nsCOMPtr<nsIBoxObject> boxObject;
  nsresult rv = mTargetElement->GetBoxObject(getter_AddRefs(boxObject));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRect boxRect;
  rv = boxObject->GetX(&boxRect.x);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = boxObject->GetY(&boxRect.y);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = boxObject->GetWidth(&boxRect.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = boxObject->GetHeight(&boxRect.height);
  NS_ENSURE_SUCCESS(rv, rv);
  
  Rect newControlBounds;
  RectToMacRect(boxRect, newControlBounds);
  
  NS_ENSURE_STATE(mControl);
  ::SetControlBounds(mControl, &newControlBounds);
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::IsMediaExtension(nsACString& aExtension,
                              PRBool* _retval)
{
  PRBool isMedia = (aExtension.EqualsLiteral("mp3") ||
                    aExtension.EqualsLiteral("m4p") ||
                    aExtension.EqualsLiteral("wav")
                   );
  
  // Check our video extensions too
  if (!isMedia) {
    nsresult rv = IsVideoExtension(aExtension, &isMedia);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  *_retval = isMedia;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::IsVideoExtension(nsACString& aExtension,
                              PRBool* _retval)
{
  PRBool isVideo = (aExtension.EqualsLiteral("mov") ||
                    aExtension.EqualsLiteral("avi") ||
                    aExtension.EqualsLiteral("mpg")
                   );
  *_retval = isVideo;
  return NS_OK;
}

// Timer methods currently unused... Kept around for the moment.
/*
NS_IMETHODIMP
sbQuickTime::StartTimer(PRUint32 aInterval)
{
  TRACE(("StartTimer"));
  if (mTimer)
    return NS_OK;
  
  nsresult rv;
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return mTimer->Init(this, aInterval, nsITimer::TYPE_REPEATING_SLACK);
}
*/
/*
NS_IMETHODIMP
sbQuickTime::StopTimer()
{
  TRACE(("StopTimer"));
  if (!mTimer)
    return NS_OK;

  nsresult rv = mTimer->Cancel();
  NS_ENSURE_SUCCESS(rv, rv);
  
  mTimer = nsnull;
  
  return NS_OK;
}
*/

/**
 * sbIQuickTime Methods
 */

NS_IMETHODIMP
sbQuickTime::GetQuickTimeVersion(PRInt32* _retval)
{
  TRACE(("GetQuickTimeVersion"));
  NS_ENSURE_ARG_POINTER(_retval);
  
  long version;
  OSErr err = Gestalt(gestaltQuickTime, &version);
  QT_ENSURE_NOERR(err);
  
  *_retval = version;
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetPlaying(PRBool* aPlaying)
{
  TRACE(("GetPlaying"));
  NS_ENSURE_ARG_POINTER(aPlaying);
  *aPlaying = mStatus == qtStatusPlaying;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetPaused(PRBool* aPaused)
{
  TRACE(("GetPaused"));
  NS_ENSURE_ARG_POINTER(aPaused);
  *aPaused = mStatus == qtStatusPaused;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetStopped(PRBool* aStopped)
{
  TRACE(("GetStopped"));
  NS_ENSURE_ARG_POINTER(aStopped);
  *aStopped = mStatus == qtStatusStopped;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetMuted(PRBool* aMuted)
{
  TRACE(("GetMuted"));
  NS_ENSURE_ARG_POINTER(aMuted);
  NS_ENSURE_STATE(mController);

  // QuickTime volumes are from -256 to 256. Negative volumes mean that
  // playback is muted.
  PRInt16 volume;
  ::MCDoAction(mController, mcActionGetVolume, (void*) &volume);
  QT_ENSURE_NOMOVIESERROR();
  
  *aMuted = volume < 0;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetMuted(PRBool aMuted)
{
  TRACE(("SetMuted"));
  NS_ENSURE_STATE(mController);

  PRInt16 volume;
  ::MCDoAction(mController, mcActionGetVolume, (void*) &volume);
  QT_ENSURE_NOMOVIESERROR();
  
  // Negative volumes mean that playback is muted.
  if ((aMuted && volume > 0) ||
      (!aMuted && volume < 0)) {
    volume *= -1;
    ::MCDoAction(mController, mcActionSetVolume, (void*) volume);
    QT_ENSURE_NOMOVIESERROR();
  }
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetVolume(PRUint32* aVolume)
{
  TRACE(("GetVolume"));
  NS_ENSURE_ARG_POINTER(aVolume);
  NS_ENSURE_STATE(mController);

  PRInt16 volume;
  ::MCDoAction(mController, mcActionGetVolume, (void*) &volume);
  QT_ENSURE_NOMOVIESERROR();
  
  // Negative volumes mean that playback is muted, but we have GetMute if
  // someone wants to know. Return the abs here.
  *aVolume = PR_ABS(volume);
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetVolume(PRUint32 aVolume)
{
  TRACE(("SetVolume"));
  NS_ENSURE_ARG_RANGE(aVolume, 0, 256);
  NS_ENSURE_STATE(mController);

  PRInt16 volume = aVolume;
  
  PRBool muted;
  nsresult rv  = GetMuted(&muted);
  NS_ENSURE_SUCCESS(rv, rv);
    
  // Preserve QuickTime's internal mute.
  if (muted)
    volume *= -1;
  
  ::MCDoAction(mController, mcActionSetVolume, (void*) volume);
  QT_ENSURE_NOMOVIESERROR();
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetLength(PRUint64* aLength)
{
  TRACE(("GetLength"));
  NS_ENSURE_ARG_POINTER(aLength);
  NS_ENSURE_STATE(mMovie);
  
  TimeValue duration = ::GetMovieDuration(mMovie);
  QT_ENSURE_NOMOVIESERROR();
  
  TimeScale scale = ::GetMovieTimeScale(mMovie);
  QT_ENSURE_NOMOVIESERROR();
  
  // No divide by zero, please.
  NS_ENSURE_STATE(scale);
  
  //LOG(("GetLength: duration = %d, scale = %d", duration, scale));
  
  PRUint64 result;
  LL_DIV(result, duration, scale);
  LL_MUL(result, result, 1000);
  
  *aLength = result;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetPosition(PRUint64* aPosition)
{
  TRACE(("GetPosition"));
  NS_ENSURE_ARG_POINTER(aPosition);
  NS_ENSURE_STATE(mMovie);
  
  TimeRecord time;
  ::GetMovieTime(mMovie, &time);
  
  // No divide by zero
  NS_ENSURE_STATE(time.scale);
  
  PRInt64 result = ::WideToSInt64(time.value);
  LL_DIV(result, result, time.scale);
  LL_MUL(result, result, 1000);
  
  LOG(("GetPosition: pos = %d", result));
  
  *aPosition = result;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetPosition(PRUint64 aPosition)
{
  TRACE(("SetPosition"));
  NS_ENSURE_STATE(mMovie);
  NS_ENSURE_STATE(mController);
  
  TimeRecord time;
  ::GetMovieTime(mMovie, &time);
  
  PRInt64 temp;
  LL_MUL(temp, aPosition, time.scale);
  LL_DIV(temp, temp, 1000);
  time.value = ::SInt64ToWide(temp);
  
  LOG(("SetPosition: pos = %d", temp));

  ::MCDoAction(mController, mcActionGoToTime, (void*) &time);
  QT_ENSURE_NOMOVIESERROR();
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetFullScreen(PRBool* aFullScreen)
{
  TRACE(("GetFullScreen"));
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetFullScreen(PRBool aFullScreen)
{
  TRACE(("SetFullScreen"));
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetId(nsAString& aId)
{
  TRACE(("GetId"));
  aId.Assign(mId);
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetId(const nsAString& aId)
{
  TRACE(("SetId"));
  mId.Assign(aId);
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetTargetElement(nsIDOMXULElement** aTargetElement)
{
  TRACE(("GetTargetElement"));
  NS_ENSURE_ARG_POINTER(aTargetElement);
  NS_ENSURE_STATE(mTargetElement);
  
  NS_ADDREF(*aTargetElement = mTargetElement);
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetTargetElement(nsIDOMXULElement* aTargetElement)
{
  TRACE(("SetTargetElement"));
  nsresult rv;

  if (mTargetElement) {
    //rv = StopTimer();
    //NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIDOMEventTarget> domTarget;
    rv = GetDomEventTargetFromElement(mTargetElement,
                                      getter_AddRefs(domTarget));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = domTarget->RemoveEventListener(NS_LITERAL_STRING("resize"), this,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = domTarget->RemoveEventListener(NS_LITERAL_STRING("unload"), this,
                                        PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mTargetElement = aTargetElement;
  
  if (!mTargetElement)
    return NS_OK;
  
  // Add the resize and unload event listeners by navigating this crazy maze.
  nsCOMPtr<nsIDOMEventTarget> domTarget;
  rv = GetDomEventTargetFromElement(mTargetElement, getter_AddRefs(domTarget));
  NS_ENSURE_SUCCESS(rv, rv);
    
  rv = domTarget->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = domTarget->AddEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWidget> widget;
  rv = GetWidgetFromElement(mTargetElement, getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIWidget> parentWidget = widget;
  nsCOMPtr<nsIWidget> tempWidget = widget->GetParent();
  while (tempWidget) {
    parentWidget = tempWidget;
    tempWidget = tempWidget->GetParent();
  }
  NS_ENSURE_TRUE(parentWidget, NS_ERROR_FAILURE);

  mWindow =
    NS_REINTERPRET_CAST(WindowRef, parentWidget->GetNativeData(NS_NATIVE_DISPLAY));
  NS_ENSURE_TRUE(mWindow, NS_ERROR_FAILURE);
  
  if (!mControl) {
    rv = CreateMovieControl();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = UpdateControlBounds();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::GetCurrentURI(nsIURI** aCurrentURI)
{
  TRACE(("GetCurrentURI"));
  NS_ENSURE_ARG_POINTER(aCurrentURI);
  NS_ADDREF(*aCurrentURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::SetCurrentURI(nsIURI* aCurrentURI)
{  
  TRACE(("SetCurrentURI"));
  
  // Don't null check here because a null value is handled appropriately by
  // the SB_CreateMovieFromURI function.
  Movie movie;
  nsresult rv = SB_CreateMovieFromURI(aCurrentURI, &movie);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = SetMovie(movie);
  if (NS_FAILED(rv)) {
    NS_WARNING("SetMovie failed");
    DisposeMovie(movie);
    return rv;
  }
  
  mURI = aCurrentURI;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::Play()
{
  TRACE(("Play"));
  PRBool playing;
  nsresult rv = GetPlaying(&playing);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (playing)
    return NS_OK;
    
  NS_ENSURE_STATE(mMovie);
  NS_ENSURE_STATE(mController);
  
  Fixed rate = ::GetMoviePreferredRate(mMovie);
  QT_ENSURE_NOMOVIESERROR();
  
  ::MCDoAction(mController, mcActionPlay, (void*) rate);
  QT_ENSURE_NOMOVIESERROR();
  
  mStatus = qtStatusPlaying;
  
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::Stop()
{
  TRACE(("Stop"));
  PRBool stopped;
  nsresult rv = GetStopped(&stopped);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (stopped)
    return NS_OK;
  
  NS_ENSURE_STATE(mController);
  ::MCDoAction(mController, mcActionPlay, (void*) 0);
  QT_ENSURE_NOMOVIESERROR();
  
  rv = SetPosition(0);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mStatus = qtStatusStopped;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::Pause()
{
  TRACE(("Pause"));
  PRBool paused;
  nsresult rv = GetPaused(&paused);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (paused)
    return NS_OK;
  
  NS_ENSURE_STATE(mController);
  ::MCDoAction(mController, mcActionPlay, (void*) 0);
  QT_ENSURE_NOMOVIESERROR();
  mStatus = qtStatusPaused;
  return NS_OK;
}

NS_IMETHODIMP
sbQuickTime::IsMediaURI(nsIURI* aURI,
                        PRBool* _retval)
{
  TRACE(("IsMediaURI"));
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsCAutoString extension;
  nsresult rv = SB_GetFileExtensionFromURI(aURI, extension);
  if (NS_FAILED(rv)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }
  
  return IsMediaExtension(extension, _retval);
}

NS_IMETHODIMP
sbQuickTime::IsVideoURI(nsIURI* aURI,
                        PRBool* _retval)
{
  TRACE(("IsVideoURI"));
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCAutoString extension;
  nsresult rv = SB_GetFileExtensionFromURI(aURI, extension);
  if (NS_FAILED(rv)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return IsVideoExtension(extension, _retval);
}

NS_IMETHODIMP
sbQuickTime::GetMetadataForKey(const nsAString& aKey,
                               nsAString& _retval)
{
  TRACE(("GetMetadataForKey"));
  return NS_OK;
}

/**
 * nsIDOMEventListener Methods
 */
NS_IMETHODIMP
sbQuickTime::HandleEvent(nsIDOMEvent* aEvent)
{
  TRACE(("HandleEvent"));
  NS_ENSURE_ARG_POINTER(aEvent);
  
  TRACE(("HandleEvent"));
  
  nsAutoString eventType;
  nsresult rv = aEvent->GetType(eventType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (eventType.EqualsLiteral("resize"))
    return UpdateControlBounds();
  else if (eventType.EqualsLiteral("unload"))
    return NS_OK;
  
  NS_WARNING("Recieved an event that was not handled properly!");
  return NS_OK;
}

/**
 * nsIObserver Methods
 */
NS_IMETHODIMP
sbQuickTime::Observe(nsISupports* aSubject,
                     const char* aTopic,
                     const PRUnichar* aData)
{
  TRACE(("Observe"));
  if (nsCRT::strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC) == 0) {
    TRACE(("Timer!!!"));
  }
  
  return NS_OK;
}
