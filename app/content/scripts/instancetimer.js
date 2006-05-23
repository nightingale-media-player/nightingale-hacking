var sbInstanceTimer = {

  instance_timer_entries : Array(),

  setTimeout : function (callback_object, delay) {
    var aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
    var guid = aUUIDGenerator.generateUUID();
    var entry = new Array();
    entry.push(callback_object);
    entry.push(guid);
    this.instance_timer_entries.push(entry);
    setTimeout("document.__INSTANCETIMER__.onTimeout('" + guid + "');", delay);
  },
  
  setInterval : function(callback_object, delay) {
    var aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
    var guid = aUUIDGenerator.generateUUID();
    var entry = new Array();
    entry.push(callback_object);
    entry.push(guid);
    entry.push(setInterval("document.__INSTANCETIMER__.onInterval('" + guid + "');", delay));
    this.instance_timer_entries.push(entry);
    return guid;
  },

  clearInterval : function(guid) {
    var idx = this.getEntryByGuid(guid);
    if (idx >= 0) {
      clearInterval(this.instance_timer_entries[idx][2]);
      this.instance_timer_entries.splice(idx, 1);
    }
  },

  getEntryByGuid : function(guid) {
    for (var i=0;i<this.instance_timer_entries.length;i++) {
      var entry = this.instance_timer_entries[i];
      if (entry && entry[1] == guid) {
        return i;
      }
    }
    return -1;
  },

  onTimeout : function(guid) {
    var idx = this.getEntryByGuid(guid);
    if (idx != -1) {
      var object = this.instance_timer_entries[idx][0];
      object.handleEvent(null);
      this.instance_timer_entries.splice(idx, 1);
    }
  },

  onInterval : function(guid) {
    var idx = this.getEntryByGuid(guid);
    if (idx != -1) {
      var object = this.instance_timer_entries[idx][0];
      object.handleEvent(null);
    }
  }

};

if (document.__INSTANCETIMER__ == null) {
  document.__INSTANCETIMER__ = sbInstanceTimer;
}

var instanceTimer = document.__INSTANCETIMER__;

