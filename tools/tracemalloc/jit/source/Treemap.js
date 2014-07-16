/*
 * File: Treemap.js
 * 
 * Author: Nicolas Garcia Belmonte
 * 
 * Copyright: Copyright 2008 by Nicolas Garcia Belmonte.
 * 
 * License: BSD License
 * 
 * Homepage: <http://thejit.org>
 * 
 * Version: 1.0.7a
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Nicolas Garcia Belmonte ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Nicolas Garcia Belmonte BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/*
   Object: Config

   Treemap global configuration object. Contains important properties to enable customization and proper behavior of treemaps.
*/

var Config = {
	//Property: tips
	//Enables tips for the Treemap
	tips: false,
	//Property: titleHeight
	//The height of the title. Set this to zero and remove all styles for node classes if you just want to show leaf nodes.
	titleHeight: 13,
	//Property: rootId
	//The id of the main container box. That is, the div that will contain this visualization. This div has to be explicitly added on your page.
	rootId: 'infovis',
	//Property: offset
	//Offset distance between nodes. Works better with even numbers. Set this to zero if you only want to show leaf nodes.
	offset:4,
	//Property: levelsToShow
	//Depth of the plotted tree. The plotted tree will be pruned in order to fit with the specified depth. Useful when using the "request" method on the controller.
	levelsToShow: 3,
	//Property: Color
	//Configuration object to add some color to the leaves.
	Color: {
		//Property: allow
		//Set this to true if you want to add color to the nodes. Color will be based upon the second "data" JSON object passed to the node. If your node has a "data" property which has at least two key-value objects, color will be based on your second key-value object value.
		allow: false,
		//Property: minValue
		//We need to know the minimum value of the property which will be taken in account to color the leaves.
		minValue: -100,
		//Property: maxValue
		//We need to know the maximum value of the property which will be taken in account to color the leaves.
		maxValue: 100,
		//Property: minColorValue
		//The color to be applied when displaying a min value (RGB format).
		minColorValue: [255, 0, 50],
		//Property: maxColorValue
		//The color to be applied when displaying a max value (RGB format).
		maxColorValue: [0, 255, 50]
	}
};


/*
   Object: TreeUtil

   An object containing some common tree manipulation methods.
*/
var TreeUtil = {

	/*
	   Method: prune
	
	   Clears all tree nodes having depth greater than maxLevel.
	
	   Parameters:
	
	      tree - A JSON tree object. <http://blog.thejit.org>
	      maxLevel - An integer specifying the maximum level allowed for this tree. All nodes having depth greater than max level will be deleted.
	
	*/
	prune: function(tree, maxLevel) {
		this.each(tree, function(elem, i) {
			if(i == maxLevel && elem.children) {
				delete elem.children;
				elem.children = [];
			}
		});
	},
	
	/*
	   Method: getParent
	
	   Returns the parent node of the node having _id_.
	
	   Parameters:
	
	      tree - A JSON tree object. <http://blog.thejit.org>
	      id - The _id_ of the child node whose parent will be returned.
	
	*/
	getParent: function(tree, id) {
		if(tree.id == id) return false;
		var ch = tree.children;
		if(ch && ch.length > 0) {
			for(var i=0; i<ch.length; i++) {
				if(ch[i].id == id) 
					return tree;
				else {
					var ans = this.getParent(ch[i], id);
					if(ans) return ans;
				}
			}
		}
		return false;		
	},

	/*
	   Method: getSubtree
	
	   Returns the subtree that matches the given id.
	
	   Parameters:
	
		  tree - A JSON tree object. <http://blog.thejit.org>
	      id - A node *unique* identifier.
	
	   Returns:
	
	      A subtree having a root node matching the given id. Returns null if no subtree matching the id is found.
	*/
	getSubtree: function(tree, id) {
		if(tree.id == id) return tree;
		for(var i=0; i<tree.children.length; i++) {
			var t = this.getSubtree(tree.children[i], id);
			if(t != null) return t;
		}
		return null;
	},

	/*
	   Method: getLeaves
	
		Returns the leaves of the tree.
	
	   Parameters:
	
	      node - A tree node (which is also a JSON tree object of course). <http://blog.thejit.org>
	
	   Returns:
	
	   An array having objects with two properties. The _node_ property contains the leaf node. The _level_ property specifies the depth of the node.
	*/
	getLeaves: function (node) {
		var leaves = new Array(), levelsToShow = Config.levelsToShow;
		this.each(node, function(elem, i) {
			if(i <= levelsToShow && (!elem.children || elem.children.length == 0 )) {
				leaves.push({
					'node':elem,
					'level':levelsToShow - i
				});
			}
		});
		return leaves;
	},

	/*
	   Method: resetPath
	
		Removes the _.in-path_ className for all tree dom elements and then adds this className to all ancestors of the given subtree.
	
	   Parameters:
	
	      tree - A tree node (which is also a JSON tree object of course). <http://blog.thejit.org>
	*/
	resetPath: function(tree) {
		var root = Config.rootId;
		var selector = "#" + root + " .in-path";
		$$(selector).each(function (elem) {
			elem.removeClass("in-path");
		});
		var container = $(tree.id);
		var getParent = function(c) { 
			var p = c.getParent();
			if(p && p.id != root) return p;
			return false;
		 };
		var _parent = (tree)? getParent(container) : false;
		while(_parent) {
			_parent.getFirst().addClass("in-path");
			_parent = getParent(_parent);
		}
	},

	/*
	   Method: eachLevel
	
		Iterates on tree nodes which relative depth is less or equal than a specified level.
	
	   Parameters:
	
	      tree - A JSON tree or subtree. <http://blog.thejit.org>
	      initLevel - An integer specifying the initial relative level. Usually zero.
	      toLevel - An integer specifying a top level. This method will iterate only through nodes with depth less than or equal this number.
	      action - A function that receives a node and an integer specifying the actual level of the node.
	      	
	*/
	eachLevel: function(tree, initLevel, toLevel, action) {
		if(initLevel <= toLevel) {
			action(tree, initLevel);
			var ch= tree.children;
			for(var i=0; i<ch.length; i++) {
				this.eachLevel(ch[i], initLevel +1, toLevel, action);	
			}
		}
	},

	/*
	   Method: each
	
		A tree iterator.
	
	   Parameters:
	
	      tree - A JSON tree or subtree. <http://blog.thejit.org>
	      action - A function that receives a node.
	      	
	*/
	each: function(tree, action) {
		this.eachLevel(tree, 0, Number.MAX_VALUE, action);
	}
};

/*
   Class: TreeUtil.Group
	
   A class that performs actions on group of nodes.

*/
TreeUtil.Group = new Class({

	/*
	   Method: initialize
	
		<TreeUtil.Group> constructor.
	
	   Parameters:
	
	      nodeArray - An array of tree nodes. <http://blog.thejit.org>
	      controller - A treemap controller. <http://blog.thejit.org/?p=8>

	   Returns:
	
	   	  A new <TreeUtil.Group> instance.
 
	*/
	initialize: function(nodeArray, controller) {
		this.array= nodeArray;
		this.objects= new Array();
		this.controller = controller;
		this.counter = 0;
	},

	/*
	   Method: loadNodes
	
		Uses a controller _request_ method to make a request for each node.
	
	   Parameters:
	
	      level - An integer array that specifies the maximum level of the subtrees to be requested.
	      onComplete - A controller having an onComplete method. This method will be triggered after all requests have been completed -i.e after a response was been received for each request. 
	*/
	loadNodes: function (onComplete) {
		var that = this, len = this.array.length, complete = $merge({ onComplete: $lambda() }, onComplete);
		this.counter = 0;
		var selectedNode = {};
		if(len > 0) {
			for(var i=0; i<len; i++) {
				selectedNode[this.array[i].node.id] = this.array[i];
				this.controller.request(this.array[i].node.id, this.array[i].level, {
					onComplete: function(nodeId, tree) {
						for(var j=0; j<tree.children.length; j++) {
							tree.children[j]._parent = selectedNode[nodeId];
						}
						selectedNode[nodeId].children = tree.children;
						if(++that.counter == len) {
							complete.onComplete();
						}
					}
				});
			}
		} else {
			complete.onComplete();
		}		
	}
});

/*
   Object: TM

	Abstract Treemap class. Squarified and Slice and Dice Treemaps will extend this class.
*/
var TM = {

	layout: {
		orientation: "v",
		vertical: function() { return this.orientation == "v"; },
		horizontal: function() { return this.orientation == "h"; },
		change: function() { this.orientation = this.vertical()? "h" : "v"; }
	},
	
	innerController: {
			onBeforeCompute: $lambda(),
			onAfterCompute:  $lambda(),
			onCreateLabel:   $lambda(),
			onPlaceLabel:    $lambda(),
			onCreateElement: $lambda(),
			onComplete:      $lambda(),
			onBeforePlotLine: $lambda(),
			onAfterPlotLine: $lambda(),
			request:         false
		},
		
		config: Config,	

	/*
	   Method: toStyle
	
		Transforms a JSON into a CSS style string.
	*/
	toStyle: function(obj) {
		var stringStyle = "";
		for(var styleKey in obj) stringStyle+= styleKey + ":" + obj[styleKey] + ";";
		return stringStyle;
	},

	/*
	   Method: leaf
	
		Returns a boolean value specifying if the node is a tree leaf or not.
	
	   Parameters:
	
	      tree - A tree node (which is also a JSON tree object of course). <http://blog.thejit.org>

	   Returns:
	
	   	  A boolean value specifying if the node is a tree leaf or not.
 
	*/
	leaf: function(tree) {
		return (tree.children == 0);
	},

	/*
	   Method: createBox
	
		Constructs the proper DOM layout from a json node.
		
		If this node is a leaf, then it creates a _leaf_ div dom element by calling <TM.newLeaf>. Otherwise it creates a content div dom element that contains <TM.newHead> and <TM.newBody> elements.
	
	   Parameters:

		  injectTo - A DOM element where this new DOM element will be injected.	
	      json - A JSON subtree. <http://blog.thejit.org>
		  coord - A coordinates object specifying width, height, left and top style properties.

	*/
	createBox: function(json, coord, html) {
		if(!this.leaf(json))
			var box = this.newHeadHTML(json, coord) + this.newBodyHTML(html, coord);
		 else 
			var box = this.newLeafHTML(json, coord);
		return this.newContentHTML(json, coord, box);
	},
	
	/*
	   Method: plot
	
		Plots a Treemap
	*/
	plot: function(json) {
		var c = (json._coord)? json._coord : json.coord;
		if(this.leaf(json)) return this.createBox(json, c, null);
		for(var i=0, html = "", ch = json.children; i<ch.length; i++) html+= this.plot(ch[i]);
		return this.createBox(json, c, html);
	},


	/*
	   Method: newHeadHTML
	
		Creates the _head_ div dom element that usually contains the name of a parent JSON tree node.
	
	   Parameters:
	
	      json - A JSON subtree. <http://blog.thejit.org>
	      coord - width and height base coordinates

	   Returns:
	
	   	  A new _head_ div dom element that has _head_ as class name.
 
	*/
	newHeadHTML: function(json, coord) {
		var config = this.config;
		var c = {
			'height': config.titleHeight + "px",
			'width': (coord.width - config.offset) + "px",
			'left': config.offset/2 + "px"
		};
		return "<div class=\"head\" style=\"" + this.toStyle(c) + "\">"
				 + json.name + "</div>";
	},

	/*
	   Method: newBodyHTML
	
		Creates the _body_ div dom element that usually contains a subtree dom element layout.
	
	   Parameters:
	
	      html - html that should be contained in the body html.

	   Returns:
	
	   	  A new _body_ div dom element that has _body_ as class name.
 
	*/
	newBodyHTML: function(html, coord) {
		var config = this.config;
		var c = {
			'width': (coord.width - config.offset) + "px",
			'height': (coord.height - config.offset - config.titleHeight) + "px",
			'top': (config.titleHeight + config.offset/2) +  "px",
			'left': (config.offset / 2) + "px"
		};
		return "<div class=\"body\" style=\""+ this.toStyle(c) +"\">" + html + "</div>";
	},



	/*
	   Method: newContentHTML
	
		Creates the _content_ div dom element that usually contains a _leaf_ div dom element or _head_ and _body_ div dom elements.
	
	   Parameters:
	
	      json - A JSON node. <http://blog.thejit.org>
	      coord - An object containing width, height, left and top coordinates.
	      html - input html wrapped by this tag.
	      
	   Returns:
	
	   	  A new _content_ div dom element that has _content_ as class name.
 
	*/
	newContentHTML: function(json, coord, html) {
		var c = {};
		for(var i in coord) c[i]= coord[i] + "px";
		return "<div class=\"content\" style=\"" + this.toStyle(c) + "\" id=\"" + json.id + "\">" + html + "</div>";
	},


	/*
	   Method: newLeafHTML
	
		Creates the _leaf_ div dom element that usually contains nothing else.
	
	   Parameters:
	
	      json - A JSON subtree. <http://blog.thejit.org>
	      coord - base with and height coordinates
	      
	   Returns:
	
	   	  A new _leaf_ div dom element having _leaf_ as class name.
 
	*/
	newLeafHTML: function(json, coord) {
		var config = this.config;
		var backgroundColor = (config.Color.allow)? this.setColor(json) : false, 
		width = coord.width - config.offset,
		height = coord.height - config.offset;
		var c = {
			'top': (config.offset / 2)  + "px",
			'height':height + "px",
			'width':width + "px",
			'left': (config.offset/2) + "px"
		};
		if(backgroundColor) c['background-color'] = backgroundColor;
		return "<div class=\"leaf\" style=\"" + this.toStyle(c) + "\">" + json.name + "</div>";
	},


	/*
	   Method: setColor
	
		A JSON tree node has usually a data property containing an Array of key-value objects. This method takes the second key-value object from that array, returning a string specifying a color relative to the value property of that object.
	
	   Parameters:
	
	      json - A JSON subtree. <http://blog.thejit.org>

	   Returns:
	
	   	  A String that represents a color in hex value.
 
	*/
	setColor: function(json) {
		var c = this.config.Color;
		var x = json.data[1].value.toFloat();
		var comp = function(i, x) {return ((c.maxColorValue[i] - c.minColorValue[i]) / (c.maxValue - c.minValue)) * (x - c.minValue) + c.minColorValue[i]};
		var newColor = new Array();
		newColor[0] = comp(0, x).toInt(); newColor[1] = comp(1, x).toInt(); newColor[2] = comp(2, x).toInt();
		return newColor.rgbToHex();
	},

	/*
	   Method: enter
	
		Sets the _elem_ parameter as root and performs the layout.
	
	   Parameters:
	
	      elem - A JSON subtree. <http://blog.thejit.org>
	*/
	enter: function(elem) {
		var id = elem.getParent().id, paramNode = TreeUtil.getSubtree(this.tree, id);
		this.post(paramNode, id, this.getPostRequestHandler(id));
	},
	
	/*
	   Method: out
	
		Takes the _parent_ node of the currently shown subtree and performs the layout.
	
	*/
	out: function() {
		var _parent = TreeUtil.getParent(this.tree, this.shownTree.id);
		if(_parent) {
			if(this.controller.request)
				TreeUtil.prune(_parent, this.config.levelsToShow);
			this.post(_parent, _parent.id, this.getPostRequestHandler(_parent.id));
		}
	},
	
	/*
	   Method: getPostRequestHandler
	
		Called to prepare some values before performing a <TM.enter> or <TM.out> action.
	
	   Parameters:
	
		  id - A node identifier

	   Returns:
	
	   	  A _postRequest_ object.
 
	*/
	getPostRequestHandler: function(id) {
		var config = this.config, that = this;
		if(config.tips) this.tips.hide();
		return  postRequest = {
			onComplete: function() {
				that.loadTree(id);
				$(config.rootId).focus();
			}
		};
	},

	/*
	   Method: post
	
		Called to perform post operations after doing a <TM.enter> or <TM.out> action.
	
	   Parameters:
	
	      _self - A <TM.Squarified> or <TM.SliceAndDice> instance.
	      paramNode - A JSON subtree. <http://blog.thejit.org>
		  id - A node identifier
		  postRequest - A _postRequest_ object.
	*/
	post: function(paramNode, id, postRequest) {
		if(this.controller.request) {
			var leaves = TreeUtil.getLeaves(TreeUtil.getSubtree(this.tree, id));
			var g = new TreeUtil.Group(leaves, this.controller).loadNodes(postRequest);
		} else setTimeout(function () { postRequest.onComplete(); }, 1); //deferred command
	},
	
	/*
	   Method: initializeBehavior
	
		Binds different methods to dom elements like tips, color changing, adding or removing class names on mouseenter and mouseleave, etc.
	
	   Parameters:
	
	      _self - A <TM.Squarified> or <TM.SliceAndDice> instance.
	*/
	initializeBehavior: function () {
		var elems = $$('.leaf', '.head'), that = this;
		if(this.config.tips) this.tips = new Tips(elems, {
										className: 'tool-tip',
										showDelay: 0,
										hideDelay: 0
									});
		
		elems.each(function(elem) {
			elem.oncontextmenu = $lambda(false);
			elem.addEvents({
				'mouseenter': function(e) {
					var id = false;
					if(elem.hasClass("leaf")) {
						id = elem.getParent().id;
						elem.addClass("over-leaf");
					}
					else if (elem.hasClass("head")) {
						id = elem.getParent().id;
						elem.addClass("over-head");
						elem.getParent().addClass("over-content");
					}
					if(id) {
						var tree = TreeUtil.getSubtree(that.tree, id);
						TreeUtil.resetPath(tree);
					}
					e.stopPropagation();
				},
				
				'mouseleave': function(e) {
					if(elem.hasClass("over-leaf")) elem.removeClass("over-leaf");
					else if (elem.getParent().hasClass("over-content")){
						 elem.removeClass("over-head");
						 elem.getParent().removeClass("over-content");
					}
					TreeUtil.resetPath(false);
					e.stopPropagation();
				},
				
				'mouseup': function(e) {
					if(e.rightClick) that.out(); else that.enter(elem);
					e.preventDefault();
					return false;
				}
			});
		});
	},
	
	/*
	   Method: loadTree
	
		Loads the subtree specified by _id_ and plots it on the layout container.
	
	   Parameters:
	
	      id - A subtree id.
	*/
	loadTree: function(id) {
		$(this.config.rootId).empty();
		this.loadFromJSON(TreeUtil.getSubtree(this.tree, id));
	}
	
};

/*
   Class: TM.SliceAndDice

	A JavaScript implementation of the Slice and Dice Treemap algorithm.

	Go to <http://blog.thejit.org> to know what kind of JSON structure feeds this object.
	
	Go to <http://blog.thejit.org/?p=8> to know what kind of controller this class accepts.
	
	Refer to the <Config> object to know what properties can be modified in order to customize this object. 

	The simplest way to create and layout a slice and dice treemap from a JSON object is:
	
	(start code)

	var tm = new TM.SliceAndDice();
	tm.loadFromJSON(json);

	(end code)

*/
TM.SliceAndDice = new Class({
	Implements: TM,
	
	/*
	   Method: initialize
	
		<TM.SliceAndDice> constructor.
	
	   Parameters:
	
	      controller - A treemap controller. <http://blog.thejit.org/?p=8>
	   
	   Returns:
	
	   	  A new <TM.SliceAndDice> instance.
 
	*/
	initialize: function (controller) {
		//Property: tree
		//The JSON tree. <http://blog.thejit.org>
		this.tree = null;
		//Property: showSubtree
		//The displayed JSON subtree. <http://blog.thejit.org>
		this.shownTree = null;
		//Property: tips
		//This property will hold a Mootools Tips instance if specified.
		this.tips = null;
		//Property: controller
		//A treemap controller <http://blog.thejit.org/?p=8>
		this.controller = $merge(this.innerController, controller);
		//Property: rootId
		//Id of the Treemap container
		this.rootId = Config.rootId;
		
	},

	/*
	   Method: loadFromJSON
	
		Loads the specified JSON tree and lays it on the main container.
	
	   Parameters:
	
	      json - A JSON subtree. <http://blog.thejit.org>
	*/
	loadFromJSON: function (json) {
		var container = $(this.rootId), config = this.config;
		var p = {};
		p.coord = {
			'top': 0,
			'left': 0,
			'width':  container.offsetWidth,
			'height': container.offsetHeight + config.titleHeight + config.offset
		};
		if(this.tree == null) this.tree = json;
		this.shownTree = json;
		this.loadTreeFromJSON(p, json, this.layout.orientation);
		container.set('html', this.plot(json))
		this.initializeBehavior(this);
		this.controller.onAfterCompute(json);
	},
	
	/*
	   Method: loadTreeFromJSON
	
		Called by loadFromJSON to calculate recursively all node positions and lay out the tree.
	
	   Parameters:

	      _parent - The parent node of the json subtree.	
	      json - A JSON subtree. <http://blog.thejit.org>
	      orientation - The currently <Layout> orientation. This value is switched recursively.
	*/
	loadTreeFromJSON: function(par, json, orientation) {
		var config = this.config;
		var width  = par.coord.width - config.offset;
		var height = par.coord.height - config.offset - config.titleHeight;
		var fact = (par.data && par.data.length > 0)? json.data[0].value / par.data[0].value : 1;
		var horizontal = (orientation == "h");
		if(horizontal) {
			var size = (width * fact).round();
			var otherSize = height;
		} else {
			var otherSize = (height * fact).round();
			var size = width;
		}
		json.coord = {
			'width':size,
			'height':otherSize,
			'top':0,
			'left':0
		};

		orientation = (horizontal)? "v" : "h";		
		var dim =     (!horizontal)? 'width' : 'height';
		var pos =     (!horizontal)? 'left' : 'top';
		var pos2 =   (!horizontal)? 'top' : 'left';
		var offsetSize =0;
		
		var tm = this;
		json.children.each(function(elem){
			tm.loadTreeFromJSON(json, elem, orientation);
			elem.coord[pos] = offsetSize;
			elem.coord[pos2] = 0;
			offsetSize += elem.coord[dim].toInt();
		});
	}
});


/*
   Class: TM.Squarified

	A JavaScript implementation of the Squarified Treemap algorithm.
	
	Go to <http://blog.thejit.org> to know what kind of JSON structure feeds this object.
	
	Go to <http://blog.thejit.org/?p=8> to know what kind of controller this class accepts.
	
	Refer to the <Config> object to know what properties can be modified in order to customize this object. 

	The simplest way to create and layout a Squarified treemap from a JSON object is:
	
	(start code)

	var tm = new TM.Squarified();
	tm.loadFromJSON(json);

	(end code)
	
*/
	
TM.Squarified = new Class({
	Implements: TM,
	/*
	   Method: initialize
	
		<TM.Squarified> constructor.
	
	   Parameters:
	
	      controller - A treemap controller. <http://blog.thejit.org/?p=8>
	   
	   Returns:
	
	   	  A new <TM.Squarified> instance.
 
	*/
	initialize: function(controller) {
		//Property: tree
		//The JSON tree. <http://blog.thejit.org>
		this.tree = null;
		//Property: showSubtree
		//The displayed JSON subtree. <http://blog.thejit.org>
		this.shownTree = null;
		//Property: tips
		//This property will hold the a Mootools Tips instance if specified.
		this.tips = null;
		//Property: controller
		//A treemap controller <http://blog.thejit.org/?p=8>
		this.controller = $merge(this.innerController, controller);
		//Property: rootId
		//Id of the Treemap container
		this.rootId = Config.rootId;
	},

	/*
	   Method: loadFromJSON
	
		Loads the specified JSON tree and lays it on the main container.
	
	   Parameters:
	
	      json - A JSON subtree. <http://blog.thejit.org>
	*/
	loadFromJSON: function (json) {
		this.controller.onBeforeCompute(json);
		var infovis = $(Config.rootId);
		json.coord =  {
			'height': infovis.offsetHeight - Config.titleHeight,
			'width':infovis.offsetWidth,
			'top': 0,
			'left': 0
		};
		json._coord =  {
			'height': infovis.offsetHeight,
			'width':infovis.offsetWidth,
			'top': 0,
			'left': 0
		};
		this.loadTreeFromJSON(false, json, json.coord);
		infovis.set('html', TM.plot(json));
		if(this.tree == null) this.tree = json;
		this.shownTree = json;
		this.initializeBehavior(this);
		this.controller.onAfterCompute(json);
	},


	/*
	   Method: worstAspectRatio
	
		Calculates the worst aspect ratio of a group of rectangles. <http://en.wikipedia.org/wiki/Aspect_ratio>
		
	   Parameters:

		  children - An array of nodes.	
	      w - The fixed dimension where rectangles are being laid out.

	   Returns:
	
	   	  The worst aspect ratio.
 

	*/
	worstAspectRatio: function(children, w) {
		if(!children || children.length == 0) return Number.MAX_VALUE;
		var areaSum = 0, maxArea = 0, minArea = Number.MAX_VALUE;
		(function (ch) {
			for(var i=0; i<ch.length; i++) {
				var area = ch[i]._area;
				areaSum += area; 
				minArea = (minArea < area)? minArea : area;
				maxArea = (maxArea > area)? maxArea : area; 
			}
		})(children);
		
		return Math.max(w * w * maxArea / (areaSum * areaSum),
						areaSum * areaSum / (w * w * minArea));
	},

	/*
	   Method: loadTreeFromJSON
	
		Called by loadFromJSON to calculate recursively all node positions and lay out the tree.
	
	   Parameters:

	      _parent - The parent node of the json subtree.	
	      json - A JSON subtree. <http://blog.thejit.org>
		  coord - A coordinates object specifying width, height, left and top style properties.
	*/
	loadTreeFromJSON: function(_parent, json, coord) {
		if (!(coord.width >= coord.height && this.layout.horizontal())) this.layout.change();
		var ch = json.children, config = this.config;
		if(ch.length > 0) {
			this.processChildrenLayout(json, ch, coord);
			for(var i=0; i<ch.length; i++) {
				var height = ch[i].coord.height - (config.titleHeight + config.offset);
				var width = ch[i].coord.width - config.offset;
				ch[i]._coord = {
					'width': ch[i].coord.width,
					'height': ch[i].coord.height,
					'top': ch[i].coord.top,
					'left': ch[i].coord.left
				};
				ch[i].coord = {
					'width':width,
					'height':height,
					'top':0,
					'left':0
				};
				this.loadTreeFromJSON(json, ch[i], ch[i].coord);
			}
		}
	},

	/*
	   Method: processChildrenLayout
	
		Computes children real areas and other useful parameters for performing the Squarified algorithm.
	
	   Parameters:

	      _parent - The parent node of the json subtree.	
	      ch - An Array of nodes
		  coord - A coordinates object specifying width, height, left and top style properties.
	*/
	processChildrenLayout: function(_parent, ch, coord) {
		//compute children real areas
		(function (par, ch) {
			var parentArea = par.coord.width * par.coord.height;
			var parentDataValue = par.data[0].value.toFloat();
			for(var i=0; i<ch.length; i++) {
				ch[i]._area = parentArea * ch[i].data[0].value.toFloat() / parentDataValue;
			}
		})(_parent, ch);
		var minimumSideValue = (this.layout.horizontal())? coord.height : coord.width;
		ch.sort(function(a, b) {
			if(a._area < b._area) return 1;
			if(a._area == b.area) return 0;
			return -1;
		});
		var initElem = [ch[0]];
		var tail = ch.slice(1);
		this.squarify(tail, initElem, minimumSideValue, coord);
	},

	/*
	   Method: squarify
	
		Performs a heuristic method to calculate div elements sizes in order to have a good aspect ratio.
	
	   Parameters:

	      tail - An array of nodes.	
	      initElem - An array of nodes
	      w - A fixed dimension where nodes will be layed out.
		  coord - A coordinates object specifying width, height, left and top style properties.
	*/
	squarify: function(tail, initElem, w, coord) {
		if(tail.length + initElem.length == 1) {
			if(tail.length == 1) this.layoutLast(tail, w, coord);
			else this.layoutLast(initElem, w, coord);
			return;
		}
		if(tail.length >= 2 && initElem.length == 0) {
			initElem = [tail[0]];
			tail = tail.slice(1);
		}
		if(tail.length == 0) {
			if(initElem.length > 0) this.layoutRow(initElem, w, coord);
			return;
		}
		var c = tail[0];
		if(this.worstAspectRatio(initElem, w) >= this.worstAspectRatio([c].concat(initElem), w)) {
			this.squarify(tail.slice(1), initElem.concat([c]), w, coord);
		} else {
			var newCoords = this.layoutRow(initElem, w, coord);
			this.squarify(tail, [], newCoords.minimumSideValue, newCoords);
		}
	},
	
	/*
	   Method: layoutLast
	
		Performs the layout of the last computed sibling.
	
	   Parameters:

	      ch - An array of nodes.	
	      w - A fixed dimension where nodes will be layed out.
		  coord - A coordinates object specifying width, height, left and top style properties.
	*/
	layoutLast: function(ch, w, coord) {
		ch[0].coord = coord;
	},

	/*
	   Method: layoutRow
	
		Performs the layout of an array of nodes.
	
	   Parameters:

	      ch - An array of nodes.	
	      w - A fixed dimension where nodes will be layed out.
		  coord - A coordinates object specifying width, height, left and top style properties.
	*/
	layoutRow: function(ch, w, coord) {
		var totalArea = (function (ch) {
			for(var i=0, collect = 0; i<ch.length; i++) {
				collect += ch[i]._area;
			}
			return collect;
		})(ch);

		var sideA = (this.layout.horizontal())? 'height' : 'width';
		var sideB = (this.layout.horizontal())? 'width' : 'height';
		var otherSide = (totalArea / w).round();
		var top = (this.layout.vertical())? coord.height - otherSide : 0; 
		var left = 0;
		for(var i=0; i<ch.length; i++) {
			var chCoord = {};
			chCoord[sideA] = (ch[i]._area / otherSide).round();
			chCoord[sideB] = otherSide;
			chCoord['top'] = (this.layout.horizontal())? coord.top + (w - chCoord[sideA] - top) : top;
			chCoord['left'] = (this.layout.horizontal())? coord.left : coord.left + left;
			ch[i].coord = chCoord;
			if(this.layout.horizontal()) top += chCoord[sideA]; else left += chCoord[sideA];
		}
		var newCoords = {};
		newCoords[sideB] = coord[sideB] - otherSide;
		newCoords[sideA] = coord[sideA];
		newCoords['left'] = (this.layout.horizontal())? coord.left + otherSide : coord.left;
		newCoords['top'] = coord.top;
		newCoords.minimumSideValue = (newCoords[sideB] > newCoords[sideA])? newCoords[sideA] : newCoords[sideB];
		if (newCoords.minimumSideValue != newCoords[sideA]) this.layout.change();
		return newCoords;
	}
});

