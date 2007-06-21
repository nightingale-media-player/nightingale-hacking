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

#include "sbRemoteCommands.h"
#include "sbSecurityMixin.h"
#include <sbClassInfoUtils.h>

#include <nsComponentManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIDOMElement.h>
#include <nsIDocument.h>
#include <nsIDOMDocument.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIPermissionManager.h>
#include <nsIProgrammingLanguage.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIScriptSecurityManager.h>
#include <nsIURI.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbRemoteCommands:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gRemoteCommandsLog = nsnull;
#define LOG(args)   if (gRemoteCommandsLog) PR_LOG(gRemoteCommandsLog, PR_LOG_WARN, args)
#else
#define LOG(args)   /* nothing */
#endif

static NS_DEFINE_CID(kRemoteCommandsCID, SONGBIRD_REMOTECOMMANDS_CID);

const static char* sPublicWProperties[] =
  { "binding:type",
    "binding:ID",
    "binding:name",
    "binding:tooltip" };

const static char* sPublicRProperties[] =
  { "binding:type",
    "binding:ID",
    "binding:name",
    "binding:tooltip",
    "classinfo:classDescription",
    "classinfo:contractID",
    "classinfo:classID",
    "classinfo:implementationLanguage",
    "classinfo:flags" };

const static char* sPublicMethods[] =
  { "binding:setCommandData",
    "binding:getNumCommands",
    "binding:getCommandType",
    "binding:getCommandId",
    "binding:getCommandText",
    "binding:getCommandToolTipText",
    "binding:register",
    "binding:addCommand",
    "binding:removeCommand"
  };

NS_IMPL_ISUPPORTS5( sbRemoteCommands,
                    nsIClassInfo,
                    nsISecurityCheckedComponent,
                    sbIPlaylistCommands,
                    sbIRemoteCommands,
                    sbISecurityAggregator )

NS_IMPL_CI_INTERFACE_GETTER4( sbRemoteCommands,
                              nsISecurityCheckedComponent,
                              sbIRemoteCommands,
                              sbIPlaylistCommands,
                              sbISecurityAggregator )

SB_IMPL_CLASSINFO( sbRemoteCommands,
                   SONGBIRD_REMOTECOMMANDS_CONTRACTID,
                   SONGBIRD_REMOTECOMMANDS_CLASSNAME,
                   nsIProgrammingLanguage::CPLUSPLUS,
                   0,
                   kRemoteCommandsCID );

sbRemoteCommands::sbRemoteCommands() 
{
#ifdef PR_LOGGING
  if (!gRemoteCommandsLog) {
    gRemoteCommandsLog = PR_NewLogModule("sbRemoteCommands");
  }
  LOG(("sbRemoteCommands::sbRemoteCommands()"));
#endif
}

sbRemoteCommands::~sbRemoteCommands()
{
  LOG(("sbRemoteCommands::~sbRemoteCommands()"));
}

nsresult
sbRemoteCommands::Init()
{
  LOG(("sbRemoteCommands::Init()"));

  nsresult rv;
  nsCOMPtr<sbISecurityMixin> mixin =
      do_CreateInstance( "@songbirdnest.com/remoteapi/security-mixin;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of IIDs to pass to the security mixin
  nsIID **iids;
  PRUint32 iidCount;
  GetInterfaces( &iidCount, &iids );

  // initialize our mixin with approved interfaces, methods, properties
  rv = mixin->Init( (sbISecurityAggregator*)this,
                    ( const nsIID** )iids, iidCount,
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),
                    sPublicRProperties, NS_ARRAY_LENGTH(sPublicRProperties),
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );
  NS_ENSURE_SUCCESS( rv, rv );

  mSecurityMixin = do_QueryInterface( mixin, &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIRemoteCommands
//
// ---------------------------------------------------------------------------


NS_IMETHODIMP
sbRemoteCommands::SetCommandData( PRUint32 aNumCommands,
                                  const PRUnichar **aTypeArray,
                                  const PRUnichar **aIDArray,
                                  const PRUnichar **aNameArray,
                                  const PRUnichar **aTooltipArray )
{
  LOG(( "sbRemoteCommands::SetCommandData(%d)", aNumCommands ));

  // holy argument checking Batman!!
  NS_ENSURE_ARG_POINTER(aTypeArray);
  NS_ENSURE_ARG_POINTER(aIDArray);
  NS_ENSURE_ARG_POINTER(aNameArray);
  NS_ENSURE_ARG_POINTER(aTooltipArray);

  for ( PRUint32 index = 0; index < aNumCommands; index++ ) {
    // check in debug only
    NS_ASSERTION( aTypeArray[index], "ERROR - null array element" );
    NS_ASSERTION( aIDArray[index], "ERROR - null array element" );
    NS_ASSERTION( aNameArray[index], "ERROR - null array element" );
    NS_ASSERTION( aTooltipArray[index], "ERROR - null array element" );

    sbCommand command;
    command.type = aTypeArray[index];
    command.id = aIDArray[index];
    command.name = aNameArray[index];
    command.tooltip = aTooltipArray[index];
    if ( !mCommands.AppendElement(command) ) {
      // AppendElement copies the object so if it fails we are:
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  DoCommandsUpdated();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::AddCommand( const nsAString &aType,
                              const nsAString &aID,
                              const nsAString &aName,
                              const nsAString &aTooltip )
{
  LOG(( "sbRemoteCommands::AddCommand(%s)",
        NS_LossyConvertUTF16toASCII(aID).get() ));
  sbCommand command;
  command.type = aType;
  command.id = aID;
  command.name = aName;
  command.tooltip = aTooltip;

  if ( !mCommands.AppendElement(command) )
    return NS_ERROR_OUT_OF_MEMORY;
  DoCommandsUpdated();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::RemoveCommand( const nsAString &aID )
{
  LOG(("sbRemoteCommands::RemoveCommand()"));
  PRUint32 num = mCommands.Length();
  for ( PRUint32 index = 0; index < num; index++ ) {
    LOG(( "sbRemoteCommands::RemoveCommand(%d:%s)",
          index,
          NS_LossyConvertUTF16toASCII(mCommands.ElementAt(index).id).get()));
    if ( mCommands.ElementAt(index).id == aID ) { 
      mCommands.RemoveElementAt(index);
      DoCommandsUpdated();
      return NS_OK;
    }
  }
  // XXXredfive check an error code here and log a warning if the command
  //            isn't found 
  return NS_OK;
}

void
sbRemoteCommands::DoCommandsUpdated()
{
  nsCOMPtr<sbIRemotePlayer> owner( do_QueryReferent(mWeakOwner) );
  if (owner)
    owner->OnCommandsChanged();
}

NS_IMETHODIMP
sbRemoteCommands::SetOwner( sbIRemotePlayer *aOwner )
{
  LOG(("sbRemoteCommands::SetOwner()"));
  nsresult rv;
  mWeakOwner = do_GetWeakReference( aOwner, &rv );
  return rv;
}

NS_IMETHODIMP
sbRemoteCommands::GetOwner( sbIRemotePlayer **aOwner )
{
  LOG(("sbRemoteCommands::GetOwner()"));
  nsresult rv;
  nsCOMPtr<sbIRemotePlayer> strong = do_QueryReferent( mWeakOwner, &rv );
  NS_IF_ADDREF( *aOwner = strong );
  return rv;
}

// ---------------------------------------------------------------------------
//
//                        sbIPlaylistCommands
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteCommands::SetContext( sbIPlaylistCommandsContext *aContext)
{
  LOG(("sbRemoteCommands::SetContext()"));
  mContext = aContext;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetNumCommands( const nsAString &aSubMenu,
                                  const nsAString &aHost,
                                  PRInt32 *_retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mCommands.Length();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandType( const nsAString &aSubMenu,
                                  PRInt32 aIndex,
                                  const nsAString &aHost,
                                  nsAString &_retval )
{
  if ( aIndex >= 0 && aIndex < mCommands.Length() ) {
    _retval = mCommands.ElementAt(aIndex).type;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandId( const nsAString &aSubMenu,
                                PRInt32 aIndex,
                                const nsAString &aHost,
                                nsAString &_retval)
{
  if ( aIndex >= 0 && aIndex < mCommands.Length() ) {
    _retval = mCommands.ElementAt(aIndex).id;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandText( const nsAString &aSubMenu,
                                  PRInt32 aIndex,
                                  const nsAString &aHost,
                                  nsAString &_retval)
{
  if ( aIndex >= 0 && aIndex < mCommands.Length() ) {
    _retval = mCommands.ElementAt(aIndex).name;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandFlex( const nsAString &aSubMenu,
                                  PRInt32 aIndex,
                                  const nsAString &aHost,
                                  PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if ( aIndex >= 0 && aIndex < mCommands.Length() ) {
    nsAutoString cmdType = mCommands.ElementAt(aIndex).type;
    if ( cmdType == NS_LITERAL_STRING("separator") )
      *_retval = 1;
    else
      *_retval = 0;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandToolTipText( const nsAString &aSubMenu,
                                         PRInt32 aIndex,
                                         const nsAString & aHost,
                                         nsAString &_retval)
{
  if ( aIndex >= 0 && aIndex < mCommands.Length() ) {
    _retval = mCommands.ElementAt(aIndex).tooltip;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandEnabled( const nsAString &aSubMenu,
                                     PRInt32 aIndex,
                                     const nsAString &aHost,
                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandVisible( const nsAString &aSubMenu,
                                     PRInt32 aIndex,
                                     const nsAString &aHost,
                                     PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandFlag( const nsAString &aSubMenu,
                                  PRInt32 aIndex,
                                  const nsAString &aHost,
                                  PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandValue( const nsAString &aSubMenu,
                                   PRInt32 aIndex,
                                   const nsAString &aHost,
                                   nsAString &_retval)
{
  _retval = EmptyString();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandShortcutModifiers( const nsAString &aSubMenu,
                                               PRInt32 aIndex,
                                               const nsAString &aHost,
                                               nsAString &_retval)
{
  _retval = EmptyString();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandShortcutKey( const nsAString &aSubMenu,
                                         PRInt32 aIndex,
                                         const nsAString &aHost,
                                         nsAString &_retval)
{
  _retval = EmptyString();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandShortcutKeycode( const nsAString &aSubMenu,
                                             PRInt32 aIndex,
                                             const nsAString &aHost,
                                             nsAString &_retval)
{
  _retval = EmptyString();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandShortcutLocal( const nsAString &aSubMenu,
                                           PRInt32 aIndex,
                                           const nsAString &aHost,
                                           PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetCommandChoiceItem( const nsAString &aChoiceMenu,
                                        const nsAString &aHost,
                                        nsAString &_retval)
{
  _retval = EmptyString();
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::InstantiateCustomCommand( const nsAString &aID,
                                            const nsAString &aHost,
                                            nsIDOMNode **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbRemoteCommands::RefreshCustomCommand( nsIDOMNode *aCustomCommandElement,
                                        const nsAString &aId,
                                        const nsAString &aHost)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
sbRemoteCommands::OnCommand( const nsAString &aID,
                             const nsAString &aValue,
                             const nsAString &aHost )
{
  LOG(( "sbRemoteCommands::OnCommand(%s %s %s)",
        NS_LossyConvertUTF16toASCII(aID).get(),
        NS_LossyConvertUTF16toASCII(aValue).get(),
        NS_LossyConvertUTF16toASCII(aHost).get() ));

  nsresult rv;
  nsCOMPtr<sbIRemotePlayer> owner( do_QueryReferent( mWeakOwner, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  return owner->FireEventToContent( NS_LITERAL_STRING("Events"), aID );
}

NS_IMETHODIMP
sbRemoteCommands::InitCommands( const nsAString &aHost )
{
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::ShutdownCommands( )
{
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::Duplicate( sbIPlaylistCommands **_retval )
{
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbRemoteCommands::Duplicate()"));
  nsresult rv;
  nsCOMPtr<sbIRemoteCommands> copy =
      do_CreateInstance( "@songbirdnest.com/remoteapi/remotecommands;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv );

  PRUint32 num = mCommands.Length();
  for ( PRUint32 index = 0; index < num; index++ ) {
    sbCommand &command = mCommands.ElementAt(index);
    rv = copy->AddCommand( command.type,
                           command.id,
                           command.name,
                           command.tooltip );
    // if AddCommand fails we are out of memory
    NS_ENSURE_SUCCESS( rv, rv );
  }
  nsCOMPtr<sbIRemotePlayer> owner( do_QueryReferent( mWeakOwner, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  rv = copy->SetOwner(owner);
  NS_ENSURE_SUCCESS( rv, rv );
  nsCOMPtr<sbIPlaylistCommands> plCommands( do_QueryInterface( copy, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  NS_ADDREF( *_retval = plCommands );
  return NS_OK;
}

