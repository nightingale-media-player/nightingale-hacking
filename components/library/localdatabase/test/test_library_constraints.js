/* vim: set sw=2 et : */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \brief Test for library constraints unserialization
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

function runTest () {
  var constraint;
  var builder = Cc["@getnightingale.com/Nightingale/Library/ConstraintBuilder;1"]
                  .createInstance(Ci.sbILibraryConstraintBuilder);

  builder.includeConstraint(LibraryUtils.standardFilterConstraint);
  assertTrue(LibraryUtils.standardFilterConstraint.equals(builder.get()),
             "constraint builder failed to include standard constraint");

  builder.includeConstraint(LibraryUtils.standardFilterConstraint);
  builder.intersect();
  builder.include(SBProperties.contentType, "audio");
  constraint = builder.get();
  builder.parseFromString(String(constraint));
  assertTrue(constraint.equals(builder.get()),
             "constraint builder failed to correctly unserialize");

  builder.includeConstraint(LibraryUtils.standardFilterConstraint);
  builder.intersect();
  builder.include(SBProperties.genre, "\u6E2C \u8A66");
  constraint = builder.get();
  builder.parseFromString(String(constraint));
  assertTrue(constraint.equals(builder.get()),
             "constraint builder failed to correctly unserialize non-ASCII");

  // test that we're actually building the right constraints
  builder.includeConstraint(LibraryUtils.standardFilterConstraint);
  builder.intersect();
  builder.include(SBProperties.contentType, "audio");
  builder.intersect();
  builder.includeList(SBProperties.trackName,
                      ArrayConverter.stringEnumerator(["alpha", "beta"]));
  constraint = builder.get();
  var expected = new Object();
  expected[SBProperties.isList] = ["0"];
  expected[SBProperties.hidden] = ["0"];
  expected[SBProperties.contentType] = ["audio"];
  expected[SBProperties.trackName] = ["alpha", "beta"];
  for (let group in ArrayConverter.JSEnum(constraint.groups)) {
    group.QueryInterface(Ci.sbILibraryConstraintGroup);
    let props = ArrayConverter.JSArray(group.properties);
    assertEqual(1, props.length, "unexpected property count");
    assertTrue(props[0] in expected, "unknown property " + props[0]);
    let values = ArrayConverter.JSArray(group.getValues(props[0]));
    assertArraysEqual(values, expected[props[0]]);
    delete expected[props[0]];
  }
  for (let prop in expected) {
    doFail("unexpected remaining property " + prop);
  }
}

