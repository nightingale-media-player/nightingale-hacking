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
				case "review":
					prov = prov.QueryInterface(Ci.sbIMashTapeReviewProvider);
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
		this.providers["review"] = new Array();
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
		this.providers["review"].sort();
		this.providers["rss"].sort();
		this.providers["photo"].sort();
		this.providers["flash"].sort();
	},

	savePrefs: function(providerType) {
		var box = document.getElementById("services-" + providerType);
		var prefKey = "extensions.mashTape." + providerType + ".providers";
		var prefServices = new Array();
		var attr;
		switch (providerType) {
			case "rss":
			case "review":
			case "flash":
				attr = "checked";
				break;
			default:
				attr = "selected";
				break;
		}

		var providerSelected = false;
		for each (var rli in box.childNodes) {
			if (rli.tagName != "richlistitem")
				continue;
			var el = rli.firstChild;
			if (el.hasAttribute(attr)) {
				prefServices.push(el.getAttribute("value"));
				providerSelected = true;
			}
		};
		if (!providerSelected) {
			document.getElementById("checkbox-" + providerType).checked = false;
			mashTapePreferences.toggle(providerType);
		}

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
		if (providerType == "photo") {
			var hbox = document.getElementById("hbox-photo-speed");
			for each (var node in hbox.childNodes)
				node.disabled = !enabled;
		}

		// ensure if we're turning it on that at least one provider is 
		// enabled
		if (enabled) {
			var box = document.getElementById("services-" + providerType);
			var attr;
			switch (providerType) {
				case "rss":
				case "review":
				case "flash":
					attr = "checked";
					break;
				default:
					attr = "selected";
					break;
			}

			var providerSelected = false;
			var firstProvider = null;
			for each (var rli in box.childNodes) {
				if (rli.tagName != "richlistitem")
					continue;
				var el = rli.firstChild;
				if (!firstProvider)
					firstProvider = el;
			};
			if (!providerSelected) {
				// no provider is selected, enable the first one
				firstProvider.setAttribute(attr, true);
				mashTapePreferences.savePrefs(providerType);
			}
		}
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

	observe: function(subject, topic, data) {
		if (subject instanceof Components.interfaces.nsIPrefBranch) {
			var pref = data.split(".");
			if (pref.length == 2 && pref[1] == "enabled") {
				var tabName = pref[0];
				this.checkDefaultTab(tabName);
			}
		}
	},

	getDefaultTabItem : function(providerType) {
		for (var i=0; i<this.defaultTabSelect.itemCount; i++) {
			var item = this.defaultTabSelect.getItemAtIndex(i);
			if (item.value == providerType)
				return item;
		}
		return null;
	},

	checkDefaultTab: function(tabName) {
		var prefKey = tabName + ".enabled";
		var enabled = this._prefBranch.getBoolPref(prefKey);
		var item = this.getDefaultTabItem(tabName);
		if (!enabled) {
			item.disabled = true;
			if (item.selected) {
				// select an enabled tab instead
				for each (var tab in ["info", "review","rss", "photo", "flash"])
				{
					if (this._prefBranch.getBoolPref(tab + ".enabled") &&
              this.providers[tab].length > 0) {
						var sItem = this.getDefaultTabItem(tab);
						this.defaultTabSelect.selectedItem = sItem;
						this._prefBranch.setCharPref("defaultpane", tab);
						break;
					}
				}
			}
		} else
			item.disabled = false;
	},

	init: function() {
		this.defaultTabSelect = document.getElementById("general-default-tab");

		// Setup our preferences observer
		this._prefBranch = Cc["@mozilla.org/preferences-service;1"]
			.getService(Ci.nsIPrefService).getBranch("extensions.mashTape.")
			.QueryInterface(Ci.nsIPrefBranch2);
		this._prefBranch.addObserver("", this, false);

		for each (var tab in ["info", "review", "rss", "photo", "flash"])
			this.checkDefaultTab(tab);

		if (!this._prefBranch.getBoolPref("photo.enabled")) {
			var hbox = document.getElementById("hbox-photo-speed");
			for each (var node in hbox.childNodes)
				node.disabled = true;
		}

		/* Mozilla bugs: https://bugzilla.mozilla.org/show_bug.cgi?id=521215,
		 *               https://bugzilla.mozilla.org/show_bug.cgi?id=499105
		 *
		 * XUL scale widget cannot load preference, so set the value based on
		 * the preference, then set the preference attribute so that the widget
		 * writes changes.
		 */
		var scale = document.getElementById("scale-photo-speed");
		scale.value = this._prefBranch.getIntPref("photo.speed");
		scale.setAttribute("preference", "photo-speed");

		this.enumerateServices();
		this.populateServices("info");
		this.populateServices("review");
		this.populateServices("rss");
		this.populateServices("photo");
		this.populateServices("flash");
	},

	unload: function() {
		this._prefBranch.removeObserver("", this);
	}
}
