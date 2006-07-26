/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
 * Purpose:
 *		Wraps Servicesource nsIRDFDataSource to provide filtering and inclusion of
 *		additional nodes.
 */


const NS_RDF_NO_VALUE           = 0xa0002;
const NS_RDF_ASSERTION_REJECTED = 0xa0003;

var RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

const NC = "http://home.netscape.com/NC-rdf#";

const NC_SERVICESOURCE = RDF.GetResource("NC:Servicesource");
const NC_LABEL = RDF.GetResource(NC + "Label");
const NC_ICON = RDF.GetResource(NC + "Icon");
const NC_URL = RDF.GetResource(NC + "URL");
const NC_PROPS = RDF.GetResource(NC + "Properties");
const NC_OPEN = RDF.GetResource(NC + "Open");

const NC_CHILD = RDF.GetResource(NC + "child");
const NC_PULSE = RDF.GetResource(NC + "pulse");
const NC_INSTANCEOF = RDF.GetResource(NC + "instanceOf");
const NC_TYPE = RDF.GetResource(NC + "type");
const NC_NEXTVAL = RDF.GetResource(NC + "nextVal");
const NC_SEQ = RDF.GetResource(NC + "Seq");





function log(str) {
    //var consoleService = Components.classes['@mozilla.org/consoleservice;1']
    //                        .getService(Components.interfaces.nsIConsoleService);
    //consoleService.logStringMessage(str);
}



/**
 *	Wraps an array to provide an nsISimpleEnumerator
 */
function ArrayEnumerator(array) 
{
	this.data = array;
}
ArrayEnumerator.prototype = {

	index: 0,
	
	getNext: function() {
		return this.data[this.index++];
	},
	
	hasMoreElements: function() {						
		if (this.index < this.data.length)
			return true;
		else
			return false;
	},
	
	QueryInterface: function(iid)
	{
		if (!iid.equals(Components.interfaces.nsISimpleEnumerator))
			throw Components.results.NS_ERROR_NO_INTERFACE;
		return this;
	}
}








/**
 * Proxy around Servicesource that filters everything but bookmarks, library, and playlists, while splicing in 
 * a tree structure specified by the nodes param. See overlay.js for structure example.
 */
function ServicesourceProxy()
{
	this.mInner = RDF.GetDataSource("rdf:Servicesource");
	this.mInner.AddObserver(this);

	this.mObservers = new Array();
	
	this.filtering = false;
	
	this.nodes = { children: [], rdfResources: [] };
	
	// Hash table used to map RDF resource names to corresponding node info
	this.rdfLookup = {};
}


ServicesourceProxy.prototype = {


	setAdditionalNodes: function(nodes) {
		if (nodes == null) nodes = { children: [], rdfResources: [] };
	
		this.nodes = nodes;
		
		// Hash table used to map RDF resource names to corresponding node info
		this.rdfLookup = {};

		// Build rdf resources for everything in the new service tree
		this._createRDFResources();
	},
	
	enableFiltering: function(enabled) {
		this.filtering = enabled;
	},
	
	getNodeByResourceId: function (id) {
	  return this.rdfLookup[id];
	},

	/* nsIRDFDataSource */
	URI: "rdf:bookmarks-servicesource-proxy",

	GetSource: function(pred, obj, tv) {
		//log("GetSource: Pred: " + pred.Value + ", Obj: " + obj.Value + ", TV: " + tv); 
		return this.mInner.GetSource(pred, obj, tv);
	},

	GetSources: function(pred, obj, tv) {
		//log("GetSources: Pred: " + pred.Value + ", Obj: " + obj.Value + ", TV: " + tv); 
		return this.mInner.GetSources(pred, obj, tv);
	},

	GetTarget: function(subj, pred, tv) {
		//log("GetTarget: Subj: " + subj.Value + ", Pred: " + pred.Value + ", TV: " + tv); 		
		
		var outstring = null;
		
		// Check to see if this subject is one of the new nodes
		var node = this.rdfLookup[subj.Value];
		if (node != null) {
			if (pred == NC_LABEL) {
				outstring = node.label;
			} else if (pred == NC_ICON) {
				outstring = node.icon;
			} else if (pred == NC_URL) {
				outstring = node.url;
			} else if (pred == NC_PROPS) {
				outstring = node.properties;
			} else if (pred == NC_OPEN) {
			
			}
		}
		
		if (outstring != null) {
			return RDF.GetLiteral(outstring);
		}
		
		return this.mInner.GetTarget(subj, pred, tv);
	},



	GetTargets: function(subj, pred, tv) {
		//log("GetTargets: Subj: " + subj.Value + ", Pred: " + pred.Value + ", TV: " + tv); 
		
		if (subj == NC_SERVICESOURCE) 
		{
		    if (pred == NC_CHILD)
			{
				var originalEnumerator = this.mInner.GetTargets(subj, pred, tv);
				
				// Add additional nodes
				var items = this._filterNodes(originalEnumerator);
			  items = items.concat(this.nodes.rdfResources);
			
				// Wrap the items in an nsISimpleEnumerator
				var enumerator = new ArrayEnumerator(items);
				
				return enumerator;
			}
		} 
		else 
		{
			if (pred == NC_CHILD) 
			{
				var node = this.rdfLookup[subj.Value];
				if (node != null && node.rdfResources != null) 
				{
					return new ArrayEnumerator(node.rdfResources); 
				}
			}
		}
		
		return this.mInner.GetTargets(subj, pred, tv);
	},
	
	Assert: function(subj, pred, obj, tv) { 
	    //log("ServicesourceProxy::Assert");	    
	    return this.mInner.Assert(subj, pred, obj, tv);
	},
	
	Unassert: function(subj, pred, obj, tv)      {  
        //log("ServicesourceProxy::Unassert");	    
	    return this.mInner.Unassert(subj, pred, obj, tv);
	},
	
	Change: function(subj, pred, oldobj, newobj) { 
	    //log("ServicesourceProxy::Change");
	    return this.mInner.Change(subj, pred, oldobj, newobj);
	},
	
	Move: function(oldsubj, newsubj, pred, obj)  {
	    //log("ServicesourceProxy::Move");
	    return this.mInner.Move(oldsubj, newsubj, pred, obj);
	},


	HasAssertion: function(subj, pred, obj, tv) {
		//log("HasAssertion: Subj: " + subj.Value + ", Pred: " + pred.Value + ", Obj: " + obj.Value +", TV: " + tv);
		return this.mInner.HasAssertion(subj, pred, obj, tv);
	},

	AddObserver: function(aObserver) {
		this.mObservers.push(aObserver);
	},

	RemoveObserver: function(aObserver) {
		for (var i = 0; i < this.mObservers.length; ++i) {
			if (aObserver == this.mObservers[i]) {
				this.mObservers.splice(i, 1);
				break;
			}
		}
	},

	ArcLabelsIn: function(obj) {
		//log("ArcLabelsIn:  Obj: " + obj.Value);	
		return this.mInner.ArcLabelsIn(obj);
	},

	ArcLabelsOut: function(subj) {
		//log("ArcLabelsOut:  subj: " + subj.Value);	
		return this.mInner.ArcLabelsOut(obj);
	},

	GetAllResources: function() {
		//log("GetAllResources");	
		return this.mInner.GetAllResources();
	},

	hasArcIn: function(obj, pred) {
		//log("hasArcIn:  Pred: " + pred.Value + ", Obj: " + obj.Value);
		return this.mInner.hasArcIn(obj, pred);
	},

	hasArcOut: function(subj, pred) {
		//log("hasArcOut: Subj: " + subj.Value + ", Pred: " + pred.Value);
		
		var node = this.rdfLookup[subj.Value];
		if (node != null && node.rdfResources != null && node.rdfResources.length > 0) 
		{
			return (pred == NC_CHILD);
		}
		
		return this.mInner.hasArcOut(subj, pred);
	},

	/* nsIRDFObserver */
	onAssert: function(ds, subj, pred, obj) {
	    log("ServicesourceProxy::onAssert");
		for (var i =0; i < this.mObservers.length; i++) {
		    this.mObservers[obs].onAssert(this, subj, pred, obj);
		}
	},

	onUnassert: function(ds, subj, pred, obj) {
	    log("ServicesourceProxy::onUnassert");
		for (var i = 0; i < this.mObservers.length; i++) {
		    this.mObservers[i].onUnassert(this, subj, pred, obj);
		}
	},

	onChange: function(ds, subj, pred, oldobj, newobj) {
        log("ServicesourceProxy::onChange");	
		for (var i = 0; i < this.mObservers.length; i++) {
		    this.mObservers[i].onChange(ds, subj, pred, oldobj, newobj);
		}
	},

	onMove: function(ds, oldsubj, newsubj, pred, obj) {
	    log("ServicesourceProxy::onMove");
		for (var i = 0; i < this.mObservers.length; i++) {
		    this.mObservers[i].onMove(ds, oldsubj, newsubj, pred, obj);
		}
	},	
	finish: function() {
		// XXX better way to do this? We need to break the circularity
		// between DS and DS.mInner.
		this.mInner.RemoveObserver(this);
	},
	
	onBeginUpdateBatch: function(ds) {
		log("ServicesourceProxy::onBeginUpdateBatch");
		for (var i = 0; i < this.mObservers.length; i++) {
		    this.mObservers[i].onBeginUpdateBatch(this);
		}
	},

	onEndUpdateBatch: function(ds) {
	    log("ServicesourceProxy::onEndUpdateBatch");
		for (var i = 0; i < this.mObservers.length; i++) {
		    this.mObservers[i].onEndUpdateBatch(this);
		}
	},
	
	
	/**
	 * Annotate sidebar resource tree with rdf nodes.
	 */ 
	_createRDFResources: function() {
		this.nodes.rdfResources = [];
		for (var i = 0; i < this.nodes.children.length; i++) 
		{
			this.nodes.rdfResources[i] = RDF.GetAnonymousResource();
			this.rdfLookup[ this.nodes.rdfResources[i].Value ] = this.nodes.children[i] 
			
			if (this.nodes.children[i].children.length > 0) 
			{
				this.nodes.children[i].rdfResources = [];
				for (var j = 0; j < this.nodes.children[i].children.length; j++) 
				{
					this.nodes.children[i].rdfResources[j] = RDF.GetAnonymousResource();
					this.rdfLookup[ this.nodes.children[i].rdfResources[j].Value ] = this.nodes.children[i].children[j];
				}
			}
		}
	},
	
	
	

	
	/**
	 * Given an nsISimpleEnumerator of nsIRDFResources, creates an array of all items
	 * that should remain on the sidebar
	 */
	_filterNodes: function(enumerator) {
		var selectedItems = [];
			
		// Walk all items and make a list of the ones that we want
		while (enumerator.hasMoreElements()) {
			var item = enumerator.getNext();
			
			// Perform lookup to see what this item is..
			item = item.QueryInterface(Components.interfaces.nsIRDFResource);
			
			
			var icon = this.mInner.GetTarget(item, NC_ICON, true);

			// If it has an icon, we may not want to keep it
			if (icon != null && this.filtering) {
				icon = icon.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
				
				// If icon is a folder, then we don't want to display it
				if (icon != "chrome://songbird/skin/default/icon_folder.png") {
					selectedItems.push(item);
				} 
			// No icon.. probably a playlist
			} else {
				selectedItems.push(item);
			}
		}
		
		return selectedItems;
	}	

};