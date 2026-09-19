#include "kvstore/command_processor.h"

#include <algorithm>
#include <cctype>

namespace kvstore {
namespace {

std::string FormatEntry(const std::pair<std::string, std::string> &entry) {
  return entry.first + "=" + entry.second;
}

std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

} // namespace

std::string ProcessCommand(const std::vector<std::string> &tokens,
                           Storage &storage) {
  if (tokens.empty() || tokens.size() > 3) {
    return "ERR: invalid input length";
  }

  const std::string command = ToLower(tokens[0]);

  if (tokens.size() == 3) {
    if (command != "insert")
      return "ERR: invalid command";

    const std::string &key = tokens[1];
    const std::string &value = tokens[2];

    bool stored = storage.Put(key, value);

    if (stored)
      return "OK: stored " + key + "=" + value;

    return "ERR: could not store " + key + "=" + value;
  }

  if (tokens.size() == 2) {
    if (command == "get") {
      const auto value = storage.Get(tokens[1]);
      if (!value.has_value())
        return "ERR: key not found";
      return "OK: " + FormatEntry({tokens[1], *value});
    }

    if (command == "delete") {
      if (storage.Delete(tokens[1]))
        return "OK: deleted " + tokens[1];
      return "ERR: invalid delete key";
    }

    return "ERR: invalid command";
  }

  // tokens.size() == 1
  if (command != "scan")
    return "ERR: invalid command";

  const auto entries = storage.GetAll();
  if (entries.empty())
    return "OK: scanning ... (empty)";

  std::string result = "OK: scanning ...\n";
  for (const auto &entry : entries) {
    result += " " + FormatEntry(entry) + "\n";
  }
  return result;
}

} // namespace kvstore
