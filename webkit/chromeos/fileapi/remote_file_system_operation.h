// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_CHROMEOS_FILEAPI_REMOTE_FILE_SYSTEM_OPERATION_H_
#define WEBKIT_CHROMEOS_FILEAPI_REMOTE_FILE_SYSTEM_OPERATION_H_

#include "webkit/chromeos/fileapi/remote_file_system_proxy.h"
#include "webkit/fileapi/file_system_operation_interface.h"

namespace base {
class Value;
}

namespace fileapi {
class FileSystemOperation;
}

namespace chromeos {

// FileSystemOperation implementation for local file systems.
class RemoteFileSystemOperation : public fileapi::FileSystemOperationInterface {
 public:
  virtual ~RemoteFileSystemOperation();

  // FileSystemOperationInterface overrides.
  virtual void CreateFile(const GURL& path,
                          bool exclusive,
                          const StatusCallback& callback) OVERRIDE;
  virtual void CreateDirectory(const GURL& path,
                               bool exclusive,
                               bool recursive,
                               const StatusCallback& callback) OVERRIDE;
  virtual void Copy(const GURL& src_path,
                    const GURL& dest_path,
                    const StatusCallback& callback) OVERRIDE;
  virtual void Move(const GURL& src_path,
                    const GURL& dest_path,
                    const StatusCallback& callback) OVERRIDE;
  virtual void DirectoryExists(const GURL& path,
                               const StatusCallback& callback) OVERRIDE;
  virtual void FileExists(const GURL& path,
                          const StatusCallback& callback) OVERRIDE;
  virtual void GetMetadata(const GURL& path,
                           const GetMetadataCallback& callback) OVERRIDE;
  virtual void ReadDirectory(const GURL& path,
                             const ReadDirectoryCallback& callback) OVERRIDE;
  virtual void Remove(const GURL& path, bool recursive,
                      const StatusCallback& callback) OVERRIDE;
  virtual void Write(const net::URLRequestContext* url_request_context,
                     const GURL& path,
                     const GURL& blob_url,
                     int64 offset,
                     const WriteCallback& callback) OVERRIDE;
  virtual void Truncate(const GURL& path, int64 length,
                        const StatusCallback& callback) OVERRIDE;
  virtual void Cancel(const StatusCallback& cancel_callback) OVERRIDE;
  virtual void TouchFile(const GURL& path,
                         const base::Time& last_access_time,
                         const base::Time& last_modified_time,
                         const StatusCallback& callback) OVERRIDE;
  virtual void OpenFile(
      const GURL& path,
      int file_flags,
      base::ProcessHandle peer_handle,
      const OpenFileCallback& callback) OVERRIDE;
  virtual fileapi::FileSystemOperation* AsFileSystemOperation() OVERRIDE;
  virtual void CreateSnapshotFile(
      const GURL& path,
      const SnapshotFileCallback& callback) OVERRIDE;

 private:
  friend class CrosMountPointProvider;

  RemoteFileSystemOperation(
      scoped_refptr<fileapi::RemoteFileSystemProxyInterface> remote_proxy);

  // Used only for internal assertions.
  // Returns false if there's another inflight pending operation.
  bool SetPendingOperationType(OperationType type);

  // Generic callback that translates platform errors to WebKit error codes.
  void DidDirectoryExists(const StatusCallback& callback,
                          base::PlatformFileError rv,
                          const base::PlatformFileInfo& file_info,
                          const FilePath& unused);
  void DidFileExists(const StatusCallback& callback,
                     base::PlatformFileError rv,
                     const base::PlatformFileInfo& file_info,
                     const FilePath& unused);
  void DidGetMetadata(const GetMetadataCallback& callback,
                      base::PlatformFileError rv,
                      const base::PlatformFileInfo& file_info,
                      const FilePath& platform_path);
  void DidReadDirectory(
      const ReadDirectoryCallback& callback,
      base::PlatformFileError rv,
      const std::vector<base::FileUtilProxy::Entry>& entries,
      bool has_more);
  void DidFinishFileOperation(const StatusCallback& callback,
                              base::PlatformFileError rv);
  void DidCreateSnapshotFile(
      const SnapshotFileCallback& callback,
      base::PlatformFileError result,
      const base::PlatformFileInfo& file_info,
      const FilePath& platform_path,
      const scoped_refptr<webkit_blob::ShareableFileReference>& file_ref);


  scoped_refptr<fileapi::RemoteFileSystemProxyInterface> remote_proxy_;
  // A flag to make sure we call operation only once per instance.
  OperationType pending_operation_;

  DISALLOW_COPY_AND_ASSIGN(RemoteFileSystemOperation);
};

}  // namespace chromeos

#endif  // WEBKIT_CHROMEOS_FILEAPI_REMOTE_FILE_SYSTEM_OPERATION_H_
