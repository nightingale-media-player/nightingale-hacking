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

#ifndef __SB_ARTICLESDATA_H__
#define __SB_ARTICLESDATA_H__

#include <nsAutoPtr.h>
#include <nsTArray.h>
#include <nsIObserver.h>
#include <nsStringGlue.h>

class sbArticlesData 
{
friend class sbArticlesDataObserver;
public:
  sbArticlesData();

  nsresult Init();
  
  static nsresult RemoveArticles(const nsAString & aInput,
                                 const nsAString & aLanguage,
                                 nsAString &_retval);

  static nsresult EnsureLoaded();

protected:
  ~sbArticlesData();

  nsresult LoadArticles();
  nsresult UnloadArticles();

private:
  class sbArticle : public nsISupports {
    public:
      sbArticle(const nsAString &aPrefix,
                const nsAString &aSuffix) :
                mPrefix(aPrefix),
                mSuffix(aSuffix) {}
      virtual ~sbArticle() {}
      NS_DECL_ISUPPORTS
      nsString mPrefix;
      nsString mSuffix;
  };
  class sbArticleLanguageData : public nsISupports {
    public:
      sbArticleLanguageData(const nsAString &aLanguage,
                            const nsTArray<nsRefPtr<sbArticle> > &aArticles) :
                            mLanguage(aLanguage),
                            mArticles(aArticles) {}
      ~sbArticleLanguageData() {}
      NS_DECL_ISUPPORTS
      nsString mLanguage;
      nsTArray<nsRefPtr<sbArticle> > mArticles;
  };
  static nsresult AddObserver();
  static nsresult RemoveObserver();
  nsresult ParsePattern(const nsAString &aPattern,
                        nsAString &aPrefix,
                        nsAString &aSuffix);
  
  nsresult _RemoveArticles(const nsAString & aInput,
                           const nsAString & aLanguage,
                           nsAString &_retval);

  nsresult RemoveArticle(const nsAString & aInput,
                         const sbArticle * aArticle,
                         nsAString &_retval);
  
  PRBool mArticlesLoaded;
  nsTArray<nsRefPtr<sbArticleLanguageData> > mLanguages;
};

class sbArticlesDataObserver : public nsIObserver
{
public:
  sbArticlesDataObserver() {}
  virtual ~sbArticlesDataObserver() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

private:
};

#endif /* __SB_ARTICLESDATA_H__ */
