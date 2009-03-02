var Log = {
	elem: $('log'),
	write: function(text) {
		if(!this.elem) this.elem = $('log');
		this.elem.set('html', text);
	}
};

function init() {
	var lineW;
	//computes page layout (not a library function, used to adjust some css thingys on the page)
	Infovis.initLayout();
	//Don't draw the background main circle
	Config.drawMainCircle = false;

	 //Allow weighted nodes. If setted to true, 
	 //it will take the first data set object value property (i.e node.data[0].value)
	 //to set the node diameter. 
	 Config.allowVariableNodeDiameters = true;
	//Set node diameters range. For variable node weights.
	Config.nodeRangeDiameters = {
		min: 10,
		max: 35
	};
	//The interval of the values of the first object of your dataSet array. A superset of the values can also be specified.
	Config.nodeRangeValues = {
		min: 1,
		max: 35
	};
	//If setted to false, the hypertree nodes won't be transformed by the moebius transformation.
	//This is usefull when showing weighted nodes, since you might want to keep the proportions
	//of the nodes comparable.
	Config.transformNodes = false;


	var json = [{"id":"node0","name":"node0 name","data":[{"key":"weight","value":16.759175934208628},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node1","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node1","name":"node1 name","data":[{"key":"weight","value":13.077119090372014},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node2","name":"node2 name","data":[{"key":"weight","value":24.937383149648717},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":3}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node3","name":"node3 name","data":[{"key":"weight","value":10.53272740718869},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node1","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":3}}]},{"id":"node4","name":"node4 name","data":[{"key":"weight","value":1.3754347037767345},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":1}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node3","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":3}}]},{"id":"node5","name":"node5 name","data":[{"key":"weight","value":32.26403873194912},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":1}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node2","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":3}}]}];
	var canvas= new Canvas('infovis', '#fff', '#dd00bb');
	var ht= new Hypertree(canvas,  {
	  	//Use onBeforePlotLine and onAfterPlotLine controller methods to
	  	//adjust your canvas lineWidth parameter in order to plot weighted
	  	//edges on your graph. You could also change the color of the lines.
		  	onBeforePlotLine: function(adj) {
		  			lineW = canvas.getContext().lineWidth;
		  			canvas.getContext().lineWidth = adj.data.weight;
		  	},
		  	
		  	onAfterPlotLine: function(adj) {
		  			canvas.getContext().lineWidth = lineW;
		  	},
		  	
			onBeforeCompute: function(node) {
		  		Log.write("centering");
		  	},
			
		  	onCreateLabel: function(domElement, node) {
		  		var d = $(domElement);
				d.set('tween', { duration: 300 }).setOpacity(0.8).set('html', node.name).addEvents({
		//Call the "onclick" method from the hypertree to move the hypertree correspondingly.
		//This method takes the native event object. Since Mootools uses a wrapper for this 
		//event, I have to put e.event to get the native event object.
				'click': function(e) {
					ht.onClick(e.event);
				},

	  			'mouseenter': function() {
	  				d.tween('opacity', 1);
	  			},
	  			
	  			'mouseleave': function() {
	  				d.tween('opacity', 0.8);
	  			}
	  		});
	  	 },
	  	 
	  	//Take the left style property and substract half of the label actual width.
	  	onPlaceLabel: function(tag, node) {
	  		var width = tag.offsetWidth;
	  		var intX = tag.style.left.toInt();
	  		intX -= width/2;
	  		tag.style.left = intX + 'px';
	  	},
	  	
	  	onAfterCompute: function() {
	  		Log.write("done");
	  		var node = GraphUtil.getClosestNodeToOrigin(ht.graph, "pos");
	  		var html = "<h4>" + node.name + "</h4><b>Connections:</b>";
	  		html += "<ul>";
	 		GraphUtil.eachAdjacency(node, function(adj) {
	 			var child = adj.nodeTo;
 				html += "<li>" + child.name + "</li>";
	 		});
	 		html+= "</ul>";
	  		$('inner-details').set("html", html);
	  	}
	});
	//load a JSON graph structure.
	ht.loadGraphFromJSON(json, 2);
	//compute positions then plot.
	ht.refresh();
	ht.controller.onBeforeCompute(GraphUtil.getNode(ht.graph, ht.root));
	ht.controller.onAfterCompute();
	
}