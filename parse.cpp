#include "parse.h"

std::vector<std::string> parseLine(const std::string &line) {
  std::vector<std::string> res;

  std::string curr;

  for (int i = 0; i < line.size(); i++) {
    if (line[i] == ' ' || line[i] == ',') {
      if (!curr.empty())
        res.push_back(curr);
      curr.clear();
      continue;
    }
    curr.push_back(line[i]);
  }

  if (!curr.empty())
    res.push_back(curr);

  return res;
}
