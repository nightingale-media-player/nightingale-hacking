/*
 * File: Spacetree.js
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
   Object: $_

   Provides some common utility functions.
*/
var $_ = {
	fn: function() { return function() {}; },

	merge: function(){
		var mix = {};
		for (var i = 0, l = arguments.length; i < l; i++){
			var object = arguments[i];
			if (typeof object != 'object') continue;
			for (var key in object){
				var op = object[key], mp = mix[key];
				mix[key] = (mp && typeof op == 'object' && typeof mp == 'object') ? this.merge(mp, op) : this.unlink(op);
			}
		}
		return mix;
	},

	unlink: function (object){
		var unlinked = null;
		
		switch (typeof object){
			case 'object':
				unlinked = {};
				for (var p in object) unlinked[p] = this.unlink(object[p]);
			break;
			case 'array':
				unlinked = [];
				for (var i = 0, l = object.length; i < l; i++) unlinked[i] = this.unlink(object[i]);
			break;
			default: return object;
		}
		
		return unlinked;
	}
} ;

/*
   Object: Config

   <ST> global configuration object. Contains important properties to enable customization and proper behavior for the <ST>.
*/

var Config= {
	//Property: orientation
	//Sets the orientation layout. Implemented orientations are _left_ (the root node will be placed on the left side of the screen) or _top_ (the root node will be placed on top of the screen).
	orientation:             "left",
	//Property: labelContainer
	//The id for the label container. The label container should be a div dom element where label div dom elements will be injected. You have to put the label container div dom element explicitly on your page to run the <ST>.
	labelContainer: 'label_container',
	//Property: levelsToShow
	//Depth of the plotted tree. The plotted tree will be pruned in order to fit with the specified depth. Useful when using the "request" method on the controller.
	levelsToShow: 2,
	//Property: offsetBase
	//Separation offset between nodes.
	offsetBase:				 8,
	//Property: Label
	//Configuration object to customize labels size and offset.		
	Label: {
		//Property: height
		//Label height (offset included)
		height:       26,
		//Property: realHeight
		//Label realHeight (offset excluded)
		realHeight:   20,			
		//Property: width
		//Label width (offset included)
		width:        95,
		//Property: realWidth
		//Label realWidth (offset excluded)
		realWidth:    90,
		//Property: offsetHeight
		//Used on the currently expanded subtree. Adds recursively offsetHeight between nodes for each expanded level.
		offsetHeight: 30,
		//Property: offsetWidth
		//Used on the currently expanded subtree. Adds recursively offsetWidth between nodes for each expanded level.
		offsetWidth:  30
	},
	//Property: Node
	//Configuration object to customize node styles. Use <Config.Label> to configure node width and height.
	Node: {
		//Property: strokeStyle
		//If the node <Config.Node.mode> property is setted to "stroke", this property will set the color of the boundary of the node. This is also the color of the lines connecting two nodes.
		strokeStyle:       '#ccb',
		//Property: fillStyle
		//If the node <Config.Node.mode> property is setted to "fill", this property will set the color of the node.
		fillStyle:         '#ccb',
		//Property: strokeStyleInPath
		//If the node <Config.Node.mode> property is setted to "stroke", this property will set the color of the boundary of the currently selected node and all its ancestors. This is also the color of the lines connecting the currently selected node and its ancestors.
		strokeStyleInPath: '#eed',
		//Property: strokeStyleInPath
		//If the node <Config.Node.mode> property is setted to "stroke", this property will set the color of the boundary of the currently selected node and all its ancestors. This is also the color of the lines connecting the currently selected node and its ancestors.
		fillStyleInPath:   '#ff7',
		//Property: mode
		//If setted to "stroke" only the boundary of the node will be plotted. If setted to fill, each node will be plotted with a background - fill.
		mode:              'fill', //stroke or fill
		//Property: style
		//Node style. Only "squared" option available.
		style:             'squared' 
	},
	//Property: animationTime
	//Time for the animation.
	animationTime: 200,
	//Property: fps
	//Animation frames per second.
	fps: 25
};

/*
   Class: Canvas

   A multi-purpose Canvas object decorator.
*/

/*
   Constructor: Canvas

   Canvas initializer.

   Parameters:

      canvasId - The canvas tag id.

   Returns:

      A new Canvas instance.
*/
var Canvas= function (canvasId) {
	//browser supports canvas element
	if ("function" == typeof(HTMLCanvasElement) || "object" == typeof(HTMLCanvasElement)) {
		this.canvasId= canvasId;
		//canvas element exists
		if((this.canvas= document.getElementById(this.canvasId)) 
			&& this.canvas.getContext) {
	      this.ctx = this.canvas.getContext('2d');
	      this.ctx.fillStyle = Config.Node.fillStyle || 'black';
	      this.ctx.strokeStyle = Config.Node.strokeStyle || 'black';
	      this.setPosition();
	  	  this.translateToCenter();
      
		} else {
			throw "canvas object with id " + canvasId + " not found";
		}
	} else {
		throw "your browser does not support canvas. Try viewing this page with firefox, safari or opera 9.5";
	}
};


Canvas.prototype= {
	/*
	   Method: getContext

	   Returns:
	
	      Canvas context handler.
	*/
	getContext: function () {
		return this.ctx;
	},
	
	/*
	   Method: makeNodeStyleSelected
	
	   Sets the fill or stroke color to fillStyleInPath or strokeStyleInPath if selected. Sets colors to default otherwise.
	
	   Parameters:
	
	      selected - A Boolean value specifying if the node is selected or not.
	      mode - A String setting the "fill" or "stroke" properties.
	*/
	makeNodeStyleSelected: function(selected, mode) {
		this.ctx[mode + "Style"] = (selected)? Config.Node[mode + "StyleInPath"] : Config.Node[mode + "Style"];
	},

	/*
	   Method: makeEdgeStyleSelected
	
	   Sets the stroke color to strokeStyleInPath if selected. Sets colors to default otherwise.
	
	   Parameters:
	
	      selected - A Boolean value specifying if the node is selected or not.
	*/
	makeEdgeStyleSelected: function(selected) {
		this.ctx.strokeStyle= (selected)? Config.Node.strokeStyleInPath : Config.Node.strokeStyle;
		this.ctx.lineWidth=   (selected)? 2 : 1;
	},

	/*
	   Method: makeRect
	
	   Draws a rectangle in canvas.
	
	   Parameters:
	
	      selected - A Boolean value specifying if the node is selected or not.
	      mode - A String sepecifying if mode is "fill" or "stroke".
	      pos - A set of two coordinates specifying top left and bottom right corners of the rectangle.
	*/
	makeRect: function(pos, mode, selected) {
		if(mode == "fill" || mode == "stroke") {
			this.makeNodeStyleSelected(selected, mode);
			this.ctx[mode + "Rect"](pos.x1, pos.y1, pos.x2, pos.y2);
		} else throw "parameter not recognized " + mode;
	},

	/*
	   Method: setPosition
	
	   Calculates canvas absolute position on HTML document.
	*/	
	setPosition: function() {
		var obj= this.canvas;
		var curleft = curtop = 0;
		if (obj.offsetParent) {
			curleft = obj.offsetLeft
			curtop = obj.offsetTop
			while (obj = obj.offsetParent) {
				curleft += obj.offsetLeft
				curtop += obj.offsetTop
			}
		}
		this.position= { x: curleft, y: curtop };
	},

	/*
	   Method: getPosition

	   Returns:
	
	      Canvas absolute position to the HTML document.
	*/
	getPosition: function() {
		return this.position;
	},

	/*
	   Method: clear
	
	   Clears the canvas object.
	*/		
	clear: function () {
		this.ctx.clearRect(-this.getSize().x / 2, -this.getSize().x / 2, this.getSize().x, this.getSize().x);
	},

	/*
	   Method: clearReactangle
	
	   Same as <clear> but only clears a section of the canvas.
	   
	   Parameters:
	   
	   	top - An integer specifying the top of the rectangle.
	   	right -  An integer specifying the right of the rectangle.
	   	bottom - An integer specifying the bottom of the rectangle.
	   	left - An integer specifying the left of the rectangle.
	*/		
	clearRectangle: function (top, right, bottom, left) {
		this.ctx.clearRect(left, top-2, right - left +2, Math.abs(bottom - top)+5);
	},

	/*
	   Method: translateToCenter
	
	   Translates canvas coordinates system to the center of the canvas object.
	*/
	translateToCenter: function() {
		this.ctx.translate(this.getSize().x / 2, this.getSize().y / 2);
	},
	

	/*
	   Method: getSize

	   Returns:
	
	      An object that contains the canvas width and height.
	      i.e. { x: canvasWidth, y: canvasHeight }
	*/
	getSize: function () {
		return { x: this.canvas.width, y: this.canvas.height };
	},
	
	/*
	   Method: path
	   
	  Performs a _beginPath_ executes _action_ doing then a _type_ ('fill' or 'stroke') and closing the path with closePath.
	*/
	path: function(type, action) {
		this.ctx.beginPath();
		action(this.ctx);
		this.ctx[type]();
		this.ctx.closePath();
	}
	
};

/*
   Class: Complex
	
	 A multi-purpose Complex Class with common methods.

*/


/*
   Constructor: Complex

   Complex constructor.

   Parameters:

      re - A real number.
      im - An real number representing the imaginary part.


   Returns:

      A new Complex instance.
*/
var Complex= function() {
	if (arguments.length > 1) {
		this.x= arguments[0];
		this.y= arguments[1];
		
	} else {
		this.x= null;
		this.y= null;
	}
	
}

Complex.prototype= {
	/*
	   Method: clone
	
	   Returns a copy of the current object.
	
	   Returns:
	
	      A copy of the real object.
	*/
	clone: function() {
		return new Complex(this.x, this.y);
	},

	/*
	   Method: norm
	
	   Calculates the complex norm.
	
	   Returns:
	
	      A real number representing the complex norm.
	*/
	norm: function () {
		return Math.sqrt(this.squaredNorm());
	},
	
	/*
	   Method: squaredNorm
	
	   Calculates the complex squared norm.
	
	   Returns:
	
	      A real number representing the complex squared norm.
	*/
	squaredNorm: function () {
		return this.x*this.x + this.y*this.y;
	},

	/*
	   Method: add
	
	   Returns the result of adding two complex numbers.
	   Does not alter the original object.

	   Parameters:
	
	      pos - A Complex initialized instance.
	
	   Returns:
	
	     The result of adding two complex numbers.
	*/
	add: function(pos) {
		return new Complex(this.x + pos.x, this.y + pos.y);
	},

	/*
	   Method: prod
	
	   Returns the result of multiplying two complex numbers.
	   Does not alter the original object.

	   Parameters:
	
	      pos - A Complex initialized instance.
	
	   Returns:
	
	     The result of multiplying two complex numbers.
	*/
	prod: function(pos) {
		return new Complex(this.x*pos.x - this.y*pos.y, this.y*pos.x + this.x*pos.y);
	},

	/*
	   Method: conjugate
	
	   Returns the conjugate for this complex.
	   Does not alter the original object.

	   Returns:
	
	     The conjugate for this complex.
	*/
	conjugate: function() {
		return new Complex(this.x, -this.y);
	},


	/*
	   Method: scale
	
	   Returns the result of scaling a Complex instance.
	   Does not alter the original object.

	   Parameters:
	
	      factor - A scale factor.
	
	   Returns:
	
	     The result of scaling this complex to a factor.
	*/
	scale: function(factor) {
		return new Complex(this.x * factor, this.y * factor);
	},

	/*
	   Method: $add
	
	   Returns the result of adding two complex numbers.
	   Alters the original object.

	   Parameters:
	
	      pos - A Complex initialized instance.
	
	   Returns:
	
	     The result of adding two complex numbers.
	*/
	$add: function(pos) {
		this.x += pos.x; this.y += pos.y;
		return this;	
	},
	
	/*
	   Method: $prod
	
	   Returns the result of multiplying two complex numbers.
	   Alters the original object.

	   Parameters:
	
	      pos - A Complex initialized instance.
	
	   Returns:
	
	     The result of multiplying two complex numbers.
	*/
	$prod:function(pos) {
		var x = this.x, y = this.y
		this.x = x*pos.x - y*pos.y;
		this.y = y*pos.x + x*pos.y;
		return this;
	},
	
	/*
	   Method: $conjugate
	
	   Returns the conjugate for this complex.
	   Alters the original object.

	   Returns:
	
	     The conjugate for this complex.
	*/
	$conjugate: function() {
		this.y = -this.y;
		return this;
	},
	
	/*
	   Method: $scale
	
	   Returns the result of scaling a Complex instance.
	   Alters the original object.

	   Parameters:
	
	      factor - A scale factor.
	
	   Returns:
	
	     The result of scaling this complex to a factor.
	*/
	$scale: function(factor) {
		this.x *= factor; this.y *= factor;
		return this;
	},
	
	/*
	   Method: $div
	
	   Returns the division of two complex numbers.
	   Alters the original object.

	   Parameters:
	
	      pos - A Complex number.
	
	   Returns:
	
	     The result of scaling this complex to a factor.
	*/
	$div: function(pos) {
		var x = this.x, y = this.y;
		var sq = pos.squaredNorm();
		this.x = x * pos.x + y * pos.y; this.y = y * pos.x - x * pos.y;
		return this.$scale(1 / sq);
	}
};

/*
   Class: Tree
	
	 Provides packages with useful methods for tree manipulation.

*/


/*
   Constructor: Tree

   Tree constructor. Enhances the json tree object with some special properties.

   Parameters:

      json - A json tree object.

*/
var Tree = function(json) {
	(function(json, _parent) {
		Tree.Node(json, _parent);
		for(var i=0; i<json.children.length; i++) {
			arguments.callee(json.children[i], json);
		}
	})(json, null);
};

/*
   Class: Tree.Node
	
	 Enhances the json tree node with special properties.

*/


/*
   Constructor: Tree.Node

   Tree.Node constructor.

   Parameters:

      json - A json tree object.
      parent - The nodes parent node.

*/
Tree.Node = function (json, _parent) {
	//Property: selected
	//sets the node as selected.
	json.selected= false;
	//Property: drawn
	//sets the node as visible or not. (as in CSS visibility:hidden)
	json.drawn= false;
	//Property: exist
	//treats the node as if it existed or not (somewhat similar to CSS display:none)
	json.exist= false;
	//Property: _parent
	//Node parent.
	json._parent= _parent;
	//Property: pos
	//Node position
	json.pos= new Complex(0, 0);
	//Property: startPos
	//node from position
	json.startPos= new Complex(0, 0);
	//Property: endPos
	//node to position
	json.endPos= new Complex(0, 0);
	//Property: startAlpha
	//not being used by the moment.	
	json.startAlpha = 1;
	//Property: endAlpha
	//not being used by the moment.	
	json.endAlpha = 1;
	//Property: alpha
	//not being used by the moment.	
	json.alpha = 1;

};


/*
   Object: Tree.Util
	
	 Provides iterators and utility methods for trees.

*/
Tree.Util = {
	/*
	   Method: set
	
	   To set multiple values to multiple properties of a tree node.
	*/
	set: function(tree, props, value) {
		if(typeof props == 'object')
			for(var prop in props) {
				tree[prop] = props[prop];
			}
		else if(typeof props == 'array')
			for(var i=0; i<props.length; i++) {
				tree[props[i]] = value;
			}
		else tree[props] = value;
	},
	
	/*
	   Method: addSubtree
	
		Makes a proper <Tree> object from a Tree JSON structure and inserts it where specified by _id_.
	
	   Parameters:
	      tree - A tree object.
	      id - A node identifier where this subtree will be appended. If the root of the appended subtree and the id match, then it will append the subtree children to the node specified by _id_
		  subtree - A JSON Tree object.

	   Returns:
	
	   The transformed and appended subtree.
	*/
	addSubtree: function(tree, id, subtree) {
		var s = this.getSubtree(tree, id);
		Tree(subtree);
		if(id == subtree.id) {
			s.children = s.children.concat(subtree.children);
			Tree.Children.each(s, function(ch) {
				ch._parent = s;
			});
		} else {
			s.children.push(subtree);
			subtree._parent = s;
		}
		return subtree;
	},
	
	/*
	   Method: removeSubtree
	
		Deletes a subtree completely.
	
	   Parameters:
	      tree - A tree object.
	      id - The root node to be deleted.
		  removeRoot - A boolean indicating if the root node of the subtree must be also removed.
	*/
	removeSubtree: function(tree, id, removeRoot) {
		var s = this.getSubtree(tree, id);
		var p = s._parent;
		if(!removeRoot) {
			delete s.children;
			s.children = [];
		} else {
			var newChildren = new Array();
			Tree.Children.each(p, function(ch) {
				if(id != ch.id) newChildren.push(ch);
			});
			p.children = newChildren;
		}
	},


	/*
	   Method: each
	
	   Iterates over tree nodes performing an action.
	*/
	each: function(tree, action) {
		this.eachLevel(tree, 0, Number.MAX_VALUE, action);
	},
	
	/*
	   Method: eachLevel
	
	   Iterates over tree nodes to a certain tree level performing an action.
	*/
	eachLevel: function(tree, levelBegin, levelEnd, action) {
		if(levelBegin <= levelEnd) {
			action(tree, levelBegin);
			var ch= tree.children;
			for(var i=0; i<ch.length; i++) {
				this.eachLevel(ch[i], levelBegin +1, levelEnd, action);	
			}
		}
	},
	/*
	   Method: atLevel
	
	   Iterates over tree nodes from a sepecified level performing an action.
	*/
	atLevel: function(tree, level, action) {
		this.eachLevel(tree, 0, level, function (elem, i) {
			if(i == level) action(elem);
		});
	},
	
	/*
	   Method: getLevel
	
	   Returns the current level of the tree node.
	*/
	getLevel: function(tree) {
		var getLevelHandle= function(tree, level) {
			if (tree._parent == null) return level;
			else return getLevelHandle(tree._parent, level +1);
		};
		return getLevelHandle(tree, 0);
	},

	/*
	   Method: getRoot
	
	   Returns the tree root node.
	*/
	getRoot: function(tree) {
		if(tree._parent == null) return tree;
		return this.getRoot(tree._parent);
	},

	/*
	   Method: getLeaves
	
	   Returns an array of the tree leaves.
	*/
	getLeaves: function(tree) {
		var leaves = new Array();
		this.eachLevel(tree, 0, Config.levelsToShow, function(elem, i) {
			if(elem.drawn && !Tree.Children.children(elem, 'exist')) {
				leaves.push(elem);
				elem._level = Config.levelsToShow - i;
			}
		});
		return leaves;
	},

	/*
	   Method: getSubtree
	
	   Returns the subtree of the node with specified id or null if it doesn't find it. 
	*/
	getSubtree: function(tree, id) {
		var ans = null;
		this.each(tree, function(elem) {
			if(elem.id == id) ans = elem;
		});
		return ans;
	}
};

/*
   Object: Tree.Children
	
	 Provides iterators and utility methods for tree children.

*/
Tree.Children = {
	/*
	   Method: each
	
	   Iterates over a nodes children performing an _action_. 
	*/
	each: function(tree, action) {
		for(var i=0, ch=tree.children; i<ch.length; i++) action(ch[i]);
	},
	
	/*
	   Method: children
	
	   Returns true if the current node has at least one node with _property_ set to true. 
	*/
	children: function(tree, property) {
		for(var i=0, ch=tree.children; i<ch.length; i++) 
			if(!property || ch[i][property]) return true;
		return false;
	},

	/*
	   Method: getChildren
	
	   Returns a filtered array of children for the current node. 
	*/
	getChildren: function(tree, property) {
		for(var i=0, ans=new Array(), ch=tree.children; i<ch.length; i++) 
			if(!property || ch[i][property]) ans.push(ch[i]);
		return ans;
	},		
	
	/*
	   Method: getLength
	
	   Returns the length of a filtered children array. 
	*/
	getLength: function(tree, property) {
		if(!property) return tree.children.length;
		for(var i=0, j= 0, ch= tree.children; i<ch.length; i++) if(ch[i][property]) j++;	
		return j;
	}

};

/*
   Object: Tree.Group
	
	 Performs operations on group of nodes.

*/
Tree.Group = {
	/*
	   Method: requestNodes
	
	   Calls the request method on the controller to request a subtree for each node. 
	*/
	requestNodes: function(nodes, controller) {
		var counter = 0, len = nodes.length, nodeSelected = {};
		var complete = function() { controller.onComplete(); };
		if(len == 0) complete();
		for(var i=0; i<len; i++) {
			nodeSelected[nodes[i].id] = nodes[i];
			controller.request(nodes[i].id, nodes[i]._level, {
				onComplete: function(nodeId, data) {
					if(data && data.children) {
						Tree(data);
						for(var j=0, ch=data.children; j<ch.length; j++) {
							ch[j]._parent = nodeSelected[nodeId];
						}
						nodeSelected[nodeId].children = data.children;
					}
					if(++counter == len) {
						complete();
					}
				}
			});
		}
	},
	/*
	   Method: hide
	
	   Collapses group of nodes. 
	*/
	hide: function(nodes, canvas, controller) {
		nodes = this.getNodesWithChildren(nodes);
		var ctx= canvas.getContext();
		
		for(var i=0; i<nodes.length; i++)
			Tree.Children.each(nodes[i], function(elem) {
				Tree.Plot.hideLabels(elem, true);
			});
		
		ctx.save();
		var animationController = {
			compute: function(delta) {
			  for(var i=0; i<nodes.length; i++) {
			    ctx.save();
			  	var node= nodes[i];
			    var bb= Tree.Geometry.getBoundingBox(node);
		        canvas.clearRectangle(bb.top, bb.right, bb.bottom, bb.left);
			  	if(delta == 1) delta = .99;
		  		Tree.Plot.plot(node, canvas, controller, 1 - delta);
				ctx.restore();
			  }
			},
			
			complete: function() {
				ctx.restore();
				for(var i=0; i<nodes.length; i++) {
					if(!controller || !controller.request) {
						if(Tree.Children.children(nodes[i], 'exist')) {
							Tree.Util.each(nodes[i], function(elem) {
								Tree.Util.set(elem, {
									'drawn':false,
									'exist':false
								});
							});
							Tree.Util.set(nodes[i], {
								'drawn':true,
								'exist':true
							});
						}						
					} else {
						delete nodes[i].children;
						nodes[i].children = [];
					}
				}
				
				controller.onComplete();
			}		
		};

		Animation.controller = animationController;
		Animation.start();
	},

	/*
	   Method: show
	
	   Expands group of nodes. 
	*/
	show: function(nodes, canvas, controller) {
		nodes = this.getNodesWithChildren(nodes), newNodes = new Array();
		var ctx= canvas.getContext();
		for(var i=0; i<nodes.length; i++)
			if(!Tree.Children.children(nodes[i], 'drawn')) {
				newNodes.push(nodes[i]);
				Tree.Util.eachLevel(nodes[i], 0, Config.levelsToShow, function(elem) {
					if(elem.exist) elem.drawn = true;
				});
			}
		nodes = newNodes;		
		ctx.save();
		var animationController = {
			compute: function(delta) {
			  for(var i=0; i<nodes.length; i++) {
			    ctx.save();
			  	var node= nodes[i];
			    var bb= Tree.Geometry.getBoundingBox(node);
		        canvas.clearRectangle(bb.top, bb.right, bb.bottom, bb.left);
			  	Tree.Plot.plot(node, canvas, controller, delta);
				ctx.restore();
			  }
			},
			
			complete: function() {
				ctx.restore();
			  	for(var i=0; i<nodes.length; i++) {
				  	var node= nodes[i];
				    var bb= Tree.Geometry.getBoundingBox(node);
			        canvas.clearRectangle(bb.top, bb.right, bb.bottom, bb.left);
				  	Tree.Plot.plot(node, canvas, controller);
				}
				controller.onComplete();
			}		
		};

		Animation.controller = animationController;
		Animation.start();
	},
	/*
	   Method: getNodesWithChildren
	
	   Filters an array of nodes leaving only nodes with children.
	*/
	getNodesWithChildren: function(nodes) {
		var ans = new Array();
		for(var i=0; i<nodes.length; i++) {
			if(Tree.Children.children(nodes[i], 'exist')) ans.push(nodes[i]);
		}
		return ans;
	}
};

/*
   Object: Tree.Plot
	
	 Performs plotting operations.

*/
Tree.Plot = {
	/*
	   Method: plot
	
	   Plots the spacetree
	*/
	plot: function(tree, canvas, controller, scale) {
		if(scale >= 0) {
			tree.drawn = false;		
			var ctx= canvas.getContext();
			var diff = Tree.Geometry.getScaledTreePosition(tree, scale);
			ctx.translate(diff.x, diff.y);
			ctx.scale(scale, scale);
		}
		this.plotTree(tree, canvas, !scale, controller);
		if(scale >= 0) tree.drawn = true;
	},
	/*
	   Method: plotTree
	
	   Plots nodes and edges of the tree.
	*/
	plotTree: function(tree, canvas, plotLabel, controller) {
		var that = this, ctx = canvas.getContext();
		var begin= Tree.Geometry.getEdge(tree.pos, 'begin');
		Tree.Children.each(tree, function(elem) {
			if(elem.exist) {
				var end= Tree.Geometry.getEdge(elem.pos, 'end');
				if(elem.drawn) {
					var adj = {
						nodeFrom: tree,
						nodeTo: elem
					};
					controller.onBeforePlotLine(adj);
					ctx.globalAlpha = Math.min(tree.alpha, elem.alpha);
					that.plotEdge(begin, end, canvas, tree.selected && elem.selected);
					controller.onAfterPlotLine(adj);
				}
				that.plotTree(elem, canvas, plotLabel, controller);
			}
		});
		if(tree.drawn && tree.exist) {
			ctx.globalAlpha = tree.alpha;
			this.plotNode(tree, canvas);
			if(plotLabel && ctx.globalAlpha >= .95) Tree.Label.plotOn(tree, canvas);
			else Tree.Label.hide(tree);
		}
	},

	/*
	   Method: plotNode
	
	   Plots a tree node.
	*/
	plotNode: function(node, canvas) {
		if(Config.Node.style == 'squared')
			this.plotNodeSquared(node, canvas);
		else throw "parameter not recognized " + Config.Node.style;
	},
	
	/*
	   Method: plotNodeSquared
	
	   Plots a square node. Eventually more functions could be appended to draw different types of nodes.
	*/
	plotNodeSquared: function(node, canvas) {
		var pos = node.pos, selected = node.selected, labelOpt = Config.Label;
		var square = {
			x1: pos.x,
			y1: pos.y - labelOpt.height + (labelOpt.height - labelOpt.realHeight)/2,
			x2: labelOpt.realWidth,
			y2: labelOpt.realHeight
		};
		canvas.makeRect(square, Config.Node.mode, selected);
	},
	
	/*
	   Method: plotEdge
	
	   Plots an Edge.	   
	*/
	plotEdge: function(begin, end, canvas, selected) {
		canvas.makeEdgeStyleSelected(selected);
		canvas.path('stroke', function(ctx) {
			ctx.moveTo(begin.x, begin.y);
			ctx.lineTo(end.x, end.y);
		});
	},
	
	/*
	   Method: hideLabels
	
	   Hides all labels of the tree.
	*/
	hideLabels: function(subtree, hide) {
		var l = Tree.Label;
		Tree.Util.each(subtree, function(elem) {
			if(hide) l.hide(elem); else l.show(elem);
		});
	},
	
	/*
	   Method: animate
	
	   Animates the graph by performing a translation from _elem.startPos_ to _elem.endPos_.
	*/
	animate: function(tree, canvas, controller) {
		var that = this;
		var comp = function(from, to, delta){ return (to.$add(from.scale(-1))).$scale(delta).$add(from); };
		var interpolate = function(elem, delta) {
			var from = elem.startPos.clone();
			var to   = elem.endPos.clone();
			elem.pos = comp(from, to, delta);
		};
		var animationController = {
			compute: function(delta) {
				canvas.clear();
				Tree.Util.each(tree, function(node) { interpolate(node, delta); });
				that.plot(tree, canvas, controller);
			},
			
			complete: function() {
				Tree.Util.each(tree, function(elem) { elem.startPos = elem.pos; });
				that.plot(tree, canvas, controller);
				controller.onComplete();
			}		
		};
		Animation.controller = animationController;
		Animation.start();
	},
	
	/*
	   Method: fade
	
	   fades in or out nodes based on _startAlpha_ and _endAlpha_.
	*/
	fade: function(tree, canvas, controller) {
		var that = this;
		var comp = function(from, to, delta){ return from + (to - from) * delta };
		var interpolate = function(elem, delta) {
			var from = elem.startAlpha;
			var to   = elem.endAlpha;
			elem.alpha = comp(from, to, delta);
		};
		var animationController = {
			compute: function(delta) {
				canvas.clear();
				Tree.Util.each(tree, function(node) { interpolate(node, delta); });
				that.plot(tree, canvas, controller);
			},
			
			complete: function() {
				Tree.Util.each(tree, function(elem) { elem.startAlpha = elem.alpha; });
				that.plot(tree, canvas, controller);
				controller.onComplete();
			}		
		};
		Animation.controller = animationController;
		Animation.start();
	}
};

/*
   Object: Tree.Label

	Permorfs all label operations like showing, hiding, setting a label to a particular position, adding/removing classNames, etc.
*/

Tree.Label = {
	nodeHash: {},
	container: false,
	st: null,
	
	/*
	   Method: chk
	
	   Checks if a label with the homologue id of the current tree node exists and if it doesn't it creates a label with this id.
	*/
	chk: function(node) {
		if(!(node.id in this.nodeHash)) 	this.init(node);
		return this.nodeHash[node.id];
	},
	
	/*
	   Method: init
	
	   Creates a label with the same id of the specified node and sets some initial properties.
	*/
	init: function(node) {
		var that = this.st;
		if(!(node.id in this.nodeHash)) {
			if(!this.container) this.container = document.getElementById(Config.labelContainer);
			var labelElement = document.createElement('a');
			labelElement.id = node.id;
			labelElement.href = '#';
			labelElement.onclick = function() {
				that.onClick(node.id);
				return false;
			};
			labelElement.innerHTML = node.name;
			this.container.appendChild(labelElement);
			that.controller.onCreateLabel(labelElement, node);
			
			this.nodeHash[node.id] = labelElement;
		}
		
		this.setClass(node, "node hidden"); this.setDimensions(node);
	},

		
	/*
	   Method: plotOn
	   
	   Plots the label (if this fits in canvas).
	
	   Parameters:
	
	      pos - The position where to put the label. This position is relative to Canvas.
	      canvas - A Canvas instance.
	*/	
	plotOn: function (node, canvas) {
			var pos = node.pos;
			if(this.fitsInCanvas(pos, canvas))
				this.setDivProperties(node, 'node', canvas);
			else this.hide(node);
	},

	/*
	   Method: fitsInCanvas
	   
	   Returns true or false if the current position is between canvas limits or not.
	
	   Parameters:
	
	      pos - The position where to put the label. This position is relative to Canvas.
	      canvas - A Canvas instance.
	*/	
	fitsInCanvas: function(pos, canvas) {
		var size = canvas.getSize();
		return !(Math.abs(pos.x + Config.Label.width/2) >= size.x/2 
			|| Math.abs(pos.y) >= size.y/2);
	},
	
	/*
	   Method: setDivProperties
	
	   Intended for private use: sets some label properties, such as positioning and className.

	   Parameters:
	
	      cssClass - A class name.
	      canvas - A Canvas instance.
	      node - The labels node reference.
	*/	
	setDivProperties: function(node, cssClass, canvas) {
		var radius= canvas.getSize(), position = canvas.getPosition(), pos = node.pos;
		var labelPos= {
				x: Math.round(pos.x + position.x + radius.x/2),
				y: Math.round(pos.y + position.y + radius.y/2 - Config.Label.height)
			};

		var div= this.chk(node);
	    div.style.top= labelPos.y+'px';
		div.style.left= labelPos.x+'px';
		this.removeClass(node, "hidden");
		this.setDimensions(node);
		
	},
	
	/*
	   Method: addClass
	   
	   Adds the specified className to the label.
	
	   Parameters:
	
	      cssClass - class name to add to label.
	     node - the node label reference.
	*/	
	addClass: function(node, cssClass) {
		var element = this.chk(node);
		if(!this.hasClass(node, cssClass)) {
			var array= element.className.split(" ");
			array.push(cssClass); 
			element.className= array.join(" ");
		}
	},

	/*
	   Method: setDimensions
	   
	   Sets label width and height based on <Config.Label> realWidth and realHeight values.
	*/	
	setDimensions: function (node) {
		var elem = this.chk(node);
		elem.style.width= Config.Label.realWidth + 'px';
		elem.style.height= Config.Label.realHeight + 'px';
		this.st.controller.onPlaceLabel(elem, node);
	},

	/*
	   Method: removeClass
	   
	   Removes a specified class from the label.
	
	   Parameters:
	
	      cssClass - A class name.
	      node - A label's node reference.
	*/	
	removeClass: function(node, cssClass) {
		var element = this.chk(node);
		var array= element.className.split(" ");
		var exit= false;
		for(var i=0; i<array.length && !exit; i++) {
			if(array[i] == cssClass) {
				array.splice(i, 1); exit= true;
			}
		}
		element.className= array.join(" ");
	},
	
	/*
	   Method: hasClass
	   
	   Returns true if the specified class name is found in the label. Returns false otherwise.
	   
	
	   Parameters:
	
	      cssClass - A class name.
	      node - A labels node reference.
	   
	  Returns:
	  	 A boolean value.
	*/	
	hasClass: function(node, cssClass) {
		var array= this.chk(node).className.split(" ");
		for(var i=0; i<array.length; i++) {
			if(cssClass == array[i]) return true;
		}
		return false;
	},
	

	/*
	   Method: setClass
	   
	   Sets the className property of the label with a cssClass String.
	   
	
	   Parameters:
	
	      cssClass - A class name.
	      node - A labels node reference.
	*/	
	setClass: function(node, cssClass) {
		this.chk(node).className= cssClass;
	},

	/*
	   Method: hide
	   
	   Hides the label by adding a "hidden" className to it.
	*/	
	hide: function(node) {
		this.addClass(node, "hidden");
	},

	/*
	   Method: show
	   
	   Displays the label by removing the "hidden" className.
	*/	
	show: function(node) {
		this.removeClass(node, "hidden");
	}	
}; 


/*
   Object: Tree.Geometry

	Performs geometrical computations like calculating bounding boxes, a subtree base size, etc.
*/
Tree.Geometry = {
	
	/*
	   Method: left
	   
	   Displays the tree current orientation.
	*/	
	left: function() {
		return Config.orientation == "left"; 
	},

	/*
	   Method: switchOrientation
	   
	   Changes the tree current orientation from top to left or viceversa.
	*/	
	switchOrientation: function() {
		Config.orientation = this.left()? 'top' : 'left';
	},

	/*
	   Method: getSize
	   
	   Returns label height or with, depending on the tree current orientation.
	*/	
	getSize: function(invert) {
		var w = Config.Label.width, h = Config.Label.height;
		if(!invert)
			return (this.left())? w : h;
		else
			return (this.left())? h : w;
	},
	
	/*
	   Method: getOffsetSize
	   
	   Returns label offsetHeight or offsetWidth, depending on the tree current orientation.
	*/	
	getOffsetSize: function() {
		return (!this.left())? Config.Label.offsetHeight : Config.Label.offsetWidth;
	},

	/*
	   Method: translate
	   
	   Applys a translation to the tree.
	*/	
	translate: function(tree, pos, prop) {
		Tree.Util.each(tree, function(elem) {
			elem[prop].$add(pos);
		});
	},
	
	/*
	   Method: getBoundingBox
	   
	   Calculates a tree bounding box.
	*/	
	getBoundingBox: function (tree) {
		var dim = Config.Label, pos = tree.pos;
		var corners = {
			top:    pos.y,
			bottom: pos.y,
			right:  pos.x,
			left:   pos.x		

		};		
		this.calculateCorners(tree, corners);
		if(this.left()) {
			return {
				left: corners.left + dim.realWidth,
				bottom: corners.bottom,
				top: corners.top - dim.height,
				right: corners.right + dim.width
			};
		} else {
			return {
				left: corners.left,
				bottom: corners.bottom,
				top: corners.top,
				right: corners.right + dim.width
			};
		}
	},

	/*
	   Method: calculateCorners
	   
	   Intended for private use. Performs intermediate calculations for a subtree bounding box calculation.
	*/	
	calculateCorners: function(tree, corners) {
		var pos = tree.pos;
		if(tree.exist) {
			if(corners.top > pos.y)    corners.top=    pos.y;
			if(corners.bottom < pos.y) corners.bottom= pos.y;
			if(corners.right < pos.x)  corners.right=  pos.x;
			if(corners.left > pos.x)   corners.left=   pos.x;
			for(var i=0, ch = tree.children; i < ch.length; i++)						
				this.calculateCorners(ch[i], corners);	
		}
	},

	/*
	   Method: getBaseSize
	   
	   Calculates a subtree base size.
	*/	
	getBaseSize: function(tree, contracted, type) {
		var size = this.getSize(true);
		if(contracted)
			return (type == 'available')? size : 
				Tree.Children.getLength(tree, 'exist') * size + Config.offsetBase;

		return this.getTreeBaseSize(tree, 'expanded');
	},

	/*
	   Method: getTreeBaseSize
	   
	   Calculates a subtree base size. This is an utility function used by _getBaseSize_
	*/	
	getTreeBaseSize: function(tree, type, level) {
		var size = this.getSize(true), comp = function (tree, level, type) { return level == 0 || (type == "expanded")? Tree.Children.getLength(tree, 'exist') == 0 : tree.children.length == 0;  }
		level = (arguments.length == 3)? level : Number.MAX_VALUE;
		if(comp(tree, level, type)) return size;
		for(var i=0, ch = tree.children, baseHeight = 0; i<ch.length; i++) {
			baseHeight+= this.getTreeBaseSize(ch[i], type, level -1);
		}
		return baseHeight + Config.offsetBase;
	},

	/*
	   Method: getEdge
	   
	   Returns a Complex instance with the begin or end position of the edge to be plotted.
	*/	
	getEdge: function(pos, type) {
		var dim = Config.Label, left = this.left();
		if(type == 'begin') {
			return left? 
				pos.add(new Complex(dim.realWidth, -dim.height / 2))
				: pos.add(new Complex(dim.realWidth / 2, (dim.realHeight - dim.height)/2));
			
		} else if(type == 'end') {
			return left?
				pos.add(new Complex(0, -dim.height / 2))
				: pos.add(new Complex(dim.realWidth / 2, -dim.realHeight));
		}
	},

	/*
	   Method: getScaledTreePosition
	   
	   Adjusts the tree position due to canvas scaling or translation.
	*/	
	getScaledTreePosition: function(tree, scale) {
		var width = Config.Label.width, height = Config.Label.height;
		if(this.left()) 
			return tree.pos.add(new Complex(width, -height / 2)).$scale(1 - scale);

		return tree.pos.add(new Complex(width / 2, 0)).$scale(1 - scale);
	},

	/*
	   Method: treeFitsInCanvas
	   
	   Returns a Boolean if the current tree fits in canvas.
	*/	
	treeFitsInCanvas: function(tree, canvas, level) {
		var size = (this.left())? canvas.getSize().y : canvas.getSize().x;
		var baseSize = this.getTreeBaseSize(tree, 'exist', level);
		return (baseSize < size);
	},
	
	/*
	   Method: getFirstPos
	   
	   Calculates the _first_ children position given a node position.
	*/	
	getFirstPos: function(initialPos, baseHeight) {
		var size = this.getSize() + this.getOffsetSize(), factor = -baseHeight / 2 + Config.offsetBase / 2;

		return this.left()? new Complex(initialPos.x + size, initialPos.y + factor)
		: new Complex(initialPos.x + factor, initialPos.y + size);  
	},
	
	/*
	   Method: nextPosition
	   
	   Calculates a siblings node position given a node position.
	*/	
	nextPosition: function(firstPos, offsetHeight) {
		return this.left()? new Complex(firstPos.x, firstPos.y + offsetHeight)
		: new Complex(firstPos.x + offsetHeight, firstPos.y);
	},
	
	/*
	   Method: setRightLevelToShow
	   
	   Hides levels of the tree until it properly fits in canvas.
	*/	
	setRightLevelToShow: function(tree, canvas) {
		var level = this.getRightLevelToShow(tree, canvas);
		Tree.Util.eachLevel(tree, 0, Config.levelsToShow, function (elem, i) {
			if(i > level) { 
				elem.drawn = false; elem.exist = false; Tree.Label.hide(elem);
			} else {
				elem.exist= true;
			}
		});
		tree.drawn= true;
	},
	
	/*
	   Method: getRightLevelToShow
	   
	   Returns the right level to show for the current tree in order to fit in canvas.
	*/	
	getRightLevelToShow: function(tree, canvas) {
		var level = Config.levelsToShow;
		while(!this.treeFitsInCanvas(tree, canvas, level) && level > 1) { level-- ; }
		return level;
	}
};

/*
 	Class: ST
 	
 	The main Spacetree class.
 */

/*
 Constructor: ST

 Creates a new ST instance.
 
 Parameters:

    canvas - A <Canvas> instance.
    controller - a ST controller <http://blog.thejit.org/?p=8>
*/	
var ST= function(canvas, controller)  {
	var innerController = {
		onBeforeCompute: $_.fn(),
		onAfterCompute:  $_.fn(),
		onCreateLabel:   $_.fn(),
		onPlaceLabel:    $_.fn(),
		onCreateElement: $_.fn(),
		onComplete:      $_.fn(),
		onBeforePlotLine: $_.fn(),
		onAfterPlotLine: $_.fn(),
		request:         false
	};
	this.controller = $_.merge(innerController, controller);
	this.canvas=       canvas;
	this.tree=           null;
	this.clickedNode=    null;
	
	//If this library was OO (which it will at version 1.1) I could do this in not an uglier way.
	Tree.Label.st = this;
	Animation.fps = Config.fps;
	Animation.duration = Config.animationTime;
};

ST.prototype= {
	/*
	 	Method: loadFromJSON
	 	
	 	Loads a json object into the ST.
	 */
	loadFromJSON: function(json) {
		this.tree = json;
		Tree(json);
	},
	
	/*
	 Method: compute
	
	 Calculates positions from root node.
	*/	
	compute: function () {
	  	Tree.Util.set(this.tree, {
	  		'drawn':true,
	  		'exist':true,
	  		'selected':true
	  	});
		this.calculatePositions(this.tree, new Complex(0, 0), "startPos");
	},
	
	/*
	 Method: calculatePositions
	
	 This method implements the core algorithm to calculate node positioning.
	*/	
	calculatePositions: function (tree, initialPos, property, contracted) {
		var Geom = Tree.Geometry, TC = Tree.Children, contracted = (arguments.length == 3)? true : contracted;
		if(this.clickedNode && (tree.id == this.clickedNode.id) && contracted) contracted = false;
		
		tree[property] = initialPos;
		var ch = Tree.Children.getChildren(tree, 'exist');
		if (ch.length > 0) {
			var baseHeight   = Geom.getBaseSize(tree, contracted);
			var prevBaseSize = Geom.getBaseSize(ch[0], contracted, 'available');
			var offsetBase   = (ch.length == 1)? Config.offsetBase : baseHeight - prevBaseSize;

			var firstPos= ch[0][property]= Geom.getFirstPos(initialPos, offsetBase);
			this.calculatePositions(ch[0], firstPos, property, contracted);
			for(var i=1; i<ch.length; i++) {
				var leaf = !TC.children(ch[i], 'exist') || !TC.children(ch[i -1], 'exist');
				var nextBaseSize = Geom.getBaseSize(ch[i], contracted, 'available');
				var offsetHeight = leaf? Geom.getSize(true) : (prevBaseSize + nextBaseSize) / 2;
				firstPos = Geom.nextPosition(firstPos, offsetHeight);
				prevBaseSize = nextBaseSize;
				this.calculatePositions(ch[i], firstPos, property, contracted);
			}
		}
	},

	/*
	 Method: plot
	
	 This method plots the tree. Note that, before plotting the tree, you have to call <compute> to properly calculatePositions.
	*/	
	plot: function() { Tree.Plot.plot(this.tree, this.canvas, this.controller); },

  
 	/*
	 Method: switchPosition
	
	 Switches the tree orientation from vertical to horizontal or viceversa.
	*/	
  switchPosition: function(onComplete) {
  	var Geom = Tree.Geometry, Plot = Tree.Plot, tree = this.tree, that = this;
  	if(!Plot.busy) {
	  	Plot.busy = true;
	  	this.contract({
	  		onComplete: function() {
	  			Geom.switchOrientation();
	  			that.calculatePositions(tree, new Complex(0, 0), 'endPos');
	  			Plot.busy = false;
	  			that.onClick(that.clickedNode.id, onComplete);
	  		}
	  	}, true);
	}
  },

	/*
	 	Method: requestNodes
	 	
	 	If the controller has a request method, it asynchonously requests subtrees for the leaves of the tree.
	 */
  requestNodes: function(node, onComplete) {
  	var handler = $_.merge(this.controller, onComplete);
  	if(handler.request)
   		Tree.Group.requestNodes(Tree.Util.getLeaves(node), handler);
  	  else
		handler.onComplete();
  },
 
	/*
	 	Method: contract
	 	
	 	Contracts selected nodes.
	 */
  contract: function(onComplete, switched) {
	var Geom = Tree.Geometry, Util = Tree.Util, Group = Tree.Group;
	var tree = this.clickedNode;
  	//get nodes that must be contracted
  	var nodesToHide = function(tree, canvas) {
		var level = Util.getLevel(tree), root = Util.getRoot(tree), nodeArray = new Array();
		Util.eachLevel(root, 0, level, function(elem, i) {
			if(elem.exist && !elem.selected) {
				nodeArray.push(elem);			
			}
		});

		level = Geom.getRightLevelToShow(tree, canvas);
		Util.atLevel(tree, level, function (elem, i) {
			if(elem.exist) {
				nodeArray.push(elem);
			}
		});
		return nodeArray;		
  	};
  	
  	if(switched) Geom.switchOrientation();
  	var nodes = nodesToHide(tree, this.canvas);
  	if(switched) Geom.switchOrientation();
  	Group.hide(nodes, this.canvas, $_.merge(this.controller, onComplete));
  },
  
	/*
	 	Method: move
	 	
	 	Performs the animation of the translation of the tree.
	 */
	move: function(node, onComplete) {
		this.calculatePositions(this.tree, new Complex(0, 0), "endPos");
		Tree.Geometry.translate(this.tree, node.endPos.scale(-1), "endPos");
		Tree.Plot.animate(this.tree, this.canvas, $_.merge(this.controller, onComplete));
	},
  
  
  	/*
	 Method: expand
	
	 Determines which nodes to expand (and expands their subtrees).
	*/	
  expand: function (node, onComplete) {
	var nodeArray= new Array();
	Tree.Util.eachLevel(node, 0, Config.levelsToShow, function (elem, i)  {
		if(!Tree.Children.children(elem, 'drawn')) {
			nodeArray.push(elem);
		}
	});
	Tree.Group.show(nodeArray, this.canvas, $_.merge(this.controller, onComplete));
  },


  /*
	 Method: selectPath
	
	 Sets a "selected" flag to nodes that are in the path.
  */	
  selectPath: function(node, nodePrev) {
  	(function(node, val) {
  		if(node == null) return arguments.callee;
  		node.selected = val;
  		return arguments.callee(node._parent, val);
  	})(nodePrev, false)(node, true);
  },
  
	/*
	   Method: addSubtree
	
		Adds a subtree, performing optionally an animation.
	
	   Parameters:
	      id - A node identifier where this subtree will be appended. If the id of the root of the appended subtree and the parameter _id_ match, then only the subtrees children will be appended to the node specified by _id_
		  subtree - A JSON Tree object.
		  method - _optional_ set this to _animate_ to animate the tree after adding the subtree. You can also set this parameter to _replot_ to just replot the subtree.
		  onComplete - An action to perform after the animation (if any).

	   Returns:
	
	   The transformed and appended subtree.
	*/
	addSubtree: function(id, subtree, method, onComplete) {
		var Util = Tree.Util, that = this, res = Util.addSubtree(this.tree, id, subtree);
		if(method == 'replot') {
			this.onClick(this.clickedNode.id, $_.merge({
				onMoveComplete: function() {
					Util.each(res, function(elem) {
						elem.drawn = elem.exist;
					});
				},
				
				onExpandComplete: function() {
					that.canvas.clear();
					that.plot();
				}
			}, onComplete));
		} else if (method == 'animate') {
			this.onClick(this.clickedNode.id, {
				onMoveComplete: function() {
					Util.each(res, function(elem) {
						elem.drawn = elem.exist;
					});
				},
				
				onExpandComplete: function() {
					Util.each(res, function(elem) {
						Util.set(elem, {
							'drawn': elem.exist,
							'startAlpha': elem.exist? 0 : 1,
							'endAlpha': 1,
							'alpha': elem.exist? 0 : 1,
						});
					});
					Tree.Plot.fade(that.tree, that.canvas, $_.merge(that.controller, onComplete));
				}
			});
		}
	},

	/*
	   Method: removeSubtree
	
		Removes a subtree, performing optionally an animation.
	
	   Parameters:
	      id - A node identifier where this subtree will be appended. If the id of the root of the appended subtree and the parameter _id_ match, then only the subtrees children will be appended to the node specified by _id_
		  removeRoot - Remove the root subtree or only its children.
		  method - _optional_ set this to _animate_ to animate the tree after adding the subtree. You can also set this parameter to _replot_ to just replot the subtree.
		  onComplete - An action to perform after the animation (if any).

	*/
	removeSubtree: function(id, removeRoot, method, onComplete) {
		var Util = Tree.Util, that = this;
		if(method == 'replot') {
			this.onClick(this.clickedNode.id, $_.merge({
				onContractComplete: function() {
					Tree.Plot.hideLabels(Util.getSubtree(that.tree, id), true);
					Util.removeSubtree(that.tree, id, removeRoot);
				}
			}, onComplete));
		} else if (method == 'animate') {
			var res = Util.getSubtree(this.tree, id);
			Util.each(res, function(elem) {
				Util.set(elem, {
					'drawn': elem.exist,
					'startAlpha': 1,
					'endAlpha': 0,
					'alpha': 1,
				});
			});
			if(!removeRoot)
				Util.set(res, {
					'drawn': elem.exist,
					'startAlpha': 1,
					'endAlpha': 1,
					'alpha': 1,
				});
			Tree.Plot.fade(this.tree, this.canvas, $_.merge(that.controller, {
				onComplete: function() {
					Util.removeSubtree(that.tree, id, removeRoot);
					that.onClick(that.clickedNode.id, onComplete);
				}
			}));
		}
	},

  /*
	 Method: onClick

	This method is called when clicking on a tree node. It mainly performs all calculations and the animation of contracting, translating and expanding pertinent nodes.
	
		
	 Parameters:
	
	    ide - The label id. The label id is usually the same as the tree node id.
	    onComplete - A controller method to perform things when the animation completes.

	*/	  
  onClick: function (id, onComplete) {
	var canvas = this.canvas, that = this, Plot = Tree.Plot,  Util = Tree.Util, Geom = Tree.Geometry;
	var innerController = {
		onRequestNodesComplete: $_.fn(),
		onContractComplete: $_.fn(),
		onMoveComplete: $_.fn(),
		onExpandComplete: $_.fn()
	};
	var complete = $_.merge(this.controller, innerController, onComplete);
	
	if(!Plot.busy) {
		Plot.busy= true;
		var node=  Util.getSubtree(this.tree, id);
		this.selectPath(node, this.clickedNode);
		this.clickedNode= node;
		complete.onBeforeCompute(node);
		this.requestNodes(node, {
			onComplete: function() {
				complete.onRequestNodesComplete();
				that.contract({
					onComplete: function() {
						Geom.setRightLevelToShow(node, canvas);
						complete.onContractComplete();
						that.move(node, {
							onComplete: function() {
								complete.onMoveComplete();
								that.expand(node, {
									onComplete: function() {
										complete.onExpandComplete();
										complete.onAfterCompute(id);
										complete.onComplete();
										Plot.busy = false;
									}
								}); //expand
							}
						}); //move
					}
				});//contract
			}
		});//request
	}
  }
};

/*
   Object: Trans
	
	 An object containing multiple type of transformations. Based on the mootools library <http://mootools.net>.

*/
var Trans = {
	linear: function(p) { return p;	},
	Quart: function(p) {
		return Math.pow(p, 4);
	},
	easeIn: function(transition, pos){
		return transition(pos);
	},
	easeOut: function(transition, pos){
		return 1 - transition(1 - pos);
	},
	easeInOut: function(transition, pos){
		return (pos <= 0.5) ? transition(2 * pos) / 2 : (2 - transition(2 * (1 - pos))) / 2;
	}
};


/*
   Object: Animation
	
	 An object that performs animations. Based on Fx.Base from Mootools.

*/
var Animation = {

	duration: Config.animationTime,
	fps: Config.fps,
	transition: function(p) {return Trans.easeInOut(Trans.Quart, p);},
//	transition: Trans.linear,
	controller: false,
	
	getTime: function() {
		var ans = (Date.now)? Date.now() : new Date().getTime();
		return ans;
	},
	
	step: function(){
		var time = this.getTime();
		if (time < this.time + this.duration){
			var delta = this.transition((time - this.time) / this.duration);
			this.controller.compute(delta);
		} else {
			this.timer = clearInterval(this.timer);
			this.controller.compute(1);
			this.controller.complete();
		}
	},

	start: function(){
		this.time = 0;
		this.startTimer();
		return this;
	},

	startTimer: function(){
		if (this.timer) return false;
		this.time = this.getTime() - this.time;
		this.timer = setInterval((function () { Animation.step(); }), Math.round(1000 / this.fps));
		return true;
	}
};