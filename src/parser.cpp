#include "kvstore/parser.h"

namespace kvstore {

std::vector<std::string> ParseLine(const std::string& line) {
  std::vector<std::string> tokens;
  std::string current;

  for (size_t i = 0; i < line.size(); i++) {
    const char c = line[i];

    if (c == ' ') {
      if (current.empty()) continue;
      if (tokens.empty()) {
        tokens.push_back(current);
        current.clear();
      } else {
        current.push_back(' ');
      }
      continue;
    }

    if (c == ',') {
      while (!current.empty() && current.back() == ' ') current.pop_back();
      if (!current.empty()) tokens.push_back(current);
      current.clear();
      continue;
    }

    current.push_back(c);
  }

  while (!current.empty() && current.back() == ' ') current.pop_back();
  if (!current.empty()) tokens.push_back(current);

  return tokens;
}

}  // namespace kvstore
