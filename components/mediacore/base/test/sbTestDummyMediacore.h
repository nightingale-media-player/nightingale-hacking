/*
 * sbMediacoreEventTarget.h
 *
 *  Created on: Aug 31, 2008
 *      Author: dbradley
 */

#ifndef SBMEDIACOREEVENTTARGET_H_
#define SBMEDIACOREEVENTTARGET_H_

#include <nsAutoPtr.h>

#include <sbIMediacore.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreEventListener.h>
#include <sbMediacoreEvent.h>

class sbBaseMediacoreEventTarget;
class sbIMediacoreEventTarget;

class sbTestDummyMediacore : public sbIMediacoreEventTarget, public sbIMediacore
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACORE
  NS_DECL_SBIMEDIACOREEVENTTARGET
  sbTestDummyMediacore();
  virtual ~sbTestDummyMediacore();
private:
  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;
};

#define SB_TEST_DUMMY_MEDIACORE_DESCRIPTION              \
  "Songbird Mediacore Event Target"
#define SB_TEST_DUMMY_MEDIACORE_CONTRACTID               \
  "@songbirdnest.com/mediacore/sbTestDummyMediacore;1"
#define SB_TEST_DUMMY_MEDIACORE_CLASSNAME                \
  "sbTestDummyMediacore"

#define SB_TEST_DUMMY_MEDIACORE_CID                      \
{ /* 00831a9c-1d9d-4515-8620-5ef8c85355a4 */               \
  0x00831a9c,                                              \
  0x1d9d,                                                  \
  0x4515,                                                  \
  { 0x86, 0x20, 0x5e, 0xf8, 0xc8, 0x53, 0x55, 0xa4 }       \
}

#endif /* SBMEDIACOREEVENTTARGET_H_ */
