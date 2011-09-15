window.addEvent('domready', function() {
	var strings = window.parent.mashTape.strings;
	var bioText = $('bio-text');
	var bioToggleLabel = $('bio-toggle-text');
	var discoText = $('discography-text');
	var discoToggleLabel = $('discography-toggle-text');

	document.getElementById("header-bio").innerHTML =
		strings.GetStringFromName("extensions.mashTape.info.bio");
	document.getElementById("header-discography").innerHTML =
		strings.GetStringFromName("extensions.mashTape.info.discography");
	document.getElementById("header-members").innerHTML =
		strings.GetStringFromName("extensions.mashTape.info.members");
	document.getElementById("header-tags").innerHTML =
		strings.GetStringFromName("extensions.mashTape.info.tags");
	document.getElementById("header-links").innerHTML =
		strings.GetStringFromName("extensions.mashTape.info.links");
	
	bioToggleLabel.innerHTML =
			strings.GetStringFromName("extensions.mashTape.msg.readmore");
	discoToggleLabel.innerHTML =
			strings.GetStringFromName("extensions.mashTape.msg.viewmore");

	$('bio-toggle-text').addEvent('click', function(e) {
		e.stop();
		if (bioText.getAttribute("mashTape-collapsed") == "true") {
			var fullHeight = bioText.getAttribute("mashTape-full-height");
			bioText.tween("height", fullHeight);
			bioToggleLabel.innerHTML =
				strings.GetStringFromName("extensions.mashTape.msg.readless");
			bioText.setAttribute("mashTape-collapsed", false);
		} else {
			window.scrollTo(0,0);
			bioText.tween("height", "93px");
			bioToggleLabel.innerHTML = 
				strings.GetStringFromName("extensions.mashTape.msg.readmore");
			bioText.setAttribute("mashTape-collapsed", true);
		}
	});
	$('discography-toggle-text').addEvent('click', function(e) {
		e.stop();
		if (discoText.getAttribute("mashTape-collapsed") == "true") {
			var fullHeight = discoText.getAttribute("mashTape-full-height");
			discoText.tween("height", fullHeight);
			discoToggleLabel.innerHTML =
				strings.GetStringFromName("extensions.mashTape.msg.viewless");
			discoText.setAttribute("mashTape-collapsed", false);
		} else {
			discoText.tween("height", "70px");
			discoToggleLabel.innerHTML = 
				strings.GetStringFromName("extensions.mashTape.msg.viewmore");
			discoText.setAttribute("mashTape-collapsed", true);
		}
	});
});

