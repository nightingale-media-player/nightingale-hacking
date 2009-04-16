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
#ifndef SBITUNESXMLPARSER_H_
#define SBITUNESXMLPARSER_H_

#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsISAXContentHandler.h>
#include <nsISAXErrorHandler.h>
#include <nsISAXXMLReader.h>
#include <nsStringAPI.h>
#include <sbIStringMap.h>

#include <sbIiTunesXMLParser.h>

class nsIInputStream;

#define SBITUNESXMLPARSER_CONTRACTID                     \
  "@songbirdnest.com/Songbird/sbiTunesXMLParser;1"
#define SBITUNESXMLPARSER_CLASSNAME                      \
  "Songbird iTunes XML Parser Interface"
// {F0EBF580-C5FB-4efc-960A-50BEF0BCEDCC}
#define SBITUNESXMLPARSER_CID                            \
{ 0xf0ebf580, 0xc5fb, 0x4efc, { 0x96, 0xa, 0x50, 0xbe, 0xf0, 0xbc, 0xed, 0xcc } }

/**
 * Implementation of iTunes XML parsing.
 * NOTE: This implementation is not thread safe. Meaning you should never have 
 * an instance of this class be exposed to multiple threads. It is OK to have
 * different instances owned by their own threads.
 */
class sbiTunesXMLParser : public sbIiTunesXMLParser, 
                          public nsISAXContentHandler, 
                          public nsISAXErrorHandler 
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIITUNESXMLPARSER
  NS_DECL_NSISAXCONTENTHANDLER
  NS_DECL_NSISAXERRORHANDLER

  /**
   * Initializes the state of the parser
   */
  sbiTunesXMLParser();

protected:
  /**
   * Cleans up 
   */
  ~sbiTunesXMLParser();
private:
  /**
   * Various states we're in as we traverse the XML file
   * NOTE: *_VALUE entries must follow their *_NAME counterparts which
   * must follow their base values: TRACK, TRACK_PROEPRTY_NAME, 
   * TRACK_PROPERTY_VALUE.
   */
  enum State
  {
    START,                     // Initial state
    TOP_LEVEL_PROPERTIES,      // Base top level properties state <dict>
    TOP_LEVEL_PROPERTY_NAME,   // key element for property name <key>
    TOP_LEVEL_PROPERTY_VALUE,  // value element for property value <key>
    
    TRACKS,                    // Found the Tracks section
    TRACKS_COLLECTION,         // In the tracks collection <dict>
    TRACK,                     // Base track state <dict>
    TRACK_PROPERTY_NAME,       // Track's property name <key>
    TRACK_PROPERTY_VALUE,      // Track's property value <key> 
    
    PLAYLISTS,                 // Playlists section
    PLAYLISTS_COLLECTION,      // Playlists collection <array>
    PLAYLIST,                  // Base playlist state <dict>
    PLAYLIST_PROPERTY_NAME,    // Playlist's property name <key>
    PLAYLIST_PROPERTY_VALUE,   // Playlist's property value <key>
    PLAYLIST_ITEMS,            // Playlist items collection <array>
    PLAYLIST_ITEM,             // Base playlist item state <dict>
    PLAYLIST_ITEM_NAME,        // Playlist item's name <key>
    PLAYLIST_ITEM_VALUE,       // Playlist item's value <key>
    DONE                       // We're done, ignore the rest
  };
  
  // Typedefs
  typedef nsCOMPtr<sbIMutableStringMap> sbIMutableStringMapPtr;
  typedef nsCOMPtr<nsISAXXMLReader> nsISAXXMLReaderPtr;
  typedef nsCOMPtr<sbIiTunesXMLParserListener> sbIiTunesXMLParserListenerPtr;
  typedef nsTArray<PRInt32> Tracks;
  typedef nsCOMPtr<nsIInputStream> nsIInputStreamPtr;
  
  /**
   * Returns the SAX Reader object, creating it if necessary
   */
  nsISAXXMLReaderPtr const & GetSAXReader();
  /**
   * Creates the properties collection if not already created
   */
  nsresult InitializeProperties();

  /**
   * Clears out the property bag
   */
  nsresult ClearProperties();
  
  PRInt32 mState;
  sbIMutableStringMapPtr mProperties;
  nsISAXXMLReaderPtr mSAXReader;
  nsString mPropertyName;
  sbIiTunesXMLParserListenerPtr mListener;
  Tracks mTracks;
};

#endif /* SBITUNESXMLPARSER_H_ */
