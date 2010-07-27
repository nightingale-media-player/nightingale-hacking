/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

  assertEqual(textInfo.id, "");
  assertEqual(textInfo.type, "text");
  
  //Should fail.
  try {
    textInfo.type = "mytype";
  }
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_ALREADY_INITIALIZED);
  }
  
  textInfo.id = "TextInfo";
  assertEqual(textInfo.id, "TextInfo");
  
  //Should fail.
  try {
    textInfo.id = "MyTextInfo";
  } 
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_ALREADY_INITIALIZED);
  }
  
  textInfo.localizationKey = "My localization key";
  assertEqual(textInfo.localizationKey, "My localization key");
  
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
  
  numberInfo.id = "NumberInfo";
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
  assertEqual(numberInfo.makeSearchable(sample), "+0000000000000000001");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.RADIX_16;
  sample = "0x00aad";
  assertEqual(numberInfo.validate(sample), true);
  assertEqual(numberInfo.format(sample), "0xAAD");
  assertEqual(numberInfo.makeSearchable(sample), "0000000000000AAD");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.RADIX_8;
  sample = "0644";
  assertEqual(numberInfo.validate(sample), true);
  assertEqual(numberInfo.format(sample), "0644");
  assertEqual(numberInfo.makeSearchable(sample), "0000000000000000000644");
  
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

function testNumberInfoFloatingPoint() {
var numberInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
                    .createInstance(Ci.sbINumberPropertyInfo);
  
  numberInfo.id = "FloatingInfo";
  assertEqual(numberInfo.type, "number");
  
  numberInfo.radix = Ci.sbINumberPropertyInfo.FLOAT;
  
  var sample = "123 not a number";
  assertEqual(numberInfo.validate(sample), false);
  try {
    numberInfo.format(sample);
  }
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_INVALID_ARG);
  }
  
  /* The replace(",", ".") in the following lines prevents
   * the test from failing if the machine has set the comma
   * as decimal separator. So in the following test
   * numberInfo.format(sample) will return "20,99" on such
   * a machine. With the replacement it changes to "20.99"
   * and is valid again. To make this visible this is also
   * used in all log-calls.
   */

  sample = "20.99";
  assertEqual(numberInfo.validate(sample), true);

  var formatted_sample = numberInfo.format(sample).replace(",", ".");
  var searchable_sample = numberInfo.makeSearchable(sample).replace(",", ".");
  log(formatted_sample);
  log(searchable_sample);
  
  assertEqual(formatted_sample, "20.99");

  var eps = 1e-10; // nearly zero, because floating points don't compare well.

  var delta = Math.abs(searchable_sample - sample);
  assertTrue(delta < eps, "make sortable doesn't perturb the value");

  sample = "0.99";
  assertEqual(numberInfo.validate(sample), true);
  
  formatted_sample = numberInfo.format(sample).replace(",", ".");
  searchable_sample = numberInfo.makeSearchable(sample).replace(",", ".");
  log(formatted_sample);
  log(searchable_sample);
  
  assertEqual(formatted_sample, "0.99");
  var delta = Math.abs(searchable_sample - sample);
  assertTrue(delta < eps, "make sortable doesn't perturb the value");

  sample = "12347120349029834.1234341235";
  assertEqual(numberInfo.validate(sample), true);

  formatted_sample = numberInfo.format(sample).replace(",", ".");
  searchable_sample = numberInfo.makeSearchable(sample).replace(",", ".");
  log(formatted_sample);
  log(searchable_sample);
  
  assertEqual(parseFloat(formatted_sample), 1.23471e+016);
  var delta = Math.abs(searchable_sample - sample);
  assertTrue(delta < eps, true, "make sortable doesn't perturb the value");
}

function testUriInfo() {
  var uriInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/URI;1"]
                  .createInstance(Ci.sbIURIPropertyInfo);
  
  uriInfo.id = "URIInfo";
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
                      
  datetimeInfo.id = "DatetimeInfo";
  assertEqual(datetimeInfo.type, "datetime");
  
  var sample = "0";
  datetimeInfo.timeType = Ci.sbIDatetimePropertyInfo.TIMETYPE_DATETIME;
  
  assertEqual(datetimeInfo.validate(sample), true);
  log(datetimeInfo.format(sample));
  
  sample = "12431235123412499";
  assertEqual(datetimeInfo.validate(sample), true);
  log(datetimeInfo.format(sample));
}

function testDurationInfo() {
  var durationInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Duration;1"]
                      .createInstance(Ci.sbIDurationPropertyInfo);
                      
  durationInfo.id = "DurationInfo";
  assertEqual(durationInfo.type, "duration");
  
  var sample = "0";
  
  assertEqual(durationInfo.validate(sample), true);
  log(durationInfo.format(sample));
  
  sample = "12431235123412499";
  assertEqual(durationInfo.validate(sample), true);
  log(durationInfo.format(sample));
  
  durationInfo.durationWithMilliseconds = true;
  log(durationInfo.format(sample));
}

function testBooleanInfo() {
  var booleanInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Boolean;1"]
                    .createInstance(Ci.sbIBooleanPropertyInfo);
  
  booleanInfo.id = "BooleanInfo";
  assertEqual(booleanInfo.type, "boolean");
  
  var sample = "not a boolean";
  assertEqual(booleanInfo.validate(sample), false);
  try {
    booleanInfo.format(sample);
  }
  catch(err) {
    assertEqual(err.result, Cr.NS_ERROR_INVALID_ARG);
  }
  
  sample = "0";
  assertEqual(booleanInfo.validate(sample), true);
  assertEqual(booleanInfo.format(sample), "");
  assertEqual(booleanInfo.makeSearchable(sample), "0");
  
  sample = "1";
  assertEqual(booleanInfo.validate(sample), true);
  assertEqual(booleanInfo.format(sample), "");
  assertEqual(booleanInfo.makeSearchable(sample), "1");
  
  sample = "";
  assertEqual(booleanInfo.validate(sample), true);
  assertEqual(booleanInfo.format(sample), "");
  assertEqual(booleanInfo.makeSearchable(sample), "");
  
  var tvpi = booleanInfo.QueryInterface(Ci.sbITreeViewPropertyInfo);
  assertEqual(tvpi.getCellValue(null), "0");
  assertEqual(tvpi.getCellValue(""),   "0");
  assertEqual(tvpi.getCellValue("0"),  "0");
  assertEqual(tvpi.getCellValue("1"),  "1");

  assertEqual(tvpi.getCellProperties(null), "checkbox unchecked");
  assertEqual(tvpi.getCellProperties(""),   "checkbox unchecked");
  assertEqual(tvpi.getCellProperties("0"),  "checkbox unchecked");
  assertEqual(tvpi.getCellProperties("1"),  "checkbox checked");
}

function runTest () {

  log("Testing TextPropertyInfo...");
  testTextInfo();
  log("OK");
  
  log("Testing NumberPropertyInfo...");
  testNumberInfo();  
  testNumberInfoFloatingPoint();
  log("OK");
 
  log("Testing URIPropertyInfo...");
  testUriInfo();
  log("OK");

  log("Testing DatetimePropertyInfo...");
  testDatetimeInfo();
  log("OK");

  log("Testing DurationPropertyInfo...");
  testDurationInfo();
  log("OK");

  log("Testing BooleanPropertyInfo...");
  testBooleanInfo();
  log("OK");
}
