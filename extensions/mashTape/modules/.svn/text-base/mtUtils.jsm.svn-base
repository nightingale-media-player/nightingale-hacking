var EXPORTED_SYMBOLS = [ "Date", "mtUtils", "jsSHA" ];

const Cc = Components.classes;
const Ci = Components.interfaces;

Date.prototype.setISO8601 = function (string) {
    var regexp = "([0-9]{4})(-([0-9]{2})(-([0-9]{2})" +
        "(T([0-9]{2}):([0-9]{2})(:([0-9]{2})(\.([0-9]+))?)?" +
        "(Z|(([-+])([0-9]{2}):([0-9]{2})))?)?)?)?";
    var d = string.match(new RegExp(regexp));

    var offset = 0;
    var date = new Date(d[1], 0, 1);

    if (d[3]) { date.setMonth(d[3] - 1); }
    if (d[5]) { date.setDate(d[5]); }
    if (d[7]) { date.setHours(d[7]); }
    if (d[8]) { date.setMinutes(d[8]); }
    if (d[10]) { date.setSeconds(d[10]); }
    if (d[12]) { date.setMilliseconds(Number("0." + d[12]) * 1000); }
    if (d[14]) {
        offset = (Number(d[16]) * 60) + Number(d[17]);
        offset *= ((d[15] == '-') ? 1 : -1);
    }

    offset -= date.getTimezoneOffset();
    var time = (Number(date) + (offset * 60 * 1000));
    this.setTime(Number(time));
}

Date.prototype.toISO8601String = function (format, offset) {
    /* accepted values for the format [1-6]:
     1 Year:
       YYYY (eg 1997)
     2 Year and month:
       YYYY-MM (eg 1997-07)
     3 Complete date:
       YYYY-MM-DD (eg 1997-07-16)
     4 Complete date plus hours and minutes:
       YYYY-MM-DDThh:mmTZD (eg 1997-07-16T19:20+01:00)
     5 Complete date plus hours, minutes and seconds:
       YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
     6 Complete date plus hours, minutes, seconds and a decimal
       fraction of a second
       YYYY-MM-DDThh:mm:ss.sTZD (eg 1997-07-16T19:20:30.45+01:00)
    */
    if (!format) { var format = 6; }
    if (!offset) {
        var offset = 'Z';
        var date = this;
    } else {
        var d = offset.match(/([-+])([0-9]{2}):([0-9]{2})/);
        var offsetnum = (Number(d[2]) * 60) + Number(d[3]);
        offsetnum *= ((d[1] == '-') ? -1 : 1);
        var date = new Date(Number(Number(this) + (offsetnum * 60000)));
    }

    var zeropad = function (num) { return ((num < 10) ? '0' : '') + num; }

    var str = "";
    str += date.getUTCFullYear();
    if (format > 1) { str += "-" + zeropad(date.getUTCMonth() + 1); }
    if (format > 2) { str += "-" + zeropad(date.getUTCDate()); }
    if (format > 3) {
        str += "T" + zeropad(date.getUTCHours()) +
               ":" + zeropad(date.getUTCMinutes());
    }
    if (format > 5) {
        var secs = Number(date.getUTCSeconds() + "." +
                   ((date.getUTCMilliseconds() < 100) ? '0' : '') +
                   zeropad(date.getUTCMilliseconds()));
        str += ":" + zeropad(secs);
    } else if (format > 4) { str += ":" + zeropad(date.getUTCSeconds()); }

    if (format > 3) { str += offset; }
    return str;
}

Date.prototype.ago = function() {
	var timestamp = this.getTime();
	var agoVal = '';
	var val;

	// Store the current time
	var current_time = new Date();

	if (current_time-0 < timestamp-0) {
		agoVal = "future";
	}
	else {
		// Determine the difference, between the time now and the timestamp
		var difference = (current_time - timestamp) / 1000;
	
		var strings = Components.classes["@mozilla.org/intl/stringbundle;1"]
            .getService(Components.interfaces.nsIStringBundleService)
            .createBundle("chrome://mashtape/locale/mashtape.properties");

		// Set the periods of time
		var periods = [
			"extensions.mashTape.date.sec",
			"extensions.mashTape.date.min",
			"extensions.mashTape.date.hour",
			"extensions.mashTape.date.day",
			"extensions.mashTape.date.week",
			"extensions.mashTape.date.month",
			"extensions.mashTape.date.year",
			"extensions.mashTape.date.decade"
		];

		// Set the number of seconds per period
		var lengths = [1, 60, 3600, 86400, 604800,
			2630880, 31570560, 315705600];

		// Determine which period we should use, based on the number of seconds
		// lapsed.  If the difference divided by the seconds is more than 1,
		// we use that. Eg 1 year / 1 decade = 0.1, so we move on. Go from
		// decades backwards to seconds
		for (val = lengths.length - 1;
				(val >= 0) && ((number = difference / lengths[val]) <= 1);
				val--);

		// Ensure the script has found a match
		if (val < 0) val = 0;

		// Determine the minor value, to recurse through
		var new_time = current_time - (difference % lengths[val]);

		// Set the current value to be floored
		var number = Math.floor(number);

		// If required, make plural
		var key = periods[val];
		if(number != 1)
			key += "s";

		// Return text
		try {
			agoVal = strings.formatStringFromName(key, [number], 1);
		} catch (e) {
		}
	}
	return agoVal;
}

var mtUtils = {
	_prefBranch : Cc["@mozilla.org/preferences-service;1"]
						.getService(Ci.nsIPrefService)
						.getBranch("extensions.mashTape."),
						
	_errorConsole : Cc["@mozilla.org/consoleservice;1"]
						.getService(Ci.nsIConsoleService),

	log: function(providerName, msg, onconsole) {
		if (!this._prefBranch.getBoolPref("debug"))
			return;

		if (onconsole == true)
			dump("[" + providerName + "] " + msg + "\n");
		else
			this._errorConsole.logStringMessage("[" + providerName + "] "+msg);
	}
}

/* A JavaScript implementation of the SHA family of hashes, as defined in FIPS PUB 180-2
 * as well as the corresponding HMAC implementation as defined in FIPS PUB 198a
 * Version 1.2 Copyright Brian Turek 2009
 * Distributed under the BSD License
 * See http://jssha.sourceforge.net/ for more information
 *
 * Several functions taken from Paul Johnson
 */
 
function jsSHA(o,p){jsSHA.charSize=8;jsSHA.b64pad="";jsSHA.hexCase=0;var q=null;var r=null;var s=function(a){var b=[];var c=(1<<jsSHA.charSize)-1;var d=a.length*jsSHA.charSize;for(var i=0;i<d;i+=jsSHA.charSize){b[i>>5]|=(a.charCodeAt(i/jsSHA.charSize)&c)<<(32-jsSHA.charSize-i%32)}return b};var u=function(a){var b=[];var c=a.length;for(var i=0;i<c;i+=2){var d=parseInt(a.substr(i,2),16);if(!isNaN(d)){b[i>>3]|=d<<(24-(4*(i%8)))}else{return"INVALID HEX STRING"}}return b};var v=null;var w=null;if("HEX"===p){if(0!==(o.length%2)){return"TEXT MUST BE IN BYTE INCREMENTS"}v=o.length*4;w=u(o)}else if(("ASCII"===p)||('undefined'===typeof(p))){v=o.length*jsSHA.charSize;w=s(o)}else{return"UNKNOWN TEXT INPUT TYPE"}var A=function(a){var b=jsSHA.hexCase?"0123456789ABCDEF":"0123456789abcdef";var c="";var d=a.length*4;for(var i=0;i<d;i++){c+=b.charAt((a[i>>2]>>((3-i%4)*8+4))&0xF)+b.charAt((a[i>>2]>>((3-i%4)*8))&0xF)}return c};var B=function(a){var b="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";var c="";var d=a.length*4;for(var i=0;i<d;i+=3){var e=(((a[i>>2]>>8*(3-i%4))&0xFF)<<16)|(((a[i+1>>2]>>8*(3-(i+1)%4))&0xFF)<<8)|((a[i+2>>2]>>8*(3-(i+2)%4))&0xFF);for(var j=0;j<4;j++){if(i*8+j*6>a.length*32){c+=jsSHA.b64pad}else{c+=b.charAt((e>>6*(3-j))&0x3F)}}}return c};var C=function(x,n){if(n<32){return(x>>>n)|(x<<(32-n))}else{return x}};var D=function(x,n){if(n<32){return x>>>n}else{return 0}};var E=function(x,y,z){return(x&y)^(~x&z)};var F=function(x,y,z){return(x&y)^(x&z)^(y&z)};var G=function(x){return C(x,2)^C(x,13)^C(x,22)};var I=function(x){return C(x,6)^C(x,11)^C(x,25)};var J=function(x){return C(x,7)^C(x,18)^D(x,3)};var L=function(x){return C(x,17)^C(x,19)^D(x,10)};var M=function(x,y){var a=(x&0xFFFF)+(y&0xFFFF);var b=(x>>>16)+(y>>>16)+(a>>>16);return((b&0xFFFF)<<16)|(a&0xFFFF)};var N=function(a,b,c,d){var e=(a&0xFFFF)+(b&0xFFFF)+(c&0xFFFF)+(d&0xFFFF);var f=(a>>>16)+(b>>>16)+(c>>>16)+(d>>>16)+(e>>>16);return((f&0xFFFF)<<16)|(e&0xFFFF)};var O=function(a,b,c,d,e){var f=(a&0xFFFF)+(b&0xFFFF)+(c&0xFFFF)+(d&0xFFFF)+(e&0xFFFF);var g=(a>>>16)+(b>>>16)+(c>>>16)+(d>>>16)+(e>>>16)+(f>>>16);return((g&0xFFFF)<<16)|(f&0xFFFF)};var P=function(j,k,l){var W=[];var a,b,c,d,e,f,g,h;var m,T2;var H;var K=[0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2];if(l==="SHA-224"){H=[0xc1059ed8,0x367cd507,0x3070dd17,0xf70e5939,0xffc00b31,0x68581511,0x64f98fa7,0xbefa4fa4]}else{H=[0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19]}j[k>>5]|=0x80<<(24-k%32);j[((k+1+64>>9)<<4)+15]=k;var n=j.length;for(var i=0;i<n;i+=16){a=H[0];b=H[1];c=H[2];d=H[3];e=H[4];f=H[5];g=H[6];h=H[7];for(var t=0;t<64;t++){if(t<16){W[t]=j[t+i]}else{W[t]=N(L(W[t-2]),W[t-7],J(W[t-15]),W[t-16])}m=O(h,I(e),E(e,f,g),K[t],W[t]);T2=M(G(a),F(a,b,c));h=g;g=f;f=e;e=M(d,m);d=c;c=b;b=a;a=M(m,T2)}H[0]=M(a,H[0]);H[1]=M(b,H[1]);H[2]=M(c,H[2]);H[3]=M(d,H[3]);H[4]=M(e,H[4]);H[5]=M(f,H[5]);H[6]=M(g,H[6]);H[7]=M(h,H[7])}switch(l){case"SHA-224":return[H[0],H[1],H[2],H[3],H[4],H[5],H[6]];case"SHA-256":return H;default:return[]}};this.getHash=function(a,b){var c=null;var d=w.slice();switch(b){case"HEX":c=A;break;case"B64":c=B;break;default:return"FORMAT NOT RECOGNIZED"}switch(a){case"SHA-224":if(q===null){q=P(d,v,a)}return c(q);case"SHA-256":if(r===null){r=P(d,v,a)}return c(r);default:return"HASH NOT RECOGNIZED"}};this.getHMAC=function(a,b,c,d){var e=null;var f=null;var g=[];var h=[];var j=null;var k=null;var l=null;switch(d){case"HEX":e=A;break;case"B64":e=B;break;default:return"FORMAT NOT RECOGNIZED"}switch(c){case"SHA-224":l=224;break;case"SHA-256":l=256;break;default:return"HASH NOT RECOGNIZED"}if("HEX"===b){if(0!==(a.length%2)){return"KEY MUST BE IN BYTE INCREMENTS"}f=u(a);k=a.length*4}else if("ASCII"===b){f=s(a);k=a.length*jsSHA.charSize}else{return"UNKNOWN KEY INPUT TYPE"}if(512<k){f=P(f,k,c);f[15]&=0xFFFFFF00}else if(512>k){f[15]&=0xFFFFFF00}for(var i=0;i<=15;i++){g[i]=f[i]^0x36363636;h[i]=f[i]^0x5C5C5C5C}j=P(g.concat(w),512+v,c);j=P(h.concat(j),512+l,c);return(e(j))}}
