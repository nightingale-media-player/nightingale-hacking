/*
 //
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/**
 * \file FileScan.h
 * \brief
 */

#ifndef __FILE_SCAN_H__
#define __FILE_SCAN_H__

// INCLUDES ===================================================================
#include <deque>

#include "sbIFileScan.h"

#include <nscore.h>
#include <nsCOMPtr.h>

#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsIMutableArray.h>
#include <nsINetUtil.h>

#include <nsStringGlue.h>
#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsHashKeys.h>
#include <nsISimpleEnumerator.h>
#include <nsTHashtable.h>
#include <nsXPCOM.h>
#include <prlock.h>
#include <prmon.h>
#include <nsIThread.h>
#include <nsIRunnable.h>

// DEFINES ====================================================================
#define NIGHTINGALE_FILESCAN_CONTRACTID                     \
  "@getnightingale.com/Nightingale/FileScan;1"
#define NIGHTINGALE_FILESCAN_CLASSNAME                      \
  "Nightingale Media Scan Interface"
// {411DD545-EAD0-41c4-8BA1-697DBE5C67EA}
#define NIGHTINGALE_FILESCAN_CID                            \
{ 0x411dd545, 0xead0, 0x41c4, { 0x8b, 0xa1, 0x69, 0x7d, 0xbe, 0x5c, 0x67, 0xea } }

#define NIGHTINGALE_FILESCANQUERY_CONTRACTID                \
  "@getnightingale.com/Nightingale/FileScanQuery;1"
#define NIGHTINGALE_FILESCANQUERY_CLASSNAME                 \
  "Nightingale Media Scan Query Interface"
// {7BB22470-E03D-4220-AC93-AC70700AF6AB}
#define NIGHTINGALE_FILESCANQUERY_CID                       \
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
  sbFileScanQuery(const nsString & strDirectory,
                  const PRBool & bRecurse,
                  sbIFileScanCallback *pCallback);

  virtual ~sbFileScanQuery();
  // Common initializations.
  void init();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILESCANQUERY

protected:
  nsString GetExtensionFromFilename(const nsAString &strFilename);
  PRBool VerifyFileExtension(const nsAString &strExtension,
                             PRBool *aOutIsFlaggedExtension);

  PRLock* m_pDirectoryLock;
  nsString m_strDirectory;

  PRLock* m_pCurrentPathLock;
  nsString m_strCurrentPath;

  PRBool m_bSearchHidden;
  PRBool m_bRecurse;
  PRBool m_bWantLibraryContentURIs;

  PRLock* m_pScanningLock;
  PRBool m_bIsScanning;

  PRLock* m_pCallbackLock;
  nsCOMPtr<sbIFileScanCallback> m_pCallback;

  // thread-safe nsIMutableArray to store the URI spec strings
  nsCOMPtr<nsIMutableArray> m_pFileStack;
  nsCOMPtr<nsIMutableArray> m_pFlaggedFileStack;

  PRLock* m_pExtensionsLock;
  nsTHashtable<nsStringHashKey> m_Extensions;

  PRLock* m_pFlaggedFileExtensionsLock;
  nsTHashtable<nsStringHashKey> m_FlaggedExtensions;

  // m_lastSeenExtension records the extension for the last added URI
  nsString m_lastSeenExtension;

  PRLock* m_pCancelLock;
  PRBool m_bCancel;
};


/**
 * \class sbFileScan
 * \brief
 */
class sbFileScan : public sbIFileScan
{
public:
  sbFileScan();
  virtual ~sbFileScan();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIFILESCAN

protected:
  //
  // @brief
  //
  nsresult StartProcessScanQueriesProcessor();

  //
  // @brief Trigger method to process any file scan queries when they become
  //        active.
  void RunProcessScanQueries();

  //
  // @brief
  //
  nsresult ScanDirectory(sbIFileScanQuery *pQuery);

  // Typedefs
  typedef std::deque<sbIFileScanQuery *>      queryqueue_t;
  typedef std::deque<nsISimpleEnumerator *>   dirstack_t;
  typedef std::deque<nsCOMPtr<nsIFile> >      fileentrystack_t;
  typedef std::deque<nsCOMPtr<nsISupports> >  entrystack_t;

  nsresult Shutdown();

  PRLock       *m_ScanQueryQueueLock;
  queryqueue_t m_ScanQueryQueue;

  PRInt32 m_ScanQueryProcessorIsRunning;

  nsCOMPtr<nsINetUtil> mNetUtil;
  PRBool               m_ThreadShouldShutdown;
  PRBool               m_Finalized;
};

#endif // __FILE_SCAN_H__

