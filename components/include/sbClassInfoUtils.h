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


#ifndef __SB_CLASSINFO_UTILS_H__
#define __SB_CLASSINFO_UTILS_H__

#include <nsIClassInfoImpl.h>
#include <nsStringAPI.h>

#define SB_IMPL_CLASSINFO( _class,                                           \
                           _contractID,                                      \
                           _description,                                     \
                           _language,                                        \
                           _flags,                                           \
                           _classID )                                        \
                                                                             \
NS_IMETHODIMP                                                                \
_class::GetInterfaces( PRUint32 *aCount, nsIID ***aArray )                   \
{                                                                            \
  NS_ENSURE_ARG_POINTER(aCount);                                             \
  NS_ENSURE_ARG_POINTER(aArray);                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)( aCount, aArray );              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetHelperForLanguage( PRUint32 language, nsISupports **_retval )     \
{                                                                            \
  *_retval = nsnull;                                                         \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetContractID( char **aContractID )                                  \
{                                                                            \
  if ( _contractID ) {                                                       \
    *aContractID = ToNewCString( NS_LITERAL_CSTRING(_contractID) );          \
    return *aContractID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;                    \
  }                                                                          \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassDescription( char **aClassDescription )                      \
{                                                                            \
  *aClassDescription = ToNewCString( NS_LITERAL_CSTRING(_description) );     \
  return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;                \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassID( nsCID **aClassID )                                       \
{                                                                            \
  *aClassID = (nsCID*) nsMemory::Alloc( sizeof(nsCID) );                     \
  return *aClassID ? GetClassIDNoAlloc(*aClassID) : NS_ERROR_OUT_OF_MEMORY;  \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetImplementationLanguage( PRUint32 *aImplementationLanguage )       \
{                                                                            \
  *aImplementationLanguage = _language;                                      \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetFlags( PRUint32 *aFlags )                                         \
{                                                                            \
  *aFlags = _flags;                                                          \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassIDNoAlloc( nsCID *aClassIDNoAlloc )                          \
{                                                                            \
  *aClassIDNoAlloc = _classID;                                               \
  return NS_OK;                                                              \
}


#define SB_IMPL_CLASSINFO_INTERFACES_ONLY(_class)                             \
NS_IMETHODIMP                                                                \
_class::GetInterfaces(PRUint32 *aCount, nsIID ***aArray)                     \
{                                                                            \
  NS_ENSURE_ARG_POINTER(aCount);                                             \
  NS_ENSURE_ARG_POINTER(aArray);                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)(aCount, aArray);                \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)       \
{                                                                            \
  *_retval = nsnull;                                                         \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetContractID(char **aContractID)                                    \
{                                                                            \
  *aContractID = nsnull;                                                     \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassDescription(char **aClassDescription)                        \
{                                                                            \
  *aClassDescription = nsnull;                                               \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassID(nsCID **aClassID)                                         \
{                                                                            \
  *aClassID = nsnull;                                                        \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetImplementationLanguage(PRUint32 *aImplementationLanguage)         \
{                                                                            \
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;              \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetFlags(PRUint32 *aFlags)                                           \
{                                                                            \
  *aFlags = 0;                                                               \
  return NS_OK;                                                              \
}                                                                            \
NS_IMETHODIMP                                                                \
_class::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)                            \
{                                                                            \
  return NS_ERROR_NOT_AVAILABLE;                                             \
}

#endif // __SB_CLASSINFO_UTILS_H__
