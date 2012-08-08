// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_HUNG_PLUGIN_FILTER_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_HUNG_PLUGIN_FILTER_H_

#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop_proxy.h"
#include "base/synchronization/lock.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_sync_message_filter.h"
#include "ppapi/proxy/host_dispatcher.h"

namespace content {

// This class monitors a renderer <-> pepper plugin channel on the I/O thread
// of the renderer for a hung plugin.
//
// If the plugin is not responding to sync messages, it will notify the browser
// process and give the user the option to kill the hung plugin.
//
// Note that this class must be threadsafe since it will get the begin/end
// block notifications on the main thread, but the filter is run on the I/O
// thread. This is important since when we're blocked on a sync message to a
// hung plugin, the main thread is frozen.
//
// NOTE: This class is refcounted (via SyncMessageStatusReceiver).
class PepperHungPluginFilter
    : public ppapi::proxy::HostDispatcher::SyncMessageStatusReceiver {
 public:
  // The |view_routing_id| is the ID of the render_view so that this class can
  // send messages to the browser via that view's route. The |plugin_child_id|
  // is the ID in the browser process of the pepper plugin process host. We use
  // this to identify the proper plugin process to terminate.
  PepperHungPluginFilter(const FilePath& plugin_path,
                         int view_routing_id,
                         int plugin_child_id);
  virtual ~PepperHungPluginFilter();

  // SyncMessageStatusReceiver implementation.
  virtual void BeginBlockOnSyncMessage() OVERRIDE;
  virtual void EndBlockOnSyncMessage() OVERRIDE;

  // MessageFilter implementation.
  virtual void OnFilterAdded(IPC::Channel* channel) OVERRIDE;
  virtual void OnFilterRemoved() OVERRIDE;
  virtual void OnChannelError() OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

 private:
  // Makes sure that the hung timer is scheduled.
  void EnsureTimerScheduled();

  // Checks whether the plugin could have transitioned from hung to unhung and
  // notifies the browser if so.
  void MayHaveBecomeUnhung();

  // Checks if the plugin is considered hung based on whether it has been
  // blocked for long enough.
  bool IsHung() const;

  // Timer handler that checks for a hang after a timeout.
  void OnHangTimer();

  // Sends the hung/unhung message to the browser process.
  void SendHungMessage(bool is_hung);

  base::Lock lock_;

  FilePath plugin_path_;
  int view_routing_id_;
  int plugin_child_id_;

  // Used to post messages to the renderer <-> browser message channel from
  // other threads without having to worry about the lifetime of that object.
  // We don't actually use the "sync" feature of this filter.
  scoped_refptr<IPC::SyncMessageFilter> filter_;

  scoped_refptr<base::MessageLoopProxy> io_loop_;

  // The time when we start being blocked on a sync message. If there are
  // nested ones, this is the time of the outermost one.
  //
  // This will be is_null() if we've never blocked.
  base::TimeTicks began_blocking_time_;

  // Time that the last message was received from the plugin.
  //
  // This will be is_null() if we've never received any messages.
  base::TimeTicks last_message_received_;

  // Number of nested sync messages that we're blocked on.
  int pending_sync_message_count_;

  // True when we've sent the "plugin is hung" message to the browser. We track
  // this so we know to look for it becoming unhung and send the corresponding
  // message to the browser.
  bool hung_plugin_showing_;

  bool timer_task_pending_;

  DISALLOW_COPY_AND_ASSIGN(PepperHungPluginFilter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_HUNG_PLUGIN_FILTER_H_