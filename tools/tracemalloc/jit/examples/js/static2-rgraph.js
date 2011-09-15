var Log = {
	elem: $('log'),
	write: function(text) {
		if(!this.elem) this.elem = $('log');
		this.elem.set('html', text);
	}
};

function init() {
	//computes page layout (not a library function, used to adjust some css thingys on the page)
	Infovis.initLayout();
	 //Stores canvas current lineWidth property value
	 var lineW;
	 
	 //Set nodes interpolation to 'polar' (you can also use 'linear')
	 Config.interpolation = 'polar';
	 //Don't draw concentric background circles
	 Config.drawConcentricCircles = false;
	 //Set the distance between concentric circles (or tree levels)
	 Config.levelDistance = 200;
	 //Set animation time
	 Config.animationTime = 2500;


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

	 var json = [{"id":"node0","name":"node0 name","data":[{"key":"weight","value":16.759175934208628},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node1","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node1","name":"node1 name","data":[{"key":"weight","value":13.077119090372014},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node2","name":"node2 name","data":[{"key":"weight","value":24.937383149648717},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":3}},{"nodeTo":"node5","data":{"weight":1}}]},{"id":"node3","name":"node3 name","data":[{"key":"weight","value":10.53272740718869},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":3}},{"nodeTo":"node1","data":{"weight":3}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":3}}]},{"id":"node4","name":"node4 name","data":[{"key":"weight","value":1.3754347037767345},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":1}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node2","data":{"weight":3}},{"nodeTo":"node3","data":{"weight":1}},{"nodeTo":"node5","data":{"weight":3}}]},{"id":"node5","name":"node5 name","data":[{"key":"weight","value":32.26403873194912},{"key":"some other key","value":"some other value"}],"adjacencies":[{"nodeTo":"node0","data":{"weight":1}},{"nodeTo":"node1","data":{"weight":1}},{"nodeTo":"node2","data":{"weight":1}},{"nodeTo":"node3","data":{"weight":3}},{"nodeTo":"node4","data":{"weight":3}}]}];

	 var canvas= new Canvas('infovis', '#ccddee', '#772277');
	 var rgraph= new RGraph(canvas,  {
	  	//Use onBeforePlotLine and onAfterPlotLine controller
	  	//methods to adjust your canvas lineWidth
	  	//parameter in order to plot weighted edges on 
	  	//your graph. You can also change the color of the lines.
	  	onBeforePlotLine: function(adj) {
	  			lineW = canvas.getContext().lineWidth;
	  			canvas.getContext().lineWidth = adj.data.weight;
	  	},
	  	
	  	onAfterPlotLine: function(adj) {
	  			canvas.getContext().lineWidth = lineW;
	  	},
	  	
	  	onBeforeCompute: function(node) {
	  		Log.write("centering " + node.name + "...");
			var html = "<h4>" + node.name + "</h4><b>Connections:</b>";
			html += "<ul>";
			GraphUtil.eachAdjacency(node, function(adj) {
				var child = adj.nodeTo;
				html += "<li>" + child.name+ "</li>";
			});
			html+= "</ul>";
			$('inner-details').set("html", html);
	  	},
	  	
    //Assign html to each label and some effects.	
	onCreateLabel: function(domElement, node) {
		var elem = $(domElement);
		elem.setOpacity(0.6).set('tween', { duration: 300 }).set('html', node.name).addEvents({

			'mouseenter': function() {
				elem.tween('opacity', 1);
			},
			
			'mouseleave': function() {
				elem.tween('opacity', 0.6);
			},
			
			'click': function() {
					rgraph.onClick(node.id);
				}
			});
	},
	
	//Take off previous width and height styles and
	//add half of the *actual* label width to the left position
	// That will center your label (do the math man). 
	onPlaceLabel: function(domElement, node) {
		domElement.innerHTML = '';
		domElement.innerHTML = node.name;
		var left = parseInt(domElement.style.left);
		domElement.style.width = '';
		domElement.style.height = '';
		var w = domElement.offsetWidth;
		domElement.style.left = (left - w /2) + 'px';
	},
	
	onAfterCompute: function() {
		Log.write("done");
	}
	  	
	});
	  
	  //load weighted graph.
	 rgraph.loadGraphFromJSON(json, 1);
	  //compute positions and plot = refresh.
	  rgraph.refresh();
	  rgraph.controller.onBeforeCompute(GraphUtil.getNode(rgraph.graph, rgraph.root));
	  rgraph.controller.onAfterCompute();
}
