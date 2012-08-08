// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_
#define CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_

namespace chromeos {

// An enum to describe whether or not a DBus method call succeeded.
enum DBusMethodCallStatus {
  DBUS_METHOD_CALL_FAILURE,
  DBUS_METHOD_CALL_SUCCESS,
};

}  // namespace

#endif  // CHROMEOS_DBUS_DBUS_METHOD_CALL_STATUS_H_