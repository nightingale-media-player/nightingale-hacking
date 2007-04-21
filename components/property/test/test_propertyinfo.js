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

/**
 * \brief Test file
 */

function testTextInfo() {
  var textInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Text;1"]
                  .createInstance(Ci.sbITextPropertyInfo);

  assertEqual(textInfo.name, "");
  assertEqual(textInfo.type, "text");
  
  //Should fail.
  try {
    textInfo.type = "mytype";
  }
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_ALREADY_INITIALIZED);
  }
  
  textInfo.name = "TextInfo";
  assertEqual(textInfo.name, "TextInfo");
  
  //Should fail.
  try {
    textInfo.name = "MyTextInfo";
  } 
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_ALREADY_INITIALIZED);
  }
  
  textInfo.displayName = "My Display Name";
  assertEqual(textInfo.displayName, "My Display Name");
  
  var sample = "some text";
  assertEqual(textInfo.validate(sample), true);
  assertEqual(textInfo.format(sample), "some text");
  
  textInfo.minLength = 10;
  sample = "too short";
  assertEqual(textInfo.validate(sample), false);
  
  textInfo.maxLength = 10;
  sample = "way way way too long";
  assertEqual(textInfo.validate(sample), false);  
  
  sample = "just right";
  assertEqual(textInfo.validate(sample), true);
  
  textInfo.enforceLowercase = true;
  sample = "JUST RIGHT";
  assertEqual(textInfo.validate(sample), true);
  assertEqual(textInfo.format(sample), "just right");

  //before validating we always compress the whitespace because
  //format always does so.
  sample = "       whitespace";
  assertEqual(textInfo.validate(sample), false);
  
  var formatted = textInfo.format(sample);
  assertEqual(formatted, "whitespace");
  assertEqual(textInfo.validate(formatted), true);
   
  return;
}

function testNumberInfo() {
  var numberInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
                    .createInstance(Ci.sbINumberPropertyInfo);
  
  numberInfo.name = "NumberInfo";
  assertEqual(numberInfo.type, "number");
  
  var sample = "not a number";
  assertEqual(numberInfo.validate(sample), false);
  try {
    numberInfo.format(sample);
  }
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_INVALID_ARG);
  }
  
  sample = "1";
  assertEqual(numberInfo.validate(sample), true);
  assertEqual(numberInfo.format(sample), "1");
  assertEqual(numberInfo.makeSortable(sample), "+0000000000000000001");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.RADIX_16;
  sample = "0x00aad";
  assertEqual(numberInfo.validate(sample), true);
  assertEqual(numberInfo.format(sample), "0xAAD");
  assertEqual(numberInfo.makeSortable(sample), "0000000000000AAD");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.RADIX_8;
  sample = "0644";
  assertEqual(numberInfo.validate(sample), true);
  assertEqual(numberInfo.format(sample), "0644");
  assertEqual(numberInfo.makeSortable(sample), "0000000000000000000644");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.RADIX_10;
  sample = "1000";
  numberInfo.minValue = 1001;
  assertEqual(numberInfo.validate(sample), false);
  try {
    numberInfo.format(sample)
  } 
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_INVALID_ARG);  
  }
  
  sample = "1002";
  numberInfo.maxValue = 1003;
  assertEqual(numberInfo.validate(sample), true);
  
  sample = "1001";
  assertEqual(numberInfo.validate(sample), true);
  
  sample = "1003";
  assertEqual(numberInfo.validate(sample), true);
  
  sample = "1004";
  assertEqual(numberInfo.validate(sample), false);
  
}

function testUriInfo() {
  var uriInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/URI;1"]
                  .createInstance(Ci.sbIURIPropertyInfo);
  
  uriInfo.name = "URIInfo";
  assertEqual(uriInfo.type, "uri");
  
  var sample = "totally not a valid url";
  assertEqual(uriInfo.validate(sample), false);
  
  sample = "choohooo://vaguely valid/";
  assertEqual(uriInfo.validate(sample), true);

  sample = "http://songbirdnest.com/aus/blog";
  assertEqual(uriInfo.validate(sample), true);
  
  sample = "file:///Users/Me/Music";
  uriInfo.constrainScheme = "http";
  assertEqual(uriInfo.validate(sample), false);
  
  sample = "http://shoot.the.tv";
  assertEqual(uriInfo.validate(sample), true);
  
  sample = "HTTp://PoorLy.FormaTTEd.URL.com/ArRrRg";
  assertEqual(uriInfo.format(sample), "http://poorly.formatted.url.com/ArRrRg");
}

function testDatetimeInfo() {
  var datetimeInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Datetime;1"]
                      .createInstance(Ci.sbIDatetimePropertyInfo);
                      
  datetimeInfo.name = "DatetimeInfo";
  assertEqual(datetimeInfo.type, "datetime");
  
  var sample = "0";
  datetimeInfo.timeType = Ci.sbIDatetimePropertyInfo.TIMETYPE_DATETIME;
  
  assertEqual(datetimeInfo.validate(sample), true);
  log(datetimeInfo.format(sample));
  
  sample = "12431235123412499";
  assertEqual(datetimeInfo.validate(sample), true);
  log(datetimeInfo.format(sample));
  
  datetimeInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Datetime;1"]
                      .createInstance(Ci.sbIDatetimePropertyInfo);
  
  datetimeInfo.name = "DatetimeInfoDuration";
  datetimeInfo.timeType = Ci.sbIDatetimePropertyInfo.TIMETYPE_DURATION;
  sample = "12431235123412499";
  log(datetimeInfo.format(sample));
  
  datetimeInfo.durationWithMilliseconds = true;
  log(datetimeInfo.format(sample));
}

function runTest () {

  log("Testing TextPropertyInfo...");
  testTextInfo();
  log("OK");
  
  log("Testing NumberPropertyInfo...");
  testNumberInfo();  
  log("OK");
  
  log("Testing URIPropertyInfo...");
  testUriInfo();
  log("OK");

  log("Testing DatetimePropertyInfo...");
  testDatetimeInfo();
  log("OK");
}
