// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/ppb_nacl_private_impl.h"

#ifndef DISABLE_NACL

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/rand_util_c.h"
#include "chrome/common/render_messages.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_thread.h"
#include "ipc/ipc_sync_message_filter.h"
#include "native_client/src/shared/imc/nacl_imc.h"
#include "ppapi/c/private/ppb_nacl_private.h"
#include "ppapi/native_client/src/trusted/plugin/nacl_entry_points.h"

#if defined(OS_WIN)
#include "content/public/common/sandbox_init.h"
#endif

namespace {

base::LazyInstance<scoped_refptr<IPC::SyncMessageFilter> >
    g_background_thread_sender = LAZY_INSTANCE_INITIALIZER;

// Launch NaCl's sel_ldr process.
bool LaunchSelLdr(const char* alleged_url, int socket_count,
                  void* imc_handles, void* nacl_process_handle,
                  int* nacl_process_id) {
  std::vector<nacl::FileDescriptor> sockets;
  IPC::Message::Sender* sender = content::RenderThread::Get();
  if (sender == NULL)
    sender = g_background_thread_sender.Pointer()->get();

  if (!sender->Send(new ChromeViewHostMsg_LaunchNaCl(
          GURL(alleged_url), socket_count, &sockets)))
    return false;

  CHECK(static_cast<int>(sockets.size()) == socket_count);
  for (int i = 0; i < socket_count; i++) {
    static_cast<nacl::Handle*>(imc_handles)[i] =
        nacl::ToNativeHandle(sockets[i]);
  }
  // TODO(mseaborn): Remove the arguments nacl_process_handle and
  // nacl_process_id from the interface.
  *reinterpret_cast<base::ProcessHandle*>(nacl_process_handle) =
      (base::ProcessHandle) -1;
  *nacl_process_id = 0;
  return true;
}

int UrandomFD(void) {
#if defined(OS_POSIX)
  return GetUrandomFD();
#else
  return 0;
#endif
}

bool Are3DInterfacesDisabled() {
  return CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisable3DAPIs);
}

void EnableBackgroundSelLdrLaunch() {
  g_background_thread_sender.Get() =
      content::RenderThread::Get()->GetSyncMessageFilter();
}

int BrokerDuplicateHandle(void* source_handle,
                          unsigned int process_id,
                          void** target_handle,
                          unsigned int desired_access,
                          unsigned int options) {
#if defined(OS_WIN)
  return content::BrokerDuplicateHandle(source_handle, process_id,
                                        target_handle, desired_access,
                                        options);
#else
  return 0;
#endif
}

const PPB_NaCl_Private nacl_interface = {
  &LaunchSelLdr,
  &UrandomFD,
  &Are3DInterfacesDisabled,
  &EnableBackgroundSelLdrLaunch,
  &BrokerDuplicateHandle,
};

}  // namespace

const PPB_NaCl_Private* PPB_NaCl_Private_Impl::GetInterface() {
  return &nacl_interface;
}

#endif  // DISABLE_NACL
