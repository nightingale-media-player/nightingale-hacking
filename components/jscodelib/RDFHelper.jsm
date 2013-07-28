/**
 * RDFHelper.jsm
 * 
 * USE: RDFHelper(aRdf, aRDFDatasource, aRDFResource, aNamespacesHash)
 * 
 * This class allows you to treat an RDF datasource as though it were a read-only structured object.
 * (Please, don't try to write to it. It will only lead to grief.)
 * 
 * Arcs are mapped to properties by trimming their namespaces (they can be reduced to prefixes if you prefer).
 * Because there may be several arcs leading from a resource, each property is returned as an array of the targets.
 * Literals become values directly, and all other arcs lead to further arcs.
 * 
 * STATIC RDF SOURCE:
 * The RDFHelper is not so clever as to notice changes to your datasource after it creates the value.
 * As a result, you should not reuse RDFHelpers unless you are sure your data has not changed in-between.
 * 
 * CYCLIC RELATIONSHIPS:
 * Links are evaluated lazily, so creating an RDFHelper object on a cyclic graph will not kill your process,
 * but trying to crawl it probably will! Objects are not reused, so if you return to the same place twice, you
 * won't be able to tell through naive object comparison, and infinitely traversing a graph cycle will be a race 
 * between overflowing your stack and eating all your memory. 
 * 
 * CONTAINERS:
 * Containers are a special case where the object returned appears to be array-like, insofar as it has numeric
 * indices and a .length property, but is not actually an instanceof Array.
 * 
 * HELP:
 * Because looking up RDF resources is tedious and unpleasant, a few helper bits are provided.
 * 
 *   RDFHelper.help:
 *     Accepts URI strings representing a datasource, a resource, and an object containing namespaces.
 *     namespaces look like: { "rdf:/namespace/prefix/whatever": "prefix_", "trimmed-prefix": "" }
 *     EXAMPLE: RDFHelper.help("rdf:addon-metadata", "urn:songbird:addon:root", RDFHelper.prototype.DEFAULT_RDF_NAMESPACES);
 * 
 *   RDFHelper.DEFAULT_RDF_NAMESPACES{_PREFIXED}:
 *     For convenience, provides rdf, mozilla, and songbird namespace objects that translate namespaces into object property prefixes.
 *     With _PREFIXED, your properties will begin with rdf_, moz_ or sb_. Without, namespaces will simply be removed.
 *     If you are concerned about collisions, be sure to map only one prefix to "".
 * 
 * USAGE EXAMPLE:
 * 
 * get the first display pane from the first addon and copy its properties into the "info" object. 
 * 
 * var addons = RDFHelper.help("rdf:addon-metadata", "urn:songbird:addon:root", RDFHelper.prototype.DEFAULT_RDF_NAMESPACES);
 * if (addons[0].displayPanes && addons[0].displayPanes[0].displayPane[0]) {
 *   var pane = addons[0].displayPanes[0].displayPane[0]
 *   var info = {};
 *   for (property in pane) {
 *     if (pane[property])
 *      info[property] = pane[property][0];
 *   }
 * }
 */

EXPORTED_SYMBOLS = [ "RDFHelper" ];

Ci = Components.interfaces;
Cc = Components.classes;

// make a constructor
function RDFHelper(aRdf, aDatasource, aResource, aNamespaces) {
  // this is a Crockfordian constructor with private methods.
  // see: http://www.crockford.com/javascript/private.html
  // the actual construction logic takes place after all these method def'ns.
  // this enables me to hide these methods from the outside world so that
  // users of the class can iterate over properties without seeing them.
  var that = this; // for use in private functions
  this.Value = aResource.Value; // TODO: is this Good Enough?
  
  _containerUtils = Cc["@mozilla.org/rdf/container-utils;1"]
                     .getService(Ci.nsIRDFContainerUtils);
  
  var createProperties = function() {
    //dump("Resource "+that.Value+" is a ")
    if (_containerUtils.IsContainer(aDatasource, aResource)) {
      //dump("container.\n");
      createContainerProperties(aResource);
    }
    else {
      //dump("normal resource.\n");
      createStandardProperties(aResource);
    }
  };
  
  var createContainerProperties = function(resource) {
    // dump("RDFHelper::createContainerProperties(resource)\n");
    var container = _containerUtils.MakeSeq(aDatasource, resource);
    var contents = container.GetElements();

    /*
    dump("RDFHelper::createContainerProperties -- aDatasource.URI = "+aDatasource.URI+"\n");
    dump("RDFHelper::createContainerProperties -- resource.ValueUTF8 = "+resource.ValueUTF8+"\n");

    dump("RDFHelper::createContainerProperties -- _containerUtils.IsEmpty(aDatasource, resource) = ");
    if (_containerUtils.IsEmpty(aDatasource, resource))
      dump("true\n");
    else
      dump("false\n");

    dump("RDFHelper::createContainerProperties -- _containerUtils.IsSeq(aDatasource, resource) = ");
    if (_containerUtils.IsSeq(aDatasource, resource)) 
      dump("true\n");
    else 
      dump("false\n");
    dump("RDFHelper::createContainerProperties -- container count = "+container.GetCount()+"\n");
    */

    // urgh, this doesn't actually mean "this" is an array 
    // but at least it's sort of like one.
    var i = 0;
    while (contents.hasMoreElements()) {
      var resource = contents.getNext()
      resource.QueryInterface(Ci.nsIRDFResource);
      that[i] = new RDFHelper(
        aRdf, 
        aDatasource, 
        resource, 
        aNamespaces
      ); 
      i++;
    }
    that.length = i;
  };
  
  var createStandardProperties = function(resource) {
    var labels = aDatasource.ArcLabelsOut(aResource);
    while(labels.hasMoreElements()) {
      var arc = labels.getNext().QueryInterface(Ci.nsIRDFResource)
      createStandardProperty(arc);
    }
  };
  
  var createStandardProperty = function(arc) {
    var alias = arc.Value;
    for (n in aNamespaces) {
       alias = alias.replace(n, aNamespaces[n]);
    }
    
    //dump("Arc "+arc.Value+" is a normal prop aliased to "+alias+".\n")
    
    // define a little get-result function
    // this lets us lazily evaluate resource links so that we don't
    // descend into infinite recursion unless we work at it and try
    // to depth-first search a cyclical graph or something
    var getResult = function() {
      ary = [];
      var itr = aDatasource.GetTargets(aResource, arc, true);
      while(itr.hasMoreElements()) {
        var resource = itr.getNext();
        if (resource instanceof Ci.nsIRDFLiteral) {
          //dump(resource.Value + "is a literal property for "+alias+".\n");
          ary.push(resource.Value);
        }
        else {
          //dump(resource.Value + " inserted an RDF property.\n");
          ary.push(new RDFHelper(
            aRdf, 
            aDatasource, 
            resource, 
            aNamespaces
          ));
        }
      } 
      // this turns out to be a really horrible idea if you want to write nice 
      // code.
      // but it's a nice idea and i'd like to talk to someone about whether or
      // not it's worth salvaging (pvh jan08) 
      /*if (ary.length == 1) {
        ary = ary[0];
      }*/
      
      // memoize the result to avoid n^2 iterations
      delete that[alias];
      that[alias] = ary;
      return ary;
    }
    
    that.__defineGetter__(alias, getResult);
    // TODO: do i really want to keep the original name?
    // i mean, really? it really ruins for (i in obj) syntax.
    /*if (alias != arc.Value) {
      that.__defineGetter__(arc.Value, getResult);
    }*/
  };
  
  createProperties();
}

RDFHelper.help = function(datasource, resource, namespaces) {
  // look up the initial values from the input strings.
  var rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
  var helper = new RDFHelper(
    rdf, 
    rdf.GetDataSourceBlocking(datasource), 
    rdf.GetResource(resource),
    namespaces
  );
  return helper;
};

RDFHelper.AMHelp = function(rdf, ds, res, namespaces) {
  var helper = new RDFHelper(
    rdf, 
    ds, 
    res,
    namespaces
  );
  return helper;
};

RDFHelper.DEFAULT_RDF_NAMESPACES = {
  "http://www.w3.org/1999/02/22-rdf-syntax-ns#": "",
  "http://www.mozilla.org/2004/em-rdf#": "",
  "http://www.songbirdnest.com/2007/addon-metadata-rdf#": ""
};

RDFHelper.DEFAULT_RDF_NAMESPACES_PREFIXED = {
  "http://www.w3.org/1999/02/22-rdf-syntax-ns#": "rdf_",
  "http://www.mozilla.org/2004/em-rdf#": "moz_",
  "http://www.songbirdnest.com/2007/addon-metadata-rdf#": "sb_"
};
