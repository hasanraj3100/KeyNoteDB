#include "command.h"
#include "storage.h"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

static std::string formatEntry(const std::pair<std::string, std::string> &kv) {
  return kv.first + "=" + kv.second;
}

std::string processTokens(const std::vector<std::string> &tokens,
                          Storage &storage) {

  if (tokens.empty() || tokens.size() > 3) {
    return "ERR: invalid input length";
  }

  std::string cmd = tokens[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (tokens.size() == 3) {
    if (cmd != "insert") {
      return "ERR: invalid command";
    }

    std::string key = tokens[1];
    std::string value = tokens[2];

    storage.Put(key, value);

    return "OK: stored " + key + "=" + value;
  }

  if (tokens.size() == 2) {
    if (cmd == "get") {
      std::string key = tokens[1];
      auto value = storage.Get(key);

      if (!value.has_value())
        return "ERR: key not found";

      return "OK: " + formatEntry({key, value.value()});
    } else if (cmd == "delete") {
      std::string key = tokens[1];

      if (storage.Delete(key))
        return "OK: deleted " + key;

      return "ERR: invalid delete key";
    }

    return "ERR: invalid command";
  }

  if (tokens.size() == 1) {
    if (cmd != "scan") {
      return "ERR: invalid command";
    }

    std::vector<std::pair<std::string, std::string>> data = storage.GetAll();
    if (data.empty())
      return "OK: scanning ... (empty)";

    std::string result = "OK: scanning ...";
    for (const auto &kv : data) {
      result += " " + formatEntry(kv);
    }
    return result;
  }

  return "ERR: invalid command";
}
