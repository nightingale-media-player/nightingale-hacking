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

//
// sbIAsyncForLoop wrapper
//

try
{
// Global setup
var sbIAsyncForLoopCount = 0;
var sbIAsyncForLoopArray = {};

function sbIAsyncForLoop( aInitEval, aWhileEval, aStepEval, aBodyEval, aFinishedEval, aStepsPerInterval, aIntervalDelay, aIndex )
{
  if ( ( aStepsPerInterval == null ) || ( aStepsPerInterval == 0 ) )
  {
    aStepsPerInterval = 1;
  }
  if ( aIntervalDelay == null )
  {
    aIntervalDelay = 0;
  }
  try
  {
    // Assign ourselves to the global space so we can have a proper "this" pointer.
    if (!aIndex)
      aIndex = sbIAsyncForLoopCount++; // Anonymous
    this.m_Index = aIndex;
    if (sbIAsyncForLoopArray[ this.m_Index ])
      sbIAsyncForLoopArray[ this.m_Index ].cancel();
    sbIAsyncForLoopArray[ this.m_Index ] = this;

    // Set the eval strings variables
    this.m_InitEval      = aInitEval;
    this.m_WhileEval     = aWhileEval;
    this.m_StepEval      = aStepEval;
    this.m_BodyEval      = aBodyEval;
    this.m_FinishedEval  = aFinishedEval;
    this.m_StepsPer      = aStepsPerInterval;
    this.m_Delay         = aIntervalDelay;
    this.m_Interval      = null;
    this.m_AsyncState    = 0;

    this.pushAsync = function()
    {
      this.m_AsyncState++;
    }

    this.popAsync = function()
    {
      if ( --this.m_AsyncState < 0 )
        throw "UNBALANCED ASYNC HANDLER";
    }

    this.cancel = function()
    {
      clearInterval( this.m_Interval );
      this.m_Interval = null;
      sbIAsyncForLoopArray[ this.m_Index ] = null; // Release things when we're done.
    }

    this.step = function( index )
    {
      if ( this.m_Interval )
      {
        for ( var StepsPerCount = 0; StepsPerCount < this.m_StepsPer; StepsPerCount++ )
        {
          var pop = false;
          var step = false;
          try
          {
            // either run a function or code to see if we are done processing
            if ( typeof( this.m_WhileEval ) == 'function' )
              step = this.m_WhileEval();
            else
              step = eval( this.m_WhileEval );
          }
          catch ( err )
          {
            Components.utils.reportError(err);
            alert( "sbIAsyncForLoop::eval( this.m_WhileEval )\r\n" + this.m_WhileEval + "\r\n" + err );
          }
          // step will be false if we have processed everything.
          if ( step )
          {
            try
            {
              // pop will be true if we found media, false otherwise
              if ( typeof( this.m_BodyEval ) == 'function' )
                pop = this.m_BodyEval() == true;
              else
                pop = eval( this.m_BodyEval ) == true;
            }
            catch ( err )
            {
              Components.utils.reportError(err);
              alert( "sbIAsyncForLoop::eval( this.m_BodyEval )\r\n" + this.m_BodyEval + "\r\n" + err );
            }
            try
            {
              if ( typeof( this.m_StepEval ) == 'function' )
                this.m_StepEval();
              else
                eval( this.m_StepEval );
            }
            catch ( err )
            {
              Components.utils.reportError(err);
              alert( "sbIAsyncForLoop::eval( this.m_StepEval )\r\n" + this.m_StepEval + "\r\n" + err );
            }
            // Optionally pop out of the m_StepsPer loop.
            if ( pop )
            {
              break;
            }
          }
          else if ( this.m_AsyncState == 0 )
          {
            try
            {
              if ( typeof( this.m_FinishedEval ) == 'function' )
                this.m_FinishedEval();
              else
                eval( this.m_FinishedEval );
            }
            catch ( err )
            {
              Components.utils.reportError(err);
              alert( "sbIAsyncForLoop::eval( this.m_FinishedEval )\r\n" + this.m_FinishedEval + "\r\n" + err );
            }
            // Now we're done.  Shut off the Interval.
            this.cancel();
            break;
          }
        }
      }
    }

    // And start things off
    try
    {
      if ( typeof( this.m_InitEval ) == 'function' )
        this.m_InitEval();
      else
        eval( this.m_InitEval );
    }
    catch ( err )
    {
      Components.utils.reportError(err);
      alert( "sbIAsyncForLoop::eval( this.m_InitEval )\r\n" + this.m_InitEval + "\r\n" + err );
    }
    this.m_Interval = setInterval( "sbIAsyncForLoopArray[ " + this.m_Index + " ].step()", this.m_Delay );
  }
  catch ( err )
  {
    Components.utils.reportError(err);
    alert( "sbIAsyncForLoop - construct\r\n" + err );
  }
}

/*

// Sample Code

function f()
{
  this.i = 0;
}
var loop = new sbIAsyncForLoop
(
  // aInitEval, aWhileEval, aStepEval, aBodyEval, aFinishedEval
  f, // "this.i = 0",
  "this.i <= 10",
  "this.i++",
  function()
  {
    alert( this.i );
  },
  "alert( this.text );"
);
// loop.i = 0;
loop.text = "'Finished!'"

*/

}
catch ( err )
{
  alert( "sbIAsyncForLoop - load\r\n" + err );
}
