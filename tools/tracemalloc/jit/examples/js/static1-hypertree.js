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

	var json ={"id":"347_0","name":"Nine Inch Nails","children":[{"id":"126510_1","name":"Jerome Dillon","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"52163_2","name":"Howlin' Maggie","data":[{"key":"Jerome Dillon","value":"member of band"}],"children":[]},{"id":"324134_3","name":"nearLY","data":[{"key":"Jerome Dillon","value":"member of band"}],"children":[]}]},{"id":"173871_4","name":"Charlie Clouser","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[]},{"id":"235952_5","name":"James Woolley","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[]},{"id":"235951_6","name":"Jeff Ward","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"2382_7","name":"Ministry","data":[{"key":"Jeff Ward","value":"member of band"}],"children":[]},{"id":"2415_8","name":"Revolting Cocks","data":[{"key":"Jeff Ward","value":"member of band"}],"children":[]},{"id":"3963_9","name":"Pigface","data":[{"key":"Jeff Ward","value":"member of band"}],"children":[]},{"id":"7848_10","name":"Lard","data":[{"key":"Jeff Ward","value":"member of band"}],"children":[]}]},{"id":"235950_11","name":"Richard Patrick","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"1007_12","name":"Filter","data":[{"key":"Richard Patrick","value":"member of band"}],"children":[]},{"id":"327924_13","name":"Army of Anyone","data":[{"key":"Richard Patrick","value":"member of band"}],"children":[]}]},{"id":"2396_14","name":"Trent Reznor","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"3963_15","name":"Pigface","data":[{"key":"Trent Reznor","value":"member of band"}],"children":[]},{"id":"32247_16","name":"1000 Homo DJs","data":[{"key":"Trent Reznor","value":"member of band"}],"children":[]},{"id":"83761_17","name":"Option 30","data":[{"key":"Trent Reznor","value":"member of band"}],"children":[]},{"id":"133257_18","name":"Exotic Birds","data":[{"key":"Trent Reznor","value":"member of band"}],"children":[]}]},{"id":"36352_19","name":"Chris Vrenna","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"1013_20","name":"Stabbing Westward","data":[{"key":"Chris Vrenna","value":"member of band"}],"children":[]},{"id":"3963_21","name":"Pigface","data":[{"key":"Chris Vrenna","value":"member of band"}],"children":[]},{"id":"5752_22","name":"Jack Off Jill","data":[{"key":"Chris Vrenna","value":"member of band"}],"children":[]},{"id":"33602_23","name":"Die Warzau","data":[{"key":"Chris Vrenna","value":"member of band"}],"children":[]},{"id":"40485_24","name":"tweaker","data":[{"key":"Chris Vrenna","value":"is person"}],"children":[]},{"id":"133257_25","name":"Exotic Birds","data":[{"key":"Chris Vrenna","value":"member of band"}],"children":[]}]},{"id":"236021_26","name":"Aaron North","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[]},{"id":"236024_27","name":"Jeordie White","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"909_28","name":"A Perfect Circle","data":[{"key":"Jeordie White","value":"member of band"}],"children":[]},{"id":"237377_29","name":"Twiggy Ramirez","data":[{"key":"Jeordie White","value":"is person"}],"children":[]}]},{"id":"235953_30","name":"Robin Finck","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"1440_31","name":"Guns N' Roses","data":[{"key":"Robin Finck","value":"member of band"}],"children":[]}]},{"id":"235955_32","name":"Danny Lohner","data":[{"key":"Nine Inch Nails","value":"member of band"}],"children":[{"id":"909_33","name":"A Perfect Circle","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"1695_34","name":"Killing Joke","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"1938_35","name":"Methods of Mayhem","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"5138_36","name":"Skrew","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"53549_37","name":"Angkor Wat","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"113510_38","name":"Puscifer","data":[{"key":"Danny Lohner","value":"member of band"}],"children":[]},{"id":"113512_39","name":"Renhold\u00ebr","data":[{"key":"Danny Lohner","value":"is person"}],"children":[]}]}],"data":[]};

	//Draw main background circle	
	Config.drawMainCircle = false;

	var canvas= new Canvas('infovis', '#ddd', '#ddd');

	var ht= new Hypertree(canvas, {
	
	onBeforeCompute: function(node) {
		Log.write("centering");
	},
	
	getName: function(node1, node2) {
		for(var i=0; i<node1.data.length; i++) {
			var dataset = node1.data[i];
			if(dataset.key == node2.name) return dataset.value;
		}
		
		for(var i=0; i<node2.data.length; i++) {
			var dataset = node2.data[i];
			if(dataset.key == node1.name) return dataset.value;
		}
	},
		
	onCreateLabel: function(domElement, node) {
		var d = $(domElement);
		d.set('tween', { duration: 300 }).set('html', node.name).setOpacity(0.8).addEvents({
	
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
		var that = this;

		//set details for this node.
		var html = "<h4>" + node.name + "</h4><b>Connections:</b>";
		html += "<ul>";
		GraphUtil.eachAdjacency(node, function(adj) {
			var child = adj.nodeTo;
			if(child.data && child.data.length > 0) {
				html += "<li>" + child.name + " " + "<div class=\"relation\">(relation: " + that.getName(node, child) + ")</div></li>";
			}
		});
		html+= "</ul>";
		$('inner-details').set("html", html);
		
		//hide labels that aren't directly connected to the centered node.
		var GPlot = GraphPlot;
		GraphUtil.eachNode(ht.graph, function(elem) {
			if(elem.id != node.id && !node.adjacentTo(elem)){
				GPlot.hideLabel(elem);
			} 
		});
	}
	  	
	});
	//load a JSON tree structure.  
	ht.loadTreeFromJSON(json);
	//compute positions then plot.
	ht.refresh();
	//optional: set an "onclick" event handler on the canvas tag to animate the tree.
	ht.prepareCanvasEvents();
	ht.controller.onAfterCompute();
}