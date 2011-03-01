/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#include "sbRequestItem.h"

#include "sbRequestThreadQueue.h"

// Mozilla includes
#include <pratom.h>

PRInt32 sbRequestItem::sLastRequestId = 0;

NS_IMPL_THREADSAFE_ADDREF(sbRequestItem);
NS_IMPL_THREADSAFE_RELEASE(sbRequestItem);

sbRequestItem::sbRequestItem() :
         mRequestId(static_cast<PRUint32>(PR_AtomicIncrement(&sLastRequestId))),
         mType(sbRequestThreadQueue::REQUEST_TYPE_NOT_SET),
         mBatchId(0),
         mBatchIndex(0),
         mTimeStamp(PR_Now()),
         mIsCountable(false),
         mIsProcessed(false)
{
}

