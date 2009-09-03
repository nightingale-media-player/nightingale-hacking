/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_
#define SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_

// Mozilla includes
#include <nsAutoLock.h>
#include <nsCOMArray.h>

// Songibrd includes
#include <sbIDeviceCapabilitiesRegistrar.h>

class sbIDevice;

/**
 *   This is a base class for implementing a default capabilities registrar for
 * a device.
 */
class sbDefaultBaseDeviceCapabilitiesRegistrar:
        public sbIDeviceCapabilitiesRegistrar
{
public:
  NS_DECL_SBIDEVICECAPABILITIESREGISTRAR

  sbDefaultBaseDeviceCapabilitiesRegistrar();

protected:
  virtual ~sbDefaultBaseDeviceCapabilitiesRegistrar();
};

#endif /* SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_ */

