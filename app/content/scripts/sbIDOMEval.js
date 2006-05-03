/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

//
//  sbIDOMEval 
//
//  This object scans a DOM tree for attributes containing references to
//  pre-registered variables and later changes the value of all these attributes 
//  according to changes in the values of the variables.
//  ---------------------------------------------------------------------------

function sbIDOMEval( domNode )
{
  try
  {
    // Init
    
    this.m_AutoUpdate = false;
    this.m_NeedUpdate = false;
    this.m_NeedScan = true;
    this.m_RootNode = domNode;
    this.m_Nodes = null;     // Array: [0] = element node, [1] [attribute name, attribute value]
    this.m_Variables = null; // Array: [0] = var name, [1] = var last value

    //
    // Methods
    //

    //  ---------------------------------------------------------------------------
    //  SetValue - Sets the value of a variable, if AutoUpdate is true, the DOM tree is 
    //             updated with the current value of all variables. (see EnableUpdate)
    //  ---------------------------------------------------------------------------
    this.SetValue = function( variable, value )
    {
      try
      {
        var _var = this.GetVariable(variable);
        if (_var == null)
        {
          this.m_NeedScan = true;
          _var = Array(variable, value);
          if (this.m_Variables == null) this.m_Variables = Array();
          this.m_Variables.push(_var);
        }
        else
        {
          _var[1] = value;
        }
        
        // a variable changed, DOM needs updating
        this.m_NeedUpdate = true;
        
        // if executed, this resets m_NeedUpdate to false
        if (this.m_AutoUpdate) this.DoUpdate(); 
      }
      catch ( err )
      {
        alert("SetValue");
        alert( err );
      }
    };

    //  ---------------------------------------------------------------------------
    //  GetValue - Get the value of a variable as it was last set
    //  ---------------------------------------------------------------------------
    this.GetValue = function( variable )
    {
      var retval = null;
      try
      {
        var thevar = this.GetVariable(variable);
        if (thevar != null) retval = thevar[1];
      }
      catch ( err )
      {
        alert("GetValue");
        alert( err );
      }
      return retval;
    };

    //  ---------------------------------------------------------------------------
    //  GetVariable - Returns a variable entry
    //  ---------------------------------------------------------------------------
    this.GetVariable = function( variable )
    {
      var retval = null;
      try
      {
        if (this.m_Variables != null)
        {
          for (var i=0; i < this.m_Variables.length; i++)
          {
            if (this.m_Variables[i][0] == variable)
            {
              retval = this.m_Variables[i];
              break;
            }
          }
        }
      }
      catch ( err )
      {
        alert("GetVariable");
        alert( err );
      }
      return retval;
    };

    //  ---------------------------------------------------------------------------
    //  EnableUpdate - Enable/disable automatic DOM update on SetValue
    //  ---------------------------------------------------------------------------
    this.EnableUpdate = function( tf )
    {
      try
      {
        this.m_AutoUpdate = tf;
        if (this.m_AutoUpdate && this.m_NeedUpdate) 
        {
          this.DoUpdate();
        }
      }
      catch ( err )
      {
        alert("EnableUpdate");
        alert( err );
      }
    };

    //  ---------------------------------------------------------------------------
    //  DoUpdate - Update the DOM tree with all the variables values
    //  ---------------------------------------------------------------------------
    this.DoUpdate = function( )
    {
      try
      {
        if (this.m_NeedScan) this.m_NeedUpdate = this.DoScan(this.m_RootNode);
        for (var i = 0; i < this.m_Nodes.length; i++)
        {
          var node =  this.m_Nodes[i][0];
          var attribute = this.m_Nodes[i][1][0];
          var value = this.m_Nodes[i][1][1];
          for (var varnum = 0; varnum < this.m_Variables.length; varnum++)
          {
            var variable = this.m_Variables[i];
            var from = '@'+variable[0]+'@';
            var to = variable[1];
            var p;
            while (1)
            {
              p = this.FindVariable(variable, value);
              if (p < 0) break;
              
              var nv = "";
              if (p > 0) nv += value.substr(0, p); 
              nv += to;
              if (p < value.length-from.length) nv += value.substr(p + from.length);
              value = nv;
            }
          }
          node.setAttribute(attribute, eval(value));
        }
      }
      catch ( err )
      {
        alert("DoUpdate");
        alert( err );
      }
    };

    //  ---------------------------------------------------------------------------
    //  DoScan - Scans the DOM for tree with all the variables values
    //  ---------------------------------------------------------------------------
    this.DoScan = function( rootnode )
    {
      if (this.m_RootNode != rootnode) this.m_NeedScan = true;
      this.m_RootNode = rootnode;
      var ret = 0;
      try
      {
        if (this.m_Variables == null)
        {
          ret = 1;
        } 
        else
        {
          // reset node table
          this.m_Nodes = Array();
          
          // scan the tree, remember each reference to a variable
          this.ScanNode(this.m_RootNode);

          // no need to rescan
          this.m_NeedScan = false;
        }
      }
      catch ( err )
      {
        alert("DoScan");
        alert( err );
        ret = 1;
      }
      return ret;
    };

    //  ---------------------------------------------------------------------------
    //  ScanNode - Scans a node in the DOM (recursive)
    //  ---------------------------------------------------------------------------
    this.ScanNode = function( node )
    {
      try
      {
        this.ScanAttributes(node);
        if (node.childNodes != null)
        {
          for (var i = 0; i < node.childNodes.length; i++)
          {
            this.ScanNode(node.childNodes[i]);
          }
        }
      }
      catch ( err )
      {
        alert("ScanNode");
        alert( err );
      }
    };

    //  ---------------------------------------------------------------------------
    //  ScanAttributes - Scans a node for references to any of our variable in any 
    //                   of its attributes
    //  ---------------------------------------------------------------------------
    this.ScanAttributes = function( node )
    {
      try
      {
        if (node.attributes != null)
        {
          for (var i = 0; i < node.attributes.length; i++)
          {
            this.ScanVariables(node, node.attributes[i]);
          }
        }
      }
      catch ( err )
      {
        alert( err );
      }
    };

    //  ---------------------------------------------------------------------------
    //  ScanVariables - Scans an attribute for references to any of our variable
    //  ---------------------------------------------------------------------------
    this.ScanVariables = function( node, attribute )
    {
      try
      {
        //attribute.nodeName, attribute.nodeValue
        for (var i = 0; i < this.m_Variables.length; i++)
        {
          if (this.FindVariable(this.m_Variables[i], attribute.nodeValue) >= 0) this.m_Nodes.push(Array(node, Array(attribute.nodeName, attribute.nodeValue)));
        }
      }
      catch ( err )
      {
        alert("ScanVariables");
        alert( err );
      }
    };


    //  ---------------------------------------------------------------------------
    //  FindVariable - Scans an attribute for references to one variable
    //  ---------------------------------------------------------------------------
    this.FindVariable = function( variable, text )
    {
      var retval = -1;
      try
      {
        retval = text.indexOf("@"+variable[0]+"@");
      }
      catch ( err )
      {
        alert("FindVariable");
        alert( err );
      }
      return retval;
    };

    this.Report = function( )
    {
      alert(this.m_Variables + '|' + this.m_Nodes);
    }

  }
  catch ( err )
  {
    alert("Global");
    alert( err );
  }

  return this;
}

