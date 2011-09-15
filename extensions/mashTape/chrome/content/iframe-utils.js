function mashTape_openLink(href) {
	Components.classes['@mozilla.org/appshell/window-mediator;1']
		.getService(Components.interfaces.nsIWindowMediator)
		.getMostRecentWindow('Songbird:Main').gBrowser
		.loadOneTab(href)
}

window.addEvent('click', function(e) {
	var target = e.target;
	while (target != null && !target.href)
		target = target.parentNode;
	if (target == null)
		return;
	if (target.href) {
		mashTape_openLink(target.href);
		e.stop();
	} 
});

