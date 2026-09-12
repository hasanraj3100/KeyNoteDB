#pragma once
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Storage {
private:
  std::map<std::string, std::string> db;
  std::fstream file;

  void applyPut(const std::string &key, const std::string &value);
  void applyDelete(const std::string &key);
  void getDataInMemory();

public:
  Storage(std::string fileName);
  bool Put(const std::string &key, const std::string &value);
  std::optional<std::string> Get(const std::string &key) const;
  std::vector<std::pair<std::string, std::string>> GetAll() const;
  bool Delete(const std::string &key);
};
