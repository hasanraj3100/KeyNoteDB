#include "kvstore/storage.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <utility>

#include "kvstore/parser.h"

namespace kvstore {
namespace {

// Snapshot lines are "key,value" with no surrounding whitespace -- unlike
// log lines, which go through ParseLine and allow spaces around the comma.
std::pair<std::string, std::string> SplitOnFirstComma(const std::string &line) {
  const size_t comma = line.find(',');
  if (comma == std::string::npos)
    return {line, ""};
  return {line.substr(0, comma), line.substr(comma + 1)};
}

} // namespace

Storage::Storage(const std::string &base_path)
    : log_path_(base_path + ".log"), snapshot_path_(base_path + ".snapshot") {
  log_file_.open(log_path_, std::ios::in | std::ios::out | std::ios::app);
  if (!log_file_) {
    throw std::runtime_error("kvstore: failed to open log file " + log_path_);
  }

  RecoverFromSnapshot();
  ReplayLogFromOffset();
}

bool Storage::Put(const std::string &key, const std::string &value) {
  entries_[key] = value;
  AppendToLog("INSERT " + key + ", " + value);
  return true;
}

std::optional<std::string> Storage::Get(const std::string &key) const {
  const auto it = entries_.find(key);
  if (it == entries_.end())
    return std::nullopt;
  return it->second;
}

std::vector<std::pair<std::string, std::string>> Storage::GetAll() const {
  std::vector<std::pair<std::string, std::string>> result;
  result.reserve(entries_.size());
  for (const auto &[key, value] : entries_) {
    result.emplace_back(key, value);
  }
  return result;
}

bool Storage::Delete(const std::string &key) {
  const bool existed = entries_.erase(key) > 0;
  if (existed) {
    AppendToLog("DELETE " + key);
  }
  return existed;
}

void Storage::ApplyPut(const std::string &key, const std::string &value) {
  entries_[key] = value;
}

void Storage::ApplyDelete(const std::string &key) { entries_.erase(key); }

void Storage::RecoverFromSnapshot() {
  std::ifstream snapshot_file(snapshot_path_);

  if (!snapshot_file)
    return;

  std::string line;
  while (std::getline(snapshot_file, line)) {
    const auto [key, value] = SplitOnFirstComma(line);
    entries_[key] = value;
  }
}

void Storage::ReplayLogFromOffset() {
  log_file_.clear();
  log_file_.seekg(0);

  std::string line;
  while (std::getline(log_file_, line)) {

    const std::vector<std::string> tokens = ParseLine(line);
    if (tokens.size() == 3 && tokens[0] == "INSERT") {
      ApplyPut(tokens[1], tokens[2]);
    } else if (tokens.size() == 2 && tokens[0] == "DELETE") {
      ApplyDelete(tokens[1]);
    }

    unsnapshotted_op_count_++;
  }

  log_file_.clear();
}

void Storage::AppendToLog(const std::string &command) {
  log_file_ << command << std::endl;
  unsnapshotted_op_count_++;
  MaybeWriteSnapshot();
}

void Storage::MaybeWriteSnapshot() {
  if (unsnapshotted_op_count_ < kSnapshotThreshold)
    return;

  const std::string temp_path = snapshot_path_ + ".temp";

  std::unique_ptr<FILE, decltype(&std::fclose)> file(
      std::fopen(temp_path.c_str(), "w"), &std::fclose);

  if (!file) {
    std::cerr << "kvstore: failed to open temp snapshot " << temp_path << ": "
              << strerror(errno) << '\n';
    return;
  }

  bool ok = true;

  for (const auto &[key, value] : entries_) {
    int written = fprintf(file.get(), "%s,%s\n", key.c_str(), value.c_str());

    if (written < 0) {
      std::cerr << "kvstore: failed to write snapshot " << temp_path << ": "
                << strerror(errno) << '\n';
      ok = false;
      break;
    }
  }

  if (ok && fflush(file.get()) != 0) {
    std::cerr << "kvstore: failed to flush snapshot " << temp_path << ": "
              << strerror(errno) << '\n';
    ok = false;
  }

  if (ok && fsync(fileno(file.get())) != 0) {
    std::cerr << "kvstore: failed to fsync snapshot " << temp_path << ": "
              << strerror(errno) << '\n';
    ok = false;
  }

  if (fclose(file.release()) != 0) {
    std::cerr << "kvstore: failed to close snapshot " << temp_path << ": "
              << strerror(errno) << '\n';
    ok = false;
  }

  if (!ok) {
    remove(temp_path.c_str());
    return;
  }

  if (rename(temp_path.c_str(), snapshot_path_.c_str()) != 0) {
    std::cerr << "kvstore: failed to rename " << temp_path << " to "
              << snapshot_path_ << ": " << strerror(errno) << '\n';
    remove(temp_path.c_str());
    return;
  }

  std::filesystem::path parent =
      std::filesystem::path(snapshot_path_).parent_path();

  if (parent.empty())
    parent = ".";

  int dir_fd = open(parent.c_str(), O_RDONLY | O_DIRECTORY);

  if (dir_fd < 0) {
    std::cerr << "kvstore: failed to open directory " << parent.string() << ": "
              << strerror(errno) << '\n';
    return;
  }

  bool dir_ok = (fsync(dir_fd) == 0);

  if (close(dir_fd) != 0)
    dir_ok = false;

  if (!dir_ok) {
    std::cerr << "kvstore: failed to fsync directory " << parent.string() << '\n';
    return;
  }

  int wal_fd = open(log_path_.c_str(), O_WRONLY);

  if (wal_fd < 0) {
    std::cerr << "kvstore: failed to open log " << log_path_
              << " for truncation: " << strerror(errno) << '\n';
    return;
  }

  bool wal_ok = (ftruncate(wal_fd, 0) == 0);

  if (wal_ok && fsync(wal_fd) != 0)
    wal_ok = false;

  if (close(wal_fd) != 0)
    wal_ok = false;

  if (!wal_ok) {
    std::cerr << "kvstore: failed to truncate log " << log_path_ << '\n';
    return;
  }

  log_file_.clear();
  unsnapshotted_op_count_ = 0;
}

} // namespace kvstore
