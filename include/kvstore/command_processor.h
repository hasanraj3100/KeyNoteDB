#pragma once

#include <string>
#include <vector>

#include "kvstore/storage.h"

namespace kvstore {

// Executes a single parsed command (INSERT/GET/DELETE/SCAN) against
// storage and returns a human-readable response line, e.g. an "OK: ..."
// or "ERR: ..." message.
std::string ProcessCommand(const std::vector<std::string> &tokens,
                           Storage &storage);

} // namespace kvstore
