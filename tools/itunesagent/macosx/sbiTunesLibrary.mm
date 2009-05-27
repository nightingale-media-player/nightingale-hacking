/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#import "SBiTunesLibrary.h"

#import <Foundation/Foundation.h>
#import "SBNSString+Utils.h"
#include "sbError.h"
#include <sstream>

// Magic keywords from the iTunes AppleScript definition file.
#define SB_GET_DESCRIPTOR_CLASS         'getd'
#define SB_GET_CLASS_COUNT              'cnte'
#define SB_SET_PROPERTY_CLASS           'setd'
#define SB_ITEM_PROPERTY_NAME           'pnam'
#define SB_ITUNES_MAINLIBRARYPLAYLIST   'cLiP'
#define SB_PLAYLIST_CLASS               'cPly'
#define SB_FOLDER_PLAYLIST_CLASS        'cFoP'
#define SB_ITUNES_ADD                   'Add '
#define SB_PARENT_PROPERTY_NAME         'pPlP'

const OSType iTunesSignature = 'hook';
const OSType AppleSignature  = 'hook';

// Apple event paramater format constants
const static NSString *gElementCountArgFormat =
  @"kocl:type(%@) , '----':@";
const static NSString *gIndexSelectArgFormat =
  @"'----':obj { form:indx, want:type(%@), seld:%i, from:@ }";
const static NSString *gPropertyArgFormat =
  @"'----':obj { form:prop, want:type(prop), seld:type(%@), from:@ }";
const static NSString *gCreateClassElementFormat =
  @"kocl:type(%@) , insh:(@)";
const static NSString *gSetPropertyArgFormat =
  @"data:@, '----':obj { form:prop, want:type(prop), seld:type(%@), from:@ }";


//------------------------------------------------------------------------------
// AppleEvent utility methods

inline sbError
sbOSErrError(const char *aMsg, OSErr & aErr)
{
  std::ostringstream msg;
  msg << "ERROR: " << aMsg << " : OSErr == " << aErr;
  return sbError(msg.str());
}

sbError
GetElementClassCount(DescType aDescType, 
                     AEDesc *aTargetDesc, 
                     int *aElementCount)
{
  if (!aElementCount) {
    return sbError("ERROR: Invalid param |aElementCount|");
  }

  *aElementCount = -1;

  // Need to use a dummy descriptor if one was not passed in.
  AEDesc *dummyDesc;
  if (!aTargetDesc) {
    dummyDesc = (AEDesc *)malloc(sizeof(AEDesc));
    dummyDesc->descriptorType = typeNull;
    dummyDesc->dataHandle = nil;
  }
  
  NSString *args =
    [NSString stringWithFormat:gElementCountArgFormat,
                               (NSString *)UTCreateStringForOSType(aDescType)];

  AppleEvent getEvent;
  OSErr err = AEBuildAppleEvent(kAECoreSuite,
                                SB_GET_CLASS_COUNT,
                                typeApplSignature,
                                &AppleSignature,
                                sizeof(typeApplSignature),
                                kAutoGenerateReturnID,
                                kAnyTransactionID,
                                &getEvent,
                                NULL,
                                [args UTF8String],
                                (aTargetDesc ? aTargetDesc : dummyDesc));
  if (err != noErr) {
    if (!aTargetDesc) {
      AEDisposeDesc(dummyDesc);
      free(dummyDesc);
    }
    return sbOSErrError("Could not build AppleEvent", err);
  }

  AppleEvent replyEvent;
  err = AESendMessage(&getEvent,
                      &replyEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&getEvent);
  
  if (!aTargetDesc) {
    AEDisposeDesc(aTargetDesc);
    free(aTargetDesc);
  }
  if (err != noErr) {
    AEDisposeDesc(&replyEvent);
    return sbOSErrError("Could not send AppleEvent", err);
  }

  DescType resultType;
  Size resultSize;
  err = AEGetParamPtr(&replyEvent,
                      keyDirectObject,
                      typeInteger,
                      &resultType,
                      aElementCount,
                      sizeof(int),
                      &resultSize);
  if (err != noErr) {
    AEDisposeDesc(&replyEvent);
    return sbOSErrError("Could not get the event param", err);
  }

  AEDisposeDesc(&replyEvent);
  return sbNoError;
}

sbError
GetAEResponseForDescClass(DescType aDescType, 
                          int aIndex, 
                          AppleEvent *aOutEvent)
{
  if (!aOutEvent) {
    return sbError("ERROR: Invalid param |aOutEvent|");
  }

  // Create a dummy descriptor to send across.
  AEDesc *descriptor = (AEDesc *)malloc(sizeof(AEDesc));
  descriptor->descriptorType = typeNull;
  descriptor->dataHandle = nil;

  NSString *args =
    [NSString stringWithFormat:gIndexSelectArgFormat,
                               (NSString *)UTCreateStringForOSType(aDescType),
                               aIndex];
  
  AppleEvent getEvent;
  OSErr err = AEBuildAppleEvent(kAECoreSuite,
                                SB_GET_DESCRIPTOR_CLASS,
                                typeApplSignature,
                                &AppleSignature,
                                sizeof(AppleSignature),
                                kAutoGenerateReturnID,
                                kAnyTransactionID,
                                &getEvent,
                                NULL,
                                [args UTF8String],
                                descriptor);
  if (err != noErr) {
    AEDisposeDesc(descriptor);
    free(descriptor);
    return sbOSErrError("Could not build AppleEvent", err);
  }

  err = AESendMessage(&getEvent, 
                      aOutEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&getEvent);
  AEDisposeDesc(descriptor);
  free(descriptor);
  if (err != noErr) {
    return sbOSErrError("Could not send the AppleEvent", err);
  }

  return sbNoError;
}

sbError
GetAEResonseForPropertyType(DescType aDescType, 
                            AEDesc *aTargetDesc,
                            AppleEvent *aOutEvent)
{
  if (!aOutEvent) {
    return sbError("ERROR: Invalid paramater |aOutEvent|");
  }

  NSString *args =
    [NSString stringWithFormat:gPropertyArgFormat,
                               (NSString *)UTCreateStringForOSType(aDescType)];

  AppleEvent getEvent;
  OSErr err = AEBuildAppleEvent(kAECoreSuite,
                                SB_GET_DESCRIPTOR_CLASS,
                                typeApplSignature,
                                &AppleSignature,
                                sizeof(AppleSignature),
                                kAutoGenerateReturnID,
                                kAnyTransactionID,
                                &getEvent,
                                NULL,
                                [args UTF8String],
                                aTargetDesc);
  if (err != noErr) {
    return sbOSErrError("Could not build AppleEvent", err);
  }

  err = AESendMessage(&getEvent,
                      aOutEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&getEvent);
  if (err != noErr) {
    return sbOSErrError("Could not send AppleEvent", err);
  }

  return sbNoError;
}

sbError
CreateElementOfDescClass(DescType aDescType, 
                         AEDesc *aTargetDesc,
                         AppleEvent *aOutEvent)
{
  if (!aOutEvent) {
    return sbError("ERROR: Invalid paramater |aOutEvent|");
  }
  
  NSString *args = 
    [NSString stringWithFormat:gCreateClassElementFormat,
                              (NSString *)UTCreateStringForOSType(aDescType)];

  // If the target descriptor was passed in as NULL, create a dumb descriptor
  AEDesc *dummyDesc = NULL;
  if (!aTargetDesc) {
    dummyDesc = (AEDesc *)malloc(sizeof(AEDesc));
    dummyDesc->descriptorType = typeNull;
    dummyDesc->dataHandle = nil; 
  }

  OSErr err;
  AppleEvent cmdEvent;
  err = AEBuildAppleEvent(kAECoreSuite,
                          kAECreateElement,
                          typeApplSignature,
                          &AppleSignature,
                          sizeof(AppleSignature),
                          kAutoGenerateReturnID,
                          kAnyTransactionID,
                          &cmdEvent,
                          NULL,
                          [args UTF8String],
                          (aTargetDesc ? aTargetDesc : dummyDesc));
  if (err != noErr) {
    AEDisposeDesc(dummyDesc);
    free(dummyDesc);
    return sbOSErrError("Could not build AppleEvent", err);
  }

  // todo use the reply event to return a descriptor...
  err = AESendMessage(&cmdEvent,
                      aOutEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&cmdEvent);
  if (!aTargetDesc) {
    AEDisposeDesc(dummyDesc);
    free(dummyDesc);
  }
  if (err != noErr) {
    return sbOSErrError("Could not send AppleEvent", err);
  }

  return sbNoError;
}

sbError
SetPropertyWithString(std::string const & aString, 
                      DescType aDescType, 
                      AEDesc *aTargetDesc)
{
  OSErr err;
  AEDesc valueDesc;

  // Create a descriptor for the new name
  NSString *value = [NSString stringWithSTDString:&aString];
  
  int valueLength = [value lengthOfBytesUsingEncoding:NSUnicodeStringEncoding];
  err = AEBuildDesc(&valueDesc, 
                    NULL, 
                    "'utxt'(@)", 
                    valueLength,  
                    [value cStringUsingEncoding:NSUnicodeStringEncoding]);
  if (err != noErr) {
    return sbOSErrError("Could not build descriptor", err);
  }

  NSString *args =
    [NSString stringWithFormat:gSetPropertyArgFormat,
                               UTCreateStringForOSType(SB_ITEM_PROPERTY_NAME)];
  AppleEvent setEvent;
  err = AEBuildAppleEvent(kAECoreSuite,
                          SB_SET_PROPERTY_CLASS,
                          typeApplSignature,
                          &AppleSignature,
                          sizeof(AppleSignature),
                          kAutoGenerateReturnID,
                          kAnyTransactionID,
                          &setEvent,
                          NULL,
                          [args UTF8String],
                          &valueDesc,
                          aTargetDesc);
  if (err != noErr) {
    return sbOSErrError("Could not build the AppleEvent", err);
  }

  err = AESendMessage(&setEvent,
                      NULL,
                      kAENoReply,
                      kAEDefaultTimeout);
  AEDisposeDesc(&setEvent);
  AEDisposeDesc(&valueDesc);
  if (err != noErr) {
    return sbOSErrError("Could not send the AppleEvent", err);
  }

  return sbNoError;
}

//------------------------------------------------------------------------------

sbiTunesObject::sbiTunesObject()
{
}

sbiTunesObject::~sbiTunesObject()
{
  AEDisposeDesc(&mDescriptor);
}

sbError
sbiTunesObject::Init(AppleEvent *aAppleEvent)
{
  if (!aAppleEvent) {
    return sbError("ERROR: Invalid param |aAppleEvent|");
  }

  OSErr err = AEGetParamDesc(aAppleEvent,
                             keyDirectObject,
                             typeWildCard,
                             &mDescriptor);
  if (err != noErr) {
    return sbOSErrError("Could not get descriptor", err);
  }
  
  return sbNoError;
}

AEDesc *
sbiTunesObject::GetDescriptor()
{
  return &mDescriptor;
}

//------------------------------------------------------------------------------

sbiTunesPlaylist::sbiTunesPlaylist()
{
}

sbiTunesPlaylist::~sbiTunesPlaylist()
{
}

sbError
sbiTunesPlaylist::GetPlaylistName(std::string *aOutString)
{
  if (!aOutString) {
    return sbError("ERROR: Invalid paramater |aOutString|!");
  }

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  AppleEvent replyEvent;
  sbError error = GetAEResonseForPropertyType(SB_ITEM_PROPERTY_NAME,
                                              &mDescriptor,
                                              &replyEvent);
  SB_ENSURE_SUCCESS(error, error);


  // Pluck the string value out of the argument
  DescType resultType;
  Size resultSize;
  OSErr err = AESizeOfParam(&replyEvent, 
                            keyDirectObject, 
                            &resultType, 
                            &resultSize);
  if (err != noErr) {
    AEDisposeDesc(&replyEvent);
    return sbOSErrError("Could not get size of param", err);
  }

  if (resultType != typeUnicodeText) {
    AEDisposeDesc(&replyEvent);
    return sbError("ERROR: Unexpected result type");
  }

  // According to the Apple Event Manager Reference, on OS X 10.4 and 
  // higher, use |typeUTF8Text| rather than |typeUnicodeText|.
  UInt8 dataChunk[resultSize];
  err = AEGetParamPtr(&replyEvent,
                      keyDirectObject,
                      typeUTF8Text,
                      &resultType,
                      &dataChunk,
                      resultSize,
                      &resultSize);
  if (err != noErr) {
    AEDisposeDesc(&replyEvent);
    return sbOSErrError("Could not get ParamPtr", err);
  }

  AEDisposeDesc(&replyEvent);

  // Use a NSString since it handles our encoding for us.
  NSString *name = [[NSString alloc] initWithBytes:dataChunk
                                            length:resultSize
                                          encoding:NSUTF8StringEncoding];
  *aOutString = [name UTF8String];
  [name release];

  [pool release];
  return sbNoError;
}

sbError
sbiTunesPlaylist::SetPlaylistName(std::string const & aPlaylistName)
{
  return SetPropertyWithString(aPlaylistName,
                               SB_ITEM_PROPERTY_NAME,
                               &mDescriptor);
}

//------------------------------------------------------------------------------

sbiTunesLibraryManager::sbiTunesLibraryManager()
  : mMainLibraryPlaylist(NULL), mSongbirdFolderPlaylist(NULL)
{
}

sbiTunesLibraryManager::~sbiTunesLibraryManager()
{
  if (mMainLibraryPlaylist) {
    delete mMainLibraryPlaylist;
  }
  if (mSongbirdFolderPlaylist) {
    delete mSongbirdFolderPlaylist;
  }
}

sbError
sbiTunesLibraryManager::GetMainLibraryPlaylist(sbiTunesPlaylist **aOutPlaylist)
{
  if (!aOutPlaylist) {
    return sbError("ERROR: Invalid paramater |aOutPlaylist|");
  }

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  if (!mMainLibraryPlaylist) {
    AppleEvent replyEvent;
    sbError error = GetAEResponseForDescClass(SB_ITUNES_MAINLIBRARYPLAYLIST,
                                              1,
                                              &replyEvent);
    SB_ENSURE_SUCCESS(error, error);

    mMainLibraryPlaylist = new sbiTunesPlaylist();
    if (!mMainLibraryPlaylist) {
      return sbError("ERROR: Coult not create a sbiTunesPlaylist object");
    }

    error = mMainLibraryPlaylist->Init(&replyEvent);
    SB_ENSURE_SUCCESS(error, error);

    AEDisposeDesc(&replyEvent);
  }

  *aOutPlaylist = mMainLibraryPlaylist;
  [pool release];
  return sbNoError;
}

sbError
sbiTunesLibraryManager::GetSongbirdPlaylistFolder(sbiTunesPlaylist **aPlaylist)
{
  if (!aPlaylist) {
    return sbError("ERROR: Invalid paramater |aPlaylist|");
  }

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  if (!mSongbirdFolderPlaylist) {
    // left off right here.
    int elementCount = 0;
    sbError error = GetElementClassCount(SB_FOLDER_PLAYLIST_CLASS,
                                         NULL,
                                         &elementCount);
    SB_ENSURE_SUCCESS(error, error);

    if (elementCount > 0) {
      int curIndex = 0;
      while (curIndex < elementCount) {
        AppleEvent responseEvent;
        error = GetAEResponseForDescClass(SB_FOLDER_PLAYLIST_CLASS,
                                          curIndex + 1,
                                          &responseEvent);
        SB_ENSURE_SUCCESS(error, error);

        sbiTunesPlaylist *curPlaylist = new sbiTunesPlaylist();
        if (!curPlaylist) {
          return sbError("ERROR: Could not create a sbiTunesPlaylist object");
        }

        error = curPlaylist->Init(&responseEvent);
        if (error != sbNoError) {
          delete curPlaylist;
          return error;
        }

        AEDisposeDesc(&responseEvent);

        std::string curPlaylistName;
        error = curPlaylist->GetPlaylistName(&curPlaylistName);
        if (error != sbNoError) {
          delete curPlaylist;
          return error;
        }

        if (curPlaylistName.compare("Songbird") == 0) {
          mSongbirdFolderPlaylist = curPlaylist;
          break;
        }

        curIndex++;
        delete curPlaylist;
      }
    }

    // If the playlist wasn't found above, create it now.
    if (!mSongbirdFolderPlaylist) {
      AppleEvent createEvent;
      error = CreateElementOfDescClass(SB_FOLDER_PLAYLIST_CLASS, 
                                       NULL, 
                                       &createEvent);
      SB_ENSURE_SUCCESS(error, error);

      mSongbirdFolderPlaylist = new sbiTunesPlaylist();
      if (!mSongbirdFolderPlaylist) {
        return sbError("ERROR: Could not create a sbiTunesPlaylist object");
      }

      error = mSongbirdFolderPlaylist->Init(&createEvent);
      SB_ENSURE_SUCCESS(error, error);

      std::string playlistName("Songbird");
      error = mSongbirdFolderPlaylist->SetPlaylistName(playlistName);
      SB_ENSURE_SUCCESS(error, error);
    }
  }

  *aPlaylist = mSongbirdFolderPlaylist;
  [pool release];
  return sbNoError;
}

sbError
sbiTunesLibraryManager::GetMainLibraryPlaylistName(std::string & aPlaylistName)
{
  sbiTunesPlaylist *mainList;
  sbError error = GetMainLibraryPlaylist(&mainList);
  SB_ENSURE_SUCCESS(error, error);

  return mainList->GetPlaylistName(&aPlaylistName);
}

sbError
sbiTunesLibraryManager::GetSongbirdPlaylist(std::string const & aPlaylistName,
                                            sbiTunesPlaylist **aOutPlaylist)
{
  if (!aOutPlaylist) {
    return sbError("ERROR: Invalid paramater |aOutPlaylist|");
  }

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  *aOutPlaylist = NULL;
  sbError error;

  sbiTunesPlaylist *songbirdFolderList;
  error = GetSongbirdPlaylistFolder(&songbirdFolderList);
  SB_ENSURE_SUCCESS(error, error);

  int listCount = 0;
  error = GetElementClassCount(SB_PLAYLIST_CLASS,
                               NULL,
                               &listCount);
  SB_ENSURE_SUCCESS(error, error);

  int curIndex = 0;
  while (curIndex < listCount) {
    AppleEvent responseEvent;
    error = GetAEResponseForDescClass(SB_PLAYLIST_CLASS, 
                                      curIndex + 1,
                                      &responseEvent);
    SB_ENSURE_SUCCESS(error, error);

    sbiTunesPlaylist *curPlaylist = new sbiTunesPlaylist();
    if (!curPlaylist) {
      AEDisposeDesc(&responseEvent);
      return sbError("ERROR: Could not create a sbiTunesPlaylist object");
    }

    error = curPlaylist->Init(&responseEvent);
    if (error != sbNoError) {
      delete curPlaylist;
      AEDisposeDesc(&responseEvent);
      return error;
    }

    AEDisposeDesc(&responseEvent);

    // Ensure that the parent of this playlist is the Songbird playlist folder.
    AppleEvent parentEvent;
    error = GetAEResonseForPropertyType(SB_PARENT_PROPERTY_NAME,
                                        curPlaylist->GetDescriptor(),
                                        &parentEvent);
    if (error != sbNoError) {
      delete curPlaylist;
      return error;
    }

    sbiTunesPlaylist curParentList;
    error = curParentList.Init(&parentEvent);
    if (error != sbNoError) {
      // If this is a playlist that doesn't have a parent?
      AEDisposeDesc(&parentEvent);
      delete curPlaylist;
      continue;
    }

    AEDisposeDesc(&parentEvent);

    std::string curParentListName;
    error = curParentList.GetPlaylistName(&curParentListName);
    if (error != sbNoError) {
      delete curPlaylist;
      continue;
    }

    if (!curParentListName.compare("Songbird")) {
      // This playlist is not a child of the Songbird playlist folder.
      delete curPlaylist;
      continue;
    }

    std::string curPlaylistName;
    error = curPlaylist->GetPlaylistName(&curPlaylistName);
    if (error != sbNoError) {
      delete curPlaylist;
      return error;
    }
    
    if (curPlaylistName.compare(aPlaylistName) == 0) {
      *aOutPlaylist = curPlaylist;
      break;
    }

    curIndex++;
    delete curPlaylist;
  }

  [pool release];
  return sbNoError;
}

sbError
sbiTunesLibraryManager::AddTrackPaths(std::deque<std::string> const & aTrackPaths,
                                      sbiTunesPlaylist *aTargetPlaylist)
{
  if (!aTargetPlaylist) {
    return sbError("ERROR: Invalid paramater |aTargetPlaylist|");
  }

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  OSErr err;
  AEDescList aliasList;
  err = AECreateDesc(typeNull, NULL, 0, &aliasList);
  if (err != noErr) {
    [pool release];
    return sbOSErrError("Could not create a AEDesc", err);
  }
  err = AECreateList(NULL, 0, false, &aliasList);
  if (err != noErr) {
    [pool release];
    return sbOSErrError("Could not create a AEDescList", err);
  }

  // Loop through the queue and push each path into the descriptor list.
  std::deque<std::string>::const_iterator begin = aTrackPaths.begin();
  std::deque<std::string>::const_iterator end = aTrackPaths.end();
  std::deque<std::string>::const_iterator next;
  for (next = begin; next != end; ++next) {
    FSRef curPathFSRef;
    err = FSPathMakeRef((UInt8 *)(*next).c_str(), &curPathFSRef, false);
    if (err != noErr) {
      continue;
    }

    // Convert the FSRef to an alias handle
    AliasHandle curPathAlias;
    err = FSNewAliasMinimal(&curPathFSRef, &curPathAlias);
    if (err != noErr) {
      continue;
    }

    AEPutPtr(&aliasList,
             0,
             typeAlias,
             *curPathAlias,
             GetHandleSize((Handle)curPathAlias));
    
    DisposeHandle((Handle)curPathAlias);
  }

  NSString *args = @"'----':@, insh:(@)";
  // Now actually build the AppleEvent to send to iTunes.
  AppleEvent cmdEvent;
  err = AEBuildAppleEvent(iTunesSignature,
                          SB_ITUNES_ADD,
                          typeApplSignature,
                          &iTunesSignature,
                          sizeof(iTunesSignature),
                          kAutoGenerateReturnID,
                          kAnyTransactionID,
                          &cmdEvent,
                          NULL,
                          [args UTF8String],
                          &aliasList,
                          aTargetPlaylist->GetDescriptor());
  if (err != noErr) {
    [pool release];
    return sbOSErrError("Could not build the AppleEvent", err);
  }

  // Phew, now send the message.
  err = AESendMessage(&cmdEvent,
                      NULL,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  if (err != noErr) {
    AEDisposeDesc(&cmdEvent);
    [pool release];
    return sbOSErrError("Could not send the AppleEvent", err);
  }

  AEDisposeDesc(&cmdEvent);
  [pool release];
  return sbNoError;
}

sbError
sbiTunesLibraryManager::CreateSongbirdPlaylist(std::string const & aPlaylistName)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  sbiTunesPlaylist *songbirdFolder;
  sbError error = GetSongbirdPlaylistFolder(&songbirdFolder);
  SB_ENSURE_SUCCESS(error, error);

  AppleEvent responseEvent;
  error = CreateElementOfDescClass(SB_PLAYLIST_CLASS,
                                   songbirdFolder->GetDescriptor(),
                                   &responseEvent);
  SB_ENSURE_SUCCESS(error, error);

  sbiTunesPlaylist *playlist = new sbiTunesPlaylist();
  if (!playlist) {
    AEDisposeDesc(&responseEvent);
    return sbError("ERROR: Could not create a sbiTunesPlaylist object");
  }

  error = playlist->Init(&responseEvent);
  AEDisposeDesc(&responseEvent);
  if (error != sbNoError) {
    delete playlist;
    return error;
  }

  error = playlist->SetPlaylistName(aPlaylistName);
  if (error != sbNoError) {
    delete playlist;
    return error;
  }

  delete playlist;
  [pool release];
  return sbNoError;
}

sbError
sbiTunesLibraryManager::DeleteSongbirdPlaylist(sbiTunesPlaylist *aPlaylist)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  if (!aPlaylist) {
    return sbError("ERROR: Invalid paramater |aPlaylist|");
  }

  OSErr err;
  AppleEvent cmdEvent;
  err = AEBuildAppleEvent(kAECoreSuite,
                          kAEDelete,
                          typeApplSignature,
                          &iTunesSignature,
                          sizeof(iTunesSignature),
                          kAutoGenerateReturnID,
                          kAnyTransactionID,
                          &cmdEvent,
                          NULL,
                          "'----':@",
                          aPlaylist->GetDescriptor());

  if (err != noErr) {
    return sbOSErrError("Could not build AppleEvent", err);
  }

  err = AESendMessage(&cmdEvent,
                      NULL,
                      kAENoReply | kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&cmdEvent);
  if (err != noErr) {
    return sbOSErrError("Could not send AppleEvent", err);
  }

  AEDisposeDesc(&cmdEvent);
  [pool release];
  return sbNoError;
}

