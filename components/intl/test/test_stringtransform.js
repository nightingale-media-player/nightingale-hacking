/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

function testTransform(aTestIn, aTestExpectedOut, aFlag) {
  var stringTransform = Cc["@getnightingale.com/Nightingale/Intl/StringTransform;1"]
                          .createInstance(Ci.sbIStringTransform);
  var testOut = stringTransform.normalizeString("", 
                                  aFlag,
                                  aTestIn);
  log("Pre-normalized string: '" + aTestIn + "'");
  log("Expected normalized string: '" + aTestExpectedOut + "'");
  log("Normalized string: '" + testOut + "'");
  assertEqual(testOut, aTestExpectedOut);
}

function runTest() {
                          
  testTransform("‡‰‚ÈÁÓÔÎ l'ÈtÈ est gÈnial", "aaaeciie l'ete est genial", 
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE);

  testTransform("DP-6", "DP-6", 
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE);

  // Sadly, the implementation of IGNORE SYMBOLS on Windows is not consistent
  // with Linux and Mac OS X :(	

  testTransform("I have $5", "I have 5",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_SYMBOLS);

  testTransform("$5 ! I have $5 !!", "5 ! I have $5 !!",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_SYMBOLS |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform("$5 ! I have $5 !!", "5  I have 5 ",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM);

  testTransform("$5 ! I have $5 !!", "5 ! I have $5 !!",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" $5 ! I have $5 !!", " 5  I have 5 ",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM);

  testTransform(" $5 ! I have $5 !!", " $5 ! I have $5 !!",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);
                
  testTransform(" $5 ! I have $5 !!", "5Ihave5",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE);

  testTransform(" $5 ! I have $5 !!", "5 ! I have $5 !!",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" +250 -25 Hello", "250 -25 Hello",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);
  

  testTransform(" +250 -25 Hello", "+250 -25 Hello",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" -45.3 Hi", "-45.3 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" -45.3 Hi", "45.3 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" -.025 Hi", "-.025 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" -.025 Hi", "025 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" .999 Hi", ".999 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" .999 Hi", "999 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" 1e10 Hi", "1e10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" 1e10 Hi", "1e10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" 1e-10 Hi", "1e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" 1e-10 Hi", "1e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" -1e10 Hi", "-1e10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" -1e10 Hi", "1e10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" +1e-10 Hi", "+1e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" +1e-10 Hi", "1e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform(" +.21e-10 Hi", "+.21e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform(" +.21e-10 Hi", "21e-10 Hi",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_LEADING);

  testTransform("I have ‡‰‚ÈÁÓÔÎ $+5! How about that?! heh!", "I have aaaeciie +5 How about that heh",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  testTransform("I have ‡‰‚ÈÁÓÔÎ $+5! How about that?! heh!", "Ihaveaaaeciie+5Howaboutthatheh",
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONSPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_NONALPHANUM_IGNORE_SPACE |
                Ci.sbIStringTransform.TRANSFORM_IGNORE_KEEPNUMBERSYMBOLS);

  return;
}
