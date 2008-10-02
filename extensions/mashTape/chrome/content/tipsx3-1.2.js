/*
Script: TipsX3.js
	Tooltips, BubbleTips, whatever they are, they will appear on mouseover

License:
	MIT-style license.

Credits:
	The idea behind Tips.js is based on Bubble Tooltips (<http://web-graphics.com/mtarchive/001717.php>) by Alessandro Fulcitiniti <http://web-graphics.com>
	TipsX3.js is based on Tips.js, with slight modifications, by razvan@e-magine.ro
*/

/*
Class: TipsX3
	Display a tip on any element with a title and/or href.

Note:
	Tips requires an XHTML doctype.

Arguments:
	elements - a collection of elements to apply the tooltips to on mouseover.
	options - an object. See options Below.

Options:
	maxTitleChars - the maximum number of characters to display in the title of the tip. defaults to 30.

	onShow - optionally you can alter the default onShow behaviour with this option (like displaying a fade in effect);
	onHide - optionally you can alter the default onHide behaviour with this option (like displaying a fade out effect);

	showDelay - the delay the onShow method is called. (defaults to 100 ms)
	hideDelay - the delay the onHide method is called. (defaults to 100 ms)

	className - the prefix for your tooltip classNames. defaults to 'tool'.

		the whole tooltip will have as classname: tool-tip

		the title will have as classname: tool-title

		the text will have as classname: tool-text

	offsets - the distance of your tooltip from the mouse. an Object with x/y properties.
	fixed - if set to true, the toolTip will not follow the mouse.
	
	loadingText - text to display as a title while loading an AJAX tooltip.
	
	errTitle, errText - text to display when there's a problem with the AJAX request.

Example:
	(start code)
	<img src="/images/i.png" title="The body of the tooltip is stored in the title" class="toolTipImg"/>
	<script>
		var myTips = new Tips($$('.toolTipImg'), {
			maxTitleChars: 50	//I like my captions a little long
		});
	</script>
	(end)

Note:
	The title of the element will always be used as the tooltip body. If you put :: on your title, the text before :: will become the tooltip title.
	If you put DOM:someElementID in your title, $('someElementID').innerHTML will be used as your tooltip contents (same syntax as above).
	If you put AJAX:http://www.example.com/path/to/ajax_file.php in your title, the response text will be used as tooltip contents (same syntax as above). Either absolute or relative paths are ok.
*/

var TipsX3 = new Class({

	options: { // modded for X3
		onShow: function(tip){
			tip.setStyle('visibility', 'visible');
		},
		onHide: function(tip){
			tip.setStyle('visibility', 'hidden');
		},
		maxTitleChars: 30,
		showDelay: 100,
		hideDelay: 100,
		className: 'tool',
		offsets: {'x': 16, 'y': 16},
		fixed: false,
		loadingText: 'Loading...',
		errTitle: 'Oops..',
		errText: 'There was a problem retrieving the tooltip.'
	},

	initialize: function(elements, options){
		this.setOptions(options);
		this.toolTip = new Element('div', {
			'class': this.options.className + '-tip',
			'styles': {
				'position': 'absolute',
				'top': '0',
				'left': '0',
				'visibility': 'hidden'
			}
		}).inject(document.body);
		this.wrapper = new Element('div').inject(this.toolTip);
		$$(elements).each(this.build, this);
		if (this.options.initialize) this.options.initialize.call(this);
	},

	build: function(el){ // modded for X3
		el.myTitle = (el.href && el.get('tag') == 'a') ? el.href.replace('http://', '') : (el.rel || false);
		if (el.title){
			
			// check if we need to extract contents from a DOM element
			if (el.title.test('^DOM:', 'i')) {
				el.title = $(el.title.split(':')[1].trim()).innerHTML;
			}
			
			// check for an URL to retrieve content from
			if (el.title.test('^AJAX:', 'i')) {
				el.title = this.options.loadingText + '::' + el.title;
			}
					
			var dual = el.title.split('::');
			if (dual.length > 1) {
				el.myTitle = dual[0].trim();
				el.myText = dual[1].trim();
			} else {
				el.myText = el.title;
			}
					
			el.removeAttribute('title');
		} else {
			el.myText = false;
		}
		if (el.myTitle && el.myTitle.length > this.options.maxTitleChars) el.myTitle = el.myTitle.substr(0, this.options.maxTitleChars - 1) + "&hellip;";
		el.addEvent('mouseenter', function(event){
			this.start(el);
			if (!this.options.fixed) this.locate(event);
			else this.position(el);
		}.bind(this));
		if (!this.options.fixed) el.addEvent('mousemove', this.locate.bindWithEvent(this));
		var end = this.end.bind(this);
		el.addEvent('mouseleave', end);
		el.addEvent('trash', end);
	},

	start: function(el){ // modded for X3
		this.wrapper.empty();
		
		// check if we have an AJAX request - if so, show a loading animation and launch the request		
		if (el.myText && el.myText.test('^AJAX:', 'i')) {
			//if (this.ajax) this.ajax.cancel();
			this.ajax = new Request ({
				url: el.myText.replace(/AJAX:/i,''),
				onComplete: function (responseText, responseXML) {
					el.title = responseText;
					this.build(el);
					this.start(el);
					}.bind(this),
				onFailure: function () {
					el.title = this.options.errTitle + '::' + this.options.errText;
					this.build(el);
					this.start(el);
					}.bind(this),
				method: 'get'
				}).send();				
			el.myText = '<div class="' + this.options.className + '-loading">&nbsp;</div>';			
		}
		
		if (el.myTitle){
			this.title = new Element('span').inject(
				new Element('div', {'class': this.options.className + '-title'}).inject(this.wrapper)
			).set('html', el.myTitle);
		}
		if (el.myText){
			this.text = new Element('span').inject(
				new Element('div', {'class': this.options.className + '-text'}).inject(this.wrapper)
			).set('html', el.myText);
		}
		$clear(this.timer);
		this.timer = this.show.delay(this.options.showDelay, this);
	},

	end: function(event){
		$clear(this.timer);
		this.timer = this.hide.delay(this.options.hideDelay, this);
	},

	position: function(element){
		var pos = element.getPosition();
		this.toolTip.setStyles({
			'left': pos.x + this.options.offsets.x,
			'top': pos.y + this.options.offsets.y
		});
	},

	locate: function(event){
		var win = {'x': window.getWidth(), 'y': window.getHeight()};
		var scroll = {'x': window.getScrollLeft(), 'y': window.getScrollTop()};
		var tip = {'x': this.toolTip.offsetWidth, 'y': this.toolTip.offsetHeight};
		var prop = {'x': 'left', 'y': 'top'};
		for (var z in prop){
			var pos = event.page[z] + this.options.offsets[z];
			if ((pos + tip[z] - scroll[z]) > win[z]) pos = event.page[z] - this.options.offsets[z] - tip[z];
			this.toolTip.setStyle(prop[z], pos);
		};
	},

	show: function(){
		if (this.options.timeout) this.timer = this.hide.delay(this.options.timeout, this);
		this.fireEvent('onShow', [this.toolTip]);
	},

	hide: function(){
		this.fireEvent('onHide', [this.toolTip]);
	}

});

TipsX3.implement(new Events, new Options);
