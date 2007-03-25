/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */


const BALLOONTIP_OUTSIDE     = 1;
const BALLOONTIP_INSIDE      = 2;

function BalloonTip() {
}

BalloonTip.prototype.constructor = BalloonTip;

BalloonTip.prototype = {

  anchorPosition: BALLOONTIP_OUTSIDE,
  autoCloseTimeout: 0,
  autoCloseOnClick: true,
  onCloseCallback: null,
  onInitCallback: null,
  onClickCallback: null,

  // Show a balloontip with simple text content
  //
  // Sample usage : 
  //
  // var tip = new BalloonTip; 
  // tip.autoCloseTimeout = 10;                     // automatically closes the tip after 10 seconds if it is no longer focused (default = 0 = do not autoclose)
  // tip.autoCloseOnClick = false;                  // automatically closes the tip when its content is clicked (default = true)
  // tip.onCloseCallback = myclosecallbackfunction; // get a callback when the tip is closed (default = no callback), params = (tipInstance, checkboxElement)
  // tip.onInitCallback = myinitcallbackfunction;   // get a callback when the tip is inited (default = no callback), params = (tipInstance, checkboxElement)
  // tip.onClickCallback = myclickcallbackfunction; // get a callback when the tip is clicked (default = no callback), params = (tipInstance, event)
  // tip.anchorPosition = BALLOONTIP_INSIDE;        // anchor tip inside the element, or BALLOONTIP_OUTSIDE to anchor on its side (default = outside)
  //
  // tip.showText('Here is a helpful tip.',                        // autowraps if a width is specified
  //              document.getElementById('some_element_id'),      // anchor element
  //              'Here is something you should know',             // title (if none specified, no title is displayed)
  //              'balloon-icon-songbird',                         // icon class (if none specified, no icon is displayed)
  //              'Do not show this again',                        // text to use on the checkbox (if none specified, checkbox is not visible)
  //              200);                                            // width (if -1 specified, text is on a single line, if none specified, wraps at 300px)

  showText: function(aText,
                     aAnchorElement,
                     aTitle,
                     aTitleImageClass,
                     aCheckboxLabel,
                     aWidth) {

    var width_val;
    var width_attr;
    if (aWidth == -1) { width_val = null; width_val = null; }
    else if (!aWidth) { width_val = '300'; width_attr = 'width'; }
    else { width_val = aWidth; width_attr = 'width'; }

    this.showContent('chrome://songbird/content/bindings/balloon.xml#balloon-text', 
                     aAnchorElement,
                     aTitle,
                     aTitleImageClass,
                     aCheckboxLabel,
                     ['value', width_attr], 
                     [aText, width_val]);
  },

  // Show a balloontip with arbitrary content
  //
  // Sample usage : 
  //
  // var tip = new BalloonTip; 
  // tip.autoCloseTimeout = 10;                     // automatically closes the tip after 10 seconds if it is no longer focused (default = 0 = do not autoclose)
  // tip.autoCloseOnClick = false;                  // automatically closes the tip when its content is clicked (default = true)
  // tip.onCloseCallback = myclosecallbackfunction; // get a callback when the tip is closed (default = no callback), params = (tipInstance, checkboxElement)
  // tip.onInitCallback = myinitcallbackfunction;   // get a callback when the tip is inited (default = no callback), params = (tipInstance, checkboxElement)
  // tip.onClickCallback = myclickcallbackfunction; // get a callback when the tip is clicked (default = no callback), params = (tipInstance, event)
  // tip.anchorPosition = BALLOONTIP_INSIDE;        // anchor tip inside the element, or BALLOONTIP_OUTSIDE to anchor on its side (default = outside)
  //
  // tip.showContent('chrome://songbird/content/bindings/balloon.xml#balloon-text', // a binding URL, or an element tag name
  //              document.getElementById('some_element_id'),                       // anchor element
  //              'Here is something you should know',                              // title (if none specified, no title is displayed)
  //              'balloon-icon-songbird',                                          // icon class (if none specified, no icon is displayed)
  //              'Do not show this again',                                         // text to use on the checkbox (if none specified, checkbox is not visible)
  //              ['value', 'width'],                                               // attributes to forward to the binding element (or null)
  //              ['Here is a helpful tip.'], '200');                               // values of the attributes to forward to the binding element (or null)                                                          
  
  showContent: function(aBindingURL, // may also be a simple element name (ie, "label")
                        aAnchorElement,
                        aTitle,
                        aTitleImageClass,
                        aCheckboxLabel,
                        aBindingAttribute,
                        aBindingAttributeValue) {
    // Init members,             
    if (!aAnchorElement) aAnchorElement = document.documentElement; // no anchor element ? use document, i guess ...
    this.bindingUrl = aBindingURL;
    this.bindingAttribute = aBindingAttribute;
    this.bindingAttributeValue = aBindingAttributeValue;
    this.titleImageClass = aTitleImageClass;
    this.titleValue = aTitle;
    this.anchorElement = aAnchorElement;
    this.originalDocument = document;
    this.originalWindow = window;
    this.checkboxLabel = aCheckboxLabel;
    // Generate a random id for that window, so that opening a new balloontip does not reuse any
    // tip window that is already visible on the screen
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
    var id = aUUIDGenerator.generateUUID();
    // 'alwaysRaised' does not work on Linux or Mac, only use it on Windows.
    // 'popup' makes the window always on top on linux, which is the next best
    // way of ensuring that the tip window stays on top (although it makes it
    // stay on top of other apps as well).
    // Neither of those two flags work on the Mac, so we use yet another method
    // (see computePositionAndOrientation function , since that method can only 
    // work after the window has been initialized)
    var raisedflag;
    switch (getPlatformString()) {
      case "Linux": raisedflag = ",popup"; break;
      case "Windows_NT": raisedflag = ",alwaysRaised"; break;
      default: raisedflag = ""; break;
    }
    // Open the window (cloaked)
    this.tipWindow = window.openDialog("chrome://songbird/content/xul/balloonTip.xul", id, "chrome,modal=no,titlebar=no,resizable=no"+raisedflag, this);
    this.initTimeStamp = new Date().getTime();
    if (this.autoCloseTimeout) setTimeout( function(obj) { obj.onAutoCloseTimeout(); }, this.autoCloseTimeout * 1000, this );
  },

  bindingUrl: null,
  bindingAttribute: null,
  bindingAttributeValue: null,
  titleImageClass: null,
  titleValue: null,
  checkboxLabel: null,
  checkboxElement: null,

  tipWindow : null,
  anchorElement: null,
  originalDocument: null,
  originalWindow: null,
  gotmetrics: false,
  
  autoCloseTimeoutElapsed: function() {
    var now = new Date().getTime();
    var diff = (now - this.initTimeStamp)/1000;
    return diff > this.autoCloseTimeout;
  },
  
  onAutoCloseTimeout: function() {
    if (this.focused) return;
    this.closeTip();
  },
  
  onTipBlur: function() {
    this.focused = false;
    if (!this.autoCloseTimeout) return;
    if (this.autoCloseTimeoutElapsed()) this.closeTip();
  },
  
  onTipFocus: function() {
    this.focused = true;
  },
  
  onTipClick: function(clickEvent) {
    if (this.onClickCallback) this.onClickCallback(this, clickEvent);
    if (this.autoCloseOnClick) this.closeTip();
  },

  // Closed the balloontip
  closeTip: function() {
    // Optional user callback
    if (this.onCloseCallback) this.onCloseCallback(this, this.checkboxElement);
    // Cleanup
    this.tipWindow.document.removeEventListener("focus", this.onfocus, true);
    this.tipWindow.document.removeEventListener("blur", this.onblur, true);
    this.tipWindow.document.removeEventListener("click", this.onclick, true);
    var wnd = this.tipWindow;
    this.tipWindow = null;
    wnd.close();
  },
  
  // The balloontip window has been created, but is not initialized yet
  onCreateTip: function(aWindow) {
    // Cloak the window as it is created, because we need to wait until it has been initialized 
    // before we can know its size, and therefore, calculate its position. Since we do not
    // want to see the window pop up and then being moved, we cloak it first, then position
    // it, and then we uncloak it (see computePositionAndOrientation)
    var windowCloak =
      Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"]
                .getService(Components.interfaces.sbIWindowCloak);
    windowCloak.cloak(aWindow); 
  },
  
  // Compute intersection of two rectangles
  intersectRect: function(aRectA, aRectB, aIntersection) {
    aIntersection.x = Math.max(aRectA.x, aRectB.x);
    aIntersection.y = Math.max(aRectA.y, aRectB.y);
    aIntersection.w  = Math.min(aRectA.x + aRectA.w, aRectB.x + aRectB.w) - aIntersection.x;
    aIntersection.h = Math.min(aRectA.y + aRectA.h, aRectB.y + aRectB.h) - aIntersection.y;

    if (!(aIntersection.x < aIntersection.x + aIntersection.w && aIntersection.y < aIntersection.y + aIntersection.h)) {
      aIntersection.x = aIntersection.y = aIntersection.w = aIntersection.height = 0;
      return 0;
    } else {
      return 1;
    }
  },

  getXULWindowFromWindow: function(win) {
    var rv;
    try {
      var requestor = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
      var nav = requestor.getInterface(Components.interfaces.nsIWebNavigation);
      var dsti = nav.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
      var owner = dsti.treeOwner;
      requestor = owner.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
      rv = requestor.getInterface(Components.interfaces.nsIXULWindow);
    }
    catch (ex) {
      rv = null;
    }
    return rv;
  },

  // Compute the optimal position and orientation of the balloontip window for the requested anchor
  // Note: at this stage, the window is cloaked. We uncloak it when we're done moving and resizing it.
  computePositionAndOrientation: function() {
    try {
      // If the tip has gone away, abort
      if (!this.tipWindow) return;

      if (!this.gotmetrics) {
        // We may have been called back before the window was initialized, wait some more
        if (this.tipWindow.document.documentElement.boxObject.width == 0 || 
            this.tipWindow.document.documentElement.boxObject.height == 0) {
          setTimeout(function(obj) { obj.computePositionAndOrientation(); }, 0, this);
          return;
        }

        // Optional user callback on init
        if (this.onInitCallback) this.onInitCallback(this, this.checkboxElement);

        // Get the tip window's size 
        this._tipX = this.tipWindow.document.documentElement.boxObject.screenX;
        this._tipY = this.tipWindow.document.documentElement.boxObject.screenY;
        this._tipWidth = this.tipWindow.document.documentElement.boxObject.width;
        this._tipHeight = this.tipWindow.document.documentElement.boxObject.height;

        this.onblur = { 
          _that: null, 
          handleEvent: function( event ) { this._that.onTipBlur(); } 
        }; this.onblur._that = this; 
        this.tipWindow.document.addEventListener("blur", this.onblur, true);

        this.onfocus = { 
          _that: null, 
          handleEvent: function( event ) { this._that.onTipFocus(); } 
        }; this.onfocus._that = this; 
        this.tipWindow.document.addEventListener("focus", this.onfocus, true);

        this.onclick = { 
          _that: null, 
          handleEvent: function( event ) { this._that.onTipClick(); } 
        }; this.onclick._that = this; 
        this.tipWindow.document.addEventListener("click", this.onclick, true);
        
        if (getPlatformString() == "Darwin") {
          // This works on MacOSX, but won't stick when another window gets dragged. 
          // There doesn't seem to be any better way though, because neither the
          // alwaysRaised nor the popup flags work as advertised on the mac.
          var xulwindow = this.getXULWindowFromWindow(this.tipWindow);
          if (xulwindow) xulwindow.zLevel = Components.interfaces.nsIXULWindow.highestZ;
        }

        // Read offsets from tip window
        this._anchor_nw = this.tipWindow.document.getElementById("anchor-nw");
        this._anchor_ne = this.tipWindow.document.getElementById("anchor-ne");
        this._anchor_sw = this.tipWindow.document.getElementById("anchor-sw");
        this._anchor_se = this.tipWindow.document.getElementById("anchor-se");
        this._frame_n = this.tipWindow.document.getElementById("frame-n");
        this._frame_s = this.tipWindow.document.getElementById("frame-s");

        // Note: as this stage, all four arrows are visible
        this._offset_nw = this._anchor_nw.boxObject.screenX - this._tipX;
        this._offset_ne = (this._tipX + this._tipWidth) - (this._anchor_ne.boxObject.screenX + this._anchor_ne.boxObject.width);
        this._offset_sw = this._anchor_sw.boxObject.screenX - this._tipX;
        this._offset_se = (this._tipX + this._tipWidth) - (this._anchor_se.boxObject.screenX + this._anchor_se.boxObject.width);
        this._margin_n = this._frame_n.boxObject.screenY - this._tipY;
        this._margin_s = (this._tipY + this._tipHeight) - (this._frame_s.boxObject.screenY + this._frame_s.boxObject.height);
        
        // don't re-read the metrics from the window, they'll have changed the next time this 
        // function is called, and we want to keep the original ones for our calculations
        this.gotmetrics = true;
      }

      // We'll record our best orientation and the corresponding position for the window here
      var best = {
        orientation: -1,
        insideArea: -1,
        x: 0,
        y: 0,
        w: 0,
        h: 0
      };
      
      // Try using the screen coordinates for the window that spawned the tip, and for the tooltip window (in case the original window is 
      // moved, these can be two different ones). Unfortunately this won't check existing monitors that offer a better position for the 
      // tip unless either the tipwindow or the window that spawned it are mostly in it. Ie, if both the tip and the window that spawned 
      // it are mostly in the same monitor, and there is a better possible position in a secondary monitor (both window are partially in 
      // that secondary monitor), then the better position will not be found.
      
      // The only way to fix this would be to enumerate all monitors, but there is no service to do that.
      
      var basewindows = [this.originalWindow, (this.tipWindow == this.originalWindow) ? null : this.tipWindow];
      
      for (var j in basewindows) {
        if (!basewindows[j]) continue;
        
        // Get the screen size (multimonitor aware)
        var screen = basewindows[j].QueryInterface(Components.interfaces.nsIDOMWindowInternal).screen;
        if (!screen) continue;
        var screenRect = {
          x: screen.availLeft,
          y: screen.availTop,
          w: screen.availWidth,
          h: screen.availHeight
        };
        

        // --- Calculate optimal position for the tooltip based on content, anchor position, and screen size
        
        // Cycle all 4 possible orientations (note, NW/NE/SW/SE refer to the direction of the arrow in the tipwindow.
        // The direction of the window itself, relative to the anchor point, is opposite the arrow.)
        
        // These are in order of preference, if all orientations fit in the screen, we'll use SW
        const ORIENT_SW = 0;
        const ORIENT_SE = 1;
        const ORIENT_NW = 2;
        const ORIENT_NE = 3;
        
        var px, py; // anchor position on anchor element
        
        for (var i = 0; i <= 3; i++) {
          // Calculate position of anchor on anchor element
          if (this.anchorPosition == BALLOONTIP_OUTSIDE) {
            // If anchored outside, stick to the top or bottom side, and position x coordinate halfway to the center (quarter width) depending on orientation
            switch (i) {
              case ORIENT_NW:
                px = this.anchorElement.boxObject.screenX + (this.anchorElement.boxObject.width / 4);
                py = this.anchorElement.boxObject.screenY + this.anchorElement.boxObject.height;
                break;
              case ORIENT_NE:
                px = this.anchorElement.boxObject.screenX + (this.anchorElement.boxObject.width / 4)*3;
                py = this.anchorElement.boxObject.screenY + this.anchorElement.boxObject.height;
                break;
              case ORIENT_SW:
                px = this.anchorElement.boxObject.screenX + (this.anchorElement.boxObject.width / 4);
                py = this.anchorElement.boxObject.screenY;
                break;
              case ORIENT_SE:
                px = this.anchorElement.boxObject.screenX + (this.anchorElement.boxObject.width / 4)*3;
                py = this.anchorElement.boxObject.screenY;
                break;
            }
          } else {
            // if anchored inside, use the center of the element
            px = this.anchorElement.boxObject.screenX + (this.anchorElement.boxObject.width / 2);
            py = this.anchorElement.boxObject.screenY + (this.anchorElement.boxObject.height / 2);
          }

          // Add or subtract anchor offset, so that the balloontip's arrow points at the anchor position
          switch (i) {
            case ORIENT_NW: px -= this._offset_nw; break;
            case ORIENT_NE: px += this._offset_ne; break;
            case ORIENT_SW: px -= this._offset_sw; break;
            case ORIENT_SE: px += this._offset_se; break;
          }

          // Calculate actual position of the tip window for this orientation and anchor position
          var wx, wy, wh = this._tipHeight, ww = this._tipWidth;
          switch (i) {
            case ORIENT_NW: 
              wx = px; 
              wy = py; 
              wh -= this._margin_s;
              break;
            case ORIENT_NE: 
              wx = px - this._tipWidth; 
              wy = py; 
              wh -= this._margin_s;
              break;
            case ORIENT_SW: 
              wx = px; 
              wy = py - this._tipHeight + this._margin_n; 
              wh -= this._margin_n;
              break;
            case ORIENT_SE: 
              wx = px - this._tipWidth; 
              wy = py - this._tipHeight + this._margin_n; 
              wh -= this._margin_n;
              break;
          }
          
          // Calculate area of the window that is going to be inside the screen if we use this orientation
          var tipRect = {
            x: wx,
            y: wy,
            w: ww,
            h: wh
          };
          
          var insideRect = { x: 0, y: 0, w: 0, h: 0};
          this.intersectRect(tipRect, screenRect, insideRect);
          var insideArea = insideRect.w * insideRect.h;

          // If this is the first calculation, or we have a better position, record it
          if (best.insideArea == -1 || insideArea > best.insideArea) {
            best.orientation = i;
            best.insideArea = insideArea;
            best.x = wx;
            best.y = wy;
            best.w = ww;
            best.h = wh;
          }
          // Try the next orientation ...
        }
      }
      
      // Use the best orientation that we found
      
      // Show/hide appropriate arrows
      switch (best.orientation) {
        case ORIENT_NW: 
          this._anchor_nw.hidden = false;
          this._anchor_ne.hidden = true;
          this._anchor_sw.hidden = true;
          this._anchor_se.hidden = true;
          break;
        case ORIENT_NE: 
          this._anchor_nw.hidden = true;
          this._anchor_ne.hidden = false;
          this._anchor_sw.hidden = true;
          this._anchor_se.hidden = true;
          break;
        case ORIENT_SW: 
          this._anchor_nw.hidden = true;
          this._anchor_ne.hidden = true;
          this._anchor_sw.hidden = false;
          this._anchor_se.hidden = true;
          break;
        case ORIENT_SE: 
          this._anchor_nw.hidden = true;
          this._anchor_ne.hidden = true;
          this._anchor_sw.hidden = true;
          this._anchor_se.hidden = false;
          break;
      }

      // Move the tip window to where it should be
      this.tipWindow.resizeTo(best.w, best.h);
      
      // And resize it to what we computed (this is necessary because removing the unused arrows while keeping the window
      // the same size makes the content bigger than it needs to be, so we have to shrink the window by the calculated 
      // offset in order for the content size to remain the same as what was automatically determined to be the best
      // by ui engine)
      this.tipWindow.moveTo(best.x, best.y);
      
      // Now that the window is correctly positioned and sized, uncloak it
      var windowCloak =
        Components.classes["@songbirdnest.com/Songbird/WindowCloak;1"]
                  .getService(Components.interfaces.sbIWindowCloak);
      windowCloak.uncloak(this.tipWindow); 
      
      // All done, but follow the position of the anchor element on the screen.
      setTimeout(function(obj) { obj.computePositionAndOrientation(); }, 100, this);
    } catch (e) {
      Components.utils.reportError(e);
    }
  }
  
};


