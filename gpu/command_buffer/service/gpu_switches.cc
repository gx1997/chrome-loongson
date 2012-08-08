// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_switches.h"
#include "base/basictypes.h"

namespace switches {

// Turn on Calling GL Error after every command.
const char kCompileShaderAlwaysSucceeds[]   = "compile-shader-always-succeeds";

// Disable the GLSL translator.
const char kDisableGLSLTranslator[]         = "disable-glsl-translator";

// Turn on Logging GPU commands.
const char kEnableGPUCommandLogging[]       = "enable-gpu-command-logging";

// Turn on Calling GL Error after every command.
const char kEnableGPUDebugging[]            = "enable-gpu-debugging";

// Enforce GL minimums.
const char kEnforceGLMinimums[]             = "enforce-gl-minimums";

const char* kGpuSwitches[] = {
  kCompileShaderAlwaysSucceeds,
  kDisableGLSLTranslator,
  kEnableGPUCommandLogging,
  kEnableGPUDebugging,
  kEnforceGLMinimums,
};

const int kNumGpuSwitches = arraysize(kGpuSwitches);

}  // namespace switches
