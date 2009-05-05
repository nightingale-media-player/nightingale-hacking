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


// Magic keywords from the iTunes AppleScript definition file.
#define SB_GET_DESCRIPTOR_CLASS         'getd'
#define SB_ITEM_PROPERTY_NAME           'pnam'
#define SB_ITUNES_MAINLIBRARYPLAYLIST   'cLiP'

const OSType iTunesSignature = 'hook';
const OSType AppleSignature  = 'hook';  

// Apple event paramater format constants
const static NSString *gIndexSelectArgFormat =
  @"'----':obj { form:indx, want:type(%@), seld:1, from:@ }";
const static NSString *gPropertyArgFormat =
  @"'----':obj { form:prop, want:type(prop), seld:type(%@), from:@ }";

// Some playlist globals.
static SBiTunesPlaylist *gMainLibraryPlaylist = nil;
static SBiTunesPlaylist *gSongbirdPlaylistFolder = nil;


//------------------------------------------------------------------------------
// AppleEvent utility methods

//
// \brief DOCUMENT ME
//
AppleEvent *
GetAEResponseForDescClass(DescType aDescType)
{
  // Gah, need to send a dummy descriptor when building the apple event.
  AEDesc *descriptor = (AEDesc *)malloc(sizeof(AEDesc));
  descriptor->descriptorType = typeNull;
  descriptor->dataHandle = nil;

  NSString *args =
    [NSString stringWithFormat:gIndexSelectArgFormat,
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
                                descriptor);
  if (err != noErr) {
    // TODO Log me
    return nil;
  }

  AppleEvent *replyEvent = (AppleEvent *)malloc(sizeof(AppleEvent));
  err = AESendMessage(&getEvent, 
                      replyEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&getEvent);

  if (err != noErr) {
    // TODO Log me!
    free(replyEvent);
    return nil;
  }

  return replyEvent;
}

//
// \brief DOCUMENT ME
//
AppleEvent *
GetAEResonseForPropertyType(DescType aDescType, AEDesc *aTargetDesc)
{
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
    // TODO Log me!
    return nil;
  }

  AppleEvent *replyEvent = (AppleEvent *)malloc(sizeof(AppleEvent));
  err = AESendMessage(&getEvent,
                      replyEvent,
                      kAEWaitReply + kAENeverInteract,
                      kAEDefaultTimeout);
  AEDisposeDesc(&getEvent);
  if (err != noErr) {
    // TODO Log Me!
    free(replyEvent);
    return nil;
  }

  return replyEvent;
}

//------------------------------------------------------------------------------

@implementation SBiTunesObject

- (id)initWithAppleEvent:(AppleEvent *)aAppleEvent
{
  if ((self = [super init]) && aAppleEvent) {
    // Pluck the descriptor fom the apple event.
    OSErr err = AEGetParamDesc(aAppleEvent, 
                               keyDirectObject, 
                               typeWildCard,
                               &mDescriptor);
    if (err != noErr) {
      NSLog(@"FATAL ERROR: Could not retrieve descriptor from AppleEvent!");
      return nil;
    }
  }

  return self;
}

- (void)dealloc
{
  AEDisposeDesc(&mDescriptor);
  [super dealloc];
}

- (AEDesc *)descriptor
{
  return &mDescriptor;
}

@end

//------------------------------------------------------------------------------

@implementation SBiTunesPlaylist

- (NSString *)playlistName
{
  NSString *name = nil;

  AppleEvent *replyEvent = 
    GetAEResonseForPropertyType(SB_ITEM_PROPERTY_NAME, &mDescriptor);
  if (!replyEvent) {
    return name;
  }

  // Pluck through the arguments to extract a string.
  DescType resultType;
  Size resultSize;
  OSErr err = AESizeOfParam(replyEvent, 
                            keyDirectObject, 
                            &resultType, 
                            &resultSize);
  if (err != noErr) {
    AEDisposeDesc(replyEvent);
    free(replyEvent);
    return name;
  }

  if (resultType == typeUnicodeText) {
    UInt8 dataChunk[resultSize];
    
    // According to the Apple Event Manager Reference, on OS X 10.4 and 
    // higher, use |typeUTF8Text| rather than |typeUnicodeText|.
    err = AEGetParamPtr(replyEvent,
                        keyDirectObject,
                        typeUTF8Text,
                        &resultType,
                        &dataChunk,
                        resultSize,
                        &resultSize);
    if (err != noErr) {
      AEDisposeDesc(replyEvent);
      free(replyEvent);
      return name;
    }

    NSString *name = (NSString *)CFStringCreateWithBytes(NULL,
                                                         dataChunk,
                                                         resultSize,
                                                         kCFStringEncodingUTF8,
                                                         false);
    NSLog(@"name = %@", name);
    [name autorelease];
  }
  
  AEDisposeDesc(replyEvent);
  free(replyEvent);

  // for now
  return name;
}

@end

//------------------------------------------------------------------------------

@implementation SBiTunesLibraryManager

+ (SBiTunesPlaylist *)mainLibraryPlaylist
{
  // Load the main library if it has not been loaded already.
  if (!gMainLibraryPlaylist) {
    AppleEvent *mainLibraryResponse = 
      GetAEResponseForDescClass(SB_ITUNES_MAINLIBRARYPLAYLIST);
    
    gMainLibraryPlaylist = 
      [[SBiTunesPlaylist alloc] initWithAppleEvent:mainLibraryResponse];

    AEDisposeDesc(mainLibraryResponse);
    free(mainLibraryResponse);
  }

  return gMainLibraryPlaylist;
}

+ (SBiTunesPlaylist *)songbirdPlaylistFolder
{
  return nil;
}


- (void)addTracks:(NSArray *)aFilePathArray
       toPlaylist:(SBiTunesPlaylist *)aTargetPlaylist
{
  // todo write me
}

- (void)addPlaylistNamed:(NSString *)aPlaylistName
{
  // todo write me
}

- (void)removePlaylistNamed:(NSString *)aPlaylistName
{
  // todo write me
}

@end

