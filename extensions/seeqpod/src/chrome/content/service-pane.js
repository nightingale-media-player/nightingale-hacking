/*
Copyright (c) 2008, Pioneers of the Inevitable, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of Pioneers of the Inevitable, Songbird, nor the names
	  of its contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

if (typeof ServicePane == 'undefined') {
    var ServicePane = {};
}

/**
 * Provides service pane manipulation methods
 */
ServicePane = {
    /* What URL does our node link to? */
    url: 'chrome://seeqpod/content/search.xul',
    /* What's our icon? */
    icon: 'http://seeqpod.com/favicon.ico',
    /* What's our service pane node id? */
    nodeId: 'SB:SeeqPod',
    /**
     * Our folder name as it appears in the service pane
     */
    _folderName: function() {
        return document.getElementById("seeqpod-strings").getString("servicePaneFolderLabel");
    },

    /**
     * Find and return our folder node from the service pane
     */
    _findFolderNode: function(node) {
        if (node.isContainer && node.name != null && node.name == this._folderName()) {
            return node;
        }

        if (node.isContainer) {
            var children = node.childNodes;
            while (children.hasMoreElements()) {
                var child = children.getNext().QueryInterface(Components.interfaces.sbIServicePaneNode);
                var result = this._findFolderNode(child);
                if (result != null) {
                    return result;
                }
            }
        }
        return null;
    },

    /**
     * Find and return our folder node from the service pane (if exists)
     */
    findFolderNode: function() {
        return this._findFolderNode(Services.servicePane().root);
    },

    /**
     * Add the main node and remove legacy nodes in the service pane
     */
    configure: function() {
        var servicePane = Services.servicePane();
        servicePane.init();

        // Walk nodes to see if a "Search" folder already exists
        var servicePaneFolder = this.findFolderNode();
        if (servicePaneFolder != null) {
            // if it does, we want to remove it.
            servicePaneFolder.parentNode.removeChild(servicePaneFolder);
        }

        // Look for our node
        var node = servicePane.getNode(this.nodeId);
        if (!node) {
            // no node? we should add one
            node = servicePane.addNode(this.nodeId, servicePane.root, false);
            // let's decorate it all pretty-like
            node.hidden = false;
            node.editable = false;
            node.name = '&servicepane.seeqpodsearch';
            node.stringbundle = 'chrome://seeqpod/locale/overlay.properties';
            node.image = this.icon;
            // point it at the right place
            node.url = this.url;
            // sort it correctly
            node.setAttributeNS('http://songbirdnest.com/rdf/servicepane#',
                                'Weight', 2);
            servicePane.sortNode(node);
        }

        servicePane.save();
    },

    /**
     * Remove the folder and all children node from the service pane
     */
    removeAllNodes: function() {
        try {
            var servicePane = Services.servicePane();
            servicePane.init();
            var bookmark = servicePane.getNode(this.nodeId);
            if (bookmark) {
                servicePane.root.removeChild(bookmark);
            }

            Services.servicePane().save();
        } catch(e) { }
    }

}
