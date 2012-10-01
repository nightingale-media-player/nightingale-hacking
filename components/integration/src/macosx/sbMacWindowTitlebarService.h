/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef SB_MAC_WINDOW_TITLEBAR_SERVICE_H_
#define SB_MAC_WINDOW_TITLEBAR_SERVICE_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMacWindowTitlebarService.h
 * \brief Songbird Mac Window Titlebar Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service imported services.
//
//------------------------------------------------------------------------------

// Mozilla imports.
#include <nsIGenericFactory.h>

// Mac OS/X imports.
#include <Cocoa/Cocoa.h>


//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service definitions.
//
//------------------------------------------------------------------------------

//
// Songbird Mac window titlebar service XPCOM component definitions.
//

#define SB_MAC_WINDOW_TITLEBAR_SERVICE_CLASSNAME "sbMacWindowTitlebarService"
#define SB_MAC_WIDNOW_TITLEBAR_SERVICE_DESCRIPTION \
          "Songbird Mac Window Titlebar Service"
#define SB_MAC_WINDOW_TITLEBAR_SERVICE_CONTRACTID \
  "@songbirdnest.com/Songbird/MacWindowTitlebarService;1"
#define SB_MAC_WINDOW_TITLEBAR_SERVICE_CID                                     \
{                                                                              \
  0x7861f6f8,                                                                  \
  0xce57,                                                                      \
  0x48c0,                                                                      \
  { 0x89, 0x17, 0xec, 0x63, 0xbb, 0x08, 0x5a, 0xa0 }                           \
}


//------------------------------------------------------------------------------
//
// Songbird Mac window titlebar service classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides services for the Mac window titlebar.  It sets the
 * titlebar text color as specified by the window "activetitlebartextcolor" and
 * "inactivetitlebartextcolor" attributes.
 */

class sbMacWindowTitlebarService : public nsISupports
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS


  //
  // Public services.
  //

  /**
   * Construct a Songbird Mac window titlebar service object.
   */
  sbMacWindowTitlebarService();

  /**
   * Destroy a Songbird Mac window titlebar service object.
   */
  virtual ~sbMacWindowTitlebarService();

  /**
   * Register the Songbird Mac window titlebar service component.
   */
  static NS_METHOD RegisterSelf(nsIComponentManager*         aCompMgr,
                                nsIFile*                     aPath,
                                const char*                  aLoaderStr,
                                const char*                  aType,
                                const nsModuleComponentInfo* aInfo);

  /**
   * Unregister the Songbird Mac window titlebar service component.
   */
  static NS_METHOD UnregisterSelf(nsIComponentManager*         aCompMgr,
                                  nsIFile*                     aPath,
                                  const char*                  aLoaderStr,
                                  const nsModuleComponentInfo* aInfo);

  /**
   * Initialize the Songbird Mac window titlebar service.
   */
  nsresult Initialize();

  /**
   * Finalize the Songbird Mac window titlebar service.
   */
  void Finalize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mInitialized               True if initialized.
  //

  PRBool                        mInitialized;
};


/**
 * This category extends NSGrayFrame in order to set the titlebar background
 * image and color and the titlebar text color.
 */

@interface NSGrayFrame : NSView

- (NSRect)titlebarRect;
- (NSRect)_titlebarTitleRect;

@end

@interface NSGrayFrame (SBGrayFrame)

/**
 * Songbird override of the NSGrayFrame "drawRect:" method.
 */

- (void)_sbDrawRect:(NSRect)aDirtyRect;

/**
 * Placeholder for original NSGrayFrame "drawRect:" method.  Will be replaced by
 * the NSGrayFrame "drawRect:" implementation.
 */

- (void)_sbOrigDrawRect:(NSRect)aDirtyRect;

@end

#endif // SB_MAC_WINDOW_TITLEBAR_SERVICE_H_

