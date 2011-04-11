/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

Components.utils.import("resource://app/jsmodules/URLUtils.jsm");

function runTest() {

  // xxx slloyd Many of these tests assume the order of enumeration for object
  //            properties in params objects. The Javascript standard does not
  //            guarantee this, but in practice our JS engine enumerates in the
  //            order that properties are added, so the assumption holds.

  var test_cases = {

    'newURI': function testNewURI(aName) {

      function assertURI(aSpec, aMessage) {
        let url = URLUtils.newURI(aSpec).QueryInterface(Ci.nsIURL);
        assertEqual(url.spec, aSpec, [aName, aMessage].join(' '));
      }

      assertURI('http://test.com/',
                'should create a new URI properly');

      assertURI('http://test.com/?a=1&b=2&c=3',
                'should handle query parameters');

    },

    'addQuery': function testAddQuery(aName) {

      function assertAdded(aURL, aParams, aExpected, aMessage) {
        let url_spec = URLUtils.addQuery(aURL, aParams);
        assertEqual(url_spec, aExpected, [aName, aMessage].join(' '));
      }

      // The expected url spec has a forward slash before the question mark due
      // to the way that nsIStandardURL.init creates urls.
      assertAdded('http://test.com',
                  { a: 1, b: 2, c: 3 },
                  'http://test.com/?a=1&b=2&c=3',
                  'should add query params to an url spec');

      assertAdded('http://test.com?d=4',
                  { a: 1, b: 2, c: 3 },
                  'http://test.com/?d=4&a=1&b=2&c=3',
                  'should add query params to an url with existing params');

      assertAdded('http://test.com/?a=has%20space',
                  { b: 'has&amp', c: 'has=eq', d: 'http://test.com' },
                  'http://test.com/?a=has%20space&b=has%26amp&c=has%3Deq&d=http%3A%2F%2Ftest.com',
                  'should handle URI-encoding properly');

      assertAdded('http://test.com/',
                  {},
                  'http://test.com/',
                  'should handle an empty params argument');

      assertAdded('http://test.com/',
                  null,
                  'http://test.com/',
                  'should handle a null params argument');

    },

    'produceQuery': function testProduceQuery(aName) {

      function assertProduced(aParams, aExpected, aMessage) {
        let query = URLUtils.produceQuery(aParams);
        assertEqual(query, aExpected, [aName, aMessage].join(' '));
      }

      assertProduced({ a: 1, b: 2, c: 3 },
                     'a=1&b=2&c=3',
                     'should produce a query string from an object');

      assertProduced({
                       a: 'has space',
                       b: 'has&amp',
                       c: 'has=eq',
                       d: 'http://test.com'
                     },
                     'a=has%20space&b=has%26amp&c=has%3Deq&d=http%3A%2F%2Ftest.com',
                     'should URI encode values');

      assertProduced({},
                     '',
                     'should return an empty string when passed empty params');

      assertProduced(null,
                     '',
                     'should return an empty string when passed null params');

    },

    'extractQuery': function testExtractQuery(aName) {

      // simple one-level deep object equality check for params objects
      function assertParamsEqual(aParams, aExpected, aMessage) {

        // We don't have JS 1.8.5 yet, so we don't get Object.keys
        function keysForObj(obj) {
          var key_array = [];

          for (let key in obj) {
            if (obj.hasOwnProperty(key)) {
              key_array.push(key);
            }
          }
          return key_array;
        }

        var param_keys = keysForObj(aParams),
            expected_keys = keysForObj(aExpected);

        // First make sure the keys are the same
        assertSetsEqual(param_keys, expected_keys);

        // Now compare the values
        for (let key in aParams) {
          assertEqual(aParams[key], aExpected[key], aMessage);
        }
      }

      // check an expected params object given an url spec
      function assertExtracted(aURLSpec, aExpected, aMessage) {
        let params = {};
        let rv = URLUtils.extractQuery(aURLSpec, params);
        assertEqual(rv, aExpected.rv, [aName, aMessage.rv].join(' '));
        assertParamsEqual(params,
                          aExpected.params,
                          [aName, aMessage.params].join(' '));
      }

      var base_message;

      base_message = 'for an url with a query string';
      assertExtracted('http://test.com?a=1&b=2&c=3',
                      {
                        rv: true,
                        params: { a: '1', b: '2', c: '3' }
                      },
                      {
                        rv: 'should return true ' + base_message,
                        params: 'should modify the passed params object ' +
                                base_message
                      });

      base_message = 'for URI-encoded params';
      assertExtracted('http://test.com?a=has%20space&b=has%26amp&c=has%3Deq&d=http%3A%2F%2Ftest.com',
                      {
                        rv: true,
                        params: {
                          a: 'has space',
                          b: 'has&amp',
                          c: 'has=eq',
                          d: 'http://test.com'
                        }
                      },
                      {
                        rv: 'should return true ' + base_message,
                        params: 'should URI-decode params ' + base_message
                      });

      base_message = 'for an url with no query string';
      assertExtracted('http://test.com',
                      {
                        rv: false,
                        params: {},
                      },
                      {
                        rv: 'should return false ' + base_message,
                        params: 'should not modify the params ' + base_message
                      });

    },

    'convertURLToDisplayName': function testConvertUrlToDisplayName(aName) {
      // xxx slloyd This method doesn't indicate anything about the expected
      //            inputs or outputs, so I'm punting on testing it now.
    }

  };

  for (let test_case in test_cases) {
    test_cases[test_case](test_case);
  }

}

