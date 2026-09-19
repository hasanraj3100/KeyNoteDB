#pragma once

#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kvstore {

// A key-value store backed by a write-ahead log.
//
// Every Put/Delete is appended to "<base_path>.log" before it takes effect
// in memory. Once the log accumulates enough operations since the last
// snapshot, the full in-memory state is written out to
// "<base_path>.snapshot" and the log is truncated back to empty -- so the
// log always holds exactly the operations applied since the last snapshot,
// never more.
//
// On construction, the store restores its in-memory state by loading
// "<base_path>.snapshot" (if it exists) and then replaying every entry in
// "<base_path>.log" on top of it. Because the log is truncated on every
// snapshot, replaying it from the beginning is always replaying only the
// operations since the last snapshot -- there's no need to track or skip
// to an offset.
class Storage {
public:
  // base_path is a filesystem path prefix; the store derives
  // "<base_path>.log" and "<base_path>.snapshot" from it.
  // Throws std::runtime_error if the log file can't be opened.
  explicit Storage(const std::string &base_path);

  bool Put(const std::string &key, const std::string &value);
  std::optional<std::string> Get(const std::string &key) const;
  std::vector<std::pair<std::string, std::string>> GetAll() const;
  bool Delete(const std::string &key);

private:
  // Number of unsnapshotted operations that triggers a new snapshot.
  static constexpr int kSnapshotThreshold = 10;

  void RecoverFromSnapshot();
  void ReplayLogFromOffset();
  void AppendToLog(const std::string &command);
  void MaybeWriteSnapshot();

  void ApplyPut(const std::string &key, const std::string &value);
  void ApplyDelete(const std::string &key);

  std::map<std::string, std::string> entries_;
  std::fstream log_file_;

  std::string log_path_;
  std::string snapshot_path_;

  int unsnapshotted_op_count_ = 0;
};

} // namespace kvstore
