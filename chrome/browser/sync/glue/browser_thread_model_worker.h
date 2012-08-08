// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_BROWSER_THREAD_MODEL_WORKER_H_
#define CHROME_BROWSER_SYNC_GLUE_BROWSER_THREAD_MODEL_WORKER_H_
#pragma once

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "content/public/browser/browser_thread.h"
#include "sync/engine/model_safe_worker.h"
#include "sync/util/syncer_error.h"

namespace base {
class WaitableEvent;
}

namespace browser_sync {

// A ModelSafeWorker for models that accept requests from the syncapi that need
// to be fulfilled on a browser thread, for example autofill on the DB thread.
// TODO(sync): Try to generalize other ModelWorkers (e.g. history, etc).
class BrowserThreadModelWorker : public ModelSafeWorker {
 public:
  BrowserThreadModelWorker(content::BrowserThread::ID thread,
                           ModelSafeGroup group);

  // ModelSafeWorker implementation. Called on the sync thread.
  virtual SyncerError DoWorkAndWaitUntilDone(
      const WorkCallback& work) OVERRIDE;
  virtual ModelSafeGroup GetModelSafeGroup() OVERRIDE;

 protected:
  virtual ~BrowserThreadModelWorker();

  // Marked pure virtual so subclasses have to override, but there is
  // an implementation that subclasses should use.  This is so that
  // (subclass)::CallDoWorkAndSignalTask shows up in callstacks.
  virtual void CallDoWorkAndSignalTask(
      const WorkCallback& work,
      base::WaitableEvent* done,
      SyncerError* error) = 0;

 private:
  content::BrowserThread::ID thread_;
  ModelSafeGroup group_;

  DISALLOW_COPY_AND_ASSIGN(BrowserThreadModelWorker);
};

// Subclass BrowserThreadModelWorker so that we can distinguish them
// from stack traces alone.

class DatabaseModelWorker : public BrowserThreadModelWorker {
 public:
  DatabaseModelWorker();

 protected:
  virtual void CallDoWorkAndSignalTask(
      const WorkCallback& work,
      base::WaitableEvent* done,
      SyncerError* error) OVERRIDE;

 private:
  virtual ~DatabaseModelWorker();
};

class FileModelWorker : public BrowserThreadModelWorker {
 public:
  FileModelWorker();

 protected:
  virtual void CallDoWorkAndSignalTask(
      const WorkCallback& work,
      base::WaitableEvent* done,
      SyncerError* error) OVERRIDE;

 private:
  virtual ~FileModelWorker();
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_BROWSER_THREAD_MODEL_WORKER_H_
