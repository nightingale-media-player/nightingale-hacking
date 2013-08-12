/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#include "sbGStreamerPlatformBase.h"

#include <prlog.h>
#include <nsDebug.h>
#include <nsStringGlue.h>

#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEventTarget.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerPlatformBase:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING
      
static PRLogModuleInfo* gGStreamerPlatformBase =
  PR_NewLogModule("sbGStreamerPlatformBase");

#define LOG(args)                                         \
  if (gGStreamerPlatformBase)                             \
    PR_LOG(gGStreamerPlatformBase, PR_LOG_WARNING, args)

#define TRACE(args)                                      \
  if (gGStreamerPlatformBase)                            \
    PR_LOG(gGStreamerPlatformBase, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

BasePlatformInterface::BasePlatformInterface(sbGStreamerMediacore *aCore)
: sbIGstPlatformInterface()
,  mDisplayWidth(0)
,  mDisplayHeight(0)
,  mDisplayX(0)
,  mDisplayY(0)
,  mDARNum(1)
,  mDARDenom(1)
,  mFullscreen(false)
,  mVideoBox(NULL)
,  mVideoSink(NULL)
,  mAudioSink(NULL)
,  mCore(aCore)
{
  TRACE(("BasePlatformInterface -- constructed"));
}

BasePlatformInterface::~BasePlatformInterface()
{
  if (mVideoSink)
    gst_object_unref(mVideoSink);
  if (mAudioSink)
    gst_object_unref(mAudioSink);
}

bool
BasePlatformInterface::GetFullscreen()
{
  TRACE(("BasePlatformInterface[0x%.8x] -- GetFullscreen", this));

  return mFullscreen;
}

void
BasePlatformInterface::SetFullscreen(bool aFullscreen)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- SetFullscreen", this));

  if (aFullscreen && !mFullscreen) {
    mFullscreen = true;
    FullScreen();
  }
  else if (!aFullscreen && mFullscreen) {
    mFullscreen = false;
    UnFullScreen();

    // Make sure we're in the right place and the right size
    ResizeToWindow();
    ResizeVideo();
  }
  // Otherwise, we're already in the right mode, so don't do anything.
}

void
BasePlatformInterface::ResizeToWindow()
{
  TRACE(("BasePlatformInterface[0x%.8x] -- ResizeToWindow", this));

  // Only resize based on our XUL element if we're not in fullscreen mode.
  if (!mFullscreen) {
    LOG(("Resizing video to fit window in non-fullscreen mode"));
    PRInt32 x, y, width, height;

    if (mVideoBox) {
      mVideoBox->GetX(&x);
      mVideoBox->GetY(&y);
      mVideoBox->GetWidth(&width);
      mVideoBox->GetHeight(&height);

      SetDisplayArea(x, y, width, height);
      ResizeVideo();
    }
    else {
      LOG(("Not resizing video: no video box"));
    }
  }
  else {
    LOG(("Not resizing video: in fullscreen mode"));
  }
}

void
BasePlatformInterface::SetDisplayArea(int x, int y, int width, int height)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- SetDisplayArea(%d, %d, %d, %d)", 
                                               this, x, y, width, height));

  LOG(("Display area set to %d,%d %d,%d", x, y, width, height));
  mDisplayX = x;
  mDisplayY = y;
  mDisplayWidth = width;
  mDisplayHeight = height;
}

void
BasePlatformInterface::ResizeVideo()
{
  LOG(("Resizing: %d, %d, %d, %d", mDisplayX, mDisplayY, 
              mDisplayWidth, mDisplayHeight));

  // We can draw anywhere in the passed area.
  // Now, calculate the area to use that will maintain the desired 
  // display-aspect-ratio.

  // First, we see if it'll fit if we use all available vertical space.
  // If it doesn't, we use all the available horizontal space.
  int height = mDisplayHeight;
  int width = mDisplayHeight * mDARNum / mDARDenom;
  int x, y;

  if (width <= mDisplayWidth) {
    // It fits; now offset appropriately.
    x = mDisplayX + (mDisplayWidth - width)/2;
    y = mDisplayY;
  }
  else {
    // it didn't fit this way, so use the other.
    width = mDisplayWidth;
    height = mDisplayWidth * mDARDenom / mDARNum;
    x = mDisplayX;
    y = mDisplayY + (mDisplayHeight - height)/2;
  }

  MoveVideoWindow(x, y, width, height);
}

void
BasePlatformInterface::SetDisplayAspectRatio(int aNumerator, int aDenominator)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- SetDisplayAspectRatio(%d, %d)", 
                                         this, aNumerator, aDenominator));

  mDARNum = aNumerator;
  mDARDenom = aDenominator;

  ResizeVideo();
}

void
BasePlatformInterface::PrepareVideoWindow(GstMessage *aMessage)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- PrepareVideoWindow", this));

  GstElement *element = NULL;
  GstVideoOverlay *videoOverlay = NULL;

  if (GST_IS_BIN(mVideoSink)) {
    /* Get the actual implementing object from the bin */
    element = gst_bin_get_by_interface(GST_BIN(mVideoSink),
                                       GST_TYPE_VIDEO_OVERLAY);
  } else {
    element = mVideoSink;
  }

  if (GST_IS_VIDEO_OVERLAY(element)) {
    videoOverlay = GST_VIDEO_OVERLAY(element);
    LOG(("video overlay interface found, setting video window"));
  } else {
    LOG(("No video overlay interface found, cannot set video window"));
    return;
  }

  SetVideoOverlayWindowID(videoOverlay);

  ResizeToWindow();
}

nsresult
BasePlatformInterface::SetVideoBox(nsIBoxObject *aVideoBox, nsIWidget *aWidget)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- SetVideoBox", this));

  mVideoBox = aVideoBox;
  mWidget = aWidget;

  return NS_OK;
}

/*virtual*/ nsresult 
BasePlatformInterface::SetDocument(nsIDOMDocument *aDocument)
{
  TRACE(("BasePlatformInterface[0x%.8x] -- SetDocument", this));

  NS_ENSURE_ARG_POINTER(aDocument);
  mDocument = aDocument;
  return NS_OK;
}

nsresult 
BasePlatformInterface::CreateDOMMouseEvent(nsIDOMMouseEvent **aMouseEvent)
{
  NS_ENSURE_ARG_POINTER(aMouseEvent);
  
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMEvent> event;
  rv = docEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"), 
                             getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mouseEvent.forget(aMouseEvent);

  return NS_OK;
}

nsresult 
BasePlatformInterface::CreateDOMKeyEvent(nsIDOMKeyEvent **aKeyEvent)
{
  NS_ENSURE_ARG_POINTER(aKeyEvent);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMEvent> event;
  rv = docEvent->CreateEvent(NS_LITERAL_STRING("keyevents"), 
                             getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(event, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  keyEvent.forget(aKeyEvent);

  return NS_OK;
}

nsresult 
BasePlatformInterface::DispatchDOMEvent(nsIDOMEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMEventTarget> eventTarget(do_QueryInterface(mDocument, &rv ));
  NS_ENSURE_SUCCESS( rv, rv );

  PRBool dummy = PR_FALSE;
  rv = eventTarget->DispatchEvent(aEvent, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
