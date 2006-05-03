/*
SMILScript: A partial SMIL implementation in script, to provide transitional functionality for Firefox, Batik, and other SVG Viewers without SMIL animation. Many thanks to Jonathan Watt and Jeff Rafter for advice.

Author:  Doug Schepers [doug.schepers@vectoreal.com]
Date:    Friday, November 04, 2005
Version: 0.1

This software is Copyright (c) Vectoreal (tm) and Doug Schepers.
All rights reserved.

The Artistic License
[http://www.opensource.org/licenses/artistic-license.php]

Preamble

The intent of this document is to state the conditions under which a Package may be copied, such that the Copyright Holder maintains some semblance of artistic control over the development of the package, while giving the users of the package the right to use and distribute the Package in a more-or-less customary fashion, plus the right to make reasonable modifications.

Definitions:

    * "Package" refers to the collection of files distributed by the Copyright Holder, and derivatives of that collection of files created through textual modification.
    * "Standard Version" refers to such a Package if it has not been modified, or has been modified in accordance with the wishes of the Copyright Holder.
    * "Copyright Holder" is whoever is named in the copyright or copyrights for the package.
    * "You" is you, if you're thinking about copying or distributing this Package.
    * "Reasonable copying fee" is whatever you can justify on the basis of media cost, duplication charges, time of people involved, and so on. (You will not be required to justify it to the Copyright Holder, but only to the computing community at large as a market that must bear the fee.)
    * "Freely Available" means that no fee is charged for the item itself, though there may be fees involved in handling the item. It also means that recipients of the item may redistribute it under the same conditions they received it.

1. You may make and give away verbatim copies of the source form of the Standard Version of this Package without restriction, provided that you duplicate all of the original copyright notices and associated disclaimers.

2. You may apply bug fixes, portability fixes and other modifications derived from the Public Domain or from the Copyright Holder. A Package modified in such a way shall still be considered the Standard Version.

3. You may otherwise modify your copy of this Package in any way, provided that you insert a prominent notice in each changed file stating how and when you changed that file, and provided that you do at least ONE of the following:

    a) place your modifications in the Public Domain or otherwise make them Freely Available, such as by posting said modifications to Usenet or an equivalent medium, or placing the modifications on a major archive site such as ftp.uu.net, or by allowing the Copyright Holder to include your modifications in the Standard Version of the Package.

    b) use the modified Package only within your corporation or organization.

    c) rename any non-standard executables so the names do not conflict with standard executables, which must also be provided, and provide a separate manual page for each non-standard executable that clearly documents how it differs from the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

4. You may distribute the programs of this Package in object code or executable form, provided that you do at least ONE of the following:

    a) distribute a Standard Version of the executables and library files, together with instructions (in the manual page or equivalent) on where to get the Standard Version.

    b) accompany the distribution with the machine-readable source of the Package with your modifications.

    c) accompany any non-standard executables with their corresponding Standard Version executables, giving the non-standard executables non-standard names, and clearly documenting the differences in manual pages (or equivalent), together with instructions on where to get the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

5. You may charge a reasonable copying fee for any distribution of this Package. You may charge any fee you choose for support of this Package. You may not charge a fee for this Package itself. However, you may distribute this Package in aggregate with other (possibly commercial) programs as part of a larger (possibly commercial) software distribution provided that you do not advertise this Package as a product of your own.

6. The scripts and library files supplied as input to or produced as output from the programs of this Package do not automatically fall under the copyright of this Package, but belong to whomever generated them, and may be sold commercially, and may be aggregated with this Package.

7. C or perl subroutines supplied by you and linked into this Package shall not be considered part of this Package.

8. The name of the Copyright Holder may not be used to endorse or promote products derived from this software without specific prior written permission.

9. THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.

The End

*/
var SVGDocument = null;
var SVGRoot = null;
var svgns = 'http://www.w3.org/2000/svg';
var xlinkns = 'http://www.w3.org/1999/xlink';

var Event = null;
var Smil  = null;

var textOutput = null;

function InitSMIL(evt)
{
   SVGDocument = evt.target.ownerDocument;
   SVGRoot     = SVGDocument.documentElement;
   textOutput = SVGDocument.getElementById('textOutput');

   try
   {
      if ( -1 != getSVGViewerVersion().indexOf('ASV') )
      {
         return;
      }
   }
   catch(er) {}

   Event       = new EventObj();
   Smil        = new SMILObj();

   Smil.getSmilElements();
};


function EventObj(){};
EventObj.prototype = {};

EventObj.prototype.eventHandler = function( evt )
{
   var triggerEl = evt.target;
   var smilArray = Smil.idArray[ triggerEl.id ];

   for ( var i = 0, iLen = smilArray.length; iLen > i; i++ )
   {
      var eachObj = smilArray[i];
      if ( evt.type == eachObj.begin
       || -1 != eachObj.begin.indexOf( evt.type )
       || -1 != eachObj.begin.indexOf( triggerEl.id + '.' + evt.type ) )
      {
         var eventValue = eachObj.begin.split('+');
         if ( 1 == eventValue.length )
         {
            Smil[ eachObj.name ]( eachObj.index );
         }
         else
         {
            var eventTimeout = Smil.normalizeTime( eventValue[1] );

            var smilCall = 'Smil.dispatch( ' + eachObj.index + ' )';
            window.setTimeout( smilCall, eventTimeout );
         }
      }
   }
};


function SMILObj()
{
   this.idArray       = [];
   this.objArray      = [];
   this.elementArray  = [];
   this.triggerArray  = [];
   this.timeline      = null;
}
SMILObj.prototype = {};

SMILObj.prototype.props = function( index, element, id, name, parentEl, targetEl, href, begin, end, attributeName, attributeType, type, from, to, by, values, path, pathEl, begin, end, dur, fill, repeatCount, initialValue )
{
   this.index              = index;
   this.element            = element;
   this.id                 = id;
   this.name               = name;
   this.parentEl           = parentEl;
   this.targetEl           = targetEl;
   this.href               = href;
   this.begin              = begin;
   this.end                = end;
   this.attributeName      = attributeName;
   this.attributeType      = attributeType;
   this.type               = type;
   this.from               = from;
   this.to                 = to;
   this.by                 = by;
   this.values             = values;
   this.path               = path;
   this.pathEl             = pathEl;
   this.begin              = begin;
   this.end                = end;
   this.dur                = dur;
   this.fill               = fill;
   this.repeatCount        = repeatCount;
   this.repeatIndex        = Number(repeatCount);
   this.initialValue       = Number(initialValue);
   this.currentValue       = Number(initialValue);
   this.increment          = null;
   this.interval           = null;
   this.initialValueArray  = [];
   this.currentValueArray  = [];
   this.finalValueArray    = [];
   this.incrementArray     = [];
   this.syncArray          = [];
};

SMILObj.prototype.getSmilElements = function()
{
   var smilElementsArray = ['set', 'animate', 'animateColor', 'animateMotion', 'animateTransform'];
   for ( var i = 0, iLen = smilElementsArray.length; iLen > i; i++ )
   {
      var eachElementType = smilElementsArray[i];

      var allElements = SVGDocument.getElementsByTagNameNS(svgns, eachElementType);
      for ( var s = 0, sLen = allElements.length; sLen > s; s++ )
      {
         var eachElement = allElements.item(s);
         var smilObj = this.registerElement( eachElement );
      }
   }
};

SMILObj.prototype.registerElement = function( element )
{
   var id            = element.id;
   var name          = element.localName;
   var parentEl      = element.parentNode;
   var targetEl      = element.parentNode;
   var href          = element.getAttributeNS(xlinkns, 'href');
   var begin         = element.getAttributeNS(null, 'begin');
   var end           = element.getAttributeNS(null, 'end');
   var attributeName = element.getAttributeNS(null, 'attributeName');
   var attributeType = element.getAttributeNS(null, 'attributeType');
   var type          = element.getAttributeNS(null, 'type');
   var from          = element.getAttributeNS(null, 'from');
   var to            = element.getAttributeNS(null, 'to');
   var by            = element.getAttributeNS(null, 'by');
   var values        = element.getAttributeNS(null, 'values');
   var path          = element.getAttributeNS(null, 'path');
   var begin         = element.getAttributeNS(null, 'begin');
   var end           = element.getAttributeNS(null, 'end');
   var dur           = element.getAttributeNS(null, 'dur');
   var fill          = element.getAttributeNS(null, 'fill');
   var repeatCount   = element.getAttributeNS(null, 'repeatCount');

   if ( href )
   {
      targetEl = GetElementByAtt( 'id', href.replace('#', '') );
   }

   if ( 'CSS' == attributeType )
   {
      var initialValue = targetEl.style.getPropertyValue( attributeName );
   }
   else
   {
      var initialValue = targetEl.getAttributeNS( null, attributeName );
   }

   if ( !from )
   {
      if ( initialValue )
      {
         from = initialValue;
      }
      else if ( propDefaults[attributeName] )
      {
         from = propDefaults[attributeName];
      }
      else
      {
         from = 0;
      }
   }

   if ( by && !to )
   {
      to = by;
   }

   var pathEl = null;
   if ( 'animateMotion' == name )
   {
      if ( !path )
      {
         var mpaths = element.getElementsByTagNameNS(svgns, 'mpath');
         var mpath = mpaths.item(0);
         if ( mpath )
         {
            var pathHref = mpath.getAttributeNS(xlinkns, 'href');
            pathEl = GetElementByAtt( 'id', pathHref.replace('#', '')  );
            path = pathEl.getAttributeNS(null, 'd');
         }
      }
      else
      {
         pathEl = SVGDocument.createElementNS(svgns, 'path');
         pathEl.setAttributeNS(null, 'd', path);
         pathEl.setAttributeNS(null, 'display', 'none');
         element.parentNode.appendChild( pathEl );
      }
   }

   begin = begin.replace(' ', '');
   to = to.replace(' ', '');

   var index = this.objArray.length;

   var smilObj = new this.props( index, element, id, name, parentEl, targetEl, href, begin, end, attributeName, attributeType, type, from, to, by, values, path, pathEl, begin, end, dur, fill, repeatCount, initialValue );

   this.objArray.push( smilObj );
   this.elementArray[ element ] = index;

   smilObj.syncArray['begin'] = [];
   smilObj.syncArray['end'] = [];

   this.processBeginTriggers( element, smilObj );

   var beginArray = begin.split(';');
   for ( var i = 0, iLen = beginArray.length; iLen > i; i++ )
   {
      var eachBegin = beginArray[i];
      var triggerElement = parentEl;

      var eventTimeout = Smil.normalizeTime( eachBegin );
      if ( !isNaN(eventTimeout) )
      {
         var smilCall = 'Smil.dispatch( ' + smilObj.index + ' )';
         window.setTimeout( smilCall, eventTimeout );
      }
      else
      {
         var triggerValue = eachBegin.split('+');
         var triggerArray = triggerValue[0].split('.');
         var triggerEvent = triggerArray[0];
         if ( 1 < triggerArray.length )
         {
            var triggerId = triggerArray[0];
            triggerElement = GetElementByAtt( 'id', triggerId  );

            triggerEvent= triggerArray[1];
         }

         if ( 'begin' == triggerEvent || 'end' == triggerEvent )
         {

            this.addTriggerToElement( triggerElement, triggerId, triggerEvent, smilObj, eachBegin );
         }
         else if ( triggerElement )
         {
            if ( this.idArray[ triggerElement.id ] )
            {
               this.idArray[ triggerElement.id ].push( smilObj );
            }
            else
            {
               this.idArray[ triggerElement.id ] = [ smilObj ];
            }

            triggerElement.addEventListener(triggerEvent, Event.eventHandler, false);
         }
      }
   }

   return smilObj;
};

SMILObj.prototype.getObjByElement = function( element )
{
   var obj = null;
   for ( var i = 0, iLen = this.objArray.length; iLen > i; i++ )
   {
      var eachObj = this.objArray[i];
      if ( element == eachObj.element )
      {
         obj = eachObj;
      }
   }
   return obj;
};

SMILObj.prototype.addTriggerToElement = function( triggerEl, triggerId, triggerEvent, smilObj, begin )
{
   if ( 'prev' == triggerId )
   {
      triggerEl = smilObj.element.previousSibling;
      while( triggerEl && 1 != triggerEl.nodeType )
      {
         triggerEl = triggerEl.previousSibling;
      }
   }

   var triggerObj = this.getObjByElement( triggerEl );
   if ( triggerObj )
   {
      this.processEachBegin( triggerObj, smilObj, begin );
   }
   else
   {
      var trigger = this.getTriggerByElement( triggerEl );
      if ( !trigger )
      {
         trigger = new this.triggerHash( triggerEl );
         this.triggerArray.push( trigger );
      }
      trigger.triggerTargets.push( smilObj );
   }
};

SMILObj.prototype.triggerHash = function( element )
{
   this.element        = element;
   this.triggerTargets = [];
};

SMILObj.prototype.getTriggerByElement = function( element )
{
   var obj = null;
   for ( var i = 0, iLen = this.triggerArray.length; iLen > i; i++ )
   {
      var eachTrigger = this.triggerArray[i];
      if ( element == eachTrigger.element )
      {
         obj = eachTrigger;
      }
   }
   return obj;
};

SMILObj.prototype.processBeginTriggers = function( triggerEl, triggerObj )
{
   var trigger = this.getTriggerByElement( triggerEl );
   if ( trigger )
   {
      var triggerArray = trigger.triggerTargets;
      for ( var i = 0, iLen = triggerArray.length; iLen > i; i++ )
      {
         var eachObj = triggerArray[i];
         var beginArray = eachObj.begin.split(';');
         for ( var b = 0, bLen = beginArray.length; bLen > b; b++ )
         {
            var eachBegin = beginArray[b];
            this.processEachBegin( triggerObj, eachObj, eachBegin );
         }
      }
   }
};

SMILObj.prototype.syncObj = function( objIndex, triggerTime )
{
   this.objIndex    = objIndex;
   this.triggerTime = triggerTime;
};

SMILObj.prototype.processEachBegin = function( triggerObj, targetObj, begin )
{
   var triggerValue = begin.split('+');
   var triggerArray = triggerValue[0].split('.');
   var triggerEvent = triggerArray[0];
   if ( 2 == triggerArray.length )
   {
      triggerEvent= triggerArray[1];
   }
   var triggerTime = this.normalizeTime( triggerValue[1] );

   var sync = new this.syncObj( targetObj.index, triggerTime );

   triggerObj.syncArray[ triggerEvent ].push( sync );
};

SMILObj.prototype.dispatch = function( objId )
{
   var obj = this.objArray[ objId ];
   this[ obj.name ]( objId );

   this.dispatchSyncs( obj.syncArray[ 'begin' ] );
};

SMILObj.prototype.set = function( objId )
{
   var obj = this.objArray[ objId ];
   var targetEl = obj.targetEl;
   this.resetRepeat( obj );

   if ( 'CSS' == obj.attributeType )
   {
      targetEl.style.setProperty(obj.attributeName, obj.to, '');
   }
   else
   {
      targetEl.setAttributeNS(null, obj.attributeName, obj.to);
   }
};

SMILObj.prototype.animate = function( objId )
{
   var obj = this.objArray[ objId ];
   this.resetRepeat( obj );

   var duration = (Smil.normalizeTime( obj.dur ) / 1000) * 70;
   var from = Number( obj.from );

   var to = Number( obj.to );

   var diff = Math.max(from, to) - Math.min(from, to);
   obj.increment = Math.round((diff/duration) * 1000 ) / 1000;

   if ( to < from )
   {
      obj.increment *= -1;
   }

   var smilCall = 'Smil.setRepeat( ' + objId + ' )';
   window.setTimeout( smilCall, 10 );
};

SMILObj.prototype.animateMotion = function( objId )
{
   var obj = this.objArray[ objId ];
   this.resetRepeat( obj );

   if ( obj.path )
   {
      try
      {
         var point = obj.pathEl.getPointAtLength( 0 );
         var distance = obj.pathEl.getTotalLength();
         var duration = (Smil.normalizeTime( obj.dur ) / 1000) * 70;

         obj.increment = distance/duration;
         obj.currentValue = 0;

         var smilCall = 'Smil.incrementPosition( ' + objId + ' )';
         window.setTimeout( smilCall, 10 );
      }
      catch(er){}
   }
   else if ( obj.to )
   {
      var duration = (Smil.normalizeTime( obj.dur ) / 1000) * 70;

      obj.finalValueArray = obj.to.split( ',' );

      obj.initialValueArray = [0];
      if ( 2 == obj.finalValueArray.length )
      {
         obj.initialValueArray.push(0);
      }
      obj.currentValueArray = [];
      obj.currentValueArray = obj.initialValueArray.concat();


      for ( var i = 0, iLen = obj.initialValueArray.length; iLen > i; i++ )
      {
         var from = Number( obj.initialValueArray[i] );

         var to = Number( obj.finalValueArray[i] );

         var diff = Math.max(from, to) - Math.min(from, to);
         obj.incrementArray[i] = Math.round((diff/duration) * 1000 ) / 1000;

         if ( to < from )
         {
            obj.incrementArray[i] *= -1;
         }
      }

      var smilCall = 'Smil.setRepeatArray( ' + objId + ' )';
      window.setTimeout( smilCall, 10 );
   }
   else { }
};

SMILObj.prototype.animateTransform = function( objId )
{
   var obj = this.objArray[ objId ];
   this.resetRepeat( obj );

   var duration = (Smil.normalizeTime( obj.dur ) / 1000) * 70;

   obj.initialValueArray = this.splitTransform( obj.from );
   obj.currentValueArray = [];
   obj.currentValueArray = obj.initialValueArray.concat();
   obj.finalValueArray = this.splitTransform( obj.to );

   for ( var i = 0, iLen = obj.initialValueArray.length; iLen > i; i++ )
   {
      var from = Number( obj.initialValueArray[i] );

      var to = Number( obj.finalValueArray[i] );

      var diff = Math.max(from, to) - Math.min(from, to);
      obj.incrementArray[i] = Math.round((diff/duration) * 1000 ) / 1000;

      if ( to < from )
      {
         obj.incrementArray[i] *= -1;
      }
   }

   var smilCall = 'Smil.setRepeatArray( ' + objId + ' )';
   window.setTimeout( smilCall, 10 );
};

SMILObj.prototype.animateColor = function( objId )
{
   var obj = this.objArray[ objId ];
   this.resetRepeat( obj );

   if ( -1 != obj.from.indexOf('#') )
   {
      var hex = colorList.normalizeHex( obj.from );
      var rgb = colorList.getRgbByHex( hex );
   }
   else if ( -1 != obj.from.indexOf('rgb') )
   {
      var rgb = colorList.normalizeRgb( obj.from );
   }
   else
   {
      var rgb = colorList.getRgbByName( obj.from );
   }

   obj.initialValueArray = colorList.splitRgb( rgb );
   obj.currentValueArray = [];
   obj.currentValueArray = obj.initialValueArray.concat();

   if ( -1 != obj.to.indexOf('#') )
   {
      var hex = colorList.normalizeHex( obj.to );
      var rgb = colorList.getRgbByHex( hex );
   }
   else if ( -1 != obj.to.indexOf('rgb') )
   {
      var rgb = colorList.normalizeRgb( obj.to );
   }
   else
   {
      var rgb = colorList.getRgbByName( obj.to );
   }

   obj.finalValueArray = colorList.splitRgb( rgb );


   var duration = (Smil.normalizeTime( obj.dur ) / 1000) * 70;

   for ( var i = 0, iLen = obj.initialValueArray.length; iLen > i; i++ )
   {
      var from = Number( obj.initialValueArray[i] );

      var to = Number( obj.finalValueArray[i] );

      var diff = Math.max(from, to) - Math.min(from, to);
      obj.incrementArray[i] = Math.round((diff/duration) * 1000 ) / 1000;

      if ( to < from )
      {
         obj.incrementArray[i] *= -1;
      }
   }

   var smilCall = 'Smil.setRepeatArray( ' + objId + ' )';
   window.setTimeout( smilCall, 10 );
};

SMILObj.prototype.setRepeat = function( objId )
{
   var obj = this.objArray[ objId ];
   var targetEl = obj.targetEl;

   obj.currentValue = Math.round((Number(obj.currentValue) * 1000) + (Number(obj.increment) * 1000)) / 1000;

   if ( (0 > obj.increment && obj.to >= obj.currentValue)
      || (0 < obj.increment && obj.to <= obj.currentValue))
   {
      obj.repeatIndex--;

      if ( 0 < obj.repeatIndex || 'freeze' != obj.fill )
      {
         obj.currentValue = obj.from;
      }
   }

   if ( 'CSS' == obj.attributeType )
   {
      targetEl.style.setProperty(obj.attributeName, obj.currentValue, '');
   }
   else
   {
      targetEl.setAttributeNS(null, obj.attributeName, obj.currentValue);
   }

   if ( 'indefinite' == obj.repeatCount )
   {
      window.setTimeout( 'Smil.setRepeat( ' + objId + ' )', 10 );
   }
   else if ( 0 < obj.repeatIndex )
   {
      window.setTimeout( 'Smil.setRepeat( ' + objId + ' )', 10 );
   }
   else
   {
      this.dispatchSyncs( obj.syncArray[ 'end' ] );
   }
};

SMILObj.prototype.setRepeatArray = function( objId )
{
   var obj = this.objArray[ objId ];
   var targetEl = obj.targetEl;

   var completeArray = [];

   for ( var i = 0, iLen = obj.currentValueArray.length; iLen > i; i++ )
   {
      var eachItem = obj.currentValueArray[i];
      if ( 0 != obj.incrementArray[i] && !isNaN(eachItem) )
      {
         obj.currentValueArray[i] = Math.round((Number(obj.currentValueArray[i]) * 1000) + (Number(obj.incrementArray[i]) * 1000)) / 1000;
         if ( 'animateColor' == obj.name )
         {
            obj.currentValueArray[i] = Math.round(obj.currentValueArray[i]);
         }

         completeArray[i] = false;
         if ( (0 >= obj.incrementArray[i] && obj.finalValueArray[i] >= obj.currentValueArray[i])
            || (0 <= obj.incrementArray[i] && obj.finalValueArray[i] <= obj.currentValueArray[i]))
         {
            completeArray[i] = true;
         }
      }
   }

   var completeString = completeArray.join(',');
   if ( -1 == completeString.indexOf('false') )
   {
      obj.repeatIndex--;

      if ( 0 < obj.repeatIndex || 'freeze' != obj.fill )
      {
         obj.currentValueArray = [];
         obj.currentValueArray = obj.initialValueArray.concat();
      }
   }

   var attName = obj.attributeName;
   var newValue = obj.currentValueArray.join(' ');
   if ( 'animateColor' == obj.name )
   {
      newValue = 'rgb(' + obj.currentValueArray.join(',') + ')';
   }
   else if ( 'animateMotion' == obj.name )
   {
      attName = 'transform';
      newValue = 'translate(' + obj.currentValueArray.join(',') + ')';
   }
   else if ( 'animateTransform' == obj.name )
   {
      newValue = obj.type + '(' + obj.currentValueArray.join(',') + ')';
   }

   if ( 'CSS' == obj.attributeType )
   {
      targetEl.style.setProperty(attName, newValue, '');
   }
   else
   {
      targetEl.setAttributeNS(null, attName, newValue);
   }

   if ( 'indefinite' == obj.repeatCount )
   {
      window.setTimeout( 'Smil.setRepeatArray( ' + objId + ' )', 10 );
   }
   else if ( 0 < obj.repeatIndex )
   {
      window.setTimeout( 'Smil.setRepeatArray( ' + objId + ' )', 10 );
   }
   else
   {
      this.dispatchSyncs( obj.syncArray[ 'end' ] );
   }
};

SMILObj.prototype.incrementPosition = function( objId )
{
   var obj = this.objArray[ objId ];
   var targetEl = obj.targetEl;

   var pos = obj.pathEl.getPointAtLength( obj.currentValue );

   obj.currentValue += obj.increment;

   if ( obj.pathEl.getTotalLength() < obj.currentValue )
   {
      obj.currentValue = 0;
      obj.repeatIndex--;
   }

   targetEl.setAttributeNS(null, 'transform', 'translate(' + pos.x + ',' + pos.y + ')' );

   if ( 'indefinite' == obj.repeatCount )
   {
      window.setTimeout( 'Smil.incrementPosition( ' + objId + ' )', 10 );
   }
   else if ( 0 < obj.repeatIndex )
   {
      window.setTimeout( 'Smil.incrementPosition( ' + objId + ' )', 10 );
   }
   else
   {
      this.dispatchSyncs( obj.syncArray[ 'end' ] );
   }
};

SMILObj.prototype.dispatchSyncs = function( syncArray )
{
   for ( var i = 0, iLen = syncArray.length; iLen > i; i++ )
   {
      var eachSync = syncArray[i];

      var eventTimeout = eachSync.triggerTime;

      var smilCall = 'Smil.dispatch( ' + eachSync.objIndex + ' )';
      window.setTimeout( smilCall, eventTimeout );
   }

};

SMILObj.prototype.splitTransform = function( value )
{
   value = value.replace(' ', '');
   var valueSplit = value.split(/(,)/);
   var newValueSplit = [];
   for ( var i = 0, iLen = valueSplit.length; iLen > i; i++ )
   {
      var eachItem = Number(valueSplit[i]);
      if ( !isNaN(eachItem) )
      {
         newValueSplit.push( eachItem );
      }

   }

   return newValueSplit;
};

SMILObj.prototype.normalizeTime = function( time )
{
   if ( 'indefinite' == time )
   {
      return time;
   }

   if ( !time )
   {
      return 0;
   }

   var milliseconds = 0;
   if ( -1 != time.indexOf('ms') )
   {
      milliseconds = Number( time.replace('ms', '') );
   }
   else if ( -1 != time.indexOf('s') )
   {
      milliseconds = Number( time.replace('s', '') ) * 1000;
   }
   else if ( -1 != time.indexOf('min') )
   {
      milliseconds = Number( time.replace('min', '') ) * 60000;
   }
   else if ( -1 != time.indexOf('h') )
   {
      milliseconds = Number( time.replace('h', '') ) * 3600000;
   }
   else
   {
      var timeArray = time.split(':');
      var hours = 0;
      var minutes = 0;
      var seconds = 0;
      if ( 3 == timeArray.length )
      {
         hours = Number( timeArray[0] ) * 3600000;
         minutes = Number( timeArray[1] ) * 60000;
         seconds = Number( timeArray[2] ) * 1000;
      }
      else if ( 2 == timeArray.length )
      {
         minutes = Number( timeArray[0] ) * 60000;
         seconds = Number( timeArray[1] ) * 1000;
      }
      else
      {
         seconds = Number( timeArray[0] ) * 1000;
      }
      milliseconds = hours + minutes + seconds;
   }

   return milliseconds;
};

SMILObj.prototype.resetRepeat = function( obj )
{
   if ( 'indefinite' != obj.repeatCount )
   {
      obj.repeatIndex = Number(obj.repeatCount);
   }
};


var propDefaults = [];
propDefaults['alignment-baseline'] = '0';
propDefaults['baseline-shift'] = 'baseline';
propDefaults['clip'] = 'auto';
propDefaults['clip-path'] = 'none';
propDefaults['clip-rule'] = 'nonzero';
propDefaults['color'] = 'depends on user agent';
propDefaults['color-interpolation'] = 'sRGB';
propDefaults['color-interpolation-filters'] = 'linearRGB';
propDefaults['color-profile'] = 'auto';
propDefaults['color-rendering'] = 'auto';
propDefaults['cursor'] = 'auto';
propDefaults['direction'] = 'ltr';
propDefaults['display'] = 'inline';
propDefaults['dominant-baseline'] = 'auto';
propDefaults['enable-background'] = 'accumulate';
propDefaults['fill'] = 'black';
propDefaults['fill-opacity'] = '1';
propDefaults['fill-rule'] = 'nonzero';
propDefaults['filter'] = 'none';
propDefaults['flood-color'] = 'black';
propDefaults['flood-opacity'] = '1';
propDefaults['font'] = 'see individual properties';
propDefaults['font-family'] = 'Arial';
propDefaults['font-size'] = 'medium';
propDefaults['font-size-adjust'] = 'none';
propDefaults['font-stretch'] = 'normal';
propDefaults['font-style'] = 'normal';
propDefaults['font-variant'] = 'normal';
propDefaults['font-weight'] = 'normal';
propDefaults['glyph-orientation-horizontal'] = '0';
propDefaults['glyph-orientation-vertical'] = 'auto';
propDefaults['image-rendering'] = 'auto';
propDefaults['kerning'] = 'auto';
propDefaults['letter-spacing'] = 'normal';
propDefaults['lighting-color'] = 'white';
propDefaults['marker-end'] = 'none';
propDefaults['marker-mid'] = 'none';
propDefaults['marker-start'] = 'none';
propDefaults['mask'] = 'none';
propDefaults['opacity'] = '1';
propDefaults['overflow'] = 'hidden';
propDefaults['pointer-events'] = 'visiblePainted';
propDefaults['shape-rendering'] = 'auto';
propDefaults['stop-color'] = 'black';
propDefaults['stop-opacity'] = '1';
propDefaults['stroke'] = 'none';
propDefaults['stroke-dasharray'] = 'none';
propDefaults['stroke-dashoffset'] = '0';
propDefaults['stroke-linecap'] = 'butt';
propDefaults['stroke-linejoin'] = 'miter';
propDefaults['stroke-miterlimit'] = '4';
propDefaults['stroke-opacity'] = '1';
propDefaults['stroke-width'] = '1';
propDefaults['text-anchor'] = 'start';
propDefaults['text-decoration'] = 'none';
propDefaults['text-rendering'] = 'auto';
propDefaults['unicode-bidi'] = 'normal';
propDefaults['visibility'] = 'visible';
propDefaults['word-spacing'] = 'normal';
propDefaults['writing-mode'] = 'lr-tb';


var colorList = {
   nameArray : ['aliceblue', 'antiquewhite', 'aqua', 'aquamarine', 'azure', 'beige', 'bisque', 'black', 'blanchedalmond', 'blue', 'blueviolet', 'brown', 'burlywood', 'cadetblue', 'chartreuse', 'chocolate', 'coral', 'cornflowerblue', 'cornsilk', 'crimson', 'cyan', 'darkblue', 'darkcyan', 'darkgoldenrod', 'darkgray', 'darkgreen', 'darkgrey', 'darkkhaki', 'darkmagenta', 'darkolivegreen', 'darkorange', 'darkorchid', 'darkred', 'darksalmon', 'darkseagreen', 'darkslateblue', 'darkslategray', 'darkslategrey', 'darkturquoise', 'darkviolet', 'deeppink', 'deepskyblue', 'dimgray', 'dimgrey', 'dodgerblue', 'firebrick', 'floralwhite', 'forestgreen', 'fuchsia', 'gainsboro', 'ghostwhite', 'gold', 'goldenrod', 'gray', 'grey', 'green', 'greenyellow', 'honeydew', 'hotpink', 'indianred', 'indigo', 'ivory', 'khaki', 'lavender', 'lavenderblush', 'lawngreen', 'lemonchiffon', 'lightblue', 'lightcoral', 'lightcyan', 'lightgoldenrodyellow', 'lightgray', 'lightgreen', 'lightgrey', 'lightpink', 'lightsalmon', 'lightseagreen', 'lightskyblue', 'lightslategray', 'lightslategrey', 'lightsteelblue', 'lightyellow', 'lime', 'limegreen', 'linen', 'magenta', 'maroon', 'mediumaquamarine', 'mediumblue', 'mediumorchid', 'mediumpurple', 'mediumseagreen', 'mediumslateblue', 'mediumspringgreen', 'mediumturquoise', 'mediumvioletred', 'midnightblue', 'mintcream', 'mistyrose', 'moccasin', 'navajowhite', 'navy', 'oldlace', 'olive', 'olivedrab', 'orange', 'orangered', 'orchid', 'palegoldenrod', 'palegreen', 'paleturquoise', 'palevioletred', 'papayawhip', 'peachpuff', 'peru', 'pink', 'plum', 'powderblue', 'purple', 'red', 'rosybrown', 'royalblue', 'saddlebrown', 'salmon', 'sandybrown', 'seagreen', 'seashell', 'sienna', 'silver', 'skyblue', 'slateblue', 'slategray', 'slategrey', 'snow', 'springgreen', 'steelblue', 'tan', 'teal', 'thistle', 'tomato', 'turquoise', 'violet', 'wheat', 'white', 'whitesmoke', 'yellow', 'yellowgreen'],

   hexArray : ['#f0f8ff', '#faebd7', '#00ffff', '#7fffd4', '#f0ffff', '#f5f5dc', '#ffe4c4', '#000000', '#ffebcd', '#0000ff', '#8a2be2', '#a52a2a', '#deb887', '#5f9ea0', '#7fff00', '#d2691e', '#ff7f50', '#6495ed', '#fff8dc', '#dc143c', '#00ffff', '#00008b', '#008b8b', '#b8860b', '#a9a9a9', '#006400', '#a9a9a9', '#bdb76b', '#8b008b', '#556b2f', '#ff8c00', '#9932cc', '#8b0000', '#e9967a', '#8fbc8f', '#483d8b', '#2f4f4f', '#2f4f4f', '#00ced1', '#9400d3', '#ff1493', '#00bfff', '#696969', '#696969', '#1e90ff', '#b22222', '#fffaf0', '#228b22', '#ff00ff', '#dcdcdc', '#f8f8ff', '#ffd700', '#daa520', '#808080', '#808080', '#008000', '#adff2f', '#f0fff0', '#ff69b4', '#cd5c5c', '#4b0082', '#fffff0', '#f0e68c', '#e6e6fa', '#fff0f5', '#7cfc00', '#fffacd', '#add8e6', '#f08080', '#e0ffff', '#fafad2', '#d3d3d3', '#90ee90', '#d3d3d3', '#ffb6c1', '#ffa07a', '#20b2aa', '#87cefa', '#778899', '#778899', '#b0c4de', '#ffffe0', '#00ff00', '#32cd32', '#faf0e6', '#ff00ff', '#800000', '#66cdaa', '#0000cd', '#ba55d3', '#9370db', '#3cb371', '#7b68ee', '#00fa9a', '#48d1cc', '#c71585', '#191970', '#f5fffa', '#ffe4e1', '#ffe4b5', '#ffdead', '#000080', '#fdf5e6', '#808000', '#6b8e23', '#ffa500', '#ff4500', '#da70d6', '#eee8aa', '#98fb98', '#afeeee', '#db7093', '#ffefd5', '#ffdab9', '#cd853f', '#ffc0cb', '#dda0dd', '#b0e0e6', '#800080', '#ff0000', '#bc8f8f', '#4169e1', '#8b4513', '#fa8072', '#f4a460', '#2e8b57', '#fff5ee', '#a0522d', '#c0c0c0', '#87ceeb', '#6a5acd', '#708090', '#708090', '#fffafa', '#00ff7f', '#4682b4', '#d2b48c', '#008080', '#d8bfd8', '#ff6347', '#40e0d0', '#ee82ee', '#f5deb3', '#ffffff', '#f5f5f5', '#ffff00', '#9acd32'],


   rgbArray : ['rgb(240, 248, 255)', 'rgb(250, 235, 215)', 'rgb(0, 255, 255)', 'rgb(127, 255, 212)', 'rgb(240, 255, 255)', 'rgb(245, 245, 220)', 'rgb(255, 228, 196)', 'rgb(0, 0, 0)', 'rgb(255, 235, 205)', 'rgb(0, 0, 255)', 'rgb(138, 43, 226)', 'rgb(165, 42, 42)', 'rgb(222, 184, 135)', 'rgb(95, 158, 160)', 'rgb(127, 255, 0)', 'rgb(210, 105, 30)', 'rgb(255, 127, 80)', 'rgb(100, 149, 237)', 'rgb(255, 248, 220)', 'rgb(220, 20, 60)', 'rgb(0, 255, 255)', 'rgb(0, 0, 139)', 'rgb(0, 139, 139)', 'rgb(184, 134, 11)', 'rgb(169, 169, 169)', 'rgb(0, 100, 0)', 'rgb(169, 169, 169)', 'rgb(189, 183, 107)', 'rgb(139, 0, 139)', 'rgb(85, 107, 47)', 'rgb(255, 140, 0)', 'rgb(153, 50, 204)', 'rgb(139, 0, 0)', 'rgb(233, 150, 122)', 'rgb(143, 188, 143)', 'rgb(72, 61, 139)', 'rgb(47, 79, 79)', 'rgb(47, 79, 79)', 'rgb(0, 206, 209)', 'rgb(148, 0, 211)', 'rgb(255, 20, 147)', 'rgb(0, 191, 255)', 'rgb(105, 105, 105)', 'rgb(105, 105, 105)', 'rgb(30, 144, 255)', 'rgb(178, 34, 34)', 'rgb(255, 250, 240)', 'rgb(34, 139, 34)', 'rgb(255, 0, 255)', 'rgb(220, 220, 220)', 'rgb(248, 248, 255)', 'rgb(255, 215, 0)', 'rgb(218, 165, 32)', 'rgb(128, 128, 128)', 'rgb(128, 128, 128)', 'rgb(0, 128, 0)', 'rgb(173, 255, 47)', 'rgb(240, 255, 240)', 'rgb(255, 105, 180)', 'rgb(205, 92, 92)', 'rgb(75, 0, 130)', 'rgb(255, 255, 240)', 'rgb(240, 230, 140)', 'rgb(230, 230, 250)', 'rgb(255, 240, 245)', 'rgb(124, 252, 0)', 'rgb(255, 250, 205)', 'rgb(173, 216, 230)', 'rgb(240, 128, 128)', 'rgb(224, 255, 255)', 'rgb(250, 250, 210)', 'rgb(211, 211, 211)', 'rgb(144, 238, 144)', 'rgb(211, 211, 211)', 'rgb(255, 182, 193)', 'rgb(255, 160, 122)', 'rgb(32, 178, 170)', 'rgb(135, 206, 250)', 'rgb(119, 136, 153)', 'rgb(119, 136, 153)', 'rgb(176, 196, 222)', 'rgb(255, 255, 224)', 'rgb(0, 255, 0)', 'rgb(50, 205, 50)', 'rgb(250, 240, 230)', 'rgb(255, 0, 255)', 'rgb(128, 0, 0)', 'rgb(102, 205, 170)', 'rgb(0, 0, 205)', 'rgb(186, 85, 211)', 'rgb(147, 112, 219)', 'rgb(60, 179, 113)', 'rgb(123, 104, 238)', 'rgb(0, 250, 154)', 'rgb(72, 209, 204)', 'rgb(199, 21, 133)', 'rgb(25, 25, 112)', 'rgb(245, 255, 250)', 'rgb(255, 228, 225)', 'rgb(255, 228, 181)', 'rgb(255, 222, 173)', 'rgb(0, 0, 128)', 'rgb(253, 245, 230)', 'rgb(128, 128, 0)', 'rgb(107, 142, 35)', 'rgb(255, 165, 0)', 'rgb(255, 69, 0)', 'rgb(218, 112, 214)', 'rgb(238, 232, 170)', 'rgb(152, 251, 152)', 'rgb(175, 238, 238)', 'rgb(219, 112, 147)', 'rgb(255, 239, 213)', 'rgb(255, 218, 185)', 'rgb(205, 133, 63)', 'rgb(255, 192, 203)', 'rgb(221, 160, 221)', 'rgb(176, 224, 230)', 'rgb(128, 0, 128)', 'rgb(255, 0, 0)', 'rgb(188, 143, 143)', 'rgb(65, 105, 225)', 'rgb(139, 69, 19)', 'rgb(250, 128, 114)', 'rgb(244, 164, 96)', 'rgb(46, 139, 87)', 'rgb(255, 245, 238)', 'rgb(160, 82, 45)', 'rgb(192, 192, 192)', 'rgb(135, 206, 235)', 'rgb(106, 90, 205)', 'rgb(112, 128, 144)', 'rgb(112, 128, 144)', 'rgb(255, 250, 250)', 'rgb(0, 255, 127)', 'rgb(70, 130, 180)', 'rgb(210, 180, 140)', 'rgb(0, 128, 128)', 'rgb(216, 191, 216)', 'rgb(255, 99, 71)', 'rgb(64, 224, 208)', 'rgb(238, 130, 238)', 'rgb(245, 222, 179)', 'rgb(255, 255, 255)', 'rgb(245, 245, 245)', 'rgb(255, 255, 0)', 'rgb(154, 205, 50)'],

   getHexByName : function( name )
   {
      for ( var i = 0, iLen = this.nameArray.length; iLen > i; i++ )
      {
         var eachItem = this.nameArray[i];
         if ( name == eachItem )
         {
            return this.hexArray[i];
         }
      }
   },

   getRgbByName : function( name )
   {
      for ( var i = 0, iLen = this.nameArray.length; iLen > i; i++ )
      {
         var eachItem = this.nameArray[i];
         if ( name == eachItem )
         {
            return this.rgbArray[i];
         }
      }
   },

   getNameByHex : function( hex )
   {
      for ( var i = 0, iLen = this.hexArray.length; iLen > i; i++ )
      {
         var eachItem = this.hexArray[i];
         if ( hex == eachItem )
         {
            return this.nameArray[i];
         }
      }
   },

   getRgbByHex : function( hex )
   {
      for ( var i = 0, iLen = this.hexArray.length; iLen > i; i++ )
      {
         var eachItem = this.hexArray[i];
         if ( hex == eachItem )
         {
            return this.rgbArray[i];
         }
      }
   },

   getNameByRgb : function( rgb )
   {
      for ( var i = 0, iLen = this.rgbArray.length; iLen > i; i++ )
      {
         var eachItem = this.rgbArray[i];
         if ( rgb == eachItem )
         {
            return this.nameArray[i];
         }
      }
   },

   getHexByRgb : function( rgb )
   {
      for ( var i = 0, iLen = this.rgbArray.length; iLen > i; i++ )
      {
         var eachItem = this.rgbArray[i];
         if ( rgb == eachItem )
         {
            return this.hexArray[i];
         }
      }
   },

   normalizeHex : function( hex )
   {
      hex = hex.toLowerCase();
      if ( 4 == hex.length )
      {
         var hexSplit = hex.split('');
         hex = hexSplit[0] + hexSplit[1] + hexSplit[1] + hexSplit[2] + hexSplit[2] + hexSplit[3] + hexSplit[3];
      }

      return hex;
   },

   normalizeRgb : function( rgb )
   {
      var rgbSplit = this.splitRgb( rgb );
      rgb = 'rgb(' + rgbSplit.join(', ') +')';

      return rgb;
   },

   splitRgb : function( rgb )
   {
      rgb = rgb.replace(' ', '');
      rgb = rgb.replace('rgb(', '');
      rgb = rgb.replace(')', '');
      var rgbSplit = rgb.split(',');

      return rgbSplit;
   }
};

function GetElementByAtt( attName, attValue )
{
   var element = null;
   if ( 'id' == attName )
   {
      element = SVGDocument.getElementById( attValue );
   }

   if ( !element )
   {
      var allElements = SVGDocument.getElementsByTagNameNS(svgns, '*');
      for ( var e = 0, eLen = allElements.length; eLen > e; e++ )
      {
         var eachElement = allElements.item(e);
         var value = eachElement.getAttributeNS(null, attName);
         if ( attValue == value )
         {
            element = eachElement;
            break;
         }
      }
   }

   return element;
}
