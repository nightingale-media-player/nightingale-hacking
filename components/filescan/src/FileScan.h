/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/**
 * \file FileScan.h
 * \brief 
 */

#ifndef __FILE_SCAN_H__
#define __FILE_SCAN_H__

// INCLUDES ===================================================================
#include <vector>
#include <deque>

#include "sbIFileScan.h"

#include <nscore.h>
#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsStringGlue.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsXPCOM.h>
#include <prlock.h>
#include <prmon.h>
#include <nsIThread.h>
#include <nsIRunnable.h>
#include <nsIObserver.h>

// DEFINES ====================================================================
#define SONGBIRD_FILESCAN_CONTRACTID                     \
  "@songbirdnest.com/Songbird/FileScan;1"
#define SONGBIRD_FILESCAN_CLASSNAME                      \
  "Songbird Media Scan Interface"
// {411DD545-EAD0-41c4-8BA1-697DBE5C67EA}
#define SONGBIRD_FILESCAN_CID                            \
{ 0x411dd545, 0xead0, 0x41c4, { 0x8b, 0xa1, 0x69, 0x7d, 0xbe, 0x5c, 0x67, 0xea } }

#define SONGBIRD_FILESCANQUERY_CONTRACTID                \
  "@songbirdnest.com/Songbird/FileScanQuery;1"
#define SONGBIRD_FILESCANQUERY_CLASSNAME                 \
  "Songbird Media Scan Query Interface"
// {7BB22470-E03D-4220-AC93-AC70700AF6AB}
#define SONGBIRD_FILESCANQUERY_CID                       \
{ 0x7bb22470, 0xe03d, 0x4220, { 0xac, 0x93, 0xac, 0x70, 0x70, 0xa, 0xf6, 0xab } }
// CLASSES ====================================================================
/**
 * \class sbFileScanQuery
 * \brief
 */
class sbFileScanQuery : public sbIFileScanQuery
{
public:
  sbFileScanQuery();
  sbFileScanQuery(const nsString &strDirectory, const PRBool &bRecurse, sbIFileScanCallback *pCallback);
  
  virtual ~sbFileScanQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILESCANQUERY

protected:
  typedef std::vector<nsString> filestack_t;
  
  nsString GetExtensionFromFilename(const nsAString &strFilename);
  PRBool VerifyFileExtension(const nsAString &strExtension);

  PRLock* m_pDirectoryLock;
  nsString m_strDirectory;

  PRLock* m_pCurrentPathLock;
  nsString m_strCurrentPath;

  PRBool m_bSearchHidden;
  PRBool m_bRecurse;

  PRLock* m_pScanningLock;
  PRBool m_bIsScanning;

  PRLock* m_pCallbackLock;
  sbIFileScanCallback *m_pCallback;

  PRLock* m_pFileStackLock;
  filestack_t m_FileStack;

  PRLock *m_pExtensionsLock;
  filestack_t m_Extensions;

  PRLock* m_pCancelLock;
  PRBool m_bCancel;
};

class sbFileScanThread;

/**
 * \class sbFileScan
 * \brief 
 */
class sbFileScan : public sbIFileScan,
                   public nsIObserver
{
public:

  friend class sbFileScanThread;

  sbFileScan();
  virtual ~sbFileScan();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIFILESCAN

  static void PR_CALLBACK QueryProcessor(sbFileScan* pFileScan);
  PRInt32 ScanDirectory(sbIFileScanQuery *pQuery);

protected:
  typedef std::deque<sbIFileScanQuery *> queryqueue_t;
  
  //typedef std::deque<nsCOMPtr<nsISimpleEnumerator> > dirstack_t;
  typedef std::deque<nsISimpleEnumerator * > dirstack_t;
  typedef std::deque<nsCOMPtr<nsIFile> > fileentrystack_t;
  typedef std::deque<nsCOMPtr<nsISupports> > entrystack_t;

  nsresult Shutdown();

  PRBool m_AttemptShutdownOnDestruction;

  PRMonitor* m_pThreadMonitor;
  nsCOMPtr<nsIThread> m_pThread;
  PRBool m_ThreadShouldShutdown;
  queryqueue_t m_QueryQueue;
  PRBool m_ThreadQueueHasItem;
};

class sbFileScanThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  sbFileScanThread(sbFileScan* pFileScan) {
    NS_ASSERTION(pFileScan, "Null pointer!");
    mpFileScan = pFileScan;
  }
  NS_IMETHOD Run() {
    sbFileScan::QueryProcessor(mpFileScan);
    return NS_OK;
  }
protected:
  sbFileScan* mpFileScan;
};

#endif // __FILE_SCAN_H__

