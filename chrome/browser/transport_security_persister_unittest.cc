// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/transport_security_persister.h"

#include <string>

#include "base/bind.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/scoped_temp_dir.h"
#include "content/test/test_browser_thread.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/transport_security_state.h"
#include "net/base/x509_cert_types.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::TransportSecurityState;
using content::BrowserThread;

class TransportSecurityPersisterTest : public testing::Test {
 public:
  TransportSecurityPersisterTest()
    : message_loop(MessageLoop::TYPE_IO),
      test_io_thread(BrowserThread::IO, &message_loop),
      persister(&state, temp_dir.path(), false) {
  }

  virtual void SetUp() { }

  MessageLoop message_loop;
  content::TestBrowserThread test_io_thread;
  ScopedTempDir temp_dir;
  TransportSecurityPersister persister;
  TransportSecurityState state;
};

TEST_F(TransportSecurityPersisterTest, SerializeData1) {
  std::string output;
  bool dirty;

  EXPECT_TRUE(persister.SerializeData(&output));
  EXPECT_TRUE(persister.LoadEntries(output, &dirty));
  EXPECT_FALSE(dirty);
}

TEST_F(TransportSecurityPersisterTest, SerializeData2) {
  TransportSecurityState::DomainState domain_state;
  const base::Time current_time(base::Time::Now());
  const base::Time expiry = current_time + base::TimeDelta::FromSeconds(1000);

  EXPECT_FALSE(state.GetDomainState("yahoo.com", true, &domain_state));
  domain_state.upgrade_mode =
      TransportSecurityState::DomainState::MODE_FORCE_HTTPS;
  domain_state.upgrade_expiry = expiry;
  domain_state.include_subdomains = true;
  state.EnableHost("yahoo.com", domain_state);

  std::string output;
  bool dirty;
  EXPECT_TRUE(persister.SerializeData(&output));
  EXPECT_TRUE(persister.LoadEntries(output, &dirty));

  EXPECT_TRUE(state.GetDomainState("yahoo.com", true, &domain_state));
  EXPECT_EQ(domain_state.upgrade_mode,
            TransportSecurityState::DomainState::MODE_FORCE_HTTPS);
  EXPECT_TRUE(state.GetDomainState("foo.yahoo.com", true, &domain_state));
  EXPECT_EQ(domain_state.upgrade_mode,
            TransportSecurityState::DomainState::MODE_FORCE_HTTPS);
  EXPECT_TRUE(state.GetDomainState("foo.bar.yahoo.com", true, &domain_state));
  EXPECT_EQ(domain_state.upgrade_mode,
            TransportSecurityState::DomainState::MODE_FORCE_HTTPS);
  EXPECT_TRUE(state.GetDomainState("foo.bar.baz.yahoo.com", true,
                                   &domain_state));
  EXPECT_EQ(domain_state.upgrade_mode,
            TransportSecurityState::DomainState::MODE_FORCE_HTTPS);
  EXPECT_FALSE(state.GetDomainState("com", true, &domain_state));
}

TEST_F(TransportSecurityPersisterTest, SerializeData3) {
  using std::string;
  using net::SHA1Fingerprint;

  // Add an entry.
  SHA1Fingerprint fp1;
  memset(fp1.data, 0, sizeof(fp1.data));
  SHA1Fingerprint fp2;
  memset(fp2.data, 1, sizeof(fp2.data));
  TransportSecurityState::DomainState example_state;
  example_state.upgrade_expiry =
      base::Time::Now() + base::TimeDelta::FromSeconds(1000);
  example_state.upgrade_mode =
      TransportSecurityState::DomainState::MODE_FORCE_HTTPS;
  example_state.dynamic_spki_hashes_expiry = example_state.upgrade_expiry;
  example_state.dynamic_spki_hashes.push_back(fp1);
  example_state.dynamic_spki_hashes.push_back(fp2);
  state.EnableHost("www.example.com", example_state);

  // Add another entry.
  memset(fp1.data, 2, sizeof(fp1.data));
  memset(fp2.data, 3, sizeof(fp2.data));
  example_state.upgrade_expiry =
      base::Time::Now() + base::TimeDelta::FromSeconds(3000);
  example_state.upgrade_mode =
      TransportSecurityState::DomainState::MODE_DEFAULT;
  example_state.dynamic_spki_hashes_expiry = example_state.upgrade_expiry;
  example_state.dynamic_spki_hashes.push_back(fp1);
  example_state.dynamic_spki_hashes.push_back(fp2);
  state.EnableHost("www.example.net", example_state);

  // Save a copy of everything.
  std::map<string, TransportSecurityState::DomainState> saved;
  TransportSecurityState::Iterator i(state);
  while (i.HasNext()) {
    saved[i.hostname()] = i.domain_state();
    i.Advance();
  }

  std::string serialized;
  EXPECT_TRUE(persister.SerializeData(&serialized));

  // Persist the data to the file. For the test to be fast and not flaky, we
  // just do it directly rather than call persister.StateIsDirty. (That uses
  // ImportantFileWriter, which has an asynchronous commit interval rather
  // than block.) Use a different basename just for cleanliness.
  FilePath path = temp_dir.path().AppendASCII("TransportSecurityPersisterTest");
  EXPECT_TRUE(file_util::WriteFile(path, serialized.c_str(),
                                   serialized.size()));

  // Read the data back.
  std::string persisted;
  EXPECT_TRUE(file_util::ReadFileToString(path, &persisted));
  EXPECT_EQ(persisted, serialized);
  bool dirty;
  EXPECT_TRUE(persister.LoadEntries(persisted, &dirty));
  EXPECT_FALSE(dirty);

  // Check that states are the same as saved.
  size_t count = 0;
  TransportSecurityState::Iterator j(state);
  while (j.HasNext()) {
    EXPECT_TRUE(saved[j.hostname()].Equals(j.domain_state()));
    count++;
    j.Advance();
  }
  EXPECT_EQ(count, saved.size());
}

TEST_F(TransportSecurityPersisterTest, SerializeDataOld) {
  // This is an old-style piece of transport state JSON, which has no creation
  // date.
  std::string output =
      "{ "
      "\"NiyD+3J1r6z1wjl2n1ALBu94Zj9OsEAMo0kCN8js0Uk=\": {"
      "\"expiry\": 1266815027.983453, "
      "\"include_subdomains\": false, "
      "\"mode\": \"strict\" "
      "}"
      "}";
  bool dirty;
  EXPECT_TRUE(persister.LoadEntries(output, &dirty));
  EXPECT_TRUE(dirty);
}

TEST_F(TransportSecurityPersisterTest, PublicKeyHashes) {
  TransportSecurityState::DomainState domain_state;
  EXPECT_FALSE(state.GetDomainState("example.com", false, &domain_state));
  net::FingerprintVector hashes;
  EXPECT_TRUE(domain_state.IsChainOfPublicKeysPermitted(hashes));

  net::SHA1Fingerprint hash;
  memset(hash.data, '1', sizeof(hash.data));
  domain_state.static_spki_hashes.push_back(hash);

  EXPECT_FALSE(domain_state.IsChainOfPublicKeysPermitted(hashes));
  hashes.push_back(hash);
  EXPECT_TRUE(domain_state.IsChainOfPublicKeysPermitted(hashes));
  hashes[0].data[0] = '2';
  EXPECT_FALSE(domain_state.IsChainOfPublicKeysPermitted(hashes));

  const base::Time current_time(base::Time::Now());
  const base::Time expiry = current_time + base::TimeDelta::FromSeconds(1000);
  domain_state.upgrade_expiry = expiry;
  state.EnableHost("example.com", domain_state);
  std::string ser;
  EXPECT_TRUE(persister.SerializeData(&ser));
  bool dirty;
  EXPECT_TRUE(persister.LoadEntries(ser, &dirty));
  EXPECT_TRUE(state.GetDomainState("example.com", false, &domain_state));
  EXPECT_EQ(1u, domain_state.static_spki_hashes.size());
  EXPECT_EQ(0, memcmp(domain_state.static_spki_hashes[0].data, hash.data,
                      sizeof(hash.data)));
}

TEST_F(TransportSecurityPersisterTest, ForcePreloads) {
  // The static state for docs.google.com, defined in
  // net/base/transport_security_state_static.h, has pins and mode strict.
  // This new policy overrides that with no pins and a weaker mode. We apply
  // this new policy with |DeserializeFromCommandLine| and expect that the
  // new policy is in effect, overriding the static policy.
  std::string preload("{"
                      "\"4AGT3lHihuMSd5rUj7B4u6At0jlSH3HFePovjPR+oLE=\": {"
                      "\"created\": 0.0,"
                      "\"expiry\": 2000000000.0,"
                      "\"include_subdomains\": false,"
                      "\"mode\": \"pinning-only\""
                      "}}");

  EXPECT_TRUE(persister.DeserializeFromCommandLine(preload));

  TransportSecurityState::DomainState domain_state;
  EXPECT_TRUE(state.GetDomainState("docs.google.com", true, &domain_state));
  EXPECT_FALSE(domain_state.HasPins());
  EXPECT_FALSE(domain_state.ShouldRedirectHTTPToHTTPS());
}

