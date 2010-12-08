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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device streaming handler.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceStreamingHandler.cpp
 * \brief Songbird Device Streaming Handler Source.
 */

//------------------------------------------------------------------------------
//
// Device streaming handler imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDeviceStreamingHandler.h"
#include "sbProxiedComponentManager.h"

// Local imports.
#include <nsIProxyObjectManager.h>

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// Device streaming handler nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceStreamingHandler,
                              sbIMediaItemControllerListener)


//------------------------------------------------------------------------------
//
// Device streaming handler sbIMediaItemControllerListener implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// OnValidatePlaybackComplete
//

NS_IMETHODIMP
sbDeviceStreamingHandler::OnValidatePlaybackComplete(sbIMediaItem* aMediaItem,
                                                     PRInt32 aResult)
{
  nsAutoMonitor monitor(mCompleteNotifyMonitor);
  mIsSupported =
    aResult == sbIMediaItemControllerListener::VALIDATEPLAYBACKCOMPLETE_PROCEED
                 ? PR_TRUE : PR_FALSE;
  PR_AtomicSet(&mIsComplete, PR_TRUE);
  monitor.Notify();

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Device streaming handler public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// New
//

/* static */ nsresult
sbDeviceStreamingHandler::New
                            (sbDeviceStreamingHandler** aDeviceStreamingHandler,
                             sbIMediaItem*              aMediaItem,
                             PRMonitor*                 aCompleteNotifyMonitor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceStreamingHandler);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aCompleteNotifyMonitor);

  // Create the device streaming handler object.
  nsRefPtr<sbDeviceStreamingHandler>
    handler = new sbDeviceStreamingHandler(aMediaItem, aCompleteNotifyMonitor);
  NS_ENSURE_TRUE(handler, NS_ERROR_OUT_OF_MEMORY);

  // Return results.
  handler.forget(aDeviceStreamingHandler);

  return NS_OK;
}

//-------------------------------------
//
// CheckTranserable
//

nsresult
sbDeviceStreamingHandler::CheckTransferable()
{
  nsresult rv;

  nsCOMPtr<sbIMediaItemController> mediaItemController;
  rv = mMediaItem->GetItemController(getter_AddRefs(mediaItemController));
  NS_ENSURE_SUCCESS(rv, rv);

  // ValidatePlayback dispatch DOM event. This should be on the
  // main thread.
  nsCOMPtr<sbIMediaItemController> proxiedMediaItemController;
  rv = do_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(sbIMediaItemController),
                            mediaItemController,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedMediaItemController));
  NS_ENSURE_SUCCESS(rv, rv);

  // FromUserAction == PR_TRUE, PromptLoginOnce == PR_TRUE
  rv = proxiedMediaItemController->ValidateStreaming(mMediaItem,
                                                     PR_TRUE,
                                                     PR_TRUE,
                                                     this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// IsComplete
//

PRBool
sbDeviceStreamingHandler::IsComplete()
{
  return mIsComplete;
}


//-------------------------------------
//
// IsStreamingItemSupported
//

PRBool
sbDeviceStreamingHandler::IsStreamingItemSupported()
{
  return mIsSupported;
}


//-------------------------------------
//
// sbDeviceStreamingHandler
//

sbDeviceStreamingHandler::sbDeviceStreamingHandler
                            (sbIMediaItem* aMediaItem,
                             PRMonitor* aCompleteNotifyMonitor)
: mMediaItem(aMediaItem),
  mCompleteNotifyMonitor(aCompleteNotifyMonitor),
  mIsComplete(PR_FALSE),
  mIsSupported(PR_FALSE)
{
}


//-------------------------------------
//
// ~sbDeviceStreamingHandler
//

sbDeviceStreamingHandler::~sbDeviceStreamingHandler()
{
}

