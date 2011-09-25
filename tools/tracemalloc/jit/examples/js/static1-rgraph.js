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
	//Set node interpolation to linear (can also be setted to 'polar')
	Config.interpolation = "linear";
	//Set distance for concentric circles
	Config.levelDistance = 100;
	//Set number of concentric circles, default's to six.
	Config.drawConcentricCircles = 4;
	
	var json= {"id":"190_0","name":"Pearl Jam","children":[{"id":"306208_1","name":"Pearl Jam &amp; Cypress Hill","data":[{"key":"Pearl Jam","value":"collaboration"}],"children":[{"id":"84_2","name":"Cypress Hill","data":[{"key":"Pearl Jam &amp; Cypress Hill","value":"collaboration"}],"children":[]}]},{"id":"107877_3","name":"Neil Young &amp; Pearl Jam","data":[{"key":"Pearl Jam","value":"collaboration"}],"children":[{"id":"964_4","name":"Neil Young","data":[{"key":"Neil Young &amp; Pearl Jam","value":"collaboration"}],"children":[]}]},{"id":"236797_5","name":"Jeff Ament","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"1756_6","name":"Temple of the Dog","data":[{"key":"Jeff Ament","value":"member of band"}],"children":[]},{"id":"14581_7","name":"Mother Love Bone","data":[{"key":"Jeff Ament","value":"member of band"}],"children":[]},{"id":"50188_8","name":"Green River","data":[{"key":"Jeff Ament","value":"member of band"}],"children":[]},{"id":"65452_9","name":"M.A.C.C.","data":[{"key":"Jeff Ament","value":"collaboration"}],"children":[]},{"id":"115632_10","name":"Three Fish","data":[{"key":"Jeff Ament","value":"member of band"}],"children":[]},{"id":"346850_11","name":"Gossman Project","data":[{"key":"Jeff Ament","value":"member of band"}],"children":[]}]},{"id":"41529_12","name":"Stone Gossard","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"1756_13","name":"Temple of the Dog","data":[{"key":"Stone Gossard","value":"member of band"}],"children":[]},{"id":"14581_14","name":"Mother Love Bone","data":[{"key":"Stone Gossard","value":"member of band"}],"children":[]},{"id":"24119_15","name":"Brad","data":[{"key":"Stone Gossard","value":"member of band"}],"children":[]},{"id":"50188_16","name":"Green River","data":[{"key":"Stone Gossard","value":"member of band"}],"children":[]},{"id":"346850_17","name":"Gossman Project","data":[{"key":"Stone Gossard","value":"member of band"}],"children":[]}]},{"id":"131161_18","name":"Eddie Vedder","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"1756_19","name":"Temple of the Dog","data":[{"key":"Eddie Vedder","value":"member of band"}],"children":[]},{"id":"72007_20","name":"Eddie Vedder &amp; Zeke","data":[{"key":"Eddie Vedder","value":"collaboration"}],"children":[]},{"id":"236657_21","name":"Bad Radio","data":[{"key":"Eddie Vedder","value":"member of band"}],"children":[]},{"id":"432176_22","name":"Beck &amp; Eddie Vedder","data":[{"key":"Eddie Vedder","value":"collaboration"}],"children":[]}]},{"id":"236583_23","name":"Mike McCready","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"1744_24","name":"Mad Season","data":[{"key":"Mike McCready","value":"member of band"}],"children":[]},{"id":"1756_25","name":"Temple of the Dog","data":[{"key":"Mike McCready","value":"member of band"}],"children":[]},{"id":"43661_26","name":"$10,000 Gold Chain","data":[{"key":"Mike McCready","value":"collaboration"}],"children":[]},{"id":"65452_27","name":"M.A.C.C.","data":[{"key":"Mike McCready","value":"collaboration"}],"children":[]},{"id":"153766_28","name":"The Rockfords","data":[{"key":"Mike McCready","value":"member of band"}],"children":[]},{"id":"346850_29","name":"Gossman Project","data":[{"key":"Mike McCready","value":"member of band"}],"children":[]}]},{"id":"236585_30","name":"Matt Cameron","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"1111_31","name":"Soundgarden","data":[{"key":"Matt Cameron","value":"member of band"}],"children":[]},{"id":"1756_32","name":"Temple of the Dog","data":[{"key":"Matt Cameron","value":"member of band"}],"children":[]},{"id":"9570_33","name":"Eleven","data":[{"key":"Matt Cameron","value":"supporting musician"}],"children":[]},{"id":"11783_34","name":"Queens of the Stone Age","data":[{"key":"Matt Cameron","value":"member of band"}],"children":[]},{"id":"61972_35","name":"Wellwater Conspiracy","data":[{"key":"Matt Cameron","value":"member of band"}],"children":[]},{"id":"65452_36","name":"M.A.C.C.","data":[{"key":"Matt Cameron","value":"collaboration"}],"children":[]},{"id":"353097_37","name":"Tone Dogs","data":[{"key":"Matt Cameron","value":"member of band"}],"children":[]}]},{"id":"236594_38","name":"Dave Krusen","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"2092_39","name":"Candlebox","data":[{"key":"Dave Krusen","value":"member of band"}],"children":[]}]},{"id":"236022_40","name":"Matt Chamberlain","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"54761_41","name":"Critters Buggin","data":[{"key":"Matt Chamberlain","value":"member of band"}],"children":[]},{"id":"92043_42","name":"Edie Brickell and New Bohemians","data":[{"key":"Matt Chamberlain","value":"member of band"}],"children":[]}]},{"id":"236611_43","name":"Dave Abbruzzese","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"276933_44","name":"Green Romance Orchestra","data":[{"key":"Dave Abbruzzese","value":"member of band"}],"children":[]}]},{"id":"236612_45","name":"Jack Irons","data":[{"key":"Pearl Jam","value":"member of band"}],"children":[{"id":"4619_46","name":"Redd Kross","data":[{"key":"Jack Irons","value":"member of band"}],"children":[]},{"id":"9570_47","name":"Eleven","data":[{"key":"Jack Irons","value":"member of band"}],"children":[]},{"id":"12389_48","name":"Red Hot Chili Peppers","data":[{"key":"Jack Irons","value":"member of band"}],"children":[]},{"id":"114288_49","name":"Anthym","data":[{"key":"Jack Irons","value":"member of band"}],"children":[]},{"id":"240013_50","name":"What Is This?","data":[{"key":"Jack Irons","value":"member of band"}],"children":[]}]}],"data":[]};//{"id":"node02","name":"0.2","data":[{"key":"key1","value":8},{"key":"key2","value":-88}],"children":[{"id":"node13","name":"1.3","data":[{"key":"key1","value":8},{"key":"key2","value":74}],"children":[{"id":"node24","name":"2.4","data":[{"key":"key1","value":10},{"key":"key2","value":55}],"children":[]},{"id":"node25","name":"2.5","data":[{"key":"key1","value":8},{"key":"key2","value":67}],"children":[]},{"id":"node26","name":"2.6","data":[{"key":"key1","value":5},{"key":"key2","value":-50}],"children":[]},{"id":"node27","name":"2.7","data":[{"key":"key1","value":8},{"key":"key2","value":10}],"children":[]},{"id":"node28","name":"2.8","data":[{"key":"key1","value":2},{"key":"key2","value":-69}],"children":[]},{"id":"node29","name":"2.9","data":[{"key":"key1","value":3},{"key":"key2","value":98}],"children":[]},{"id":"node210","name":"2.10","data":[{"key":"key1","value":6},{"key":"key2","value":12}],"children":[]},{"id":"node211","name":"2.11","data":[{"key":"key1","value":2},{"key":"key2","value":-95}],"children":[]}]},{"id":"node112","name":"1.12","data":[{"key":"key1","value":1},{"key":"key2","value":96}],"children":[{"id":"node213","name":"2.13","data":[{"key":"key1","value":6},{"key":"key2","value":-58}],"children":[]},{"id":"node214","name":"2.14","data":[{"key":"key1","value":9},{"key":"key2","value":-42}],"children":[]},{"id":"node215","name":"2.15","data":[{"key":"key1","value":10},{"key":"key2","value":92}],"children":[]},{"id":"node216","name":"2.16","data":[{"key":"key1","value":7},{"key":"key2","value":-15}],"children":[]},{"id":"node217","name":"2.17","data":[{"key":"key1","value":3},{"key":"key2","value":29}],"children":[]},{"id":"node218","name":"2.18","data":[{"key":"key1","value":8},{"key":"key2","value":-59}],"children":[]},{"id":"node219","name":"2.19","data":[{"key":"key1","value":3},{"key":"key2","value":21}],"children":[]},{"id":"node220","name":"2.20","data":[{"key":"key1","value":2},{"key":"key2","value":78}],"children":[]}]},{"id":"node121","name":"1.21","data":[{"key":"key1","value":3},{"key":"key2","value":53}],"children":[{"id":"node222","name":"2.22","data":[{"key":"key1","value":5},{"key":"key2","value":10}],"children":[]},{"id":"node223","name":"2.23","data":[{"key":"key1","value":10},{"key":"key2","value":21}],"children":[]},{"id":"node224","name":"2.24","data":[{"key":"key1","value":6},{"key":"key2","value":-32}],"children":[]},{"id":"node225","name":"2.25","data":[{"key":"key1","value":5},{"key":"key2","value":-42}],"children":[]},{"id":"node226","name":"2.26","data":[{"key":"key1","value":2},{"key":"key2","value":75}],"children":[]},{"id":"node227","name":"2.27","data":[{"key":"key1","value":1},{"key":"key2","value":-74}],"children":[]},{"id":"node228","name":"2.28","data":[{"key":"key1","value":2},{"key":"key2","value":52}],"children":[]},{"id":"node229","name":"2.29","data":[{"key":"key1","value":10},{"key":"key2","value":-49}],"children":[]}]},{"id":"node130","name":"1.30","data":[{"key":"key1","value":9},{"key":"key2","value":-29}],"children":[{"id":"node231","name":"2.31","data":[{"key":"key1","value":6},{"key":"key2","value":-23}],"children":[]},{"id":"node232","name":"2.32","data":[{"key":"key1","value":10},{"key":"key2","value":19}],"children":[]},{"id":"node233","name":"2.33","data":[{"key":"key1","value":1},{"key":"key2","value":92}],"children":[]}]},{"id":"node134","name":"1.34","data":[{"key":"key1","value":9},{"key":"key2","value":71}],"children":[{"id":"node235","name":"2.35","data":[{"key":"key1","value":5},{"key":"key2","value":-65}],"children":[]}]},{"id":"node136","name":"1.36","data":[{"key":"key1","value":3},{"key":"key2","value":-11}],"children":[{"id":"node237","name":"2.37","data":[{"key":"key1","value":6},{"key":"key2","value":-85}],"children":[]},{"id":"node238","name":"2.38","data":[{"key":"key1","value":3},{"key":"key2","value":-13}],"children":[]},{"id":"node239","name":"2.39","data":[{"key":"key1","value":1},{"key":"key2","value":80}],"children":[]},{"id":"node240","name":"2.40","data":[{"key":"key1","value":10},{"key":"key2","value":-69}],"children":[]}]},{"id":"node141","name":"1.41","data":[{"key":"key1","value":10},{"key":"key2","value":-4}],"children":[{"id":"node242","name":"2.42","data":[{"key":"key1","value":8},{"key":"key2","value":-27}],"children":[]},{"id":"node243","name":"2.43","data":[{"key":"key1","value":9},{"key":"key2","value":-44}],"children":[]},{"id":"node244","name":"2.44","data":[{"key":"key1","value":9},{"key":"key2","value":24}],"children":[]},{"id":"node245","name":"2.45","data":[{"key":"key1","value":8},{"key":"key2","value":-66}],"children":[]}]}]};

	var canvas= new Canvas('infovis', '#ccddee', '#772277');

	var rgraph= new RGraph(canvas,  {
	  	onBeforeCompute: function(node) {
	  		Log.write("centering " + node.name + "...");
			var _self = this;
			var html = "<h4>" + node.name + "</h4><b>Connections:</b>";
			html += "<ul>";
			GraphUtil.eachAdjacency(node, function(adj) {
				var child = adj.nodeTo;
				if(child.data && child.data.length > 0) {
					html += "<li>" + child.name + " " + "<div class=\"relation\">(relation: " + _self.getName(node, child) + ")</div></li>";
				}
			});
			html+= "</ul>";
			$('inner-details').set("html", html);
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
	  	
	  //Add a controller to assign the node's name to the created label.	
	onCreateLabel: function(domElement, node) {
		var d = $(domElement);
		d.setOpacity(0.6).set('tween', { duration: 300 }).set('html', node.name).addEvents({
			'mouseenter': function() {
				d.tween('opacity', 1);
			},
			
			'mouseleave': function() {
				d.tween('opacity', 0.6);
			},
			
			'click': function() {
				rgraph.onClick(d.id);
			}
		});
		
	},
	
	//Take off previous width and height styles and
	//add half of the *actual* label width to the left position
	// That will center your label (do the math man). 
	onPlaceLabel: function(domElement, node) {
		domElement.innerHTML = '';
		 if(node._depth <= 1) {
			domElement.innerHTML = node.name;
			var left = parseInt(domElement.style.left);
			domElement.style.width = '';
			domElement.style.height = '';
			var w = domElement.offsetWidth;
			domElement.style.left = (left - w /2) + 'px';
	
		} 
	},
	
	onAfterCompute: function() {
		Log.write("done");
		}
	  	
	  });
	    
	  //load tree from tree data.
	  rgraph.loadTreeFromJSON(json);
	  //compute positions and plot = refresh.
	  rgraph.refresh();
	
	  rgraph.controller.onBeforeCompute(GraphUtil.getNode(rgraph.graph, rgraph.root));
	  rgraph.controller.onAfterCompute();
}