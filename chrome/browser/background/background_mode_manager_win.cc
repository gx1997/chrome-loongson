// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "chrome/browser/background/background_mode_manager.h"
#include "chrome/installer/util/auto_launch_util.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;

void BackgroundModeManager::EnableLaunchOnStartup(bool should_launch) {
  // This functionality is only defined for default profile, currently.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kUserDataDir))
    return;
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      should_launch ?
          base::Bind(auto_launch_util::EnableBackgroundStartAtLogin) :
          base::Bind(auto_launch_util::DisableBackgroundStartAtLogin));
}

void BackgroundModeManager::DisplayAppInstalledNotification(
    const Extension* extension) {
  // Create a status tray notification balloon explaining to the user that
  // a background app has been installed.
  CreateStatusTrayIcon();
  status_icon_->DisplayBalloon(
      SkBitmap(),
      l10n_util::GetStringUTF16(IDS_BACKGROUND_APP_INSTALLED_BALLOON_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_BACKGROUND_APP_INSTALLED_BALLOON_BODY,
          UTF8ToUTF16(extension->name()),
          l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
}

string16 BackgroundModeManager::GetPreferencesMenuLabel() {
  return l10n_util::GetStringUTF16(IDS_OPTIONS);
}
