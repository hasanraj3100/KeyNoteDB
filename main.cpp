#include "command.h"
#include "parse.h"
#include "storage.h"
#include <climits>
#include <iostream>
#include <string>
#include <vector>

bool handleInput(Storage &storage) {
  std::string line;
  if (!std::getline(std::cin, line)) {
    return false;
  }

  std::vector<std::string> tokens = parseLine(line);
  std::cout << processTokens(tokens, storage) << std::endl;
  return true;
}

int main() {

  Storage storage("log.txt");

  while (handleInput(storage)) {
  }
  return 0;
}
