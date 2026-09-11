#include "storage.h"
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

bool Storage::Put(const std::string &key, const std::string &value) {
  db[key] = value;
  return true;
}

std::optional<std::string> Storage::Get(const std::string &key) const {
  auto valuePtr = db.find(key);

  if (valuePtr != db.end())
    return (*valuePtr).second;
  return std::nullopt;
}

std::vector<std::pair<std::string, std::string>> Storage::GetAll() const {
  std::vector<std::pair<std::string, std::string>> res;

  for (const auto &[i, j] : db) {
    res.push_back({i, j});
  }

  return res;
}

bool Storage::Delete(const std::string &key) { return db.erase(key); }
