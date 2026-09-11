#include "command.h"
#include "parse.h"
#include <iostream>
#include <string>
#include <vector>

bool handleInput() {
  std::string line;
  if (!std::getline(std::cin, line)) {
    return false;
  }

  std::vector<std::string> tokens = parseLine(line);
  std::cout << processTokens(tokens) << std::endl;
  return true;
}

int main() {

  while (handleInput()) {
  }
  return 0;
}
