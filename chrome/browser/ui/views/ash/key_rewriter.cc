// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ash/key_rewriter.h"

#include <vector>

#include "ash/shell.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/base/keycodes/keyboard_code_conversion.h"

#if defined(OS_CHROMEOS)
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>

#include "base/chromeos/chromeos_version.h"
#include "chrome/browser/chromeos/xinput_hierarchy_changed_event_listener.h"
#include "ui/base/keycodes/keyboard_code_conversion_x.h"
#include "ui/base/x/x11_util.h"
#endif

namespace {

const int kBadDeviceId = -1;

}  // namespace

KeyRewriter::KeyRewriter() : last_device_id_(kBadDeviceId) {
  // The ash shell isn't instantiated for our unit tests.
  if (ash::Shell::HasInstance())
    ash::Shell::GetInstance()->GetRootWindow()->AddRootWindowObserver(this);
#if defined(OS_CHROMEOS)
  if (base::chromeos::IsRunningOnChromeOS()) {
    chromeos::XInputHierarchyChangedEventListener::GetInstance()
        ->AddObserver(this);
  }
  RefreshKeycodes();
#endif
}

KeyRewriter::~KeyRewriter() {
  if (ash::Shell::HasInstance())
    ash::Shell::GetInstance()->GetRootWindow()->RemoveRootWindowObserver(this);
#if defined(OS_CHROMEOS)
  if (base::chromeos::IsRunningOnChromeOS()) {
    chromeos::XInputHierarchyChangedEventListener::GetInstance()
        ->RemoveObserver(this);
  }
#endif
}

KeyRewriter::DeviceType KeyRewriter::DeviceAddedForTesting(
    int device_id,
    const std::string& device_name) {
  return DeviceAddedInternal(device_id, device_name);
}

// static
KeyRewriter::DeviceType KeyRewriter::GetDeviceType(
    const std::string& device_name) {
  std::vector<std::string> tokens;
  Tokenize(device_name, " .", &tokens);

  // If the |device_name| contains the two words, "apple" and "keyboard", treat
  // it as an Apple keyboard.
  bool found_apple = false;
  bool found_keyboard = false;
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (!found_apple && LowerCaseEqualsASCII(tokens[i], "apple"))
      found_apple = true;
    if (!found_keyboard && LowerCaseEqualsASCII(tokens[i], "keyboard"))
      found_keyboard = true;
    if (found_apple && found_keyboard)
      return kDeviceAppleKeyboard;
  }

  return kDeviceUnknown;
}

void KeyRewriter::RewriteCommandToControlForTesting(aura::KeyEvent* event) {
  RewriteCommandToControl(event);
}

void KeyRewriter::RewriteNumPadKeysForTesting(aura::KeyEvent* event) {
  RewriteNumPadKeys(event);
}

ash::KeyRewriterDelegate::Action KeyRewriter::RewriteOrFilterKeyEvent(
    aura::KeyEvent* event) {
  const ash::KeyRewriterDelegate::Action kActionRewrite =
      ash::KeyRewriterDelegate::ACTION_REWRITE_EVENT;
  if (!event->HasNativeEvent()) {
    // Do not handle a fabricated event generated by tests.
    return kActionRewrite;
  }

  if (RewriteCommandToControl(event))
    return kActionRewrite;
  if (RewriteNumPadKeys(event))
    return kActionRewrite;

  // TODO(yusukes): Implement crbug.com/115112 (Search/Ctrl/Alt remapping) and
  // crosbug.com/27167 (allow sending function keys) here.

  return kActionRewrite;  // Do not drop the event.
}

void KeyRewriter::OnKeyboardMappingChanged(const aura::RootWindow* root) {
#if defined(OS_CHROMEOS)
  RefreshKeycodes();
#endif
}

#if defined(OS_CHROMEOS)
void KeyRewriter::DeviceAdded(int device_id) {
  DCHECK_NE(XIAllDevices, device_id);
  DCHECK_NE(XIAllMasterDevices, device_id);
  if (device_id == XIAllDevices || device_id == XIAllMasterDevices) {
    LOG(ERROR) << "Unexpected device_id passed: " << device_id;
    return;
  }

  int ndevices_return = 0;
  XIDeviceInfo* device_info = XIQueryDevice(ui::GetXDisplay(),
                                            device_id,
                                            &ndevices_return);

  // Since |device_id| is neither XIAllDevices nor XIAllMasterDevices,
  // the number of devices found should be either 0 (not found) or 1.
  if (!device_info) {
    LOG(ERROR) << "XIQueryDevice: Device ID " << device_id << " is unknown.";
    return;
  }

  DCHECK_EQ(1, ndevices_return);
  for (int i = 0; i < ndevices_return; ++i) {
    DCHECK_EQ(device_id, device_info[i].deviceid);  // see the comment above.
    DCHECK(device_info[i].name);
    DeviceAddedInternal(device_info[i].deviceid, device_info[i].name);
  }

  XIFreeDeviceInfo(device_info);
}

void KeyRewriter::DeviceRemoved(int device_id) {
  device_id_to_type_.erase(device_id);
}

void KeyRewriter::DeviceKeyPressedOrReleased(int device_id) {
  std::map<int, DeviceType>::const_iterator iter =
      device_id_to_type_.find(device_id);
  if (iter == device_id_to_type_.end()) {
    // |device_id| is unknown. This means the device was connected before
    // booting the OS. Query the name of the device and add it to the map.
    DeviceAdded(device_id);
  }

  last_device_id_ = device_id;
}

void KeyRewriter::RefreshKeycodes() {
  Display* display = ui::GetXDisplay();
  control_l_xkeycode_ = XKeysymToKeycode(display, XK_Control_L);
  control_r_xkeycode_ = XKeysymToKeycode(display, XK_Control_R);
  kp_0_xkeycode_ = XKeysymToKeycode(display, XK_KP_0);
  kp_1_xkeycode_ = XKeysymToKeycode(display, XK_KP_1);
  kp_2_xkeycode_ = XKeysymToKeycode(display, XK_KP_2);
  kp_3_xkeycode_ = XKeysymToKeycode(display, XK_KP_3);
  kp_4_xkeycode_ = XKeysymToKeycode(display, XK_KP_4);
  kp_5_xkeycode_ = XKeysymToKeycode(display, XK_KP_5);
  kp_6_xkeycode_ = XKeysymToKeycode(display, XK_KP_6);
  kp_7_xkeycode_ = XKeysymToKeycode(display, XK_KP_7);
  kp_8_xkeycode_ = XKeysymToKeycode(display, XK_KP_8);
  kp_9_xkeycode_ = XKeysymToKeycode(display, XK_KP_9);
  kp_decimal_xkeycode_ = XKeysymToKeycode(display, XK_KP_Decimal);
}
#endif

bool KeyRewriter::RewriteCommandToControl(aura::KeyEvent* event) {
  bool rewritten = false;
  if (last_device_id_ == kBadDeviceId)
    return rewritten;

  // Check which device generated |event|.
  std::map<int, DeviceType>::const_iterator iter =
      device_id_to_type_.find(last_device_id_);
  if (iter == device_id_to_type_.end()) {
    LOG(ERROR) << "Device ID " << last_device_id_ << " is unknown.";
    return rewritten;
  }

  const DeviceType type = iter->second;
  if (type != kDeviceAppleKeyboard)
    return rewritten;

#if defined(OS_CHROMEOS)
  XEvent* xev = event->native_event();
  XKeyEvent* xkey = &(xev->xkey);

  // Mod4 is the Windows key on a PC keyboard or Command key on an Apple
  // keyboard.
  if (xkey->state & Mod4Mask) {
    xkey->state &= ~Mod4Mask;
    xkey->state |= ControlMask;
    event->set_flags(event->flags() | ui::EF_CONTROL_DOWN);
  }

  const KeySym keysym = XLookupKeysym(xkey, 0);
  switch (keysym) {
    case XK_Super_L:
      // left Command -> left Control
      Rewrite(event, control_l_xkeycode_, xkey->state,
              ui::VKEY_LCONTROL, event->flags());
      rewritten = true;
      break;
    case XK_Super_R:
      // right Command -> right Control
      Rewrite(event, control_r_xkeycode_, xkey->state,
              ui::VKEY_RCONTROL, event->flags());
      rewritten = true;
      break;
    default:
      break;
  }

  DCHECK_NE(ui::VKEY_LWIN, ui::KeyboardCodeFromXKeyEvent(xev));
  DCHECK_NE(ui::VKEY_RWIN, ui::KeyboardCodeFromXKeyEvent(xev));
#else
  // TODO(yusukes): Support Ash on other platforms if needed.
#endif
  return rewritten;
}

bool KeyRewriter::RewriteNumPadKeys(aura::KeyEvent* event) {
  bool rewritten = false;
#if defined(OS_CHROMEOS)
  XEvent* xev = event->native_event();
  XKeyEvent* xkey = &(xev->xkey);

  const KeySym keysym = XLookupKeysym(xkey, 0);
  switch (keysym) {
    case XK_KP_Insert:
      Rewrite(event, kp_0_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD0, event->flags());
      rewritten = true;
      break;
    case XK_KP_Delete:
      Rewrite(event, kp_decimal_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_DECIMAL, event->flags());
      rewritten = true;
      break;
    case XK_KP_End:
      Rewrite(event, kp_1_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD1, event->flags());
      rewritten = true;
      break;
    case XK_KP_Down:
      Rewrite(event, kp_2_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD2, event->flags());
      rewritten = true;
      break;
    case XK_KP_Next:
      Rewrite(event, kp_3_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD3, event->flags());
      rewritten = true;
      break;
    case XK_KP_Left:
      Rewrite(event, kp_4_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD4, event->flags());
      rewritten = true;
      break;
    case XK_KP_Begin:
      Rewrite(event, kp_5_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD5, event->flags());
      rewritten = true;
      break;
    case XK_KP_Right:
      Rewrite(event, kp_6_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD6, event->flags());
      rewritten = true;
      break;
    case XK_KP_Home:
      Rewrite(event, kp_7_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD7, event->flags());
      rewritten = true;
      break;
    case XK_KP_Up:
      Rewrite(event, kp_8_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD8, event->flags());
      rewritten = true;
      break;
    case XK_KP_Prior:
      Rewrite(event, kp_9_xkeycode_, xkey->state | Mod2Mask,
              ui::VKEY_NUMPAD9, event->flags());
      rewritten = true;
      break;
    case XK_KP_Divide:
    case XK_KP_Multiply:
    case XK_KP_Subtract:
    case XK_KP_Add:
    case XK_KP_Enter:
      // Add Mod2Mask for consistency.
      Rewrite(event, xkey->keycode, xkey->state | Mod2Mask,
              event->key_code(), event->flags());
      rewritten = true;
      break;
    default:
      break;
  }
#else
  // TODO(yusukes): Support Ash on other platforms if needed.
#endif
  return rewritten;
}

void KeyRewriter::Rewrite(aura::KeyEvent* event,
                          unsigned int new_native_keycode,
                          unsigned int new_native_state,
                          ui::KeyboardCode new_keycode,
                          int new_flags) {
#if defined(OS_CHROMEOS)
  XEvent* xev = event->native_event();
  XKeyEvent* xkey = &(xev->xkey);
  xkey->keycode = new_native_keycode;
  xkey->state = new_native_state;
  event->set_key_code(new_keycode);
  event->set_character(ui::GetCharacterFromKeyCode(event->key_code(),
                                                   new_flags));
  event->set_flags(new_flags);
#else
  // TODO(yusukes): Support Ash on other platforms if needed.
#endif
}

KeyRewriter::DeviceType KeyRewriter::DeviceAddedInternal(
    int device_id,
    const std::string& device_name) {
  const DeviceType type = KeyRewriter::GetDeviceType(device_name);
  if (type == kDeviceAppleKeyboard) {
    VLOG(1) << "Apple keyboard '" << device_name << "' connected: "
            << "id=" << device_id;
  }
  // Always overwrite the existing device_id since the X server may reuse a
  // device id for an unattached device.
  device_id_to_type_[device_id] = type;
  return type;
}