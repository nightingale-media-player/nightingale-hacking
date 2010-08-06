/*
 * This file contains information proprietary to Nullsoft and AOL.  For
 * further information on rules and restrictions governing the use of the
 * SHOUTcast API, the SHOUTcast Directory and other related SHOUTcast
 * services, please visit: http://www.winamp.com/legal/terms
 */

const shoutcastTuneURL = "http://shoutcast.com/sbin/tunein-station.pls?id=";

var ShoutcastRadio = {
  // Super whizzy seeeeeecret sauce
  getListenURL : function(id) {
    return (shoutcastTuneURL + id);
  },

  // OMG don't look at this, it's top-secret.  The universe may implode
  // if you understand the crazy algorithms involved here.
  getStationList : function(genre) {
    var req = new XMLHttpRequest();
    if (genre == "sbITop")
      genre = "BigAll&limit=200";
    req.open("GET",
      "http://yp.shoutcast.com/sbin/newxml.phtml?genre="+genre, false);
    req.genre = genre;
    req.send(null);
    var xml = (new DOMParser()).
        parseFromString(req.responseText, "text/xml");
    var stationList = xml.getElementsByTagName("stationlist")[0].
        getElementsByTagName("station");
    return (stationList);
  }
}
