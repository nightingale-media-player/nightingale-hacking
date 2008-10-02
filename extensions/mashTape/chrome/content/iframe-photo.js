document.addEvent('mousewheel', function(event) {
	event = new Event(event);
 
	if (typeof(nS5) == "undefined")
		return;
	/* Mousewheel UP */
	if (event.wheel > 0) {
		nS5.previous();
	} 
	/* Mousewheel DOWN*/
	else if (event.wheel < 0) {
		nS5.next();
	}
});

function mashTape_resetPhotoStream() {
	if (typeof(nS5) != 'undefined') {
		nS5.stop();
		delete nS5;
		$('playpause').removeEvents();
		$('icon-playpause').src = "chrome://mashtape/skin/pause.png";
	}
}

function _mashTape_setupPhotoStream(imageItems, paneWidth, paneHeight) {
	var info = $('info').set('opacity', 0.5);

	imageItems[0].offset = 0;
	$('box').getElements('img').each(function(img, idx) {
		//dump("image[" + idx + "].width = " + img.width + " - " +
		//	img.src + "\n");
		imageItems[idx].index = idx;
		imageItems[idx].imgEl = img;
	});
	
	nS5 = new noobSlide({
		box: $('box'),
		size: paneWidth,
		interval: 2000,
		items: imageItems,
		autoPlay: true,
		addButtons: {
			previous: $('prev'),
			next: $('next'),
			playpause: $('playpause'),
		},
		onButtons: function(command, isPlaying) {
			//dump("> " + command + "|" + isPlaying + "\n");
			if (isPlaying) {
				// Switch the icon to 'pause'
				$('icon-playpause').src = "chrome://mashtape/skin/pause.png";
			} else {
				// Switch the icon to 'play'
				$('icon-playpause').src = "chrome://mashtape/skin/play.png";
			}
		},
		onWalk: function(currentItem) {
			info.empty();
			/*
			dump("> Advanced to " + currentItem.index +
					" - width: " + currentItem.imgEl.width +
					" - offset: " + currentItem.offset +
					"\n");
					*/
			new Element('h4').set('html',
				'<a href="' +
				currentItem.link +'">' + currentItem.title + '</a>')
				.inject(info);
			var date = new Date(parseInt(currentItem.date));
			new Element('p').set('html',
					window.parent.mashTape.strings
							.GetStringFromName("extensions.mashTape.msg.by") +
					' <b><a href="' + currentItem.authorUrl + '">' +
					currentItem.author + '</a></b> (' + "<span title='" +
					date.toLocaleString() + "'>" + date.ago() +
					"</span>" + ")").inject(info);
		}
	});
}

function mashTape_triggerPhotoStream(imageItems, paneWidth) {
	setTimeout(_mashTape_setupPhotoStream, 500, imageItems, paneWidth);
}

