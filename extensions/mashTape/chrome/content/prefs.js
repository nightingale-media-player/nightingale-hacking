var mashTapePreferences = {
	populateServices: function(providerType) {
		var box = document.getElementById("services-" + providerType);
		var enabledKey = "extensions.mashTape." + providerType + ".enabled";
		var enabled = Application.prefs.getValue(enabledKey, true);

		for each (var contract in this.providers[providerType]) {
			var classId = this.compMgr.contractIDToCID(contract);
			var prov = Cc[contract].createInstance(Ci.sbIMashTapeProvider);
			
			var element = "checkbox";
			var attribute = "checked";
			switch(providerType) {
				case "info":
					prov = prov.QueryInterface(Ci.sbIMashTapeInfoProvider);
					element = "radio";
					attribute = "selected";
					break;
				case "rss":
					prov = prov.QueryInterface(Ci.sbIMashTapeRSSProvider);
					break;
				case "photo":
					prov = prov.QueryInterface(Ci.sbIMashTapePhotoProvider);
					element = "radio";
					attribute = "selected";
					break;
				case "flash":
					prov = prov.QueryInterface(Ci.sbIMashTapeFlashProvider);
					break;
				default:
					break;
			}
			
			var li = document.createElement("richlistitem");

			var providers = Application.prefs.getValue(
				"extensions.mashTape." + providerType + ".providers", "")
				.split(",");
			var el = document.createElement(element);
			el.setAttribute("command", "cmd-" + providerType);
			el.setAttribute("value", classId);
			for each (var provider in providers) {
				if (classId == provider)
					el.setAttribute(attribute, true);
			}
			li.appendChild(el);

			if (providerType == "info") {
				var icons = new Object;
				var favicon = document.createElement("image");
				favicon.setAttribute("src", prov.providerIconBio);
				li.appendChild(favicon);
				icons[prov.providerIconBio] = 1;
				if (typeof(icons[prov.providerIconTags]) == "undefined") {
					favicon = document.createElement("image");
					favicon.setAttribute("src", prov.providerIconTags);
					li.appendChild(favicon);
					icons[prov.providerIconTags] = 1;
				}
				if (typeof(icons[prov.providerIconDiscography]) == "undefined") {
					favicon = document.createElement("image");
					favicon.setAttribute("src", prov.providerIconDiscography);
					li.appendChild(favicon);
					icons[prov.providerIconDiscography] = 1;
				}
				if (typeof(icons[prov.providerIconMembers]) == "undefined") {
					favicon = document.createElement("image");
					favicon.setAttribute("src", prov.providerIconMembers);
					li.appendChild(favicon);
					icons[prov.providerIconMembers] = 1;
				}
				if (typeof(icons[prov.providerIconLinks]) == "undefined") {
					favicon = document.createElement("image");
					favicon.setAttribute("src", prov.providerIconLinks);
					li.appendChild(favicon);
					icons[prov.providerIconLinks] = 1;
				}
			} else {
				var favicon = document.createElement("image");
				favicon.setAttribute("src", prov.providerIcon);
				li.appendChild(favicon);
			}

			var serviceLabel = document.createElement("label");
			serviceLabel.setAttribute("value", prov.providerName);
			li.appendChild(serviceLabel);

			box.appendChild(li);
		}
		
		// Set initial disabled or enabled state
		this.toggleServices(box, providerType, !enabled);
	},
	

	enumerateServices: function() {
		this.compMgr =
			Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

		this.providers = new Array();
		this.providers["info"] = new Array();
		this.providers["rss"] = new Array();
		this.providers["photo"] = new Array();
		this.providers["flash"] = new Array();

		for (var contract in Cc) {
			if (contract.indexOf("mashTape/provider") >= 0) {
				var mtClass = contract.substring(
					contract.indexOf("mashTape/provider") + 18,contract.length);
				mtClass = mtClass.substring(0, mtClass.indexOf("/"));
				this.providers[mtClass].push(contract);
			}
		}
		this.providers["info"].sort();
		this.providers["rss"].sort();
		this.providers["photo"].sort();
		this.providers["flash"].sort();
	},

	savePrefs: function(providerType) {
		var box = document.getElementById("services-" + providerType);
		var prefKey = "extensions.mashTape." + providerType + ".providers";
		var prefServices = new Array();
		var attr;
		if (providerType == "rss" || providerType == "flash")
			attr = "checked";
		else
			attr = "selected";
		for each (var rli in box.childNodes) {
			if (rli.tagName != "richlistitem")
				continue;
			var el = rli.firstChild;
			if (el.hasAttribute(attr))
				prefServices.push(el.getAttribute("value"));
		};
		Application.prefs.setValue(prefKey, prefServices.join(","));
	},

	toggle: function(providerType) {
		var checkbox = document.getElementById("checkbox-" + providerType);
		var box = document.getElementById("services-" + providerType);
		
		// Save the pref first
		var enabledKey = "extensions.mashTape." + providerType + ".enabled";
		var enabled = checkbox.hasAttribute("checked");
		Application.prefs.setValue(enabledKey, enabled);

		// Now see if we should disable the richlistbox
		this.toggleServices(box, providerType, !enabled);
	},
	
	toggleServices: function(box, providerType, disabled) {
		var helpText = document.getElementById("help-" + providerType);
		helpText.disabled = disabled;
		helpText.setAttribute("disabled", disabled);
		box.setAttribute("disabled", disabled);
		for each (var rli in box.childNodes) {
			if (rli.tagName != "richlistitem")
				continue;
			var el = rli.firstChild;
			el.setAttribute("disabled", disabled);
		}
	},

	init: function() {
		this.enumerateServices();
		this.populateServices("info");
		this.populateServices("rss");
		this.populateServices("photo");
		this.populateServices("flash");
	}
}
