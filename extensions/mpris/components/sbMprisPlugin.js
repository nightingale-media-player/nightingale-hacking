/*
 * Written by Logan F. Smyth © 2009
 * http://logansmyth.com
 * me@logansmyth.com
 * 
 * Feel free to use/modify this code, but if you do
 * then please at least tell me!
 *
 */


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");  

const CLASS_ID = Components.ID("{03638fa0-5327-11de-8a39-0800200c9a66}");
const CLASS_NAME = "sbMprisPlugin";
const CONTRACT_ID = "@logansmyth.com/Songbird/MprisPlugin;1";


function sbMprisPlugin() {

  


  this.info_obj = Components.classes['@mozilla.org/xre/app-info;1'].getService(Components.interfaces.nsIXULAppInfo);
  this.mediacore = Components.classes['@songbirdnest.com/Songbird/Mediacore/Manager;1'].getService(Components.interfaces.sbIMediacoreManager);
  this.appStartup = Components.classes['@mozilla.org/toolkit/app-startup;1'].getService(Components.interfaces.nsIAppStartup);
  this.url_formatter = Components.classes['@mozilla.org/toolkit/URLFormatterService;1'].getService(Components.interfaces.nsIURLFormatter);

  this.seq = this.mediacore.sequencer;
  
  
  
  var xml_header = "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\"><node name=\"/org/mpris/songbird\"><interface name=\"org.mpris.songbird\">";
  var xml_footer = "</interface></node>";
  this.introspect_root = xml_header +"	\
	<method name=\"Identity\">	\
	  <arg type=\"s\" direction=\"out\"/>		\
	</method>					\
	<method name=\"Quit\" />			\
	<method name=\"MprisVersion\">		\
	  <arg type=\"(qq)\" direction=\"out\"/>	\
	</method>" + xml_footer;
	
  this.introspect_player = xml_header + "		\
	<method name=\"Next\" />		\
	<method name=\"Prev\" />		\
	<method name=\"Pause\" />		\
	<method name=\"Stop\" />		\
	<method name=\"Play\" />		\
	<method name=\"Repeat\">			\
	  <arg type=\"b\" direction=\"in\"/>				\
	</method>					\
	<method name=\"GetStatus\">			\
	  <arg type=\"(iiii)\" direction=\"out\"/>	\
	</method>					\
	<method name=\"GetMetadata\">			\
	  <arg type=\"a{sv}\" direction=\"out\" />	\
	</method>				\
	<method name=\"SetMetadata\">			\
	  <arg type=\"s\" direction=\"in\" />	\
	  <arg type=\"s\" direction=\"in\" />	\
	</method>					\
	<method name=\"GetCaps\">			\
	  <arg type=\"i\" direction=\"out\" />	\
	</method>					\
	<method name=\"VolumeSet\">			\
	  <arg type=\"i\" direction=\"in\"/>				\
	</method>					\
	<method name=\"VolumeGet\">			\
	  <arg type=\"i\" direction=\"out\"/>		\
	</method>					\
	<method name=\"PositionSet\">			\
	  <arg type=\"i\" direction=\"in\"/>				\
	</method>					\
	<method name=\"PositionGet\">			\
	  <arg type=\"i\" direction=\"out\"/>		\
	</method>					\
	<signal name=\"TrackChange\">			\
	  <arg type=\"a{sv}\"/>			\
	</signal>					\
	<signal name=\"StatusChange\">		\
	 <arg type=\"(iiii)\"/>			\
	</signal>					\
	<signal name=\"CapsChange\">			\
	  <arg type=\"i\" />				\
	</signal>" + xml_footer;

  this.introspect_tracklist = xml_header + "		\
	<method name=\"GetLength\">			\
	  <arg type=\"i\" direction=\"out\" />	\
	</method>					\
	<method name=\"GetMetadata\">			\
	  <arg type=\"i\" direction=\"in\" />		\
	  <arg type=\"a{sv}\" direction=\"out\" />	\
	</method>			\
	<method name=\"SetMetadata\">			\
	  <arg type=\"i\" direction=\"in\" />		\
	  <arg type=\"s\" direction=\"in\" />	\
	  <arg type=\"s\" direction=\"in\" />	\
	</method>					\
	<method name=\"GetCurrentTrack\">		\
	  <arg type=\"i\" direction=\"out\" />	\
	</method>					\
	<method name=\"GetLength\">			\
	  <arg type=\"i\" direction=\"out\" />	\
	</method>					\
	<method name=\"AddTrack\">			\
	  <arg type=\"s\" direction=\"in\" />		\
	  <arg type=\"b\" direction=\"in\" />		\
	  <arg type=\"i\" direction=\"out\" />	\
	</method>					\
	<method name=\"DelTrack\">			\
	  <arg type=\"i\"  direction=\"in\"/>				\
	</method>					\
	<method name=\"SetLoop\">			\
	  <arg type=\"b\"  direction=\"in\"/>				\
	</method>					\
	<method name=\"SetRandom\">			\
	  <arg type=\"b\"  direction=\"in\"/>				\
	</method>					\
	<signal name=\"TrackListChange\">		\
	  <arg type=\"i\" />				\
	</signal>" + xml_footer;
	

};

sbMprisPlugin.prototype = {
  classDescription: CLASS_NAME,
  classID: CLASS_ID,
  contractID: CONTRACT_ID,
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.sbIMprisPlugin, Components.interfaces.sbIMethodHandler]),
  dbus: null,
  debug_mode: false,
  
  init: function(debug){
    this.debug_mode = debug;
    
    this.dbus = Components.classes['@logansmyth.com/Songbird/DbusConnection;1'].createInstance(Components.interfaces.sbIDbusConnection);
    this.dbus.init("org.mpris.songbird", this.debug_mode);
    this.dbus.setMatch("type='signal',interface='org.freedesktop.MediaPlayer'");
    this.dbus.setMethodHandler(this);

    var TimerCallback = function(dbus) { 
      this.dbus = dbus;
    }
    TimerCallback.prototype = {
      dbus: null,
      handler: null,
      QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsITimerCallback]),
      notify: function(timer){
	this.dbus.check();
      }
    }

    
    var timer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer)
      
    timer.initWithCallback(new TimerCallback(this.dbus), 100, timer.TYPE_REPEATING_SLACK);
    
    var mediacore = this.mediacore;
    var dbus = this.dbus;
    var plugin = this;
	  
    this.mediacore.addListener({
      onMediacoreEvent : function(event) {
	var item = event.data;
	if (mediacore.sequencer.view == null) return;
	var list = mediacore.sequencer.view.mediaList;
	
	switch (event.type) {
	  case Components.interfaces.sbIMediacoreEvent.TRACK_CHANGE:
	    dbus.prepareSignal("/Player", "org.freedesktop.MediaPlayer", "TrackChange");
	    plugin.getMetadata(mediacore.sequencer.viewPosition);
	    dbus.sendSignal();
	    break;
	  case Components.interfaces.sbIMediacoreEvent.STREAM_START:
	  case Components.interfaces.sbIMediacoreEvent.STREAM_PAUSE:
	  case Components.interfaces.sbIMediacoreEvent.STREAM_STOP:
	  case Components.interfaces.sbIMediacoreEvent.STREAM_END:
	    dbus.prepareSignal("/Player", "org.freedesktop.MediaPlayer", "StatusChange");
	    plugin.getStatus();
	    dbus.sendSignal();
	    break;
	  default:
	    break;
	}
      }
    });
    
    //Have to use dataremotes because there aren't mediacore events for these
    //There might be an undocumented event for shuffle at type 0x1505, but the remotes works fine
    this.shuffleRemote = Components.classes['@songbirdnest.com/Songbird/DataRemote;1'].createInstance(Components.interfaces.sbIDataRemote);
    this.shuffleRemote.init("playlist.shuffle");
    this.shuffleRemote.bindObserver({
      observe: function(event){
	dbus.prepareSignal("/Player", "org.freedesktop.MediaPlayer", "StatusChange");
	plugin.getStatus();
	dbus.sendSignal();
      }
    });
    
    this.repeatRemote = Components.classes['@songbirdnest.com/Songbird/DataRemote;1'].createInstance(Components.interfaces.sbIDataRemote);
    this.repeatRemote.init("playlist.repeat");
    this.repeatRemote.bindObserver({
      observe: function(event){
	dbus.prepareSignal("/Player", "org.freedesktop.MediaPlayer", "StatusChange");
	plugin.getStatus();
	dbus.sendSignal();
      }
    });
    
  
  },
  
  //Callback called my DBusConnection for each method call received
  handleMethod: function(inter, path, member){
    this.play = this.mediacore.playbackControl;
  
    switch(inter){
      case "org.freedesktop.DBus.Introspectable":
	if(member == "Introspect"){
	  switch(path){
	    case "/":
	      this.dbus.setStringArg(this.introspect_root);
	      break;
	    case "/Player":
	      this.dbus.setStringArg(this.introspect_player);
	      break;
	    case "/TrackList":
	      this.dbus.setStringArg(this.introspect_tracklist);
	      break;
	  }
	}
	break;
      case null:
      case "":
      case "org.freedesktop.MediaPlayer":
	switch(path){
	  case "/org/mpris/songbird/":
	  case "/":
	    switch(member){
	      case "Identity":
		this.dbus.setStringArg(this.info_obj.name+" "+this.info_obj.version);
		break;
	      case "Quit":
		this.appStartup.quit(Components.interfaces.nsIAppStartup.eForceQuit);
		break;
	      case "MprisVersion":
	      
		this.dbus.openStruct();
		this.dbus.setUInt16Arg(1);
		this.dbus.setUInt16Arg(0);
		this.dbus.closeStruct();
	      
		break;
	    }
	    break;
	  case "/org/mpris/songbird/Player":
	  case "/Player":
	    switch(member){
	      case "Next":
		this.seq.next();
		break;
	      case "Prev":
		this.seq.previous();
		break;
	      case "Pause":
		if(this.play != null){
		  if(this.mediacore.status.state == this.mediacore.status.STATUS_PAUSED){
		    this.play.play();
		  }
		  else if(this.mediacore.status.state == this.mediacore.status.STATUS_PLAYING){
		    this.play.pause();
		  }
		}
		break;
	      case "Stop":
		this.seq.stop();
		break;
	      case "Play":
		this.seq.play();	
		break;
	      case "Repeat":
		if(this.dbus.getBoolArg()) this.seq.repeatMode = this.seq.MODE_REPEAT_ONE;
		else{
		  this.seq.repeatMode = this.seq.MODE_REPEAT_NONE;
		}
		break;
	      case "GetStatus":
		this.getStatus();
		break;
	      case "GetMetadata":
		this.getMetadata(this.seq.viewPosition);
		break;
		
	      // Nonstandard method for setting metadata
	      // Proposed by Panflute project
	      case "SetMetadata":
		this.setMetadata(this.seq.viewPosition);
		break;
	      // End nonstandard
		
	      case "GetCaps":
		this.getCaps();
		break;
	      case "VolumeGet":
		this.dbus.setInt32Arg(this.mediacore.volumeControl.volume*100);
		break;
	      case "VolumeSet":
		this.mediacore.volumeControl.volume = this.dbus.getInt32Arg()/100;
		break;
	      case "PositionGet":
		if(this.play != null) this.dbus.setInt32Arg(this.play.position);
		else this.dbus.setInt32Arg(0);
		break;
	      case "PositionSet":
		if(this.play != null) this.play.position = this.dbus.getInt32Arg();
		break;
	    }
	    break;
	  case "/org/mpris/songbird/TrackList":
	  case "/TrackList":
	    switch(member){
	      case "GetLength":
		if(this.play != null){
		  this.dbus.setInt32Arg(this.seq.view.length);
		}
		else this.dbus.setInt32Arg(0);
		break;
	      case "GetCurrentTrack":
		this.dbus.setInt32Arg(this.seq.viewPosition);
		break;
	      case "GetMetadata":
		this.getMetadata(this.dbus.getInt32Arg());
		break;
		
		
	      // Nonstandard method for setting metadata
	      // Proposed by Panflute project
	      case "SetMetadata":
		this.setMetadata(this.dbus.getInt32Arg());
		break;
	      // End nonstandard
		
	      case "AddTrack":
		this.addTrack(this.dbus.getStringArg(), this.dbus.getBoolArg());
		break;
	      case "DelTrack":
		this.delTrack(this.dbus.getInt32Arg());
		break;
	      case "SetLoop":
		if(this.dbus.getBoolArg()) this.seq.repeatMode = this.seq.MODE_REPEAT_ALL;
		else{
		  this.seq.repeatMode = this.seq.MODE_REPEAT_NONE;
		}
		break;
	      case "SetRandom":
		if(this.dbus.getBoolArg()) this.seq.mode = this.seq.MODE_SHUFFLE;
		else{
		  this.seq.mode = this.seq.MODE_FORWARD;
		}
		break;
	    }
	    break;
	}
	break;
    }
  },
  
  setMetadata: function(track_num) {
    var key = this.dbus.getStringArg();
    var value = this.dbus.getStringArg();
    
    var track_info = this.seq.view.getItemByIndex(track_num);
    
    switch (key) {
      case 'rating':
	track_info.setProperty(SBProperties.rating, value);
	break;
      
    }
    
  },
  
  getMetadata: function(track_num) {
    if(this.seq.view == null || track_num >= this.seq.view.length){
      this.dbus.openDictEntryArray();
      this.dbus.closeDictEntryArray();
      return;
    }
    
    var track_info = this.seq.view.getItemByIndex(track_num);
    var str;
    
    this.dbus.openDictEntryArray();
    
    if(typeof track_info.getProperty(SBProperties.contentURL) == 'string'){
      str = track_info.getProperty(SBProperties.contentURL);
      this.dbus.setDictSSEntryArg("location", str);
    }
    if(typeof track_info.getProperty(SBProperties.trackName) == 'string'){
      str = track_info.getProperty(SBProperties.trackName);
      this.dbus.setDictSSEntryArg("title", str);
    }
    if(typeof track_info.getProperty(SBProperties.artistName) == 'string'){
      str = track_info.getProperty(SBProperties.artistName);
      this.dbus.setDictSSEntryArg("artist", str);
    }
    if(typeof track_info.getProperty(SBProperties.albumName) == 'string'){
      str = track_info.getProperty(SBProperties.albumName);
      this.dbus.setDictSSEntryArg("album", str);
    }
    if(typeof track_info.getProperty(SBProperties.trackNumber) == 'string'){
      str = track_info.getProperty(SBProperties.trackNumber);
      this.dbus.setDictSSEntryArg("tracknumber", str);    
    }
    
    if(typeof track_info.getProperty(SBProperties.duration) == 'string'){
      this.dbus.setDictSIEntryArg("time", parseInt(track_info.getProperty(SBProperties.duration))/1000000);
      this.dbus.setDictSIEntryArg("mtime", parseInt(track_info.getProperty(SBProperties.duration))/1000);
    }
    
    if(typeof track_info.getProperty(SBProperties.genre) == 'string'){
      str = track_info.getProperty(SBProperties.genre);
      this.dbus.setDictSSEntryArg("genre", str);
    }
    if(typeof track_info.getProperty(SBProperties.comment) == 'string'){
      str = track_info.getProperty(SBProperties.comment);
      this.dbus.setDictSSEntryArg("comment", str);
    }
    
    if(typeof track_info.getProperty(SBProperties.year) == 'string'){
    
      this.dbus.setDictSIEntryArg("year", parseInt(track_info.getProperty(SBProperties.year)));
    
      var date = new Date();
      date.setFullYear(parseInt(track_info.getProperty(SBProperties.year)));
      this.dbus.setDictSIEntryArg("date", date.getTime());
    }
    
    if(typeof track_info.getProperty(SBProperties.primaryImageURL) == 'string'){
      this.dbus.setDictSSEntryArg("arturl", track_info.getProperty(SBProperties.primaryImageURL));
    }
    
    //TODO
    //~ this.dbus.setDictSSEntryArg("asin", "");
    //~ this.dbus.setDictSSEntryArg("puid_fingerprint", "");
    //~ this.dbus.setDictSSEntryArg("mb_track_id", "");
    //~ this.dbus.setDictSSEntryArg("mb_artist_id", "");
    //~ this.dbus.setDictSSEntryArg("mb_artist_sort_name", "");
    //~ this.dbus.setDictSSEntryArg("mb_album_id", "");
    //~ this.dbus.setDictSSEntryArg("mb_release_date", "");
    //~ this.dbus.setDictSSEntryArg("mb_album_artist", "");
    //~ this.dbus.setDictSSEntryArg("mb_album_artist_id", "");
    //~ this.dbus.setDictSSEntryArg("mb_album_artist_sort_name", "");
    
    if(typeof track_info.getProperty(SBProperties.bitRate) == 'string'){
      this.dbus.setDictSIEntryArg("audio_bitrate", parseInt(track_info.getProperty(SBProperties.bitRate)));
    }
    if(typeof track_info.getProperty(SBProperties.sampleRate) == 'string'){
      this.dbus.setDictSIEntryArg("audio_samplerate", parseInt(track_info.getProperty(SBProperties.sampleRate)));
    }
    
    
    if(typeof track_info.getProperty(SBProperties.rating) == 'string'){
      this.dbus.setDictSIEntryArg("rating", parseInt(track_info.getProperty(SBProperties.rating)));
    }
    
    
    this.dbus.closeDictEntryArray();
  },
  getStatus: function(){
    this.dbus.openStruct();
    
    switch(this.mediacore.status.state){
      case this.mediacore.status.STATUS_PLAYING:
	this.dbus.setInt32Arg(0);
	break;
      case this.mediacore.status.STATUS_PAUSED:
	this.dbus.setInt32Arg(1);
	break;
      default:  
	this.dbus.setInt32Arg(2);
	break;
    }
    
    switch(this.seq.mode){
      case this.seq.MODE_SHUFFLE:
	this.dbus.setInt32Arg(1);
	break;
      default:
	this.dbus.setInt32Arg(0);
	break;
    }
    
    switch(this.seq.repeatMode & this.seq.MODE_REPEAT_ONE){
      case this.seq.MODE_REPEAT_ONE:
	this.dbus.setInt32Arg(1);
	break;
      default:
	this.dbus.setInt32Arg(0);
	break;
    }
    
    switch(this.seq.repeatMode & this.seq.MODE_REPEAT_ALL){
      case this.seq.MODE_REPEAT_ALL:
	this.dbus.setInt32Arg(1);
	break;
      default:
	this.dbus.setInt32Arg(0);
	break;
    }
    
    this.dbus.closeStruct();
  },
  addTrack: function(uri, play_now) {
    //TODO
    
    this.dbus.setInt32Arg(1);
  },

  delTrack: function(track_num) {
    //TODO
    this.dbus.setInt32Arg(1);
  },
  getCaps: function() {
      var caps = 0;
      //TODO
      
      
      if(true){
	  caps += 1;// Has Next
      }
      if(true){
	  caps += 2;// Has Prev
      }
      if(true){
	  caps += 4;// Can Pause
      }
      if(true){
	  caps += 8;// Can Play
      }
      if(true){
	  caps += 16;// Can seek
      }
      if(true){
	  caps += 32;// Can give Metadata
      }
      if(true){
	  caps += 64;// Has Tracklist
      }
      
      this.dbus.setInt32Arg(caps);
  },
  
  
  
};
sbMprisPlugin.prototype.constructor = sbMprisPlugin;

var components = [sbMprisPlugin];
function NSGetModule(compMgr, fileSpec) { 
  return XPCOMUtils.generateModule(components);
}