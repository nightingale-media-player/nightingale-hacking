/*
Author:
	luistar15, <leo020588 [at] gmail.com>
	with modifications by Stephen Lau <stevel [at] songbirdnest.com>
License:
	MIT License
 
Class
	noobSlide (rev.19-06-08)

Arguments:
	Parameters - see Parameters below

Parameters:
	box: dom element | required
	items: dom collection | required
	size: int | item size (px) | default: 240
	mode: string | 'horizontal', 'vertical' | default: 'horizontal'
	addButtons:{
		previous: single dom element OR dom collection| default: null
		next:  single dom element OR dom collection | default: null
		play:  single dom element OR dom collection | default: null
		playback:  single dom element OR dom collection | default: null
		stop:  single dom element OR dom collection | default: null
	}
	button_event: string | event type | default: 'click'
	handles: dom collection | default: null
	handle_event: string | event type| default: 'click'
	fxOptions: object | Fx.Tween options | default: {duration:500,wait:false}
	interval: int | for periodical | default: 5000
	autoPlay: boolean | default: false
	onWalk: event | pass arguments: currentItem, currentHandle | default: null
	startItem: int | default: 0

Properties:
	box: dom element
	items: dom collection
	size: int
	mode: string
	buttons: object
	button_event: string
	handles: dom collection
	handle_event: string
	previousIndex: int
	nextIndex: int
	fx: Fx.Tween instance
	interval: int
	autoPlay: boolean
	onWalk: function
	
Methods:
	previous(manual): walk to previous item
		manual: bolean | default:false
	next(manual): walk to next item
		manual: bolean | default:false
	play (interval,direction,wait): auto walk items
		interval: int | required
		direction: string | "previous" or "next" | required
		wait: boolean | required
	stop(): stop auto walk
	walk(item,manual,noFx): walk to item
		item: int | required
		manual: bolean | default:false
		noFx: boolean | default:false
	addHandleButtons(handles):
		handles: dom collection | required
	addActionButtons(action,buttons):
		action: string | "previous", "next", "play", "playback", "stop" | required
		buttons: dom collection | required

Requires:
	mootools 1.2 core
*/
var noobSlide = new Class({

	initialize: function(params){
		this.items = params.items;
		this.mode = params.mode || 'horizontal';
		this.modes = {horizontal:['left','width'], vertical:['top','height']};
		this.size = params.size || 240;
		this.box = params.box.setStyle(this.modes[this.mode][1],(this.size*this.items.length)+'px');
		this.button_event = params.button_event || 'click';
		this.handle_event = params.handle_event || 'click';
		this.onButtons = params.onButtons || null;
		this.onWalk = params.onWalk || null;
		this.currentIndex = null;
		this.previousIndex = null;
		this.nextIndex = null;
		this.interval = params.interval || 5000;
		this.autoPlay = params.autoPlay || false;
		this._play = null;
		this.handles = params.handles || null;
		if(this.handles){
			this.addHandleButtons(this.handles);
		}
		this.buttons = {
			previous: [],
			next: [],
			play: [],
			playback: [],
			stop: [],
			playpause: []
		};
		if(params.addButtons){
			for(var action in params.addButtons){
				this.addActionButtons(action, $type(params.addButtons[action])=='array' ? params.addButtons[action] : [params.addButtons[action]]);
			}
		}
		this.fx = new Fx.Tween(this.box,$extend((params.fxOptions||{duration:500,wait:false}),{property:this.modes[this.mode][0]}));
		this.walk((params.startItem||0),true,true);
	},

	addHandleButtons: function(handles){
		for(var i=0;i<handles.length;i++){
			handles[i].addEvent(this.handle_event,this.walk.bind(this,[i,true]));
		}
	},

	addActionButtons: function(action,buttons){
		for(var i=0; i<buttons.length; i++){
			switch(action){
				case 'previous': buttons[i].addEvent(this.button_event,this.previous.bind(this,[true])); break;
				case 'next': buttons[i].addEvent(this.button_event,this.next.bind(this,[true])); break;
				case 'play': buttons[i].addEvent(this.button_event,this.play.bind(this,[this.interval,'next',false])); break;
				case 'playback': buttons[i].addEvent(this.button_event,this.play.bind(this,[this.interval,'previous',false])); break;
				case 'stop': buttons[i].addEvent(this.button_event,this.stop.bind(this)); break;
				case 'playpause': buttons[i].addEvent(this.button_event,this.playpause.bind(this,[this.interval,'next'])); break;
			}
			this.buttons[action].push(buttons[i]);
		}
	},

	cleanup: function() {
		for (var commands in this.buttons) {
			for (var button in commands) {
				button.removeEvents();
			}
		}
	},

	previous: function(manual){
		this.walk((this.currentIndex>0 ? this.currentIndex-1 : this.items.length-1),false);
	},

	next: function(manual){
		this.walk((this.currentIndex<this.items.length-1 ? this.currentIndex+1 : 0),false);
	},

	play: function(interval,direction,wait){
		this.isPlaying = true;
		this.stop();
		if(!wait){
			this[direction](false);
		}
		this._play = this[direction].periodical(interval,this,[false]);
	},

	playpause: function(interval,direction) {
		if (this.isPlaying) {
			$clear(this._play);
			this.isPlaying = false;
		} else {
			this._play = this[direction].periodical(interval,this,[false]);
			this.isPlaying = true;
		}

		if (this.onButtons) {
			this.onButtons("playpause", this.isPlaying);
		}
	},

	stop: function(){
		$clear(this._play);
	},

	gotoSlide: function(index) {
		//dump("proceeding to slide: " + index);
		this.walk(index, true, true);
	},

	walk: function(item,manual,noFx){
		if(item!=this.currentIndex){
			this.currentIndex=item;
			if (this.currentIndex < 0)
				this.currentIndex = 0;
			//dump("> advancing track:" + this.items[this.currentIndex].title + "\n");
			this.previousIndex = this.currentIndex + (this.currentIndex>0 ? -1 : this.items.length-1);
			this.nextIndex = this.currentIndex + (this.currentIndex<this.items.length-1 ? 1 : 1-this.items.length);
			if(manual){
				this.stop();
			}
			if(noFx){
				this.fx.cancel().set((this.size*-this.currentIndex)+'px');
			}else{
				this.fx.start(0-this.items[this.currentIndex].el.offsetLeft);
			}
			if(manual && this.autoPlay){
				this.play(this.interval,'next',true);
			}
			if(this.onWalk){
				this.onWalk((this.items[this.currentIndex] || null), (this.handles && this.handles[this.currentIndex] ? this.handles[this.currentIndex] : null));
			}
		}
	}
	
});
