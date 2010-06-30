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

#include "sbURIPropertyInfo.h"

#include <nsAutoPtr.h>
#include <nsIFileURL.h>

#include <nsServiceManagerUtils.h>
#include <nsNetUtil.h>

#include <sbLockUtils.h>

NS_IMPL_ADDREF_INHERITED(sbURIPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbURIPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbURIPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbURIPropertyInfo, sbIURIPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbURIPropertyInfo, sbIPropertyInfo, sbIURIPropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbURIPropertyInfo::sbURIPropertyInfo()
: mURISchemeConstraintLock(nsnull)
, mIOServiceLock(nsnull)
{
  mType = NS_LITERAL_STRING("uri");

  mURISchemeConstraintLock = PR_NewLock();
  NS_ASSERTION(mURISchemeConstraintLock,
    "sbURIPropertyInfo::mURISchemeConstraintLock failed to create lock!");

  mIOServiceLock = PR_NewLock();
  NS_ASSERTION(mIOServiceLock,
    "sbURIPropertyInfo::mIOServiceLock failed to create lock!");
}

sbURIPropertyInfo::~sbURIPropertyInfo()
{
  if(mURISchemeConstraintLock) {
    PR_DestroyLock(mURISchemeConstraintLock);
  }
}

nsresult
sbURIPropertyInfo::Init()
{
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbURIPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_CONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.contains"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTCONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_contain"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is_not"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.starts"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTBEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_start"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_ENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.ends"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_end"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;

  nsresult rv = EnsureIOService();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
  if(NS_FAILED(rv)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  if(!mURISchemeConstraint.IsEmpty()) {
    NS_ConvertUTF16toUTF8 narrow(mURISchemeConstraint);
    PRBool valid = PR_FALSE;

    rv = uri->SchemeIs(narrow.get(), &valid);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!valid) {
      *_retval = PR_FALSE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRBool sbURIPropertyInfo::IsInvalidEmpty(const nsAString &aValue) {
  // search for ":", ":/" and "://" with no trailing chars
  if (aValue.IsEmpty()) 
    return PR_FALSE;
  
  PRUint32 pos = aValue.FindChar(':');
  if (pos < 0) 
    return PR_FALSE;
  if (pos == aValue.Length()-1 ||
      (pos == aValue.Length()-2 && aValue.CharAt(pos + 1) == '/') ||
      (pos == aValue.Length()-3 && aValue.CharAt(pos + 1) == '/' && aValue.CharAt(pos + 2) == '/')) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP sbURIPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  if (aValue.IsVoid()) {
    _retval.Truncate();
    return NS_OK;
  }

  nsresult rv;
  nsCString escapedSpec;
  
  if (!IsInvalidEmpty(aValue)) {
    rv = EnsureIOService();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
    NS_ENSURE_SUCCESS(rv, rv);

    {
      // Unlock when we're done with mURISchemeConstraint.
      sbSimpleAutoLock lock(mURISchemeConstraintLock);
      if (!mURISchemeConstraint.IsEmpty()) {
        NS_ConvertUTF16toUTF8 narrow(mURISchemeConstraint);
        PRBool valid = PR_FALSE;

        rv = uri->SchemeIs(narrow.get(), &valid);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!valid) {
          return NS_ERROR_FAILURE;
        }
      }
    }

    nsCOMPtr<nsIFileURL> url = do_QueryInterface(uri, &rv);
    if (NS_SUCCEEDED(rv)) {
      // Get the file path from file url
      nsCOMPtr<nsIFile> file;
      rv = url->GetFile(getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString path;
      rv = file->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);
      // path won't be escaped.
      _retval = path;
      return NS_OK;
    } else {
      rv = uri->GetSpec(escapedSpec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    CopyUTF16toUTF8(aValue, escapedSpec);
  }

  nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Unescape the spec for display purposes.
  nsCString unescapedSpec;
  rv = netUtil->UnescapeString(escapedSpec,
                               nsINetUtil::ESCAPE_ALL,
                               unescapedSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Encode bidirectional formatting characters.
  // (RFC 3987 sections 3.2 and 4.1 paragraph 6)

  PRInt32 offset = 0;
  char buffer[] = {'%','E','2','%','8','0','%','x','x'}; //escaped string
  while ((offset = unescapedSpec.Find(NS_LITERAL_CSTRING("\xE2\x80"), offset)) != -1) {
    switch (unescapedSpec[offset + 2]) {
      case 0x8E:  // U+200E
      case 0x8F:  // U+200F
        buffer[7] = '8';
        buffer[8] = unescapedSpec[offset + 2] - 0x48;
        break;
      case 0xAA:  // U+202A
      case 0xAB:  // U+202B
      case 0xAC:  // U+202C
      case 0xAD:  // U+202D
      case 0xAE:  // U+202E
        buffer[7] = 'A';
        buffer[8] = unescapedSpec[offset + 2] - 0x5F;
        break;
      default:
        offset += 2;  // skip over this occurrence, not a character we want
        continue;
    }
    unescapedSpec.Replace(offset, 3, buffer, sizeof(buffer));
  }
  
  CopyUTF8toUTF16(unescapedSpec, _retval);

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  _retval = EmptyString();
  return NS_OK;
}

// We provide this implementation because the base class calls
// MakeSearchable to find the MakeSortable value and though
// we don't want URIs to be searchable, we still want to sort them.
NS_IMETHODIMP sbURIPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  PRBool bFailed = PR_FALSE;
  nsresult rv;
  
  if (!IsInvalidEmpty(aValue)) {
    rv = EnsureIOService();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get file path
    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsILocalFile> localFile =
        do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = localFile->InitWithPath(NS_ConvertUTF8toUTF16(spec));
    if (NS_SUCCEEDED(rv)) {
      // MakeSortable is called by smart playlist builder. The return value
      // will be compared with the top level property contentURL.
      // So update file path to file url so it can be matched.
      nsCOMPtr<nsIFile> file = do_QueryInterface(localFile, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<nsIURI> fileUri;
      rv = mIOService->NewFileURI(file, getter_AddRefs(fileUri));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString escapedSpec;
      rv = fileUri->GetSpec(escapedSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      // Need to unescape the value after URI tweak.
      nsCOMPtr<nsINetUtil> netUtil =
        do_GetService("@mozilla.org/network/util;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = netUtil->UnescapeString(escapedSpec,
                                   nsINetUtil::ESCAPE_ALL,
                                   spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_ConvertUTF8toUTF16 wide(spec);
    _retval = wide;
  } else {
    _retval = aValue;
  }

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::GetConstrainScheme(nsAString & aConstrainScheme)
{
  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  aConstrainScheme = mURISchemeConstraint;
  return NS_OK;
}
NS_IMETHODIMP sbURIPropertyInfo::SetConstrainScheme(const nsAString & aConstrainScheme)
{
  sbSimpleAutoLock lock(mURISchemeConstraintLock);

  if(mURISchemeConstraint.IsEmpty()) {
    mURISchemeConstraint = aConstrainScheme;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbURIPropertyInfo::EnsureIOService()
{
  nsresult rv = NS_OK;

  if(!mIOService) {
    mIOService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  }

  return rv;
}
