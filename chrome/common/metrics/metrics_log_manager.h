// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_METRICS_METRICS_LOG_MANAGER_H_
#define CHROME_COMMON_METRICS_METRICS_LOG_MANAGER_H_
#pragma once

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"

#include <string>
#include <vector>

class MetricsLogBase;

// Manages all the log objects used by a MetricsService implementation. Keeps
// track of both an in progress log and a log that is staged for uploading as
// text, as well as saving logs to, and loading logs from, persistent storage.
class MetricsLogManager {
 public:
  MetricsLogManager();
  ~MetricsLogManager();

  // Stores both XML and protocol buffer serializations for a log.
  struct SerializedLog {
   public:
    // Exposed to reduce code churn as we transition from the XML pipeline to
    // the protocol buffer pipeline.
    bool empty() const;
    void swap(SerializedLog& log);

    std::string xml;
    std::string proto;
  };

  enum LogType {
    INITIAL_LOG,  // The first log of a session.
    ONGOING_LOG,  // Subsequent logs in a session.
    NO_LOG,       // Placeholder value for when there is no log.
  };

  // Takes ownership of |log|, which has type |log_type|, and makes it the
  // current_log. This should only be called if there is not a current log.
  void BeginLoggingWithLog(MetricsLogBase* log, LogType log_type);

  // Returns the in-progress log.
  MetricsLogBase* current_log() { return current_log_.get(); }

  // Closes current_log(), compresses it, and stores the compressed log for
  // later, leaving current_log() NULL.
  void FinishCurrentLog();

  // Returns true if there are any logs waiting to be uploaded.
  bool has_unsent_logs() const {
    return !unsent_initial_logs_.empty() || !unsent_ongoing_logs_.empty();
  }

  // Populates staged_log_text() with the next stored log to send.
  // Should only be called if has_unsent_logs() is true.
  void StageNextLogForUpload();

  // Returns true if there is a log that needs to be, or is being, uploaded.
  bool has_staged_log() const;

  // Returns true if there is a protobuf log that needs to be uploaded.
  // In the case that an XML upload needs to be re-issued due to a previous
  // failure, has_staged_log() will return true while this returns false.
  bool has_staged_log_proto() const;

  // The text of the staged log, in compressed XML or protobuf format. Empty if
  // there is no staged log, or if compression of the staged log failed.
  const SerializedLog& staged_log_text() const {
    return staged_log_text_;
  }

  // Discards the staged log (both the XML and the protobuf data).
  void DiscardStagedLog();

  // Discards the protobuf data in the staged log.
  // This is useful to prevent needlessly re-issuing successful protobuf uploads
  // due to XML upload failures.
  void DiscardStagedLogProto();

  // Closes and discards |current_log|.
  void DiscardCurrentLog();

  // Sets current_log to NULL, but saves the current log for future use with
  // ResumePausedLog(). Only one log may be paused at a time.
  // TODO(stuartmorgan): Pause/resume support is really a workaround for a
  // design issue in initial log writing; that should be fixed, and pause/resume
  // removed.
  void PauseCurrentLog();

  // Restores the previously paused log (if any) to current_log().
  // This should only be called if there is not a current log.
  void ResumePausedLog();

  // Saves the staged log, then clears staged_log().
  // This can only be called if has_staged_log() is true.
  void StoreStagedLogAsUnsent();

  // Sets the threshold for how large an onging log can be and still be written
  // to persistant storage. Ongoing logs larger than this will be discarded
  // before persisting. 0 is interpreted as no limit.
  void set_max_ongoing_log_store_size(size_t max_size) {
    max_ongoing_log_store_size_ = max_size;
  }

  // Interface for a utility class to serialize and deserialize logs for
  // persistent storage.
  class LogSerializer {
   public:
    virtual ~LogSerializer() {}

    // Serializes |logs| to persistent storage, replacing any previously
    // serialized logs of the same type.
    virtual void SerializeLogs(const std::vector<SerializedLog>& logs,
                               LogType log_type) = 0;

    // Populates |logs| with logs of type |log_type| deserialized from
    // persistent storage.
    virtual void DeserializeLogs(LogType log_type,
                                 std::vector<SerializedLog>* logs) = 0;
  };

  // Sets the serializer to use for persisting and loading logs; takes ownership
  // of |serializer|.
  void set_log_serializer(LogSerializer* serializer) {
    log_serializer_.reset(serializer);
  }

  // Saves any unsent logs to persistent storage using the current log
  // serializer. Can only be called after set_log_serializer.
  void PersistUnsentLogs();

  // Loads any unsent logs from persistent storage using the current log
  // serializer. Can only be called after set_log_serializer.
  void LoadPersistedUnsentLogs();

 private:
  // Saves |log_text| as the given type (or discards it in accordance with
  // |max_ongoing_log_store_size_|).
  // NOTE: This clears the contents of |log_text| (to avoid an expensive
  // string copy), so the log should be discarded after this call.
  void StoreLog(SerializedLog* log_text, LogType log_type);

  // Compresses current_log_ into compressed_log.
  void CompressCurrentLog(SerializedLog* compressed_log);

  // Compresses the text in |input| using bzip2, store the result in |output|.
  static bool Bzip2Compress(const std::string& input, std::string* output);

  // The log that we are still appending to.
  scoped_ptr<MetricsLogBase> current_log_;
  LogType current_log_type_;

  // A paused, previously-current log.
  scoped_ptr<MetricsLogBase> paused_log_;
  LogType paused_log_type_;

  // Helper class to handle serialization/deserialization of logs for persistent
  // storage. May be NULL.
  scoped_ptr<LogSerializer> log_serializer_;

  // The text representations of the staged log, ready for upload to the server.
  // The first item in the pair is the compressed XML representation; the second
  // is the protobuf representation.
  SerializedLog staged_log_text_;
  LogType staged_log_type_;

  // Logs from a previous session that have not yet been sent.
  // The first item in each pair is the XML representation; the second item is
  // the protobuf representation.
  // Note that the vector has the oldest logs listed first (early in the
  // vector), and we'll discard old logs if we have gathered too many logs.
  std::vector<SerializedLog> unsent_initial_logs_;
  std::vector<SerializedLog> unsent_ongoing_logs_;

  size_t max_ongoing_log_store_size_;

  DISALLOW_COPY_AND_ASSIGN(MetricsLogManager);
};

#endif  // CHROME_COMMON_METRICS_METRICS_LOG_MANAGER_H_
