// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/breakpad_field_trial_win.h"

#include "base/lazy_instance.h"
#include "base/string_util.h"
#include "base/string16.h"
#include "base/stringprintf.h"
#include "breakpad/src/client/windows/common/ipc_protocol.h"
#include "chrome/app/breakpad_win.h"
#include "chrome/common/child_process_logging.h"

extern "C" void __declspec(dllexport) __cdecl SetExperimentList(
    const std::vector<string16>& experiment_strings) {
  // Make sure we were initialized before we start writing data
  if (breakpad_win::g_experiment_chunks_offset == 0)
    return;

  size_t num_chunks = 0;
  size_t current_experiment = 0;
  string16 current_chunk(google_breakpad::CustomInfoEntry::kValueMaxLength, 0);
  while (current_experiment < experiment_strings.size() &&
         num_chunks < kMaxReportedExperimentChunks) {
    // Check if we have enough room to add another experiment to the current
    // chunk string. If not, we commit the current chunk string and start over.
    if (current_chunk.size() + experiment_strings[current_experiment].size() >
        google_breakpad::CustomInfoEntry::kValueMaxLength) {
      base::wcslcpy(
          (*breakpad_win::g_custom_entries)[
              breakpad_win::g_experiment_chunks_offset + num_chunks].value,
          current_chunk.c_str(),
          current_chunk.size() + 1);  // This must include the NULL termination.
      ++num_chunks;
      current_chunk = experiment_strings[current_experiment];
    } else {
      if (!current_chunk.empty())
        current_chunk += L",";
      current_chunk += experiment_strings[current_experiment];
    }
    ++current_experiment;
  }

  // Commit the last chunk that didn't get big enough yet.
  if (!current_chunk.empty() && num_chunks < kMaxReportedExperimentChunks) {
    base::wcslcpy(
        (*breakpad_win::g_custom_entries)[
            breakpad_win::g_experiment_chunks_offset + num_chunks].value,
        current_chunk.c_str(),
        current_chunk.size() + 1);  // This must include the NULL termination.
  }

  // Make note of the total number of experiments,
  // even if it's > kMaxReportedExperimentChunks. This is useful when
  // correlating stability with the number of experiments running
  // simultaneously.
  base::wcslcpy(
      (*breakpad_win::g_custom_entries)[
          breakpad_win::g_num_of_experiments_offset].value,
      base::StringPrintf(L"%d", experiment_strings.size()).c_str(),
      google_breakpad::CustomInfoEntry::kValueMaxLength);
}

namespace testing {

void SetExperimentList(const std::vector<string16>& experiment_strings) {
  ::SetExperimentList(experiment_strings);
}

}  // namespace testing
