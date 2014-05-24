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
	//Create random generated tree.
	  var json= Feeder.makeTree();
	  //Create a new canvas instance.
	  var canvas= new Canvas('infovis');
	  //Create a new ST instance
	  st= new ST(canvas, {
		onBeforeCompute: function(node) {
				Log.write("loading " + node.name);
		},
		
		onAfterCompute: function() {
			Log.write("done");
		},
			
		request: function(nodeId, level, onComplete) {
			Feeder.request(nodeId, level, onComplete);
		}
	  });
	  //load json data
	  st.loadFromJSON(json);
	  //compute node positions and layout
	  st.compute();
	  //optional: make a translation of the tree
	  Tree.Geometry.translate(st.tree, new Complex(-200, 0), "startPos");
	  //Emulate a click on the root node.
	  st.onClick(st.tree.id);
	  
	  //Add click handler to switch spacetree orientation.
	  var checkbox = document.getElementById('switch');
	  checkbox.onclick = function () {
	  	st.switchPosition({
	  		onComplete: function() {}
	  	});
	  };
}

//Just a random tree generator. This code is pretty ugly and not useful to read at all.
//The only thing that does is to provide a random JSON tree with the specified depth level.
var Feeder = {
	counter:0,
	
	p: {
		idPrefix: "node",
		levelStart: 0,
		levelEnd: 3,
		maxChildrenPerNode: 5,
		counter: 0
	},
	
	makeTree: function () {
		var le = this.p.levelEnd;
		if(le == 0) return {children: []};
		this.counter = 1;
		return this.makeTreeWithParameters(this.p.idPrefix,
										   this.p.levelStart,
										   this.p.levelEnd, 
										   this.p.maxChildrenPerNode);
	},
	
	makeTreeWithParameters: function(idPrefix, levelStart, levelEnd, maxChildrenPerNode) {
		if(levelStart == levelEnd) return null;
		this.counter++;
		var numb = Math.floor(Math.random() * 10) + 1;
		var numb2 = Math.floor(Math.random() * 200 - 100);
		var ans= {
			id:   idPrefix + levelStart + this.counter,
			name: levelStart + "." + this.counter,
			data: [
				{key: 'key1', value: numb},
				{key: 'key2', value: numb2}
			],
			children: []
		};
		var childCount= Math.floor(Math.random() * maxChildrenPerNode) + 1;
		if(childCount == 1 && levelStart == 0) childCount++;
		levelStart++;
		for(var i=0; i<childCount; i++) {
			var ch= this.makeTreeWithParameters(idPrefix, levelStart, levelEnd, maxChildrenPerNode);
			if(ch != null) ans.children[i]=ch;
		}
		return ans;
   },
  
   request: function (nodeId, level, onComplete) {
   		this.p.idPrefix = nodeId;
   		this.p.levelEnd = level + 1;
   		var json = this.makeTree();
   		onComplete.onComplete(nodeId, json);
   }
};
