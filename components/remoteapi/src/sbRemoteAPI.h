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

#ifndef __SB_REMOTE_API_H__
#define __SB_REMOTE_API_H__

#define SB_DECL_SECURITYCHECKEDCOMP_INIT nsresult Init();

#define SB_IMPL_SECURITYCHECKEDCOMP_INIT(_class)                              \
nsresult                                                                      \
_class::Init()                                                                \
{                                                                             \
  nsresult rv;                                                                \
  nsCOMPtr<sbISecurityMixin> mixin =                                          \
     do_CreateInstance( "@songbirdnest.com/remoteapi/security-mixin;1", &rv );\
  NS_ENSURE_SUCCESS(rv, rv);                                                  \
  /* Get the list of IIDs to pass to the security mixin */                    \
  nsIID **iids;                                                               \
  PRUint32 iidCount;                                                          \
  GetInterfaces( &iidCount, &iids );                                          \
  /* initialize our mixin with approved interfaces, methods, properties */    \
  rv = mixin->Init( (sbISecurityAggregator*)this,                             \
                    ( const nsIID** )iids, iidCount,                          \
                    sPublicMethods, NS_ARRAY_LENGTH(sPublicMethods),          \
                    sPublicRProperties, NS_ARRAY_LENGTH(sPublicRProperties),  \
                    sPublicWProperties, NS_ARRAY_LENGTH(sPublicWProperties) );\
  NS_ENSURE_SUCCESS( rv, rv );                                                \
  mSecurityMixin = do_QueryInterface( mixin, &rv );                           \
  NS_ENSURE_SUCCESS( rv, rv );                                                \
  return NS_OK;                                                               \
}

// This macro requires the class to have a mContentDoc data member and a
// mSecurityMixn data member that points to an impl of nsISCC
#define SB_IMPL_SECURITYCHECKEDCOMP_WITH_INIT(_class)                         \
NS_IMETHODIMP                                                                 \
_class::CanCreateWrapper( const nsIID *aIID, char **_retval )                 \
{                                                                             \
  NS_ENSURE_ARG_POINTER(aIID);                                                \
  NS_ENSURE_ARG_POINTER(_retval);                                             \
  if ( !mInitialized && NS_FAILED( Init() ) ) {                               \
    return NS_ERROR_FAILURE;                                                  \
  }                                                                           \
  nsresult rv = mSecurityMixin->CanCreateWrapper( aIID, _retval );            \
  if ( sbRemotePlayer::ShouldNotifyUser( NS_SUCCEEDED(rv) ) ) {               \
    sbRemotePlayer::FireRemoteAPIAccessedEvent(mContentDoc);                  \
  }                                                                           \
  return rv;                                                                  \
}                                                                             \
NS_IMETHODIMP                                                                 \
_class::CanCallMethod( const nsIID *aIID,                                     \
                       const PRUnichar *aMethodName,                          \
                       char **_retval )                                       \
{                                                                             \
  NS_ENSURE_ARG_POINTER(aIID);                                                \
  NS_ENSURE_ARG_POINTER(aMethodName);                                         \
  NS_ENSURE_ARG_POINTER(_retval);                                             \
  if ( !mInitialized && NS_FAILED( Init() ) ) {                               \
    return NS_ERROR_FAILURE;                                                  \
  }                                                                           \
  nsresult rv = mSecurityMixin->CanCallMethod( aIID, aMethodName, _retval );  \
  if ( sbRemotePlayer::ShouldNotifyUser( NS_SUCCEEDED(rv) ) ) {               \
    sbRemotePlayer::FireRemoteAPIAccessedEvent(mContentDoc);                  \
  }                                                                           \
  return rv;                                                                  \
}                                                                             \
NS_IMETHODIMP                                                                 \
_class::CanGetProperty( const nsIID *aIID,                                    \
                        const PRUnichar *aPropertyName,                       \
                        char **_retval )                                      \
{                                                                             \
  NS_ENSURE_ARG_POINTER(aIID);                                                \
  NS_ENSURE_ARG_POINTER(aPropertyName);                                       \
  NS_ENSURE_ARG_POINTER(_retval);                                             \
  if ( !mInitialized && NS_FAILED( Init() ) ) {                               \
    return NS_ERROR_FAILURE;                                                  \
  }                                                                           \
  nsresult rv = mSecurityMixin->CanGetProperty(aIID, aPropertyName, _retval); \
  if ( sbRemotePlayer::ShouldNotifyUser( NS_SUCCEEDED(rv) ) ) {               \
    sbRemotePlayer::FireRemoteAPIAccessedEvent(mContentDoc);                  \
  }                                                                           \
  return rv;                                                                  \
}                                                                             \
NS_IMETHODIMP                                                                 \
_class::CanSetProperty( const nsIID *aIID,                                    \
                        const PRUnichar *aPropertyName,                       \
                        char **_retval )                                      \
{                                                                             \
  NS_ENSURE_ARG_POINTER(aIID);                                                \
  NS_ENSURE_ARG_POINTER(aPropertyName);                                       \
  NS_ENSURE_ARG_POINTER(_retval);                                             \
  if ( !mInitialized && NS_FAILED( Init() ) ) {                               \
    return NS_ERROR_FAILURE;                                                  \
  }                                                                           \
  nsresult rv = mSecurityMixin->CanSetProperty(aIID, aPropertyName, _retval); \
  if ( sbRemotePlayer::ShouldNotifyUser( NS_SUCCEEDED(rv) ) ) {               \
    sbRemotePlayer::FireRemoteAPIAccessedEvent(mContentDoc);                  \
  }                                                                           \
  return rv;                                                                  \
}

#endif // __SB_REMOTE_API_H__

