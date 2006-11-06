#ifndef __SB_QUICKTIME_H__
#define __SB_QUICKTIME_H__

#include "sbIQuickTime.h"

#include <nsCOMPtr.h>
#include <nsIDOMEventListener.h>
#include <nsIObserver.h>
#include <nsRect.h>
#include <nsString.h>

#include <Movies.h>

#define SONGBIRD_QUICKTIME_CONTRACTID                     \
  "@songbirdnest.com/Songbird/Playback/QuickTime;1"
#define SONGBIRD_QUICKTIME_CLASSNAME                      \
  "Songbird QuickTime Component"
#define SONGBIRD_QUICKTIME_CID                            \
{ /* 57169958-8fa9-48cb-b644-4576fed1d79c */              \
  0x57169958,                                             \
  0x8fa9,                                                 \
  0x48cb,                                                 \
  { 0xb6, 0x44, 0x45, 0x76, 0xfe, 0xd1, 0xd7, 0x9c }      \
}

class nsIDOMXULElement;
class nsIDOMEventTarget;
class nsIFile;
class nsITimer;
class nsIURI;
class nsIURL;
class nsIWidget;
class nsAString;
class nsACString;

enum qtStatus {
  qtStatusPlaying,
  qtStatusPaused,
  qtStatusStopped,
};

class sbQuickTime : public sbIQuickTime
                  , public nsIDOMEventListener
                  , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIQUICKTIME

  sbQuickTime();
  ~sbQuickTime();
  
  NS_IMETHOD Initialize();

protected:
  PRBool IsMinimumQuickTimeVersionPresent();
  NS_IMETHOD InitializeQuickTime();
  static PRBool IsQuickTimeInitialized();
  NS_IMETHOD CreateMovieControl();
  NS_IMETHOD CreateOffscreenGWorld();
  NS_IMETHOD SetMovie(Movie aMovie);
  NS_IMETHOD SetGWorld(GWorldPtr aGWorld);
  //NS_IMETHOD StartTimer(PRUint32 aInterval);
  //NS_IMETHOD StopTimer();
  NS_IMETHOD UpdateControlBounds();
  NS_IMETHOD IsMediaExtension(nsACString& aExtension, PRBool* _retval);
  NS_IMETHOD IsVideoExtension(nsACString& aExtension, PRBool* _retval);

protected:
  static PRBool mInitialized;
  MovieController mController;
  Movie mMovie;
  ControlRef mControl;
  GWorldPtr mOffscreenGWorld;
  WindowRef mWindow;
  nsCOMPtr<nsIDOMXULElement> mTargetElement;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIURI> mURI;
  nsString mId;
  qtStatus mStatus;
};

#endif /* __SB_QUICKTIME_H__ */