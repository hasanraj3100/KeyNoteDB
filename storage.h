#pragma once
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Storage {
private:
  std::map<std::string, std::string> db;

public:
  bool Put(const std::string &key, const std::string &value);

  std::optional<std::string> Get(const std::string &key) const;

  std::vector<std::pair<std::string, std::string>> GetAll() const;

  bool Delete(const std::string &key);
};
