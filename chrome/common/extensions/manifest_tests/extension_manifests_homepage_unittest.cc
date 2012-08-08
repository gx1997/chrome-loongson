// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_tests/extension_manifest_test.h"

#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace errors = extension_manifest_errors;

TEST_F(ExtensionManifestTest, ParseHomepageURLs) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("homepage_valid.json"));

  Testcase testcases[] = {
    Testcase("homepage_empty.json",
             errors::kInvalidHomepageURL),
    Testcase("homepage_invalid.json",
             errors::kInvalidHomepageURL),
    Testcase("homepage_bad_schema.json",
             errors::kInvalidHomepageURL)
  };
  RunTestcases(testcases, arraysize(testcases),
               EXPECT_TYPE_ERROR);
}

TEST_F(ExtensionManifestTest, GetHomepageURL) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("homepage_valid.json"));
  EXPECT_EQ(GURL("http://foo.com#bar"), extension->GetHomepageURL());

  // The Google Gallery URL ends with the id, which depends on the path, which
  // can be different in testing, so we just check the part before id.
  extension = LoadAndExpectSuccess("homepage_google_hosted.json");
  EXPECT_TRUE(StartsWithASCII(extension->GetHomepageURL().spec(),
                              "https://chrome.google.com/webstore/detail/",
                              false));

  extension = LoadAndExpectSuccess("homepage_externally_hosted.json");
  EXPECT_EQ(GURL(), extension->GetHomepageURL());
}