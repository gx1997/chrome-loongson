// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/test/fake_encryptor.h"

#include "base/base64.h"

namespace browser_sync {

FakeEncryptor::~FakeEncryptor() {}

bool FakeEncryptor::EncryptString(const std::string& plaintext,
                                  std::string* ciphertext) {
  return base::Base64Encode(plaintext, ciphertext);
}

bool FakeEncryptor::DecryptString(const std::string& ciphertext,
                                  std::string* plaintext) {
  return base::Base64Decode(ciphertext, plaintext);
}

}  // namespace browser_sync