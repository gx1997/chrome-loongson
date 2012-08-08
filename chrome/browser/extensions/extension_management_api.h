// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_API_H__
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_API_H__
#pragma once

#include "base/compiler_specific.h"
#include "chrome/browser/extensions/extension_function.h"
#include "chrome/browser/extensions/extension_install_ui.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class ExtensionService;

class ExtensionManagementFunction : public SyncExtensionFunction {
 protected:
  virtual ~ExtensionManagementFunction() {}

  ExtensionService* service();
};

class AsyncExtensionManagementFunction : public AsyncExtensionFunction {
 protected:
  virtual ~AsyncExtensionManagementFunction() {}

  ExtensionService* service();
};

class GetAllExtensionsFunction : public ExtensionManagementFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.getAll");

 protected:
  virtual ~GetAllExtensionsFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class GetExtensionByIdFunction : public ExtensionManagementFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.get");

 protected:
  virtual ~GetExtensionByIdFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class GetPermissionWarningsByIdFunction : public ExtensionManagementFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.getPermissionWarningsById");

 protected:
  virtual ~GetPermissionWarningsByIdFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class GetPermissionWarningsByManifestFunction : public AsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME(
      "management.getPermissionWarningsByManifest");

  // Called when utility process finishes.
  void OnParseSuccess(base::DictionaryValue* parsed_manifest);
  void OnParseFailure(const std::string& error);

 protected:
  virtual ~GetPermissionWarningsByManifestFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class LaunchAppFunction : public ExtensionManagementFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.launchApp");

 protected:
  virtual ~LaunchAppFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class SetEnabledFunction : public AsyncExtensionManagementFunction,
                           public ExtensionInstallUI::Delegate {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.setEnabled");

  SetEnabledFunction();

 protected:
  virtual ~SetEnabledFunction();

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;

  // ExtensionInstalUI::Delegate.
  virtual void InstallUIProceed() OVERRIDE;
  virtual void InstallUIAbort(bool user_initiated) OVERRIDE;

 private:
  std::string extension_id_;

  // Used for prompting to re-enable items with permissions escalation updates.
  scoped_ptr<ExtensionInstallUI> install_ui_;
};

class UninstallFunction : public ExtensionManagementFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("management.uninstall");

 protected:
  virtual ~UninstallFunction() {}

  // ExtensionFunction:
  virtual bool RunImpl() OVERRIDE;
};

class ExtensionManagementEventRouter : public content::NotificationObserver {
 public:
  explicit ExtensionManagementEventRouter(Profile* profile);
  virtual ~ExtensionManagementEventRouter();

  void Init();

 private:
  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  content::NotificationRegistrar registrar_;

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionManagementEventRouter);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_MANAGEMENT_API_H__
