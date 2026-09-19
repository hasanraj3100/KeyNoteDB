#include "kvstore/storage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
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
    throw std::runtime_error("kvstore: failed to open log file " +
                              log_path_);
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

  std::ofstream snapshot_file(snapshot_path_, std::ios::trunc);
  if (!snapshot_file) {
    std::cerr << "kvstore: failed to write snapshot for " << log_path_
              << std::endl;
    return;
  }

  for (const auto &[key, value] : entries_) {
    snapshot_file << key << "," << value << "\n";
  }

  unsnapshotted_op_count_ = 0;

  // clearing the log file since snapshot has the data
  std::filesystem::resize_file(log_path_, 0);
}

} // namespace kvstore
