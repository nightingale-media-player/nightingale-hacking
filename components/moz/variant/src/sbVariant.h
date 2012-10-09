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

/*
// Original code is nsVariant.h from the Mozilla Foundation.
*/

/* The long avoided variant support for xpcom. */

#include <nsIVariant.h>
#include <nsStringGlue.h>
#include <xpt_struct.h>
#include <mozilla/Mutex.h>

/** 
* Map the nsAUTF8String, nsUTF8String classes to the nsACString and
* nsCString classes respectively for now.  These defines need to be removed
* once Jag lands his nsUTF8String implementation.
*/
#define nsAUTF8String nsACString
#define nsUTF8String nsCString
#define PromiseFlatUTF8String PromiseFlatCString

/**
* nsDiscriminatedUnion is a type that nsIVariant implementors *may* use 
* to hold underlying data. It has no methods. So, its use requires no linkage
* to the xpcom module.
*/

struct nsDiscriminatedUnion
{
  union {
    PRInt8         mInt8Value;
    PRInt16        mInt16Value;
    PRInt32        mInt32Value;
    PRInt64        mInt64Value;
    PRUint8        mUint8Value;
    PRUint16       mUint16Value;
    PRUint32       mUint32Value;
    PRUint64       mUint64Value;
    float          mFloatValue;
    double         mDoubleValue;
    bool         mBoolValue;
    char           mCharValue;
    PRUnichar      mWCharValue;
    nsIID          mIDValue;
    nsAString*     mAStringValue;
    nsAUTF8String* mUTF8StringValue;
    nsACString*    mCStringValue;
    struct {
      nsISupports* mInterfaceValue;
      nsIID        mInterfaceID;
    } iface;
    struct {
      nsIID        mArrayInterfaceID;
      void*        mArrayValue;
      PRUint32     mArrayCount;
      PRUint16     mArrayType;
    } array;
    struct {
      char*        mStringValue;
      PRUint32     mStringLength;
    } str;
    struct {
      PRUnichar*   mWStringValue;
      PRUint32     mWStringLength;
    } wstr;
  } u;
  PRUint16       mType;
};

/**
 * sbVariant implements the generic variant support. 
 * They are created 'empty' and 'writable'. 
 *
 * nsIVariant users won't usually need to see this class.
 *
 * This class is threadsafe.
 */

class sbVariant : public nsIWritableVariant
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVARIANT
  NS_DECL_NSIWRITABLEVARIANT

  sbVariant();

  static nsresult Initialize(nsDiscriminatedUnion* data);
  static nsresult Cleanup(nsDiscriminatedUnion* data);

  static nsresult ConvertToInt8(const nsDiscriminatedUnion& data, PRUint8 *_retval);
  static nsresult ConvertToInt16(const nsDiscriminatedUnion& data, PRInt16 *_retval);
  static nsresult ConvertToInt32(const nsDiscriminatedUnion& data, PRInt32 *_retval);
  static nsresult ConvertToInt64(const nsDiscriminatedUnion& data, PRInt64 *_retval);
  static nsresult ConvertToUint8(const nsDiscriminatedUnion& data, PRUint8 *_retval);
  static nsresult ConvertToUint16(const nsDiscriminatedUnion& data, PRUint16 *_retval);
  static nsresult ConvertToUint32(const nsDiscriminatedUnion& data, PRUint32 *_retval);
  static nsresult ConvertToUint64(const nsDiscriminatedUnion& data, PRUint64 *_retval);
  static nsresult ConvertToFloat(const nsDiscriminatedUnion& data, float *_retval);
  static nsresult ConvertToDouble(const nsDiscriminatedUnion& data, double *_retval);
  static nsresult ConvertToBool(const nsDiscriminatedUnion& data, bool *_retval);
  static nsresult ConvertToChar(const nsDiscriminatedUnion& data, char *_retval);
  static nsresult ConvertToWChar(const nsDiscriminatedUnion& data, PRUnichar *_retval);
  static nsresult ConvertToID(const nsDiscriminatedUnion& data, nsID * _retval);
  static nsresult ConvertToAString(const nsDiscriminatedUnion& data, nsAString & _retval);
  static nsresult ConvertToAUTF8String(const nsDiscriminatedUnion& data, nsAUTF8String & _retval);
  static nsresult ConvertToACString(const nsDiscriminatedUnion& data, nsACString & _retval);

  //Cannot accept a const nsDiscriminationUnion struct for this method because of the kinds of
  //conversion that need to occur internally. The data itself will _not_ be modified.
  static nsresult ConvertToString(nsDiscriminatedUnion& data, char **_retval);

  static nsresult ConvertToWString(const nsDiscriminatedUnion& data, PRUnichar **_retval);
  static nsresult ConvertToISupports(const nsDiscriminatedUnion& data, nsISupports **_retval);
  static nsresult ConvertToInterface(const nsDiscriminatedUnion& data, nsIID * *iid, void * *iface);
  static nsresult ConvertToArray(const nsDiscriminatedUnion& data, PRUint16 *type, nsIID* iid, PRUint32 *count, void * *ptr);

  //Cannot accept a const nsDiscriminationUnion struct for this method because of the kinds of
  //conversion that need to occur internally. The data itself will _not_ be modified.
  static nsresult ConvertToStringWithSize(nsDiscriminatedUnion& data, PRUint32 *size, char **str);

  static nsresult ConvertToWStringWithSize(const nsDiscriminatedUnion& data, PRUint32 *size, PRUnichar **str);

  static nsresult SetFromVariant(nsDiscriminatedUnion* data, nsIVariant* aValue);

  static nsresult SetFromInt8(nsDiscriminatedUnion* data, PRUint8 aValue);
  static nsresult SetFromInt16(nsDiscriminatedUnion* data, PRInt16 aValue);
  static nsresult SetFromInt32(nsDiscriminatedUnion* data, PRInt32 aValue);
  static nsresult SetFromInt64(nsDiscriminatedUnion* data, PRInt64 aValue);
  static nsresult SetFromUint8(nsDiscriminatedUnion* data, PRUint8 aValue);
  static nsresult SetFromUint16(nsDiscriminatedUnion* data, PRUint16 aValue);
  static nsresult SetFromUint32(nsDiscriminatedUnion* data, PRUint32 aValue);
  static nsresult SetFromUint64(nsDiscriminatedUnion* data, PRUint64 aValue);
  static nsresult SetFromFloat(nsDiscriminatedUnion* data, float aValue);
  static nsresult SetFromDouble(nsDiscriminatedUnion* data, double aValue);
  static nsresult SetFromBool(nsDiscriminatedUnion* data, bool aValue);
  static nsresult SetFromChar(nsDiscriminatedUnion* data, char aValue);
  static nsresult SetFromWChar(nsDiscriminatedUnion* data, PRUnichar aValue);
  static nsresult SetFromID(nsDiscriminatedUnion* data, const nsID & aValue);
  static nsresult SetFromAString(nsDiscriminatedUnion* data, const nsAString & aValue);
  static nsresult SetFromAUTF8String(nsDiscriminatedUnion* data, const nsAUTF8String & aValue);
  static nsresult SetFromACString(nsDiscriminatedUnion* data, const nsACString & aValue);
  static nsresult SetFromString(nsDiscriminatedUnion* data, const char *aValue);
  static nsresult SetFromWString(nsDiscriminatedUnion* data, const PRUnichar *aValue);
  static nsresult SetFromISupports(nsDiscriminatedUnion* data, nsISupports *aValue);
  static nsresult SetFromInterface(nsDiscriminatedUnion* data, const nsIID& iid, nsISupports *aValue);
  static nsresult SetFromArray(nsDiscriminatedUnion* data, PRUint16 type, const nsIID* iid, PRUint32 count, void * aValue);
  static nsresult SetFromStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const char *aValue);
  static nsresult SetFromWStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const PRUnichar *aValue);

  static nsresult SetToVoid(nsDiscriminatedUnion* data);
  static nsresult SetToEmpty(nsDiscriminatedUnion* data);
  static nsresult SetToEmptyArray(nsDiscriminatedUnion* data);

private:
  ~sbVariant();

protected:
  nsDiscriminatedUnion mData;
  mozilla::Mutex mDataLock;

  bool               mWritable;
};

#define SONGBIRD_VARIANT_CID \
{ /*{B522C2E8-826F-477f-86B7-4A143CD1A188}*/ \
  0xb522c2e8,                                \
  0x826f,                                    \
  0x477f,                                    \
{ 0x86, 0xb7, 0x4a, 0x14, 0x3c, 0xd1, 0xa1, 0x88 }}

#define SONGBIRD_VARIANT_CLASSNAME "sbVariant"

#define SONGBIRD_VARIANT_CONTRACTID "@songbirdnest.com/Songbird/Variant;1"
