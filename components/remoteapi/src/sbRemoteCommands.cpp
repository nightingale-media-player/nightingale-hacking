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

#include <nsComponentManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIClassInfoImpl.h>
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
  { "binding:setMediaList",
    "binding:setCommandData",
    "binding:getNumCommands",
    "binding:getCommandType",
    "binding:getCommandId",
    "binding:getCommandText",
    "binding:getCommandToolTipText",
    "binding:register",
    "binding:addCommand"
  };

NS_IMPL_ISUPPORTS5( sbRemoteCommands,
                    sbIRemoteCommands,
                    sbIPlaylistCommands,
                    nsIClassInfo,
                    sbISecurityAggregator,
                    nsISecurityCheckedComponent )

NS_IMPL_CI_INTERFACE_GETTER4( sbRemoteCommands,
                              sbIRemoteCommands,
                              sbIPlaylistCommands,
                              sbISecurityAggregator,
                              nsISecurityCheckedComponent )

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
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::AddCommand( const nsAString &aType,
                              const nsAString &aID,
                              const nsAString &aName,
                              const nsAString &aTooltip )
{
  sbCommand command;
  command.type = aType;
  command.id = aID;
  command.name = aName;
  command.tooltip = aTooltip;

  if ( !mCommands.AppendElement(command) )
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::SetOwner( sbIRemotePlayer *aOwner )
{
  LOG(("sbRemoteCommands::SetOwner()"));
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
sbRemoteCommands::GetOwner( sbIRemotePlayer **aOwner )
{
  NS_IF_ADDREF( *aOwner = mOwner );
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        sbIPlaylistCommands
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteCommands::SetMediaList( nsIDOMNode *aNode )
{
  LOG(("sbRemoteCommands::SetMediaList()"));
  mMediaList = aNode;
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
/*
  // Attempt to get the media list so we have context, for things like selection.
  //   This code will change to call an interface implemented on the playlist
  //   binding that will return the mediaListView property, giving us the view,
  //   the list, the treeview and friends, everything a growing boy needs.
  if (mMediaList) {
    nsAutoString name;
    mMediaList->GetNodeName(name);
    
    LOG(("     ***** %s ", NS_LossyConvertUTF16toASCII(name).get() ));

    nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mMediaList));
    PRBool hasProp;
    element->HasAttribute(NS_LITERAL_STRING("foobar"), &hasProp);
    if (hasProp) {
      LOG(("     ***** has foobar property "));
    } else {
      LOG(("     ***** does not have foobar property "));
    }
    nsCOMPtr<sbIRemoteFoo> fooelement(do_QueryInterface(mMediaList));
    if (fooelement) {
      nsAutoString foo;
      fooelement->GetFoobar(foo);
      LOG(("     ***** is a RemoteFoo(%s)", NS_LossyConvertUTF16toASCII(foo).get() ));
    } else {
      LOG(("     ***** is NOT a RemoteFoo"));
    }
    
  }
*/
  NS_ENSURE_TRUE( mOwner, NS_ERROR_FAILURE );
  mOwner->FireEventToContent( NS_LITERAL_STRING("Events"), aID );
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
  rv = copy->SetOwner(mOwner);
  NS_ENSURE_SUCCESS( rv, rv );
  nsCOMPtr<sbIPlaylistCommands> plCommands( do_QueryInterface( copy, &rv ) );
  NS_ENSURE_SUCCESS( rv, rv );
  NS_ADDREF( *_retval = plCommands );
  return NS_OK;
}

// ---------------------------------------------------------------------------
//
//                        nsISecurityCheckedComponent
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP
sbRemoteCommands::CanCreateWrapper( const nsIID *aIID, char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbRemoteCommands::CanCreateWrapper()"));

  return mSecurityMixin->CanCreateWrapper( aIID, _retval );
} 

NS_IMETHODIMP
sbRemoteCommands::CanCallMethod( const nsIID *aIID,
                                 const PRUnichar *aMethodName,
                                 char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aMethodName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemoteCommands::CanCallMethod(%s)", 
        NS_LossyConvertUTF16toASCII(aMethodName).get() ));

  return mSecurityMixin->CanCallMethod( aIID, aMethodName, _retval );
}

NS_IMETHODIMP
sbRemoteCommands::CanGetProperty( const nsIID *aIID,
                                  const PRUnichar *aPropertyName,
                                  char **_retval )
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemoteCommands::CanGetProperty(%s)",
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  return mSecurityMixin->CanGetProperty( aIID, aPropertyName, _retval );
}

NS_IMETHODIMP
sbRemoteCommands::CanSetProperty( const nsIID *aIID,
                                  const PRUnichar *aPropertyName,
                                  char **_retval)
{
  NS_ENSURE_ARG_POINTER(aIID);
  NS_ENSURE_ARG_POINTER(aPropertyName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(( "sbRemoteCommands::CanSetProperty(%s)", 
        NS_LossyConvertUTF16toASCII(aPropertyName).get() ));

  return mSecurityMixin->CanSetProperty( aIID, aPropertyName, _retval );
}

// ---------------------------------------------------------------------------
//
//                            nsIClassInfo
//
// ---------------------------------------------------------------------------

NS_IMETHODIMP 
sbRemoteCommands::GetInterfaces( PRUint32 *aCount, nsIID ***aArray )
{ 
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aArray);
  LOG(("sbRemoteCommands::GetInterfaces()"));
  return NS_CI_INTERFACE_GETTER_NAME(sbRemoteCommands)( aCount, aArray );
}

NS_IMETHODIMP 
sbRemoteCommands::GetHelperForLanguage( PRUint32 language,
                                        nsISupports **_retval )
{
  LOG(("sbRemoteCommands::GetHelperForLanguage()"));
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteCommands::GetContractID( char **aContractID )
{
  LOG(("sbRemoteCommands::GetContractID()"));
  *aContractID = ToNewCString(
                      NS_LITERAL_CSTRING(SONGBIRD_REMOTECOMMANDS_CONTRACTID) );
  return *aContractID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteCommands::GetClassDescription( char **aClassDescription )
{
  LOG(("sbRemoteCommands::GetClassDescription()"));
  *aClassDescription = ToNewCString(
                       NS_LITERAL_CSTRING(SONGBIRD_REMOTECOMMANDS_CLASSNAME) );
  return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteCommands::GetClassID( nsCID **aClassID )
{
  LOG(("sbRemoteCommands::GetClassID()"));
  *aClassID = (nsCID*) nsMemory::Alloc( sizeof(nsCID) );
  return *aClassID ? GetClassIDNoAlloc(*aClassID) : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
sbRemoteCommands::GetImplementationLanguage( PRUint32 *aImplementationLanguage )
{
  LOG(("sbRemoteCommands::GetImplementationLanguage()"));
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteCommands::GetFlags( PRUint32 *aFlags )
{
  LOG(("sbRemoteCommands::GetFlags()"));
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP 
sbRemoteCommands::GetClassIDNoAlloc( nsCID *aClassIDNoAlloc )
{
  LOG(("sbRemoteCommands::GetClassIDNoAlloc()"));
  *aClassIDNoAlloc = kRemoteCommandsCID;
  return NS_OK;
}

