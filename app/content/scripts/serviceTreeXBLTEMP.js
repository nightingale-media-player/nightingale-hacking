// JScript source code
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \file serviceTreeXBLTEMP.js
 * \brief This is a temporary file to house methods that need to roll into
 * our Servicetree XBL object when it is cleaned up for 0.3
 * \deprecated This file will be removed.
 */

function getItemByUrl( tree, url )
{
  var retval = null;

  for ( var i = 0; i < tree.view.rowCount; i++ )
  {
    var cell_url =  tree.view.getCellText( i, tree.columns["url"] );
    
    if ( cell_url == url )
    {
      retval = tree.contentView.getItemAtIndex( i );
      break;
    }
  }
  
  return retval;
}

function SBGetUrlFromService( service )
{
  retval = service;
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";
        
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_url = theServiceTree_tree.view.getCellText( i, urlcolumn );
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          if ( tree_label == service )
          {
            retval = tree_url;
            break;
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBGetUrlFromService - " + err )
  }
  return retval;
}

function SBTabcompleteService( service )
{
  retval = service;
  var service_lc = service.toLowerCase();
  try
  {
    var theServiceTree = document.getElementById( "frame_servicetree" );
    if ( theServiceTree )
    {
      var theServiceTree_tree = theServiceTree.tree;
      if (theServiceTree_tree)
      {
        // Find the columns. 
        var urlcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["url"] : "url";
        var labelcolumn = theServiceTree_tree.columns ? theServiceTree_tree.columns["frame_service_tree_label"] : "frame_service_tree_label";

        var found_one = false;      
        for ( var i = 0; i < theServiceTree_tree.view.rowCount; i++ )
        {
          // Get the text of the hidden tree cell, this contains the url.
          var tree_label = theServiceTree_tree.view.getCellText( i, labelcolumn );
          
          var label_lc = tree_label.toLowerCase();
          
          // If we are the beginning of the label string
          if ( label_lc.indexOf( service_lc ) == 0 )
          {
            if ( found_one )
            {
              retval = service; // only find ONE!
              break;
            }
            else
            {
              found_one = true; // only find ONE!
              retval = tree_label;
            }
          }
        }
      }
    }
  }
  catch ( err )
  {
    alert( "SBTabcompleteService - " + err )
  }
  return retval;
}


