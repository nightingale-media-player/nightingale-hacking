/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
 * \file MediaScan.h
 * \brief 
 */

#pragma once

// INCLUDES ===================================================================
#include <vector>
#include <deque>
#include <string>

#include "IMediaLibrary.h"

#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsIFile.h>
#include <xpcom/nsILocalFile.h>
#include <string/nsString.h>
#include <string/nsStringAPI.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsISimpleEnumerator.h>
#include <xpcom/nsXPCOM.h>
#include <prlock.h>
#include <prmon.h>
#include <nsIRunnable.h>

#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
};
#endif


// DEFINES ====================================================================
#define SONGBIRD_MEDIASCAN_CONTRACTID  "@songbird.org/Songbird/MediaScan;1"
#define SONGBIRD_MEDIASCAN_CLASSNAME   "Songbird Media Scan Interface"
#define SONGBIRD_MEDIASCAN_CID { 0x12bc75d3, 0x8fd4, 0x4b19, { 0x8f, 0x5b, 0xce, 0xf3, 0x53, 0xe5, 0x16, 0x8d } }

// {32CD5811-77BF-4800-A4A8-FE764B8DDFD1}
#define SONGBIRD_MEDIASCANQUERY_CONTRACTID "@songbird.org/Songbird/MediaScanQuery;1"
#define SONGBIRD_MEDIASCANQUERY_CLASSNAME  "Songbird Media Scan Query Interface"
#define SONGBIRD_MEDIASCANQUERY_CID { 0x32cd5811, 0x77bf, 0x4800, { 0xa4, 0xa8, 0xfe, 0x76, 0x4b, 0x8d, 0xdf, 0xd1 } }

// CLASSES ====================================================================
/**
 * \class CMediaScanQuery
 * \brief
 */
class CMediaScanQuery : public sbIMediaScanQuery
{
public:
  CMediaScanQuery();
  CMediaScanQuery(const std::prustring &strDirectory, const PRBool &bRecurse, sbIMediaScanCallback *pCallback);
  
  virtual ~CMediaScanQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIASCANQUERY

protected:
  typedef std::vector<std::prustring> filestack_t;

  PRLock* m_pDirectoryLock;
  std::prustring m_strDirectory;

  PRLock* m_pCurrentPathLock;
  std::prustring m_strCurrentPath;

  PRBool m_bRecurse;

  PRLock* m_pScanningLock;
  PRBool m_bIsScanning;

  PRLock* m_pCallbackLock;
  sbIMediaScanCallback *m_pCallback;

  PRLock* m_pFileStackLock;
  filestack_t m_FileStack;
};

class sbMediaScanThread;

/**
 * \class CMediaScan
 * \brief 
 */
class CMediaScan : public sbIMediaScan
{
public:

  friend class sbMediaScanThread;

  CMediaScan();
  virtual ~CMediaScan();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIASCAN

  static void PR_CALLBACK QueryProcessor(CMediaScan* pMediaScan);
  PRInt32 ScanDirectory(sbIMediaScanQuery *pQuery);

protected:
  typedef std::deque<sbIMediaScanQuery *> queryqueue_t;
  typedef std::deque<nsISimpleEnumerator *> dirstack_t;

  queryqueue_t m_QueryQueue;
  PRBool m_ThreadShouldShutdown;
  PRBool m_ThreadHasShutdown;
  PRBool m_ThreadQueueHasItem;
  PRMonitor* m_pThreadMonitor;
};

class sbMediaScanThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  sbMediaScanThread(CMediaScan* pMediaScan) {
    NS_ASSERTION(pMediaScan, "Null pointer!");
    mpMediaScan = pMediaScan;
  }
  NS_IMETHOD Run() {
    CMediaScan::QueryProcessor(mpMediaScan);
    return NS_OK;
  }
protected:
  CMediaScan* mpMediaScan;
};