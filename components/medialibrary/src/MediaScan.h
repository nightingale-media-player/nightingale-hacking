/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

#ifndef __MEDIA_SCAN_H__
#define __MEDIA_SCAN_H__

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
#include <nsIThread.h>
#include <nsIRunnable.h>

#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
}
#endif


// DEFINES ====================================================================
#define SONGBIRD_MEDIASCAN_CONTRACTID                     \
  "@songbird.org/Songbird/MediaScan;1"
#define SONGBIRD_MEDIASCAN_CLASSNAME                      \
  "Songbird Media Scan Interface"
#define SONGBIRD_MEDIASCAN_CID                            \
{ /* 28516ce7-fdee-4947-aad0-e5403c7866d5 */              \
  0x28516ce7,                                             \
  0xfdee,                                                 \
  0x4947,                                                 \
  {0xaa, 0xd0, 0xe5, 0x40, 0x3c, 0x78, 0x66, 0xd5}        \
}

#define SONGBIRD_MEDIASCANQUERY_CONTRACTID                \
  "@songbird.org/Songbird/MediaScanQuery;1"
#define SONGBIRD_MEDIASCANQUERY_CLASSNAME                 \
  "Songbird Media Scan Query Interface"
#define SONGBIRD_MEDIASCANQUERY_CID                       \
{ /* 4d72b67f-3eba-4621-a053-4881fcfbf667 */              \
  0x4d72b67f,                                             \
  0x3eba,                                                 \
  0x4621,                                                 \
  {0xa0, 0x53, 0x48, 0x81, 0xfc, 0xfb, 0xf6, 0x67}        \
}
// CLASSES ====================================================================
/**
 * \class CMediaScanQuery
 * \brief
 */
class CMediaScanQuery : public sbIMediaScanQuery
{
public:
  CMediaScanQuery();
  CMediaScanQuery(const nsString &strDirectory, const PRBool &bRecurse, sbIMediaScanCallback *pCallback);
  
  virtual ~CMediaScanQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIASCANQUERY

protected:
  typedef std::vector<nsString> filestack_t;

  PRLock* m_pDirectoryLock;
  nsString m_strDirectory;

  PRLock* m_pCurrentPathLock;
  nsString m_strCurrentPath;

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
  
  //typedef std::deque<nsCOMPtr<nsISimpleEnumerator> > dirstack_t;
  typedef std::deque<nsISimpleEnumerator * > dirstack_t;
  typedef std::deque<nsCOMPtr<nsIFile> > fileentrystack_t;
  typedef std::deque<nsCOMPtr<nsISupports> > entrystack_t;

  PRMonitor* m_pThreadMonitor;
  nsCOMPtr<nsIThread> m_pThread;
  PRBool m_ThreadShouldShutdown;
  queryqueue_t m_QueryQueue;
  PRBool m_ThreadQueueHasItem;
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

#endif // __MEDIA_SCAN_H__

