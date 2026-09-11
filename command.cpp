#include "command.h"
#include <algorithm>
#include <string>
#include <vector>

std::string processTokens(const std::vector<std::string> &tokens) {

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

    return "OK: stored " + key + "=" + value;
  }

  if (tokens.size() == 2) {
    if (cmd == "get") {
      std::string key = tokens[1];
      return "OK: getting " + key;
    } else if (cmd == "delete") {
      std::string key = tokens[1];
      return "OK: deleted " + key;
    }

    return "ERR: invalid command";
  }

  if (tokens.size() == 1) {
    if (cmd != "scan") {
      return "ERR: invalid command";
    }

    return "OK: scanning ...";
  }

  return "ERR: invalid command";
}
