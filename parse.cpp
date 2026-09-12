#include "parse.h"

std::vector<std::string> parseLine(const std::string &line) {
  std::vector<std::string> res;

  std::string curr;

  for (int i = 0; i < line.size(); i++) {
    if (line[i] == ' ') {
      if (curr.empty())
        continue;
      if (res.empty()) {
        res.push_back(curr);
        curr.clear();
      } else {
        curr.push_back(' ');
      }
      continue;
    } else if (line[i] == ',') {
      while (!curr.empty() && curr.back() == ' ')
        curr.pop_back();
      if (!curr.empty()) {
        res.push_back(curr);
      }
      curr.clear();
      continue;
    }
    curr.push_back(line[i]);
  }

  // ignore trailing spaces
  while (!curr.empty() && curr.back() == ' ')
    curr.pop_back();

  if (!curr.empty())
    res.push_back(curr);

  return res;
}
