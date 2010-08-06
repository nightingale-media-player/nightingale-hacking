/*
 * This file contains information proprietary to Nullsoft and AOL.  For
 * further information on rules and restrictions governing the use of the
 * SHOUTcast API, the SHOUTcast Directory and other related SHOUTcast
 * services, please visit: http://www.winamp.com/legal/terms
 */

const shoutcastURL = "http://yp.shoutcast.com";
const shoutcastStationListURL = "/sbin/newxml.phtml?genre=";
// This *could* change, it will be updated in getStationList.
var shoutcastTuneURL = "/sbin/tunein-station.pls";

var ShoutcastRadio = {
  // Super whizzy seeeeeecret sauce
  getListenURL : function(id) {
    return (shoutcastURL + shoutcastTuneURL + "?id=" + id);
  },

  // OMG don't look at this, it's top-secret.  The universe may implode
  // if you understand the crazy algorithms involved here.
  getStationList : function(genre) {
    var req = new XMLHttpRequest();
    if (genre == "sbITop")
      genre = "BigAll&limit=200";
    req.open("GET",
      shoutcastURL+shoutcastStationListURL+genre, false);
    req.genre = genre;
    var stationList = [];
    try {
      req.send(null);
      var xml = (new DOMParser()).
          parseFromString(req.responseText, "text/xml");
      stationList = xml.getElementsByTagName("stationlist")[0].
          getElementsByTagName("station");
      // Update shoutcastTuneURL with the tunein-base recived with the list
      shoutcastTuneURL = xml.getElementsByTagName("stationlist")[0].
          getElementsByTagName("tunein").item(0).getAttribute("base");
    } catch (e) {} // Drop connection errors, we'll return an empty array
    return (stationList);
  }
}
