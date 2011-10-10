/*
 //
 // BEGIN NIGHTINGALE GPL
 //
 // This file is part of the Nightingale web player.
 //
 // Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbiTunesXMLParser.h"

#include <prlog.h>
#include <nsComponentManagerUtils.h>
#include <nsCRTGlue.h>
#include <nsISAXAttributes.h>
#include <nsISAXLocator.h>
#include <nsIInputStreamPump.h>
#include <nsThreadUtils.h>

#include <sbIiTunesXMLParserListener.h>
#include <sbProxiedComponentManager.h>

#include <sbFileUtils.h>

inline 
nsString BuildErrorMessage(char const * aType,
                            nsISAXLocator * aLocator,
                            nsAString const & aError) {
  PRInt32 line = 0;
  PRInt32 column = 0;
  aLocator->GetLineNumber(&line);
  aLocator->GetColumnNumber(&column);
  nsString msg;
  msg.AppendLiteral(aType);
  msg.AppendLiteral(" occurred at line ");
  msg.AppendInt(line, 10);
  msg.AppendLiteral(" column ");
  msg.AppendInt(column, 10); 
  msg.Append(aError);
  return msg;
}

#ifdef PR_LOGGING
static PRLogModuleInfo* giTunesXMLParserLog = nsnull;
#define TRACE(args) \
  PR_BEGIN_MACRO \
  if (!giTunesXMLParserLog) \
  giTunesXMLParserLog = PR_NewLogModule("sbiTunesXMLParser"); \
  PR_LOG(giTunesXMLParserLog, PR_LOG_DEBUG, args); \
  PR_END_MACRO
#define LOG(args) \
  PR_BEGIN_MACRO \
  if (!giTunesXMLParserLog) \
  giTunesXMLParserLog = PR_NewLogModule("sbiTunesXMLParser"); \
  PR_LOG(giTunesXMLParserLog, PR_LOG_WARN, args); \
  PR_END_MACRO

/**
 * Logs the start element data
 */
inline
void LogStartElement(const nsAString & aURI,
    const nsAString & aLocalName,
    const nsAString & aQName,
    nsISAXAttributes * aAttributes) {
  LOG(("StartElement: URI=%s localName=%s qName=%s\n",
          NS_LossyConvertUTF16toASCII(aURI).get(),
          NS_LossyConvertUTF16toASCII(aLocalName).get(),
          NS_LossyConvertUTF16toASCII(aQName).get()));
  PRInt32 length;
  aAttributes->GetLength(&length);
  nsString name;
  nsString value;
  for (PRInt32 index = 0; index < length; ++index) {
    aAttributes->GetLocalName(index, name);
    aAttributes->GetValue(index, value);
    LOG(("  %s:%s\n",
            NS_LossyConvertUTF16toASCII(name).get(),
            NS_LossyConvertUTF16toASCII(value).get()));
  }
}

/**
 * Logs errors and warnings
 */
inline
void LogError(char const * aType,
    nsISAXLocator * aLocator,
    nsAString const & aError) {
  LOG((NS_LossyConvertUTF16toASCII(BuildErrorMessage(aType, aLocator, aError)).get()));
}

#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
inline
void LogStartElement(const nsAString & aURI,
                     const nsAString & aLocalName,
                     const nsAString & qName,
                     nsISAXAttributes * aAttributes) {
}
inline
void LogError(char const * aType,
              nsISAXLocator * aLocator,
              nsAString const & aError) {
}
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS3(sbiTunesXMLParser,
    sbIiTunesXMLParser,
    nsISAXContentHandler,
    nsISAXErrorHandler)

/* Constants */
char const XML_CONTENT_TYPE[] = "text/xml";
char const NS_SAXXMLREADER_CONTRACTID[] = "@mozilla.org/saxparser/xmlreader;1";

sbiTunesXMLParser * sbiTunesXMLParser::New() {
  return new sbiTunesXMLParser;
}

sbiTunesXMLParser::sbiTunesXMLParser() : mState(START), mBytesRead(0) {
  MOZ_COUNT_CTOR(sbiTunesXMLParser);
  nsISAXXMLReaderPtr const & reader = GetSAXReader();
}

sbiTunesXMLParser::~sbiTunesXMLParser() {
  Finalize();MOZ_COUNT_DTOR(sbiTunesXMLParser);
}

/* sbIiTunesXMLParser implementation */

NS_IMETHODIMP sbiTunesXMLParser::Parse(nsIInputStream * aiTunesXMLStream,
                                       sbIiTunesXMLParserListener * aListener) {

  nsresult rv;
  
  NS_ENSURE_ARG_POINTER(aiTunesXMLStream);
  NS_ENSURE_ARG_POINTER(aListener);
  /* Update status. */

  mListener = aListener;
  
  rv = InitializeProperties();
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsISAXXMLReaderPtr const & reader = GetSAXReader();

  rv = reader->SetContentHandler(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader->SetErrorHandler(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader->ParseAsync(nsnull);
  
  mPump = do_CreateInstance("@mozilla.org/network/input-stream-pump;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mPump->Init(aiTunesXMLStream, -1, -1, 0, 0, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIStreamListener> streamListener = do_QueryInterface(reader);
  rv = mPump->AsyncRead(streamListener, nsnull);
  
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/* void finalize(); */
NS_IMETHODIMP sbiTunesXMLParser::Finalize() {
  mSAXReader = nsnull;
  mProperties = nsnull;
  mListener = nsnull;
  mTracks.Clear();
  return NS_OK;
}

nsISAXXMLReaderPtr const & sbiTunesXMLParser::GetSAXReader() {
  if (!mSAXReader) {
    nsresult rv;
    mSAXReader = do_CreateInstance(NS_SAXXMLREADER_CONTRACTID, &rv);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to create xmlreader");
  }
  return mSAXReader;
}

/* nsISAXContentHandler implementation */

/* void startDocument (); */
NS_IMETHODIMP
sbiTunesXMLParser::StartDocument() {
  LOG(("StartDocument\n"));
  // Add bytes in for the <?xml ...
  // <!DOCTYPE and the <plist
  mBytesRead = 38 + 112 + 21;
  mState = START;
  return NS_OK;
}

/* void endDocument (); */
NS_IMETHODIMP
sbiTunesXMLParser::EndDocument() {
  LOG(("EndDocument\n"));
  mSAXReader = nsnull;
  return NS_OK;
}

/* void startElement (in AString uri, in AString localName, in AString qName, in nsISAXAttributes attributes); */
NS_IMETHODIMP 
sbiTunesXMLParser::StartElement(const nsAString & uri,
                                const nsAString & localName,
                                const nsAString & qName,
                                nsISAXAttributes *attributes) {
  LogStartElement(uri, localName, qName, attributes);
  
  // If we're done then ignore everything else
  if (mState == DONE) {
    return NS_OK;
  }
  // Is this a key element
  if (localName.EqualsLiteral("true") || localName.EqualsLiteral("false")) {
    // Handle boolean values which are just an element
    if (!mPropertyName.IsEmpty()) {
      mProperties->Set(mPropertyName, localName);
      mPropertyName.Truncate();
    }
    mCharacters.Truncate();
    return NS_OK;
  }
  mListener->OnProgress(mBytesRead);
  // Add the local name length and then 2 for the < and >. iTunes doesn't use 
  // name spaces or attributes
  mBytesRead += localName.Length() + 2;
  
  if (localName.EqualsLiteral("dict")) {
    // Based on the current state, figure out what type of collection we're in
    switch (mState) {
      case START: {
        mState = TOP_LEVEL_PROPERTIES;
      }
      break;
      case TRACKS: {
        mState = TRACKS_COLLECTION;
      }
      break;
      case TRACKS_COLLECTION: {
        mState = TRACK;
      }
      break;
      case PLAYLISTS_COLLECTION: {
        mState = PLAYLIST;
      }
      break;
      case PLAYLIST_ITEMS: {
        mState = PLAYLIST_ITEM;
      }
      break;
    }
  }
  else if (localName.EqualsLiteral("array")) {
    // If we're in the Playlists section, then enter the 
    // playlist collection state
    switch (mState) {
      case PLAYLISTS: {
         mState = PLAYLISTS_COLLECTION;
      }
      break;
    }
  }
  mCharacters.Truncate();
  return NS_OK;
}

/* void endElement (in AString uri, in AString localName, in AString qName); */
NS_IMETHODIMP sbiTunesXMLParser::EndElement(const nsAString & uri,
                                            const nsAString & localName,
                                            const nsAString & qName) {
  LOG(("EndElement: URI=%s localName=%s qName=%s\n",
          NS_LossyConvertUTF16toASCII(uri).get(),
          NS_LossyConvertUTF16toASCII(localName).get(),
          NS_LossyConvertUTF16toASCII(qName).get()));
  nsresult rv;
  mListener->OnProgress(mBytesRead);
  // Add the local name length and the 3 for the </> characters.
  mBytesRead += localName.Length() + 3;
  
  // If we're done then ignore everything else
  if (mState == DONE) {
    return NS_OK;
  }
  // Save off the characters and clear the member so we don't have to
  // worry about return paths
  nsString characters(mCharacters);
  mCharacters.Truncate();
  
  nsString const propertyName(mPropertyName);
  mPropertyName.Truncate();
  
  if (localName.EqualsLiteral("key")) {
    switch (mState) {
      case TOP_LEVEL_PROPERTIES: {
        if (characters.EqualsLiteral("Tracks")) {
          rv = mListener->OnTopLevelProperties(mProperties);
          NS_ENSURE_SUCCESS(rv, rv);
          mProperties->Clear();
          mState = TRACKS;
        }
        else if (characters.EqualsLiteral("Playlists")) {
          mState = PLAYLISTS; 
        }
        else {
          mPropertyName = characters;
        }
      }
      break;
      case PLAYLIST: {
        if (characters.EqualsLiteral("Playlist Items")) {
          mState = PLAYLIST_ITEMS;
        }
        else {
          mPropertyName = characters;
        }
      }
      break;
      case TRACK:
      case PLAYLIST_ITEM: {
        mPropertyName = characters;
      }
      break;
      // Nothing to do here, skip the key which is the track ID, we'll pick it up later
      case TRACKS_COLLECTION:
      break;
      default: {
        NS_WARNING("Unexpected state in sbiTunesXMLParser::EndElement (dict)");
      }
      break;
    }
  }
  // If we're ending a dict element (dictionary collection
  else if (localName.EqualsLiteral("dict")) {
    // There's probably work to be done
    switch (mState) {
      case TRACKS_COLLECTION:
        mState = TOP_LEVEL_PROPERTIES;
        rv = mListener->OnTracksComplete();
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case TRACK: {  // We're leaving a track so notify
                     // Then go back to track collection
        mState = TRACKS_COLLECTION;
        LOG(("onTrack\n"));

        // Don't bother with this item if it has the "Movie" property.
        nsString isMovieProp;
        mProperties->Get(NS_LITERAL_STRING("Movie"), isMovieProp);
        if (isMovieProp.IsEmpty()) {
          rv = mListener->OnTrack(mProperties);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        mProperties->Clear();
      }
      break;
      case PLAYLIST: {  // We're leaving a playlist so notify
                        // Then go back to the playlists collection
        mState = PLAYLISTS_COLLECTION;
        LOG(("onPlaylist\n"));
        rv = mListener->OnPlaylist(mProperties, mTracks.Elements(), mTracks.Length());
        NS_ENSURE_SUCCESS(rv, rv);
        mTracks.Clear();
        mProperties->Clear();
      }
      break;
      case PLAYLIST_ITEM: {  // We're leaving a playlist item
                             // Go back to the playlist items
        mState = PLAYLIST_ITEMS;
      }
      break;
      default: {
        NS_WARNING("Unexpected state in sbiTunesXMLParser::EndElement (dict)");
      }
      break;
    }
  }
  // if We're leaving an array, see if it's the playlist's array of items
  // and if so, set the state back to the playlist.
  else if (localName.EqualsLiteral("array")) {
    switch (mState) {
      case PLAYLIST_ITEMS: {
        mState = PLAYLIST;
      }
      break;
      // Leaving the playlsits array, go back to top level properties
      case PLAYLISTS_COLLECTION: {                                          
        mState = TOP_LEVEL_PROPERTIES;      
        rv = mListener->OnPlaylistsComplete();
        NS_ENSURE_SUCCESS(rv, rv);
        mState = DONE;
      }
      break;
      default: {
        NS_WARNING("Unexpected state in sbiTunesXMLParser::EndElement (array)");
      }
      break;
    }
  }
  else {
    if (mState == PLAYLIST_ITEM && propertyName.EqualsLiteral("Track ID")) {
      PRInt32 const trackID = characters.ToInteger(&rv, 10);
      if (NS_SUCCEEDED(rv)) {
        PRInt32 const * newTrackID = mTracks.AppendElement(trackID);
        NS_ENSURE_TRUE(newTrackID, NS_ERROR_OUT_OF_MEMORY);
      }
    }
    else if (!propertyName.IsEmpty()) {
      mProperties->Set(propertyName, characters);  
    }
  }
  return NS_OK;
}

/* void characters (in AString value); */
NS_IMETHODIMP sbiTunesXMLParser::Characters(const nsAString & value) {
  LOG(("Characters: %s\n", NS_LossyConvertUTF16toASCII(value).get()));
  // Calculate the bytes in the stream. This isn't perfect, but should
  // be good enough for progress calculations
  PRUnichar const * begin;
  PRUnichar const * end;
  value.BeginReading(&begin, &end);
  while (begin != end) {
    mBytesRead += NS_IsAscii(*begin++) ? 1 : 2;
  }
  
  mCharacters.Append(value);
  return NS_OK;
}

/* void processingInstruction (in AString target, in AString data); */
NS_IMETHODIMP sbiTunesXMLParser::ProcessingInstruction(const nsAString & target,
                                                       const nsAString & data) {
  // Ignore
  return NS_OK;
}

/* void ignorableWhitespace (in AString whitespace); */
NS_IMETHODIMP sbiTunesXMLParser::IgnorableWhitespace(const nsAString & whitespace) {
  // Ignore
  return NS_OK;
}

/* void startPrefixMapping (in AString prefix, in AString uri); */
NS_IMETHODIMP sbiTunesXMLParser::StartPrefixMapping(const nsAString & prefix,
                                                    const nsAString & uri) {
  // Ignore
  return NS_OK;
}

/* void endPrefixMapping (in AString prefix); */
NS_IMETHODIMP sbiTunesXMLParser::EndPrefixMapping(const nsAString & prefix) {
  // Ignore
  return NS_OK;
}

/* nsSAXErrorHandler */

/* void error (in nsISAXLocator locator, in AString error); */
NS_IMETHODIMP sbiTunesXMLParser::Error(nsISAXLocator *locator,
                                       const nsAString & error) {
  LogError("Error", locator, error);
  PRBool continueParsing = PR_FALSE;
  nsresult rv = mListener->OnError(BuildErrorMessage("Error", locator, error), &continueParsing);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return continueParsing ? NS_OK : NS_ERROR_FAILURE;
}

/* void fatalError (in nsISAXLocator locator, in AString error); */
NS_IMETHODIMP sbiTunesXMLParser::FatalError(nsISAXLocator *locator,
                                            const nsAString & error) {
  LogError("Fatal error", locator, error);
  PRBool continueParsing = PR_FALSE;
  nsresult rv = mListener->OnError(BuildErrorMessage("Fatal error", locator, error), &continueParsing);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return continueParsing ? NS_OK : NS_ERROR_FAILURE;
}

/* void ignorableWarning (in nsISAXLocator locator, in AString error); */
NS_IMETHODIMP sbiTunesXMLParser::IgnorableWarning(nsISAXLocator *locator,
                                                  const nsAString & error) {
  LogError("Warning", locator, error);
  PRBool continueParsing = PR_FALSE;
  nsresult rv = mListener->OnError(BuildErrorMessage("Warning", locator, error), &continueParsing);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return continueParsing ? NS_OK : NS_ERROR_FAILURE;
}

nsresult sbiTunesXMLParser::InitializeProperties() {
  
  nsresult rv = NS_OK;
  if (!mProperties) {
    mProperties = do_CreateInstance(SB_STRINGMAP_CONTRACTID, &rv);
  }
  else {
    mProperties->Clear();
  }
  return rv;
}
