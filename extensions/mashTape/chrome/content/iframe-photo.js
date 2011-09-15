Components.utils.import("resource://mashTape/mtUtils.jsm");

var nS5;

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

function _mashTape_setupPhotoStream(imageItems, paneWidth, paneHeight) {
	var info = $('info').set('opacity', 0.5);

	imageItems[0].offset = 0;
	$('box').getElements('img').each(function(img, idx) {
		//dump("image[" + idx + "].width = " + img.width + " - " +
		//	img.src + "\n");
		imageItems[idx].index = idx;
		imageItems[idx].imgEl = img;
	});
	
	var speed =
		Application.prefs.getValue("extensions.mashTape.photo.speed", 50);
	speed = speed * 40;

	nS5 = new noobSlide({
		box: $('box'),
		size: paneWidth,
		interval: speed,
		items: imageItems,
		autoPlay: true,
		addButtons: {
			previous: $('prev'),
			next: $('next'),
			playpause: $('playpause')
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
 
/**
 * updatePhotoStreamSpeed
 * If the preference for the photo speed changes we update the interval here
 * so that while the slidshow is playing we can automatically adjust to the
 * changes.
 */
function mashTape_updatePhotoStreamSpeed() {
	if (!nS5) return;

	var speed =
		Application.prefs.getValue("extensions.mashTape.photo.speed", 50);
	speed = speed * 40;
  
	nS5.interval = speed;
	if (nS5.isPlaying) {
		nS5.playpause(nS5.interval, 'next');
		nS5.playpause(nS5.interval, 'next');
	}
}

function mashTape_triggerPhotoStream(imageItems, paneWidth) {
	setTimeout(_mashTape_setupPhotoStream, 500, imageItems, paneWidth);
}

