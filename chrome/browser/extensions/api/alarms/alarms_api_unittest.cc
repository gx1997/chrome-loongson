// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the chrome.alarms extension API.

#include "base/values.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/extensions/api/alarms/alarm_manager.h"
#include "chrome/browser/extensions/api/alarms/alarms_api.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace utils = extension_function_test_utils;

namespace extensions {

namespace {

// Test delegate which quits the message loop when an alarm fires.
class AlarmDelegate : public AlarmManager::Delegate {
 public:
  virtual ~AlarmDelegate() {}
  virtual void OnAlarm(const std::string& extension_id,
                       const AlarmManager::Alarm& alarm) {
    alarms_seen.push_back(alarm.name);
    MessageLoop::current()->Quit();
  }

  std::vector<std::string> alarms_seen;
};

class ExtensionAlarmsTest : public BrowserWithTestWindowTest {
 public:
  virtual void SetUp() {
    BrowserWithTestWindowTest::SetUp();

    TestExtensionSystem* system = static_cast<TestExtensionSystem*>(
        ExtensionSystem::Get(browser()->profile()));
    system->CreateAlarmManager();
    alarm_manager_ = system->alarm_manager();

    alarm_delegate_ = new AlarmDelegate();
    alarm_manager_->set_delegate(alarm_delegate_);

    extension_ = utils::CreateEmptyExtension();
  }

  base::Value* RunFunctionWithExtension(
      UIThreadExtensionFunction* function, const std::string& args) {
    function->set_extension(extension_.get());
    return utils::RunFunctionAndReturnResult(function, args, browser());
  }

  base::DictionaryValue* RunFunctionAndReturnDict(
      UIThreadExtensionFunction* function, const std::string& args) {
    base::Value* result = RunFunctionWithExtension(function, args);
    return result ? utils::ToDictionary(result) : NULL;
  }

  base::ListValue* RunFunctionAndReturnList(
      UIThreadExtensionFunction* function, const std::string& args) {
    base::Value* result = RunFunctionWithExtension(function, args);
    return result ? utils::ToList(result) : NULL;
  }

  void RunFunction(UIThreadExtensionFunction* function,
                   const std::string& args) {
    scoped_ptr<base::Value> result(RunFunctionWithExtension(function, args));
  }

  std::string RunFunctionAndReturnError(UIThreadExtensionFunction* function,
                                        const std::string& args) {
    function->set_extension(extension_.get());
    return utils::RunFunctionAndReturnError(function, args, browser());
  }

  // Takes a JSON result from a function and converts it to a vector of
  // Alarms.
  AlarmManager::AlarmList ToAlarmList(base::ListValue* value) {
    AlarmManager::AlarmList list;
    for (size_t i = 0; i < value->GetSize(); ++i) {
      linked_ptr<AlarmManager::Alarm> alarm(new AlarmManager::Alarm);
      base::DictionaryValue* alarm_value;
      if (!value->GetDictionary(i, &alarm_value)) {
        ADD_FAILURE() << "Expected a list of Alarm objects.";
        return list;
      }
      EXPECT_TRUE(AlarmManager::Alarm::Populate(*alarm_value, alarm.get()));
      list.push_back(alarm);
    }
    return list;
  }

  // Creates up to 3 alarms using the extension API.
  void CreateAlarms(size_t num_alarms) {
    CHECK(num_alarms <= 3);

    const char* kCreateArgs[] = {
      "[null, {\"delayInSeconds\": 1, \"repeating\": true}]",
      "[\"7\", {\"delayInSeconds\": 7, \"repeating\": true}]",
      "[\"0\", {\"delayInSeconds\": 0}]"
    };
    for (size_t i = 0; i < num_alarms; ++i) {
      scoped_ptr<base::DictionaryValue> result(RunFunctionAndReturnDict(
          new AlarmsCreateFunction(), kCreateArgs[i]));
      EXPECT_FALSE(result.get());
    }
  }

 protected:
  AlarmManager* alarm_manager_;
  AlarmDelegate* alarm_delegate_;
  scoped_refptr<Extension> extension_;
};

}  // namespace

TEST_F(ExtensionAlarmsTest, Create) {
  // Create 1 non-repeating alarm.
  RunFunction(new AlarmsCreateFunction(), "[null, {\"delayInSeconds\": 0}]");

  const AlarmManager::Alarm* alarm =
      alarm_manager_->GetAlarm(extension_->id(), "");
  ASSERT_TRUE(alarm);
  EXPECT_EQ("", alarm->name);
  EXPECT_EQ(0, alarm->delay_in_seconds);
  EXPECT_FALSE(alarm->repeating);

  // Now wait for the alarm to fire. Our test delegate will quit the
  // MessageLoop when that happens.
  MessageLoop::current()->Run();

  ASSERT_EQ(1u, alarm_delegate_->alarms_seen.size());
  EXPECT_EQ("", alarm_delegate_->alarms_seen[0]);

  // Ensure the alarm is gone.
  {
    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_FALSE(alarms);
  }
}

TEST_F(ExtensionAlarmsTest, CreateRepeating) {
  // Create 1 repeating alarm.
  // TODO(mpcmoplete): Use a shorter delay if we switch to allow fractions.
  // A repeating timer with a 0-second delay fires infinitely.
  RunFunction(new AlarmsCreateFunction(),
              "[null, {\"delayInSeconds\": 1, \"repeating\": true}]");

  const AlarmManager::Alarm* alarm =
      alarm_manager_->GetAlarm(extension_->id(), "");
  ASSERT_TRUE(alarm);
  EXPECT_EQ("", alarm->name);
  EXPECT_EQ(1, alarm->delay_in_seconds);
  EXPECT_TRUE(alarm->repeating);

  // Now wait for the alarm to fire. Our test delegate will quit the
  // MessageLoop when that happens.
  MessageLoop::current()->Run();

  // Wait again, and ensure the alarm fires again.
  MessageLoop::current()->Run();

  ASSERT_EQ(2u, alarm_delegate_->alarms_seen.size());
  EXPECT_EQ("", alarm_delegate_->alarms_seen[0]);
}

TEST_F(ExtensionAlarmsTest, CreateDupe) {
  // Create 2 duplicate alarms. The first should be overridden.
  RunFunction(new AlarmsCreateFunction(), "[\"dup\", {\"delayInSeconds\": 1}]");
  RunFunction(new AlarmsCreateFunction(), "[\"dup\", {\"delayInSeconds\": 7}]");

  {
    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_TRUE(alarms);
    EXPECT_EQ(1u, alarms->size());
    EXPECT_EQ(7, (*alarms)[0]->delay_in_seconds);
  }
}

TEST_F(ExtensionAlarmsTest, Get) {
  // Create 2 alarms, and make sure we can query them.
  CreateAlarms(2);

  // Get the default one.
  {
    AlarmManager::Alarm alarm;
    scoped_ptr<base::DictionaryValue> result(RunFunctionAndReturnDict(
        new AlarmsGetFunction(), "[null]"));
    ASSERT_TRUE(result.get());
    EXPECT_TRUE(AlarmManager::Alarm::Populate(*result, &alarm));
    EXPECT_EQ("", alarm.name);
    EXPECT_EQ(1, alarm.delay_in_seconds);
    EXPECT_TRUE(alarm.repeating);
  }

  // Get "7".
  {
    AlarmManager::Alarm alarm;
    scoped_ptr<base::DictionaryValue> result(RunFunctionAndReturnDict(
        new AlarmsGetFunction(), "[\"7\"]"));
    ASSERT_TRUE(result.get());
    EXPECT_TRUE(AlarmManager::Alarm::Populate(*result, &alarm));
    EXPECT_EQ("7", alarm.name);
    EXPECT_EQ(7, alarm.delay_in_seconds);
    EXPECT_TRUE(alarm.repeating);
  }

  // Get a non-existent one.
  {
    std::string error = RunFunctionAndReturnError(
        new AlarmsGetFunction(), "[\"nobody\"]");
    EXPECT_FALSE(error.empty());
  }
}

TEST_F(ExtensionAlarmsTest, GetAll) {
  // Test getAll with 0 alarms.
  {
    scoped_ptr<base::ListValue> result(RunFunctionAndReturnList(
        new AlarmsGetAllFunction(), "[]"));
    AlarmManager::AlarmList alarms = ToAlarmList(result.get());
    EXPECT_EQ(0u, alarms.size());
  }

  // Create 2 alarms, and make sure we can query them.
  CreateAlarms(2);

  {
    scoped_ptr<base::ListValue> result(RunFunctionAndReturnList(
        new AlarmsGetAllFunction(), "[null]"));
    AlarmManager::AlarmList alarms = ToAlarmList(result.get());
    EXPECT_EQ(2u, alarms.size());

    // Test the "7" alarm.
    AlarmManager::Alarm* alarm = alarms[0].get();
    if (alarm->name != "7")
      alarm = alarms[1].get();
    EXPECT_EQ("7", alarm->name);
    EXPECT_EQ(7, alarm->delay_in_seconds);
    EXPECT_TRUE(alarm->repeating);
  }
}

TEST_F(ExtensionAlarmsTest, Clear) {
  // Clear a non-existent one.
  {
    std::string error = RunFunctionAndReturnError(
        new AlarmsClearFunction(), "[\"nobody\"]");
    EXPECT_FALSE(error.empty());
  }

  // Create 3 alarms.
  CreateAlarms(3);

  // Clear all but the 1-second one.
  {
    RunFunction(new AlarmsClearFunction(), "[\"7\"]");
    RunFunction(new AlarmsClearFunction(), "[\"0\"]");

    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_TRUE(alarms);
    EXPECT_EQ(1u, alarms->size());
    EXPECT_EQ(1, (*alarms)[0]->delay_in_seconds);
  }

  // Now wait for the alarms to fire, and ensure the cancelled alarms don't
  // fire.
  MessageLoop::current()->Run();

  ASSERT_EQ(1u, alarm_delegate_->alarms_seen.size());
  EXPECT_EQ("", alarm_delegate_->alarms_seen[0]);

  // Ensure the 1-second alarm is still there, since its repeating.
  {
    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_TRUE(alarms);
    EXPECT_EQ(1u, alarms->size());
    EXPECT_EQ(1, (*alarms)[0]->delay_in_seconds);
  }
}

TEST_F(ExtensionAlarmsTest, ClearAll) {
  // ClearAll with no alarms set.
  {
    scoped_ptr<base::Value> result(RunFunctionWithExtension(
        new AlarmsClearAllFunction(), "[]"));
    EXPECT_FALSE(result.get());
  }

  // Create 3 alarms.
  {
    CreateAlarms(3);
    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_TRUE(alarms);
    EXPECT_EQ(3u, alarms->size());
  }

  // Clear them.
  {
    RunFunction(new AlarmsClearAllFunction(), "[]");
    const AlarmManager::AlarmList* alarms =
        alarm_manager_->GetAllAlarms(extension_->id());
    ASSERT_FALSE(alarms);
  }
}

}  // namespace extensions
