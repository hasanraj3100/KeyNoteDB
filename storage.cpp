#include "storage.h"
#include "parse.h"
#include <cstdio>
#include <ios>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

void Storage::applyPut(const std::string &key, const std::string &value) {
  db[key] = value;
}

void Storage::applyDelete(const std::string &key) { db.erase(key); }

void Storage::getDataInMemory() {
  file.clear();
  file.seekg(0, std::ios::beg);

  std::string line;
  while (getline(file, line)) {
    std::vector<std::string> parsed = parseLine(line);

    if (parsed[0] == "INSERT")
      applyPut(parsed[1], parsed[2]);
    else if (parsed[0] == "DELETE")
      applyDelete(parsed[1]);
  }

  file.clear();
}

Storage::Storage(std::string fileName) {
  file.open(fileName, std::ios::in | std::ios::out | std::ios::app);
  getDataInMemory();
}

bool Storage::Put(const std::string &key, const std::string &value) {
  db[key] = value;
  file << "INSERT " << key << ", " << value << std::endl;
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

bool Storage::Delete(const std::string &key) {
  file << "DELETE " << key << std::endl;
  return db.erase(key);
}
