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

#ifndef __SBLOCALDATABASESCHEMAINFO_H__
#define __SBLOCALDATABASESCHEMAINFO_H__

struct sbStaticProperty {
  const PRUnichar* mName;
  const PRUnichar* mColumn;
  PRUint32         mID;
};

static sbStaticProperty sStaticProperties[] = {
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#created").get(),
    NS_LITERAL_STRING("created").get(),
    PR_UINT32_MAX,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#updated").get(),
    NS_LITERAL_STRING("updated").get(),
    PR_UINT32_MAX - 1,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentUrl").get(),
    NS_LITERAL_STRING("content_url").get(),
    PR_UINT32_MAX - 2,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentMimeType").get(),
    NS_LITERAL_STRING("content_mime_type").get(),
    PR_UINT32_MAX - 3,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#contentLength").get(),
    NS_LITERAL_STRING("content_length").get(),
    PR_UINT32_MAX - 4,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#visible").get(),
    NS_LITERAL_STRING("visible").get(),
    PR_UINT32_MAX - 5,
  },
  {
    NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#isList").get(),
    NS_LITERAL_STRING("media_list_type_id").get(),
    PR_UINT32_MAX - 6,
  }
};

static const PRUint32 sStaticPropertyCount = 7;

static PRBool
SB_IsTopLevelPropertyID(PRUint32 aPropertyID)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(sStaticProperties[i].mID == aPropertyID)
      return PR_TRUE;
  }
  return PR_FALSE;
}

static PRBool
SB_IsTopLevelProperty(const nsAString& aProperty)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.Equals(sStaticProperties[i].mName))
      return PR_TRUE;
  }
  return PR_FALSE;
}

static nsresult
SB_GetTopLevelPropertyColumn(const nsAString& aProperty,
                             nsAString& aColumnName)
{
  for(PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    if(aProperty.Equals(sStaticProperties[i].mName)) {
      aColumnName.Assign(sStaticProperties[i].mColumn);
      return NS_OK;
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}
#endif // __SBLOCALDATABASESCHEMAINFO_H__

