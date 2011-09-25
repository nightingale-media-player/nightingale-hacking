/**
 * LocalStore.jsm
 */

EXPORTED_SYMBOLS = [ "LocalStore" ];

Ci = Components.interfaces;
Cc = Components.classes;

var LocalStore = {
  dirty: false,

  // Get an attribute value for an element id in a given file
  getPersistedAttribute: function(file, id, attribute) {
    var rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
    var localStore = rdf.GetDataSource("rdf:local-store");
    var source = rdf.GetResource(file + "#" + id);
    var property = rdf.GetResource(attribute);
    var target = localStore.GetTarget(source, property, true);
    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;
    return null;
  },
  
  // Set an attribute on an element id in a given file
  setPersistedAttribute: function(file, id, attribute, value) {
    var rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
    var localStore = rdf.GetDataSource("rdf:local-store");
    var source = rdf.GetResource(file + "#" +  id);    
    var property = rdf.GetResource(attribute);
    try {
      var oldTarget = localStore.GetTarget(source, property, true);
      if (oldTarget) {
        if (value)
          localStore.Change(source, property, oldTarget,
                                  rdf.GetLiteral(value));
        else
          localStore.Unassert(source, property, oldTarget);
      }
      else {
        localStore.Assert(source, property,
                                rdf.GetLiteral(value), true);
      }
      LocalStore.dirty = true;
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
  },
          
  // Save changes if needed
  flush: function flush() {
    if (LocalStore.dirty) {
      var rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
      var localStore = rdf.GetDataSource("rdf:local-store");
      localStore.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();
    }
  }
}
