// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_constants.h"

namespace extensions {
namespace declarative_webrequest_constants {

// Signals to which WebRequestRulesRegistries are registered.
const char kOnRequest[] = "experimental.webRequest.onRequest";

// Keys of dictionaries.
const char kInstanceTypeKey[] = "instanceType";
const char kRedirectUrlKey[] = "redirectUrl";
const char kResourceTypeKey[] = "resourceType";
const char kUrlKey[] = "url";

// Values of dictionaries, in particular instance types
const char kCancelRequestType[] = "experimental.webRequest.CancelRequest";
const char kRedirectRequestType[] = "experimental.webRequest.RedirectRequest";
const char kRequestMatcherType[] = "experimental.webRequest.RequestMatcher";

}  // namespace declarative_webrequest_constants
}  // namespace extensions
