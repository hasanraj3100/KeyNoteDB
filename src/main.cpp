#include <iostream>
#include <string>
#include <vector>

#include "kvstore/command_processor.h"
#include "kvstore/parser.h"
#include "kvstore/storage.h"

namespace kvstore {
namespace {

bool HandleNextLine(Storage& storage) {
  std::string line;
  if (!std::getline(std::cin, line)) return false;

  const std::vector<std::string> tokens = ParseLine(line);
  std::cout << ProcessCommand(tokens, storage) << std::endl;
  return true;
}

}  // namespace
}  // namespace kvstore

int main() {
  kvstore::Storage storage("kvstore_data");

  while (kvstore::HandleNextLine(storage)) {
  }

  return 0;
}
