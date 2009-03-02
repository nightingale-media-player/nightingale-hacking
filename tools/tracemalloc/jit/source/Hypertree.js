/*
 * File: Hypertree.js
 * 
 * Author: Nicolas Garcia Belmonte
 * 
 * Homepage: <http://thejit.org>
 * 
 * Version: 1.0.7a
 *
 * Copyright: Copyright 2008 by Nicolas Garcia Belmonte.
 * 
 * License: BSD License
 * 
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
		if(this.isArray(object)) {
				unlinked = [];
				for (var i = 0, l = object.length; i < l; i++) unlinked[i] = this.unlink(object[i]);
		} else if(this.isObject(object)) {
				unlinked = {};
				for (var p in object) unlinked[p] = this.unlink(object[p]);
		} else return object;

		return unlinked;
	},
	
	isArray: function(obj) {
		return obj.constructor.toString().match(/array/i);
	},
	
	isString: function(obj) {
		return obj.constructor.toString().match(/string/i);
	},
	
	isObject: function(obj) {
		return obj.constructor.toString().match(/object/i);
	}
} ;

/*
   Class: Canvas

   A multi-purpose Canvas object decorator.
*/

/*
   Constructor: Canvas

   Canvas initializer.

   Parameters:

      canvasId - The canvas tag id.
      fillStyle - (optional) fill color style. Default's to white
      strokeStyle - (optional) stroke color style. Default's to white

   Returns:

      A new Canvas instance.
*/
var Canvas= function (canvasId, fillStyle, strokeStyle) {
	//browser supports canvas element
		this.canvasId= canvasId;
		//canvas element exists
		if((this.canvas= document.getElementById(this.canvasId)) 
			&& this.canvas.getContext) {
		      this.ctx = this.canvas.getContext('2d');
		      this.ctx.fillStyle = fillStyle || 'white';
		      this.ctx.strokeStyle = strokeStyle || 'white';
		      this.setPosition();
		      this.translateToCenter();
		} else {
			throw "Canvas object could not initialize.";
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
		var size = this.getSize();
		this.ctx.clearRect(-size.x / 2, -size.y / 2, size.x, size.y);
	},

	/*
	   Method: drawMainCircle
	
	   Draws the boundary circle for the Hyperbolic Tree.
	*/	
	drawMainCircle: function () {	
	  var ctx= this.ctx;
	  ctx.beginPath();
	  	ctx.arc(0, 0, this.getSmallerSize() / 2, 0, Math.PI*2, true);
	  	ctx.stroke();
 		ctx.closePath();
	},
	
	/*
	   Method: translateToCenter
	
	   Translates canvas coordinates system to the center of the canvas object.
	*/
	translateToCenter: function() {
		this.ctx.translate(this.canvas.width / 2, this.canvas.height / 2);
	},
	

	/*
	   Method: getSize

	   Returns:
	
	      An object that contains the canvas width and height.
	      i.e. { x: canvasWidth, y: canvasHeight }
	*/
	getSize: function () {
		var width = this.canvas.width;
		var height = this.canvas.height;
		return { x: width, y: height };
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
	},
	
	/*
	   Method: getSmallerSize
	   
	  Returns min(width, height) for the canvas object.
	*/
	getSmallerSize: function() {
		var s = this.getSize();
		return (s.x <= s.y)? s.x : s.y;
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
	   Method: toPolar
	
	   Transforms cartesian to polar coordinates.
	
	   Returns:
	
	      A new <Polar> instance.
	*/
	toPolar: function() {
		var rho = this.norm();
		var atan = Math.atan2(this.y, this.x);
		if(atan < 0) atan += Math.PI * 2;
		return new Polar(atan, rho);
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
	
	   Returns the conjugate por this complex.

	   Returns:
	
	     The conjugate por this complex.
	*/
	conjugate: function() {
		return new Complex(this.x, -this.y);
	},
	
	div: function(pos) {
		var sq = pos.squaredNorm();
		return new Complex(this.x * pos.x + this.y * pos.y, this.y * pos.x - this.x * pos.y).$scale(1 / sq);
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
	   Method: equals
	
	   Comparison method.
	*/
	equals: function(c) {
		return this.x == c.x && this.y == c.y;
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
	},
	

	/*
	   Method: moebiusTransformation
	
	   Calculates a moebius transformation for this point / complex.
	   	For more information go to:
			http://en.wikipedia.org/wiki/Moebius_transformation.

	   Parameters:
	
	      c - An initialized Complex instance representing a translation Vector.
	*/
	moebiusTransformation: function(c) {
		var num = this.add(c);
		var den = c.$conjugate().$prod(this); den.x++;
		num.$div(den);
		return num;
	}
};

Complex.KER = new Complex(0, 0);


/*
   Class: Polar

   A multi purpose polar representation.

*/

/*
   Constructor: Polar

   Polar constructor.

   Parameters:

      theta - An angle.
      rho - The norm.


   Returns:

      A new Polar instance.
*/
var Polar = function(theta, rho) {
	this.theta = theta;
	this.rho = rho;
};

Polar.prototype = {
	/*
	   Method: toComplex
	
	    Translates from polar to cartesian coordinates and returns a new <Complex> instance.
	
	   Returns:
	
	      A new Complex instance.
	*/
	toComplex: function() {
		return new Complex(Math.cos(this.theta), Math.sin(this.theta)).scale(this.rho);
	},

	/*
	   Method: add
	
	    Adds two <Polar> instances.
	
	   Returns:
	
	      A new Polar instance.
	*/
	add: function(polar) {
		return new Polar(this.theta + polar.theta, this.rho + polar.rho);
	},
	
	/*
	   Method: scale
	
	    Scales a polar norm.
	
	   Returns:
	
	      A new Polar instance.
	*/
	scale: function(number) {
		return new Polar(this.theta, this.rho * number);
	},
	
	/*
	   Method: equals
	
	   Comparison method.
	*/
	equals: function(c) {
		return this.theta == c.theta && this.rho == c.rho;
	},
	
	/*
	   Method: interpolate
	
	    Calculates a polar interpolation between two points at a given delta moment.
	
	   Returns:
	
	      A new Polar instance representing an interpolation between _this_ and _elem_
	*/
	interpolate: function(elem, delta) {
		var pi2 = Math.PI * 2;
		var ch = function(t) {
			return (t < 0)? (t % pi2) + pi2 : t % pi2;
		};
		var tt = ch(this.theta) , et = ch(elem.theta);
		var sum;
		if(Math.abs(tt - et) > Math.PI) {
			if(tt - et > 0) {
				sum =ch((et + ((tt - pi2) - et)* delta)) ;
			} else {
				sum =ch((et - pi2 + (tt - (et - pi2))* delta));
			}
			
		} else {
				sum =ch((et + (tt - et)* delta)) ;
		}
		var  t = (sum);
		var r = (this.rho - elem.rho) * delta + elem.rho;
		return new Polar(t, r);
	}

};

Polar.KER = new Polar(0, 0);

/*
   Object: Config

   <HT> global configuration object. Contains important properties to enable customization and proper behavior for the <HT>.
*/

var Config = {
		//Property: labelContainer
		//id for label container
		labelContainer: 'label_container',
		
		//Property: drawMainCircle
		//show/hide main circle
		drawMainCircle: true,
        
		//Property: allowVariableNodeDiameters
		//Set this to true if you want your node diameters to be proportional to you first dataset object value property (i.e _data[0].value_).
		//This will allow you to represent weighted tree/graph nodes.
		allowVariableNodeDiameters: false,

		//Property: transformNodes
		//Apply the moebius transformation to the nodes. Applying a moebius transformation to the nodes might be annoying
		// to the user if you are plotting a tree/graph with weighted nodes, since they will deformed by the transformation and thus
		// not having a meaninfull comparative proportion. 
		transformNodes: true,
		
		//Property: nodeDiameters
		//Diameters range.
		nodeRangeDiameters: {
			min: 10,
			max: 55
		},
			
		//Property: nodeRangeValues
		//The interval of the values of the first object of your dataSet array. A superset of the values can also be specified.
		nodeRangeValues: {
			min: 1,
			max: 35
		},

		//Property: fps
		//animation frames per second
		fps:40,

		//Property: animationTime
		//Time of the animation
		animationTime: 1500,

		//Property: nodeRadius
		//The radius of the nodes displayed
		nodeRadius: 4
};


/*
   Object: GraphUtil

   A multi purpose object to do graph traversal and processing.
*/
var GraphUtil = {
	/*
	   Method: filter
	
	   For internal use only. Provides a filtering function based on flags.
	*/
	filter: function(param) {
		if(!param || !$_.isString(param)) return function() { return true; };
		var props = param.split(" ");
		return function(elem) {
			for(var i=0; i<props.length; i++) if(elem[props[i]]) return false;
			return true;
		};
	},

	/*
	   Method: clean
	
	   Cleans flags from nodes (by setting the _flag_ property to false).
	*/
	clean: function(graph) { this.eachNode(graph, function(elem) { elem._flag = false; }); },
	
	/*
	   Method: eachNode
	
	   Iterates over graph nodes performing an action.
	*/
	eachNode: function(graph, action, flags) {
		var filter = this.filter(flags);
		for(var i in graph.nodes) if(filter(graph.nodes[i])) action(graph.nodes[i]);
	},
	
	/*
	   Method: eachAdjacency
	
	   Iterates over a _node_ adjacencies applying the _action_ function.
	*/
	eachAdjacency: function(node, action, flags) {
		var adj = node.adjacencies, filter = this.filter(flags);
		for(var id in adj) if(filter(adj[id])) action(adj[id], id);
	},

	/*
	   Method: computeLevels
	
	   Performs a BFS traversal setting correct level for nodes.
	*/
	computeLevels: function(graph, id, flags) {
		var filter = this.filter(flags);
		this.eachNode(graph, function(elem) {
			elem._flag = false;
			elem._depth = -1;
		}, flags);
		var root = graph.getNode(id);
		root._depth = 0;
		var queue = [root];
		while(queue.length != 0) {
			var node = queue.pop();
			node._flag = true;
			this.eachAdjacency(node, function(adj) {
				var n = adj.nodeTo;
				if(n._flag == false && filter(n)) {
					if(n._depth < 0) n._depth = node._depth + 1;
					queue.unshift(n);
				}
			}, flags);
		}
	},
	
	/*
	   Method: getNode
	
	   Returns a graph node with a specified _id_.
	*/
	getNode: function(graph, id) {
		return graph.getNode(id);
	},
	
	/*
	   Method: eachBFS
	
	   Performs a BFS traversal of a graph beginning by the node of id _id_ and performing _action_ on each node.
	   This traversal ignores nodes or edges having the property _ignore_ setted to _true_.
	*/
	eachBFS: function(graph, id, action, flags) {
		var filter = this.filter(flags);
		this.clean(graph);
		var queue = [graph.getNode(id)];
		while(queue.length != 0) {
			var node = queue.pop();
			node._flag = true;
			action(node, node._depth);
			this.eachAdjacency(node, function(adj) {
				var n = adj.nodeTo;
				if(n._flag == false && filter(n)) {
					n._flag = true;
					queue.unshift(n);
				}
			}, flags);
		}
	},
	
	/*
	   Method: eachSubnode
	
	   After a BFS traversal the _depth_ property of each node has been modified. Now the graph can be traversed as a tree. This method iterates for each subnode that has depth larger than the specified node.
	*/
	eachSubnode: function(graph, node, action, flags) {
		var d = node._depth, filter = this.filter(flags);
		this.eachAdjacency(node, function(adj) {
			var n = adj.nodeTo;
			if(n._depth > d && filter(n)) action(n);
		}, flags);
	},
	
	/*
	   Method: getParents
	
	   Returns all nodes having a depth that is less than the node's depth property.
	*/
	getParents: function(graph, node) {
		var adj = node.adjacencies;
		var ans = new Array();
		this.eachAdjacency(node, function(adj) {
			var n = adj.nodeTo;
			if(n._depth < node._depth) ans.push(n);
		});
		return ans;
	},
	
	/*
	   Method: getSubnodes
	
	   Collects all subnodes for a specified node. The _level_ parameter filters nodes having relative depth of _level_ from the root node.
	*/
	getSubnodes: function(graph, id, level, flags) {
		var ans = new Array(), that = this, node = graph.getNode(id);
		(function(graph, node) {
			var fn = arguments.callee;
			if(!level || level <= node._depth)	ans.push(node);
			that.eachSubnode(graph, node, function(elem) {
				fn(graph, elem);
			}, flags);
		})(graph, node);
		return ans;
	},
	
	/*
	   Method: getClosestNodeToOrigin
	
	   Returns the closest node to the center of canvas.
	*/
	getClosestNodeToOrigin: function(graph, prop, flags) {
		var node = null;
		this.eachNode(graph, function(elem) {
			node = (node == null || elem[prop].rho < node[prop].rho)? elem : node;
		}, flags);
		return node;
	},
	
	/*
	 Method: moebiusTransformation
	
	Calculates a moebius transformation for the hyperbolic tree.
	For more information go to:
	<http://en.wikipedia.org/wiki/Moebius_transformation>
	 
	 Parameters:
	
	    theta - Rotation angle.
	    c - Translation Complex.
	*/	
	moebiusTransformation: function(graph, pos, prop, startPos, flags) {
  		this.eachNode(graph, function(elem) {
  			for(var i=0; i<prop.length; i++) {
  				var p = pos[i].scale(-1), property = startPos? startPos : prop[i]; 
  				elem[prop[i]] = elem[property].toComplex().moebiusTransformation(p).toPolar();
  			}
  		}, flags);
    }
};

/*
   Object: GraphOp

   An object holding unary and binary graph operations such as removingNodes, removingEdges, adding two graphs and morphing.
*/
var GraphOp = {

	options: {
		type: 'nothing',
		duration: 2000,
		fps:30
	},
	
	/*
	   Method: removeNode
	
	   Removes one or more nodes from the visualization. It can also perform several animations like fading sequentially, fading concurrently, iterating or replotting.

	   Parameters:
	
	      viz - The visualization object (an RGraph instance in this case).
	      node - The node's id. Can also be an array having many ids.
	      opt - Animation options. It's an object with two properties: _type_, which can be _nothing_, _replot_, _fade:seq_,  _fade:con_ or _iter_. The other property is the _duration_ in milliseconds. 
	
	*/
	removeNode: function(viz, node, opt) {
		var options = $_.merge(viz.controller, this.options, opt);
		var n = $_.isString(node)? [node] : node;
		switch(options.type) {
			case 'nothing':
				for(var i=0; i<n.length; i++) 	viz.graph.removeNode(n[i]);
				break;
			
			case 'replot':
				this.removeNode(viz, n, { type: 'nothing' });
				GraphPlot.clearLabels(viz);
				viz.refresh(true);
				break;
			
			case 'fade:seq': case 'fade':
				var GPlot = GraphPlot, that = this;
				//set alpha to 0 for nodes to remove.
				for(var i=0; i<n.length; i++) {
					var nodeObj = viz.graph.getNode(n[i]);
					nodeObj.endAlpha = 0;
				}
				GPlot.animate(viz, $_.merge(options, {
					modes: ['fade:nodes'],
					onComplete: function() {
						that.removeNode(viz, n, { type: 'nothing' });
						GPlot.clearLabels(viz);
						viz.reposition();
						GPlot.animate(viz, $_.merge(options, {
							modes: ['linear'],
							hideLabels: true
						}));
					}
				}));
				break;
			
			case 'fade:con':
				var GPlot = GraphPlot, that = this;
				//set alpha to 0 for nodes to remove. Tag them for being ignored on computing positions.
				for(var i=0; i<n.length; i++) {
					var nodeObj = viz.graph.getNode(n[i]);
					nodeObj.endAlpha = 0;
					nodeObj.ignore = true;
				}
				viz.reposition();
				GPlot.animate(viz, $_.merge(options, {
					modes: ['fade:nodes', 'linear'],
					hideLabels:true,
					onComplete: function() {
						that.removeNode(viz, n, { type: 'nothing' });
					}
				}));
				break;
			
			case 'iter':
				var that = this, GPlot = GraphPlot;
				GPlot.sequence(viz, {
					condition: function() { return n.length != 0; },
					step: function() { that.removeNode(viz, n.shift(), { type: 'nothing' });  GPlot.clearLabels(viz); },
					onComplete: function() { options.onComplete(); },
					duration: Math.ceil(options.duration / n.length)
				});
				break;
				
			default: this.doError();
		}
	},
	
	/*
	   Method: removeEdge
	
	   Removes one or more edges from the visualization. It can also perform several animations like fading sequentially, fading concurrently, iterating or replotting.

	   Parameters:
	
	      viz - The visualization object (an RGraph instance in this case).
	      vertex - An array having two strings which are the ids of the nodes connected by this edge: ['id1', 'id2']. Can also be a two dimensional array holding many edges: [['id1', 'id2'], ['id3', 'id4'], ...].
	      opt - Animation options. It's an object with two properties: _type_, which can be _nothing_, _replot_, _fade:seq_,  _fade:con_ or _iter_. The other property is the _duration_ in milliseconds. 
	
	*/
	removeEdge: function(viz, vertex, opt) {
		var options = $_.merge(viz.controller, this.options, opt);
		var v = $_.isString(vertex[0])? [vertex] : vertex;
		switch(options.type) {
			case 'nothing':
				for(var i=0; i<v.length; i++) 	viz.graph.removeAdjacence(v[i][0], v[i][1]);
				break;
			
			case 'replot':
				this.removeEdge(viz, v, { type: 'nothing' });
				viz.refresh(true);
				break;
			
			case 'fade:seq': case 'fade':
				var GPlot = GraphPlot, that = this;
				//set alpha to 0 for nodes to remove.
				for(var i=0; i<v.length; i++) {
					var adjs = viz.graph.getAdjacence(v[i][0], v[i][1]);
					if(adjs) {
						adjs[0].endAlpha = 0;
						adjs[1].endAlpha = 0;
					}
				}
				GPlot.animate(viz, $_.merge(options, {
					modes: ['fade:vertex'],
					onComplete: function() {
						that.removeEdge(viz, v, { type: 'nothing' });
						viz.reposition();
						GPlot.animate(viz, $_.merge(options, {
							modes: ['linear'],
							hideLabels:true
						}));
					}
				}));
				break;
			
			case 'fade:con':
				var GPlot = GraphPlot, that = this;
				//set alpha to 0 for nodes to remove. Tag them for being ignored when computing positions.
				for(var i=0; i<v.length; i++) {
					var adjs = viz.graph.getAdjacence(v[i][0], v[i][1]);
					if(adjs) {
						adjs[0].endAlpha = 0;
						adjs[0].ignore = true;
						adjs[1].endAlpha = 0;
						adjs[1].ignore = true;
					}
				}
				viz.reposition();
				GPlot.animate(viz, $_.merge(options, {
					modes: ['fade:vertex', 'linear'],
					onComplete: function() {
						that.removeEdge(viz, v, { type: 'nothing' });
					}
				}));
				break;
			
			case 'iter':
				var that = this, GPlot = GraphPlot;
				GPlot.sequence(viz, {
					condition: function() { return v.length != 0; },
					step: function() { that.removeEdge(viz, v.shift(), { type: 'nothing' }); GPlot.clearLabels(viz); },
					onComplete: function() { options.onComplete(); },
					duration: Math.ceil(options.duration / v.length)
				});
				break;
				
			default: this.doError();
		}
	},
	
	/*
	   Method: sum
	
	   Adds a new graph to the visualization. The json graph (or tree) must at least have a common node with the current graph plotted by the visualization. The resulting graph can be defined as follows: <http://mathworld.wolfram.com/GraphSum.html>

	   Parameters:
	
	      viz - The visualization object (an RGraph instance in this case).
	      json - A json tree <http://blog.thejit.org/2008/04/27/feeding-json-tree-structures-to-the-jit/>, a json graph <http://blog.thejit.org/2008/07/02/feeding-json-graph-structures-to-the-jit/> or an extended json graph <http://blog.thejit.org/2008/08/05/weighted-nodes-weighted-edges/>.
	      opt - Animation options. It's an object with two properties: _type_, which can be _nothing_, _replot_, _fade:seq_,  or _fade:con_. The other property is the _duration_ in milliseconds. 
	
	*/
	sum: function(viz, json, opt) {
		var options = $_.merge(viz.controller, this.options, opt), root = viz.root;
		viz.root = opt.id || viz.root;
		switch(options.type) {
			case 'nothing':
				var graph = viz.construct(json), GUtil = GraphUtil;
				GUtil.eachNode(graph, function(elem) {
					GUtil.eachAdjacency(elem, function(adj) {
						viz.graph.addAdjacence(adj.nodeFrom, adj.nodeTo, adj.data);
					});
				});
				break;
			
			case 'replot':
				this.sum(viz, json, { type: 'nothing' });
				viz.refresh(true);
				break;
			
			case 'fade:seq': case 'fade': case 'fade:con':
				var GUtil = GraphUtil, GPlot = GraphPlot, that = this, graph = viz.construct(json);
				//set alpha to 0 for nodes to add.
				var fadeEdges = this.preprocessSum(viz, graph);
				var modes = !fadeEdges? ['fade:nodes'] : ['fade:nodes', 'fade:vertex'];
				viz.reposition();
				if(options.type != 'fade:con') {
					GPlot.animate(viz, $_.merge(options, {
						modes: ['linear'],
						hideLabels:true,
						onComplete: function() {
							GPlot.animate(viz, $_.merge(options, {
								modes: modes,
								onComplete: function() {
									options.onComplete();
								}
							}));
						}
					}));
				} else {
					GUtil.eachNode(viz.graph, function(elem) {
						if(elem.id != root && elem.pos.equals(Polar.KER)) elem.pos = elem.startPos = elem.endPos;
					});
					GPlot.animate(viz, $_.merge(options, {
						modes: ['linear'].concat(modes),
						hideLabels:true,
						onComplete: function() {
							options.onComplete();
						}
					}));
				}
				break;

			default: this.doError();
		}
	},
	
	/*
	   Method: morph
	
	   This method will _morph_ the current visualized graph into the new _json_ representation passed in the method. Can also perform multiple animations. The _json_ object must at least have the root node in common with the current visualized graph.

	   Parameters:
	
	      viz - The visualization object (an RGraph instance in this case).
	      json - A json tree <http://blog.thejit.org/2008/04/27/feeding-json-tree-structures-to-the-jit/>, a json graph <http://blog.thejit.org/2008/07/02/feeding-json-graph-structures-to-the-jit/> or an extended json graph <http://blog.thejit.org/2008/08/05/weighted-nodes-weighted-edges/>.
	      opt - Animation options. It's an object with two properties: _type_, which can be _nothing_, _replot_, or _fade_. The other property is the _duration_ in milliseconds. 
	
	*/
	morph: function(viz, json, opt) {
		var options = $_.merge(viz.controller, this.options, opt), root = viz.root;
		viz.root = opt.id || viz.root;
		switch(options.type) {
			case 'nothing':
				var graph = viz.construct(json), GUtil = GraphUtil;
				GUtil.eachNode(graph, function(elem) {
					GUtil.eachAdjacency(elem, function(adj) {
						viz.graph.addAdjacence(adj.nodeFrom, adj.nodeTo, adj.data);
					});
				});
				GUtil.eachNode(viz.graph, function(elem) {
					GUtil.eachAdjacency(elem, function(adj) {
						if(!graph.getAdjacence(adj.nodeFrom.id, adj.nodeTo.id)) {
							viz.graph.removeAdjacence(adj.nodeFrom.id, adj.nodeTo.id);
						}
						if(!viz.graph.hasNode(elem.id)) viz.graph.removeNode(elem.id);
					});
				});
				
				break;
			
			case 'replot':
				this.morph(viz, json, { type: 'nothing' });
				viz.refresh(true);
				break;
			
			case 'fade:seq': case 'fade': case 'fade:con':
				var GUtil = GraphUtil, GPlot = GraphPlot, that = this, graph = viz.construct(json);
				//preprocessing for adding nodes.
				var fadeEdges = this.preprocessSum(viz, graph);
				//preprocessing for nodes to delete.
				GUtil.eachNode(viz.graph, function(elem) {
					if(!graph.hasNode(elem.id)) {
						elem.alpha = 1; elem.startAlpha = 1; elem.endAlpha = 0; elem.ignore = true;
					}
				});	
				GUtil.eachNode(viz.graph, function(elem) {
					if(elem.ignore) return;
					GUtil.eachAdjacency(elem, function(adj) {
						if(adj.nodeFrom.ignore || adj.nodeTo.ignore) return;
						var nodeFrom = graph.getNode(adj.nodeFrom.id);
						var nodeTo = graph.getNode(adj.nodeTo.id);
						if(!nodeFrom.adjacentTo(nodeTo)) {
							var adjs = viz.graph.getAdjacence(nodeFrom.id, nodeTo.id);
							fadeEdges = true;
							adjs[0].alpha = 1; adjs[0].startAlpha = 1; adjs[0].endAlpha = 0; adjs[0].ignore = true;
							adjs[1].alpha = 1; adjs[1].startAlpha = 1; adjs[1].endAlpha = 0; adjs[1].ignore = true;
						}
					});
				});	
				var modes = !fadeEdges? ['fade:nodes'] : ['fade:nodes', 'fade:vertex'];
				viz.reposition();
				GUtil.eachNode(viz.graph, function(elem) {
					if(elem.id != root && elem.pos.equals(Polar.KER)) elem.pos = elem.startPos = elem.endPos;
				});
				GPlot.animate(viz, $_.merge(options, {
					modes: ['polar'].concat(modes),
					hideLabels:true,
					onComplete: function() {
						GUtil.eachNode(viz.graph, function(elem) {
							if(elem.ignore) viz.graph.removeNode(elem.id);
						});
						GUtil.eachNode(viz.graph, function(elem) {
							GUtil.eachAdjacency(elem, function(adj) {
								if(adj.ignore) viz.graph.removeAdjacence(adj.nodeFrom.id, adj.nodeTo.id);
							});
						});
						options.onComplete();
					}
				}));
				break;

			default: this.doError();
		}
	},
	
	preprocessSum: function(viz, graph) {
		var GUtil = GraphUtil;
		GUtil.eachNode(graph, function(elem) {
			if(!viz.graph.hasNode(elem.id)) {
				viz.graph.addNode(elem);
				var n = viz.graph.getNode(elem.id);
				n.alpha = 0; n.startAlpha = 0; n.endAlpha = 1;
			}
		});	
		var fadeEdges = false;
		GUtil.eachNode(graph, function(elem) {
			GUtil.eachAdjacency(elem, function(adj) {
				var nodeFrom = viz.graph.getNode(adj.nodeFrom.id);
				var nodeTo = viz.graph.getNode(adj.nodeTo.id);
				if(!nodeFrom.adjacentTo(nodeTo)) {
					var adjs = viz.graph.addAdjacence(nodeFrom, nodeTo, adj.data);
					if(nodeFrom.startAlpha == nodeFrom.endAlpha 
					&& nodeTo.startAlpha == nodeTo.endAlpha) {
						fadeEdges = true;
						adjs[0].alpha = 0; adjs[0].startAlpha = 0; adjs[0].endAlpha = 1;
						adjs[1].alpha = 0; adjs[1].startAlpha = 0; adjs[1].endAlpha = 1;
					} 
				}
			});
		});	
		return fadeEdges;
	}
};



/*
   Object: GraphPlot

   An object that performs specific radial layouts for a generic graph structure.
*/
var GraphPlot = {
	
	Interpolator: {
		'moebius': function(elem, delta, vector) {
			vector = vector.clone();
			elem.pos = elem.startPos.toComplex().moebiusTransformation(vector).toPolar();
		},

		'linear': function(elem, delta) {
			var from = elem.startPos.toComplex();
			var to = elem.endPos.toComplex();
			elem.pos = ((to.$add(from.scale(-1))).$scale(delta).$add(from)).toPolar();
		},

		'fade:nodes': function(elem, delta) {
			if(elem.endAlpha != elem.alpha) {
				var from = elem.startAlpha;
				var to   = elem.endAlpha;
				elem.alpha = from + (to - from) * delta;
			}
		},
		
		'fade:vertex': function(elem, delta) {
			var adjs = elem.adjacencies;
			for(var id in adjs) this['fade:nodes'](adjs[id], delta);
		},
		
		'polar': function(elem, delta) {
			var from = elem.startPos;
			var to = elem.endPos;
			elem.pos = to.interpolate(from, delta);
		}
	},
	
	//Property: labelsHidden
	//A flag value indicating if node labels are being displayed or not.
	labelsHidden: false,
	//Property: labelContainer
	//Label DOM element
	labelContainer: false,
	//Property: labels
	//Label DOM elements hash.
	labels: {},

	/*
	   Method: getLabelContainer
	
	   Lazy fetcher for the label container.
	*/
	getLabelContainer: function() {
		return this.labelContainer? this.labelContainer : this.labelContainer = document.getElementById(Config.labelContainer);
	},
	
	/*
	   Method: getLabel
	
	   Lazy fetcher for the label DOM element.
	*/
	getLabel: function(id) {
		return (id in this.labels && this.labels[id] != null)? this.labels[id] : this.labels[id] = document.getElementById(id);
	},
	
	
	/*
	   Method: hideLabels
	
	   Hides all labels.
	*/
	hideLabels: function (hide) {
		var container = this.getLabelContainer();
		if(hide) container.style.display = 'none';
		else container.style.display = '';
		this.labelsHidden = hide;
	},
	
	/*
	   Method: clearLabels
	
	   Clears the label container.
	*/
	clearLabels: function(viz) {
		for(var id in this.labels) 
			if(!viz.graph.hasNode(id)) {
				this.disposeLabel(id);
				delete this.labels[id];
			}
	},
	
	/*
	   Method: disposeLabel
	
	   Removes a label.
	*/
	disposeLabel: function(id) {
		var elem = this.getLabel(id);
		if(elem && elem.parentNode) elem.parentNode.removeChild(elem); 
	},

	/*
	   Method: sequence
	
	   Iteratively performs an action while refreshing the state of the visualization.
	*/
	sequence: function(viz, options) {
		options = $_.merge({
			condition: function() { return false; },
			step: $_.fn(),
			onComplete: $_.fn(),
			duration: 200
		}, options);

		var interval = setInterval(function() {
			if(options.condition()) {
				options.step();
			} else {
				clearInterval(interval);
				options.onComplete();
			}
			viz.refresh(true);
		}, options.duration);
	},
	
	/*
	   Method: animate
	
	   Animates the graph by performing a moebius transformation from the current state to the given position.
	*/
	animate: function(viz, opt, versor) {
		var that = this, graph  = viz.graph, GUtil = GraphUtil, Anim = Animation, duration = opt.duration || Anim.duration, fps = opt.fps || Anim.fps;
		 var root = graph.getNode(viz.root);
		//Should be changed eventually, when Animation becomes a class.
		var prevDuration = Anim.duration, prevFps = Anim.fps;
		Anim.duration = duration;
		Anim.fps = fps;
		
		if(opt.hideLabels) this.hideLabels(true);
		var animationController = {
			compute: function(delta) {
				var vector = versor? versor.scale(-delta) : null;
				GUtil.eachNode(graph, function(node) { 
					for(var i=0; i<opt.modes.length; i++) 
							that.Interpolator[opt.modes[i]](node, delta, vector);
				});
				that.plot(viz, opt);
			},
			complete: function() {
				GUtil.eachNode(graph, function(node) { 
					node.startPos = node.pos;
					node.startAlpha = node.alpha;
				});
				if(opt.hideLabels) that.hideLabels(false);
				that.plot(viz, opt);
				opt.onComplete();
				opt.onAfterCompute();
				Anim.duration = prevDuration;
				Anim.fps = prevFps;
			}		
		};
		Animation.controller = animationController;
		Animation.start();
	},
	
	/*
	   Method: plot
	
	   Plots a Graph.
	*/
	plot: function(viz, opt) {
		var aGraph = viz.graph, canvas = viz.canvas, id = viz.root;
		var that = this, ctx = canvas.getContext(), GUtil = GraphUtil;
		canvas.clear();
		if(Config.drawMainCircle) canvas.drawMainCircle();
		var T = !!aGraph.getNode(id).visited;
		GUtil.eachNode(aGraph, function(node) {
			GUtil.eachAdjacency(node, function(adj) {
				if(!!adj.nodeTo.visited === T) {
					opt.onBeforePlotLine(adj);
					ctx.save();
					ctx.globalAlpha = Math.min(Math.min(node.alpha, adj.nodeTo.alpha), adj.alpha);
					that.plotLine(adj, canvas);
					ctx.restore();
					opt.onAfterPlotLine(adj);
				}
			});
			ctx.save();
			ctx.globalAlpha = node.alpha;
			that.plotNode(node, canvas);
	 		if(!that.labelsHidden && ctx.globalAlpha >= .95) that.plotLabel(canvas, node, opt);
	 		else if(!that.labelsHidden && ctx.globalAlpha < .95) that.hideLabel(node);
			ctx.restore();
			node.visited = !T;
		});
	},

	/*
	   Method: plotNode
	
	   Plots a graph node.
	*/
	plotNode: function(node, canvas) {
		var scale = canvas.getSmallerSize() / 2;
		var p = node.pos.toComplex(), pos = p.scale(scale);
		canvas.path('fill', function(context) {
			var prod = Config.transformNodes?  node._radius * (1 - p.squaredNorm()) : node._radius;
	  		//TODO set a minRadius for this.
	  		if(prod >= 4) context.arc(pos.x, pos.y, prod, 0, Math.PI*2, true);			
		});
	},

	/*
	   Method: plotLine
	
	   Plots a hyperline between two nodes. A hyperline is an arc of a circle which is orthogonal to the main circle.
	*/
	plotLine: function(adj, canvas) {
		var node = adj.nodeFrom, child = adj.nodeTo, data = adj.data;
		var pos = node.pos.toComplex(), posChild = child.pos.toComplex();
		var centerOfCircle = this.computeArcThroughTwoPoints(pos, posChild);
		var scale = canvas.getSmallerSize()/2;
		var angleBegin = Math.atan2(posChild.y - centerOfCircle.y, posChild.x - centerOfCircle.x);
   		var angleEnd    = Math.atan2(pos.y - centerOfCircle.y, pos.x - centerOfCircle.x);
		var sense        = this.sense(angleBegin, angleEnd);
		var context = canvas.getContext();
		context.save();
		canvas.path('stroke', function(ctx) {
		 	if(centerOfCircle.a > 1000 || centerOfCircle.b > 1000 || centerOfCircle.ratio > 1000) {
				var posScaled = pos.scale(scale), posChildScaled = posChild.scale(scale); 
		  		ctx.moveTo(posScaled.x, posScaled.y);
		  		ctx.lineTo(posChildScaled.x, posChildScaled.y);
		  	} else {
				ctx.arc(centerOfCircle.x*scale, centerOfCircle.y*scale, centerOfCircle.ratio*scale, angleBegin, angleEnd, sense);
		  	}
		});
		context.restore();
	},
	
	/*
	   Method: computeArcThroughTwoPoints
	
	   Calculates the arc parameters through two points. More information in <http://en.wikipedia.org/wiki/Poincar%C3%A9_disc_model#Analytic_geometry_constructions_in_the_hyperbolic_plane>
	*/
	computeArcThroughTwoPoints: function(p1, p2) {
		var aDen = (p1.x*p2.y - p1.y*p2.x), bDen = aDen;
	  	var sq1 = p1.squaredNorm(), sq2 = p2.squaredNorm();
		//Setting ratio to > 1000 to draw a straight line.
	  	if (aDen == 0) return { x:0, y:0, ratio: 1001 };

	  	var a = (p1.y*(sq2) - p2.y * (sq1) + p1.y - p2.y) / aDen;
	  	var b = (p2.x*(sq1) - p1.x * (sq2) + p2.x - p1.x) / bDen;
	  	var x = -a / 2;
	  	var y = -b / 2;
		var squaredRatio = (a*a + b*b) / 4 -1;
		
		if(squaredRatio < 0) return { x:0, y:0, ratio: 1001 };
		var ratio = Math.sqrt(squaredRatio);
	  	var out= {
	  		x: x,
	  		y: y,
	  		ratio: ratio,
	  		a: a,
	  		b: b
	  	};

		return out;
  },

	/*
	   Method: plotLabel
	
	   Plots a label for a given node.
	*/
	plotLabel: function(canvas, node, controller) {
		var id = node.id, tag = this.getLabel(id);
		if(!tag && !(tag = document.getElementById(id))) {
			tag = document.createElement('div');
			var container = this.getLabelContainer();
			container.appendChild(tag);
			tag.id = id;
			tag.className = 'node';
			tag.style.position = 'absolute';
			controller.onCreateLabel(tag, node);
		}
		var pos = node.pos.toComplex();
		var radius= canvas.getSize();
		var scale = canvas.getSmallerSize() / 2;
		var canvasPos = canvas.getPosition();
		var labelPos= {
			x: Math.round(pos.x * scale + canvasPos.x + radius.x/2),
			y: Math.round(pos.y * scale + canvasPos.y + radius.y/2)
		};
		tag.style.left = labelPos.x + 'px';
		tag.style.top = labelPos.y  + 'px';
		tag.style.display = '';
		controller.onPlaceLabel(tag, node);
	},

	/*
	   Method: hideLabel
	
	   Hides a label having _node.id_ as id.
	*/
	hideLabel: function(node) {
		var n; if(n = this.getLabel(node.id)) n.style.display = "none";
	},
	
	/*
	   Method: sense

	   For private use only: sets angle direction to clockwise (true) or counterclockwise (false).
	   
	   Parameters:
	
	      angleBegin - Starting angle for drawing the arc.
	      angleEnd - The HyperLine will be drawn from angleBegin to angleEnd.

	   Returns:
	
	      A Boolean instance describing the sense for drawing the HyperLine.
	*/
  sense: function(angleBegin, angleEnd) {
  	return (angleBegin < angleEnd)? ((angleBegin + Math.PI > angleEnd)? false : true) : ((angleEnd + Math.PI > angleBegin)? true : false);
  }
};

/*
   Class: Hypertree

	An animated Graph with radial layout.

	Go to <http://blog.thejit.org/?p=7> to know what kind of JSON structure feeds this object.
	
	Go to <http://blog.thejit.org/?p=8> to know what kind of controller this class accepts.
	
	Refer to the <Config> object to know what properties can be modified in order to customize this object. 

	The simplest way to create and layout a Hypertree is:
	
	(start code)

	  var canvas= new Canvas('infovis', '#fff', '#fff');
	  var ht= new Hypertree(canvas);
	  ht.loadTreeFromJSON(json);
	  ht.compute();
	  ht.plot();
	  ht.prepareCanvasEvents();

	(end code)

	A user should only interact with the Canvas, Hypertree and Config objects/classes.
	By implementing Hypertree controllers you can also customize the Hypertree behavior.
*/

/*
 Constructor: Hypertree

 Creates a new Hypertree instance.
 
 Parameters:

    canvas - A <Canvas> instance.
    controller - _optional_ a Hypertree controller <http://blog.thejit.org/?p=8>
*/
var Hypertree = function(canvas, controller) {
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
	this.graph = new Graph();
	this.json = null;
	this.canvas = canvas;
	this.root = null;
	this.busy = false;

	Animation.fps = Config.fps;
	Animation.duration = Config.animationTime;

};

Hypertree.prototype = {

	construct: function(json) {
		var isGraph = (typeof json == "object" && json.constructor.toString().match(/array/i));
		var ans = new Graph();
		if(!isGraph) 
			//make tree
			(function (ans, json) {
				ans.addNode(json);
				for(var i=0, ch = json.children; i<ch.length; i++) {
					ans.addAdjacence(json, ch[i]);
					arguments.callee(ans, ch[i]);
				}
			})(ans, json);
		else
			//make graph
			(function (ans, json) {
				var getNode = function(id) {
					for(var w=0; w<json.length; w++) if(json[w].id == id) return json[w];
				};
				for(var i=0; i<json.length; i++) {
					ans.addNode(json[i]);
					for(var j=0, adj = json[i].adjacencies; j<adj.length; j++) {
						var node = adj[j], data;
						if(typeof adj[j] != 'string') {
							data = node.data;
							node = node.nodeTo;
						}
						ans.addAdjacence(json[i], getNode(node), data);
					}
				}
			})(ans, json);

		return ans;
	},
	
	/*
	 Method: loadTree
	
	 Loads a Graph from a json tree object <http://blog.thejit.org>
	 
	*/
	loadTree: function(json) {
		this.graph = this.construct(json);
	},

	/*
	 Method: loadGraph
	
	 Loads a Graph from a json graph object <http://blog.thejit.org>
	 
	*/
	loadGraph: function(json) {
		this.graph = this.construct(json);
	},
	
	/*
	 Method: flagRoot
	
	 Flags a node specified by _id_ as root.
	*/
	flagRoot: function(id) {
		this.unflagRoot();
		this.graph.nodes[id]._root = true;
	},
	
	/*
	 Method: unflagRoot
	
	 Unflags all nodes.
	*/
	unflagRoot: function() {
		GraphUtil.eachNode(this.graph, function(elem) {elem._root = false;});
	},
	
	/*
	 Method: getRoot
	
	 Returns the node flagged as root.
	*/
	getRoot: function() {
		var root = false;
		GraphUtil.eachNode(this.graph, function(elem){ if(elem._root) root = elem; });
		return root;
	},
	
	/*
	 Method: loadTreeFromJSON
	
	 Loads a Hypertree from a _json_ object <http://blog.thejit.org/?p=7>
	*/
	loadTreeFromJSON: function(json) {
		this.json = json;
		this.loadTree(json);
		this.root = json.id;
	},
	
	/*
	 Method: loadGraphFromJSON
	
	 Loads a Hypertree from a _json_ object <http://blog.thejit.org>
	*/
	loadGraphFromJSON: function(json, i) {
		this.json = json;
		this.loadGraph(json);
		this.root = json[i? i : 0].id;
	},
	
	/*
	 Method: refresh
	
	 Computes positions and then plots.
	 
	*/
	refresh: function(reposition) {
		if(reposition) {
			this.reposition();
			GraphUtil.eachNode(this.graph, function(node) {
				node.startPos = node.pos = node.endPos;
			});
		} else {
			this.compute();
		}
		this.plot();
	},
	
	/*
	 Method: reposition
	
	 Computes new positions and repositions the tree where it was before. (For calculating new positions the root must be on its origin, and that may lead to an undesired repositioning of the graph, this method attempts to solve this problem.
	 
	*/
	reposition: function() {
		this.compute('endPos');
		var vector = this.graph.getNode(this.root).pos.toComplex().$scale(-1);
		GraphUtil.moebiusTransformation(this.graph, [vector], ['endPos'], 'endPos', "ignore");
		GraphUtil.eachNode(this.graph, function(node) {
			if(node.ignore) node.endPos = node.pos;
		});
	},

	/*
	 Method: plot
	
	 Plots the Hypertree
	*/
	plot: function() {
		GraphPlot.plot(this, this.controller);
	},
	
	/*
	 Method: compute
	
	 Computes the graph nodes positions and stores this positions on _property_.
	*/
	compute: function(property) {
		var prop = property || ['pos', 'startPos'];
		var node = GraphUtil.getNode(this.graph, this.root);
		node._depth = 0;
		this.flagRoot(this.root);
		GraphUtil.computeLevels(this.graph, this.root, "ignore");
		this.computeAngularWidths();
		this.computePositions(prop);
	},
	
	/*
	 Method: computePositions
	
	 Performs the main algorithm for computing node positions.
	*/
	computePositions: function(property) {
		var propArray = (typeof property == 'array' || typeof property == 'object')? property : [property];
		var aGraph = this.graph, GUtil = GraphUtil;
		var root = this.graph.getNode(this.root), that = this, config = Config;

		//Set default values for the root node
		for(var i=0; i<propArray.length; i++) 
			root[propArray[i]] = new Polar(0, 0);
		root.angleSpan = {
			begin: 0,
			end: 2 * Math.PI
		};
		root._rel = 1;
		
		//Estimate better edge length.
		var edgeLength = (function() {
			var depth = 0;
			GUtil.eachNode(aGraph, function(node) {
				depth = (node._depth > depth)? node._depth : depth;
			}, "ignore");
			for(var i=0.51; i<=1; i+=.01) {
				var valSeries = (function(a, n) {
					return (1 - Math.pow(a, n)) / (1 - a);
				})(i, depth + 1);
				if(valSeries >= 2) return i - .01;
			}
			return .5;
		})();
		
		GUtil.eachBFS(this.graph, this.root, function (elem) {
			var angleSpan = elem.angleSpan.end - elem.angleSpan.begin;
			var angleInit = elem.angleSpan.begin;
			var totalAngularWidths = (function (element){
				var total = 0;
				GUtil.eachSubnode(aGraph, element, function(sib) {
					total += sib._treeAngularWidth;
				}, "ignore");
				return total;
			})(elem);

			var rho = (function(depth, edgeLength) {
				for(var i=1, acum = 0, lenAcum = edgeLength; i<=depth+1; i++) {
					acum+= lenAcum;
					lenAcum*= edgeLength;
				}
				return acum;	
			})(elem._depth, edgeLength);
			
			GUtil.eachSubnode(aGraph, elem, function(child) {
				if(!child._flag) {
					child._rel = child._treeAngularWidth / totalAngularWidths;
					var angleProportion = child._rel * angleSpan;
					var theta = angleInit + angleProportion / 2;

					for(var i=0; i<propArray.length; i++)
						child[propArray[i]] = new Polar(theta, rho);

					child.angleSpan = {
						begin: angleInit,
						end: angleInit + angleProportion
					};
					angleInit += angleProportion;
				}
			}, "ignore");

		}, "ignore");
	},
	
	/*
	 Method: setAngularWidthForNodes
	
	 Sets nodes angular widths.
	*/
	setAngularWidthForNodes: function() {
		var rVal = Config.nodeRangeValues, rDiam = Config.nodeRangeDiameters, nr = Config.nodeRadius; 
		var diam = function(value) { return (((rDiam.max - rDiam.min) / (rVal.max - rVal.min)) * (value - rVal.min) + rDiam.min) };
		GraphUtil.eachBFS(this.graph, this.root, function(elem, i) {
			var dataValue = (Config.allowVariableNodeDiameters && elem.data && elem.data.length > 0)? elem.data[0].value : nr;
			var diamValue = diam(dataValue);
			var rho = i;
			elem._angularWidth = diamValue / rho;
			elem._radius = diamValue / 2;
		});
	},
	
	/*
	 Method: setSubtreesAngularWidths
	
	 Sets subtrees angular widths.
	*/
	setSubtreesAngularWidth: function() {
		var that = this;
		GraphUtil.eachNode(this.graph, function(elem) {
			that.setSubtreeAngularWidth(elem);
		});
	},
	
	/*
	 Method: setSubtreeAngularWidth
	
	 Sets the angular width for a subtree.
	*/
	setSubtreeAngularWidth: function(elem) {
		var that = this, nodeAW = elem._angularWidth, sumAW = 0;
		GraphUtil.eachSubnode(this.graph, elem, function(child) {
			that.setSubtreeAngularWidth(child);
			sumAW++;
		});
		elem._treeAngularWidth = Math.max(nodeAW, sumAW);
	},
	
	/*
	 Method: computeAngularWidths
	
	 Computes nodes and subtrees angular widths.
	*/
	computeAngularWidths: function () {
		this.setAngularWidthForNodes();
		this.setSubtreesAngularWidth();
	},
	
	/*
	 Method: onClick
	
	 Performs all calculations and animation when clicking on a label specified by _id_. The label id is the same id as its homologue node.
	*/
	onClick: function(e) {
		Mouse.capturePosition(e);
		var mousePosition = Mouse.getPosition(this.canvas);
		this.move(mousePosition);
	},
	
	move: function(mousePosition) {
		if(this.busy === false && mousePosition.norm() < 1) {
			this.busy = true;
			var root = this.graph.getNode(this.root), that = this;
			this.controller.onBeforeCompute(root);
			var versor = new Complex(mousePosition.x, mousePosition.y);
			if(versor.norm() < 1)
				GraphPlot.animate(this, $_.merge(this.controller, {
					modes: ['moebius'],
					hideLabels: true,
					onComplete: function() {
						that.busy = false;
					}
				}), versor);
		}
	},
	
	/*
	 Method: prepareCanvasEvents
	
	 Adds a click handler to the canvas in order to translate the Hypertree when clicking anywhere on the canvas. You could set a translation handler on the labels and not activate this one if you want to. You can perform that by using a controller <http://blog.thejit.org/?p=8>
	*/
	prepareCanvasEvents: function() {
		var that = this;
		this.canvas.canvas.onclick = function(e) { that.onClick(e); };
	}
	
};


/*
   Class: Mouse

   A multi-purpose Mouse class.
*/
var Mouse = {
	  
	  position: null,

		/*
		   Method: getPosition
		
		   Returns mouse position relative to canvas.
		
		   Parameters:
		
		      canvas - A canvas object.
		
		   Returns:
		
		      A Complex instance representing the mouse position on the canvas.
		*/
		getPosition: function (canvas) {
			var posx = this.posx;
			var posy = this.posy;
			var position = canvas.getPosition();
			var s = canvas.getSmallerSize();
			var size = canvas.getSize();
			var coordinates= {
			  x: ((posx - position.x) - size.x / 2) / (s / 2),
			  y: ((posy - position.y) - size.y / 2) / (s / 2)
			};
			
			this.position= new Complex(coordinates.x, coordinates.y);
			return this.position;
		},


		/*
		   Method: capturePosition
		
		   Captures mouse position.
		
		   Parameters:
		
		      e - Triggered event.
		*/
	  capturePosition: function(e) {
			var posx = 0;
			var posy = 0;
			if (!e) var e = window.event;
			if (e.pageX || e.pageY) 	{
				posx = e.pageX;
				posy = e.pageY;
			}
			else if (e.clientX || e.clientY) 	{
				posx = e.clientX + document.body.scrollLeft
					+ document.documentElement.scrollLeft;
				posy = e.clientY + document.body.scrollTop
					+ document.documentElement.scrollTop;
			}
			
			this.posx= posx;
			this.posy= posy;
	}
};



/*
 Class: Graph

 A generic Graph class.
 
*/	

/*
 Constructor: Graph

 Creates a new Graph instance.
 
*/	
var Graph= function()  {
	//Property: nodes
	//graph nodes
	this.nodes= {};
};
	
	
Graph.prototype= {

/*
	 Method: getNode
	
	 Returns a <Graph.Node> from a specified _id_.
*/	
 getNode: function(id) {
 	if(this.hasNode(id)) 	return this.nodes[id];
 	return false;
 },


/*
	 Method: getAdjacence
	
	 Returns an array of <Graph.Adjacence> that connects nodes with id _id_ and _id2_.
*/	
  getAdjacence: function (id, id2) {
	var adjs = [];
	if(this.hasNode(id) 	&& this.hasNode(id2) 
	&& this.nodes[id].adjacentTo({ 'id':id2 }) && this.nodes[id2].adjacentTo({ 'id':id })) {
		adjs.push(this.nodes[id].getAdjacency(id2));
		adjs.push(this.nodes[id2].getAdjacency(id));
		return adjs;
	}
	return false;	
 },

	/*
	 Method: addNode
	
	 Adds a node.
	 
	 Parameters:
	
	    obj - A <Graph.Node> object.
	*/	
  addNode: function(obj) {
  	if(!this.nodes[obj.id]) {
	  	this.nodes[obj.id] = new Graph.Node(obj.id, obj.name, obj.data);
  	}
  	return this.nodes[obj.id];
  },
  
	/*
	 Method: addAdjacence
	
	 Connects nodes specified by *obj* and *obj2*. If not found, nodes are created.
	 
	 Parameters:
	
	    obj - a <Graph.Node> object.
	    obj2 - Another <Graph.Node> object.
	    data - A DataSet object.
	*/	
  addAdjacence: function (obj, obj2, weight) {
  	var adjs = []
  	if(!this.hasNode(obj.id)) this.addNode(obj);
  	if(!this.hasNode(obj2.id)) this.addNode(obj2);
	obj = this.nodes[obj.id]; obj2 = this.nodes[obj2.id];
	
  	for(var i in this.nodes) {
  		if(this.nodes[i].id == obj.id) {
  			if(!this.nodes[i].adjacentTo(obj2)) {
  				adjs.push(this.nodes[i].addAdjacency(obj2, weight));
  			}
  		}
  		
  		if(this.nodes[i].id == obj2.id) {	
  			if(!this.nodes[i].adjacentTo(obj)) {
  				adjs.push(this.nodes[i].addAdjacency(obj, weight));
  			}
  		}
  	}
  	return adjs;
 },

	/*
	 Method: removeNode
	
	 Removes a <Graph.Node> from <Graph> that matches the specified _id_.
	*/	
  removeNode: function(id) {
  	if(this.hasNode(id)) {
  		var node = this.nodes[id];
  		for(var i=0 in node.adjacencies) {
  			var adj = node.adjacencies[i];
  			this.removeAdjacence(id, adj.nodeTo.id);
  		}
  		delete this.nodes[id];
  	}
  },
  
/*
	 Method: removeAdjacence
	
	 Removes a <Graph.Adjacence> from <Graph> that matches the specified _id1_ and _id2_.
*/	
  removeAdjacence: function(id1, id2) {
  	if(this.hasNode(id1)) this.nodes[id1].removeAdjacency(id2);
  	if(this.hasNode(id2)) this.nodes[id2].removeAdjacency(id1);
  },

	/*
	 Method: hasNode
	
	 Returns a Boolean instance indicating if node belongs to graph or not.
	 
	 Parameters:
	
	    id - Node id.

	 Returns:
	  
	 		A Boolean instance indicating if node belongs to graph or not.
	*/	
  hasNode: function(id) {
	return id in this.nodes;
  }
};
/*
   Class: Graph.Node
	
	 Behaviour of the <Graph> node.

*/
/*
   Constructor: Graph.Node

   Node constructor.

   Parameters:

      id - The node *unique identifier* id.
      name - A node's name.
      data - Place to store some extra information (can be left to null).


   Returns:

      A new <Graph.Node> instance.
*/
Graph.Node = function(id, name, data) {
	//Property: id
	//A node's id
	this.id= id;
	//Property: name
	//A node's name
	this.name = name;
	//Property: data
	//The dataSet object <http://blog.thejit.org/?p=7>
	this.data = data;
	//Property: drawn
	//Node flag
	this.drawn= false;
	//Property: angle span
	//allowed angle span for adjacencies placement
	this.angleSpan= {
		begin:0,
		end:0
	};
	//Property: pos
	//node position
	this.pos= new Polar(0, 0);
	//Property: startPos
	//node from position
	this.startPos= new Polar(0, 0);
	//Property: endPos
	//node to position
	this.endPos= new Polar(0, 0);
	//Property: alpha
	//node alpha
	this.alpha = 1;
	//Property: startAlpha
	//node start alpha
	this.startAlpha = 1;
	//Property: endAlpha
	//node end alpha
	this.endAlpha = 1;
	//Property: adjacencies
	//node adjacencies
	this.adjacencies= {};
};

Graph.Node.prototype= {
	
	/*
	   Method: adjacentTo
	
	   Indicates if the node is adjacent to the node indicated by the specified id

	   Parameters:
	
	      id - A node id.
	
	   Returns:
	
	     A Boolean instance indicating whether this node is adjacent to the specified by id or not.
	*/
	adjacentTo: function(node) {
		return node.id in this.adjacencies;
	},

	/*
	   Method: getAdjacency
	
	   Returns a <Graph.Adjacence> that connects the current <Graph.Node> with the node having _id_ as id.

	   Parameters:
	
	      id - A node id.
	*/	
	getAdjacency: function(id) {
		return this.adjacencies[id];
	},
	/*
	   Method: addAdjacency
	
	   Connects the node to the specified by id.

	   Parameters:
	
	      id - A node id.
	*/	
	addAdjacency: function(node, data) {
		var adj = new Graph.Adjacence(this, node, data);
		return this.adjacencies[node.id] = adj;
	},
	
	/*
	   Method: removeAdjacency
	
	   Deletes the <Graph.Adjacence> by _id_.

	   Parameters:
	
	      id - A node id.
	*/	
	removeAdjacency: function(id) {
		delete this.adjacencies[id];
	}
};
/*
   Class: Graph.Adjacence
	
	 Creates a new <Graph> adjacence.

*/
Graph.Adjacence = function(nodeFrom, nodeTo, data) {
	//Property: nodeFrom
	//One of the two <Graph.Node>s connected by this edge.
	this.nodeFrom = nodeFrom;
	//Property: nodeTo
	//One of the two <Graph.Node>s connected by this edge.
	this.nodeTo = nodeTo;
	//Property: data
	//A dataset object
	this.data = data;
	//Property: alpha
	//node alpha
	this.alpha = 1;
	//Property: startAlpha
	//node start alpha
	this.startAlpha = 1;
	//Property: endAlpha
	//node end alpha
	this.endAlpha = 1;
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
	//transition: Trans.linear,
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
