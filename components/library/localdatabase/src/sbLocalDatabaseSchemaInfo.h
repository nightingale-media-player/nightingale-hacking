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
  const char* mPropertyID;
  const char* mColumn;
  PRUint32    mColumnType;
  PRUint32    mDBID;
};
const PRUint32 SB_COLUMN_TYPE_TEXT    = 0;
const PRUint32 SB_COLUMN_TYPE_INTEGER = 1;

// Top-level media item properties should not be registered using
// sbITextPropertyInfo, as the media_items table does not contain
// obj_searchable and obj_sortable columns with the transformed
// strings expected when searching for and sorting on entries in
// the resource_properties table.
static sbStaticProperty sStaticProperties[] = {
  {
    SB_PROPERTY_GUID,
    "guid",
    SB_COLUMN_TYPE_TEXT,
    PR_UINT32_MAX,
  },
  {
    SB_PROPERTY_CREATED,
    "created",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 1,
  },
  {
    SB_PROPERTY_UPDATED,
    "updated",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 2,
  },
  {
    SB_PROPERTY_CONTENTURL,
    "content_url",
    SB_COLUMN_TYPE_TEXT,
    PR_UINT32_MAX - 3,
  },
  {
    SB_PROPERTY_CONTENTTYPE,
    "content_mime_type",
    SB_COLUMN_TYPE_TEXT,
    PR_UINT32_MAX - 4,
  },
  {
    SB_PROPERTY_CONTENTLENGTH,
    "content_length",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 5,
  },
  {
    SB_PROPERTY_HIDDEN,
    "hidden",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 6,
  },
  {
    SB_PROPERTY_LISTTYPE,
    "media_list_type_id",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 7,
  },
  {
    SB_PROPERTY_HASH,
    "content_hash",
    SB_COLUMN_TYPE_TEXT,
    PR_UINT32_MAX - 8,
  },
  {
    SB_PROPERTY_ISLIST,
    "is_list",
    SB_COLUMN_TYPE_INTEGER,
    PR_UINT32_MAX - 9,
  },
  {
    SB_PROPERTY_METADATA_HASH_IDENTITY,
    "metadata_hash_identity",
    SB_COLUMN_TYPE_TEXT,
    PR_UINT32_MAX - 10,
  },
};

static const PRUint32 sStaticPropertyCount = 11;

static inline bool
SB_IsTopLevelProperty(PRUint32 aPropertyDBID)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(sStaticProperties[i].mDBID == aPropertyDBID)
      return PR_TRUE;
  }
  return PR_FALSE;
}

static inline bool
SB_IsTopLevelProperty(const nsAString& aProperty)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.EqualsLiteral(sStaticProperties[i].mPropertyID))
      return PR_TRUE;
  }
  return PR_FALSE;
}

static inline nsresult
SB_GetTopLevelPropertyColumn(const nsAString& aProperty,
                             nsAString& aColumnName)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.EqualsLiteral(sStaticProperties[i].mPropertyID)) {
      aColumnName.AssignLiteral(sStaticProperties[i].mColumn);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static inline nsresult
SB_GetTopLevelPropertyColumnType(const nsAString& aProperty,
                                 PRUint32 &aColumnType)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.EqualsLiteral(sStaticProperties[i].mPropertyID)) {
      aColumnType = sStaticProperties[i].mColumnType;
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static inline nsresult
SB_GetTopLevelPropertyColumnType(const PRUint32 aPropertyDBID,
                                 PRUint32 &aColumnType)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aPropertyDBID == sStaticProperties[i].mDBID) {
      aColumnType = sStaticProperties[i].mColumnType;
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static inline nsresult
SB_GetTopLevelPropertyColumn(const PRUint32 aPropertyDBID,
                             nsAString& aColumnName)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aPropertyDBID == sStaticProperties[i].mDBID) {
      aColumnName.AssignLiteral(sStaticProperties[i].mColumn);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

static inline PRInt32
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

