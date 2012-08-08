// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_
#pragma once

#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/string16.h"
#include "base/time.h"
#include "content/public/browser/dom_storage_context.h"
#include "webkit/dom_storage/dom_storage_types.h"

namespace dom_storage {
class DomStorageContext;
}

namespace quota {
class SpecialStoragePolicy;
}

// This is owned by BrowserContext (aka Profile) and encapsulates all
// per-profile dom storage state.
class CONTENT_EXPORT DOMStorageContextImpl :
    NON_EXPORTED_BASE(public content::DOMStorageContext),
    public base::RefCountedThreadSafe<DOMStorageContextImpl> {
 public:
  // If |data_path| is empty, nothing will be saved to disk.
  DOMStorageContextImpl(const FilePath& data_path,
                        quota::SpecialStoragePolicy* special_storage_policy);

  // DOMStorageContext implementation.
  virtual void GetAllStorageFiles(
      const GetAllStorageFilesCallback& callback) OVERRIDE;
  virtual FilePath GetFilePath(const string16& origin_id) const OVERRIDE;
  virtual void DeleteForOrigin(const string16& origin_id) OVERRIDE;
  virtual void DeleteLocalStorageFile(const FilePath& file_path) OVERRIDE;
  virtual void DeleteDataModifiedSince(const base::Time& cutoff) OVERRIDE;

  // Called to free up memory that's not strictly needed.
  void PurgeMemory();

  // Used by content settings to alter the behavior around
  // what data to keep and what data to discard at shutdown.
  // The policy is not so straight forward to describe, see
  // the implementation for details.
  void SetClearLocalState(bool clear_local_state);
  void SaveSessionState();

  // Called when the BrowserContext/Profile is going away.
  void Shutdown();

  // See render_message_filter.cc for details.
  // TODO(michaeln): Remove this method when that bug is fixed.
  int64 LeakyCloneSessionStorage(int64 existing_namespace_id);

 private:
  friend class DOMStorageMessageFilter;  // for access to context()
  friend class SessionStorageNamespaceImpl;  // ditto
  friend class base::RefCountedThreadSafe<DOMStorageContextImpl>;

  virtual ~DOMStorageContextImpl();
  dom_storage::DomStorageContext* context() const { return context_.get(); }

  scoped_refptr<dom_storage::DomStorageContext> context_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DOMStorageContextImpl);
};

#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_
