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

#ifndef __SBLOCALDATABASESCHEMAINFO_H__
#define __SBLOCALDATABASESCHEMAINFO_H__

#include <nsComponentManagerUtils.h>

#include <sbISQLBuilder.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbStandardProperties.h>
#include <sbSQLBuilderCID.h>
#include <sbILocalDatabasePropertyCache.h>

#define MAX_IN_LENGTH 5000

struct sbStaticProperty {
  const PRUnichar* mPropertyID;
  const PRUnichar* mColumn;
  PRUint32         mDBID;
};

static sbStaticProperty sStaticProperties[] = {
  {
    NS_LITERAL_STRING(SB_PROPERTY_GUID).get(),
    NS_LITERAL_STRING("guid").get(),
    PR_UINT32_MAX,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_CREATED).get(),
    NS_LITERAL_STRING("created").get(),
    PR_UINT32_MAX - 1,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_UPDATED).get(),
    NS_LITERAL_STRING("updated").get(),
    PR_UINT32_MAX - 2,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL).get(),
    NS_LITERAL_STRING("content_url").get(),
    PR_UINT32_MAX - 3,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_CONTENTMIMETYPE).get(),
    NS_LITERAL_STRING("content_mime_type").get(),
    PR_UINT32_MAX - 4,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH).get(),
    NS_LITERAL_STRING("content_length").get(),
    PR_UINT32_MAX - 5,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_HIDDEN).get(),
    NS_LITERAL_STRING("hidden").get(),
    PR_UINT32_MAX - 6,
  },
  {
    NS_LITERAL_STRING(SB_PROPERTY_ISLIST).get(),
    NS_LITERAL_STRING("media_list_type_id").get(),
    PR_UINT32_MAX - 7,
  }
};

static const PRUint32 sStaticPropertyCount = 8;

static PRBool
SB_IsTopLevelProperty(PRUint32 aPropertyDBID)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(sStaticProperties[i].mDBID == aPropertyDBID)
      return PR_TRUE;
  }
  return PR_FALSE;
}

static PRBool
SB_IsTopLevelProperty(const nsAString& aProperty)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.Equals(sStaticProperties[i].mPropertyID))
      return PR_TRUE;
  }
  return PR_FALSE;
}

static nsresult
SB_GetTopLevelPropertyColumn(const nsAString& aProperty,
                             nsAString& aColumnName)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.Equals(sStaticProperties[i].mPropertyID)) {
      aColumnName.Assign(sStaticProperties[i].mColumn);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static nsresult
SB_GetTopLevelPropertyColumn(const PRUint32 aPropertyDBID,
                             nsAString& aColumnName)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aPropertyDBID == sStaticProperties[i].mDBID) {
      aColumnName.Assign(sStaticProperties[i].mColumn);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static PRInt32
SB_GetPropertyId(const nsAString& aProperty,
                 sbILocalDatabasePropertyCache* aPropertyCache)
{
  nsresult rv;
  PRUint32 id;

  rv = aPropertyCache->GetPropertyDBID(aProperty, &id);
  if (NS_FAILED(rv)) {
    return -1;
  }

  return id;
}

#endif // __SBLOCALDATABASESCHEMAINFO_H__

