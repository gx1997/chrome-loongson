// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines methods relevant to all code that wants to work with browsing data.

#ifndef CHROME_BROWSER_BROWSING_DATA_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_HELPER_H_
#pragma once

#include <string>

#include "base/basictypes.h"

namespace WebKit {
class WebString;
}

class GURL;

class BrowsingDataHelper {
 public:
  // Returns true iff the provided scheme is (really) web safe, and suitable
  // for treatment as "browsing data". This relies on the definition of web safe
  // in ChildProcessSecurityPolicy, but excluding schemes like
  // `chrome-extension`.
  static bool IsValidScheme(const std::string& scheme);
  static bool IsValidScheme(const WebKit::WebString& scheme);
  static bool HasValidScheme(const GURL& origin);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BrowsingDataHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_HELPER_H_
