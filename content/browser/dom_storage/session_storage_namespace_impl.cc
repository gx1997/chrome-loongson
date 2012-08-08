// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_namespace_impl.h"

#include "content/browser/dom_storage/dom_storage_context_impl.h"
#include "webkit/dom_storage/dom_storage_session.h"

using dom_storage::DomStorageSession;

SessionStorageNamespaceImpl::SessionStorageNamespaceImpl(
    DOMStorageContextImpl* context)
    : session_(new DomStorageSession(context->context())) {
}

int64 SessionStorageNamespaceImpl::id() const {
  return session_->namespace_id();
}

SessionStorageNamespaceImpl* SessionStorageNamespaceImpl::Clone() {
  return new SessionStorageNamespaceImpl(session_->Clone());
}

SessionStorageNamespaceImpl::SessionStorageNamespaceImpl(
    DomStorageSession* clone)
    : session_(clone) {
}

SessionStorageNamespaceImpl::~SessionStorageNamespaceImpl() {
}
