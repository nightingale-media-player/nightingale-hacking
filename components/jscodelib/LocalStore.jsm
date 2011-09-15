/**
 * LocalStore.jsm
 */

EXPORTED_SYMBOLS = [ "LocalStore" ];

Ci = Components.interfaces;
Cc = Components.classes;

var LocalStore = {
  _dirty: false,
  _rdf: null,
  _localStore: null,

  _initialize: function() {
    this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"]
                  .getService(Ci.nsIRDFService);
    this._localStore = this._rdf.GetDataSource("rdf:local-store");
  },

  // Get an attribute value for an element id in a given file
  getPersistedAttribute: function(file, id, attribute) {
    if (!this._localStore)
      this._initialize();

    var source = this._rdf.GetResource(file + "#" + id);
    var property = this._rdf.GetResource(attribute);
    var target = this._localStore.GetTarget(source, property, true);
    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;
    return null;
  },

  // Set an attribute on an element id in a given file
  setPersistedAttribute: function(file, id, attribute, value) {
    if (!this._localStore)
      this._initialize();

    var source = this._rdf.GetResource(file + "#" +  id);
    var property = this._rdf.GetResource(attribute);
    try {
      var oldTarget = this._localStore.GetTarget(source, property, true);
      if (oldTarget) {
        if (value)
          this._localStore.Change(source, property, oldTarget,
                                  this._rdf.GetLiteral(value));
        else
          this._localStore.Unassert(source, property, oldTarget);
      }
      else {
        this._localStore.Assert(source, property,
                                this._rdf.GetLiteral(value), true);
      }
      this._dirty = true;
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
  },

  // Save changes if needed
  flush: function flush() {
    if (this._localStore && this._dirty) {
      this._localStore.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();
      this._dirty = false;
    }
  }
}
