#pragma once

#include <string>
#include <vector>

namespace kvstore {

// Splits a raw input line into command tokens.
//
// Only the very first space (splitting the command from the rest of the
// line) acts as a delimiter; after that, commas are the only delimiter,
// which is what lets a key or value contain spaces. Leading and trailing
// spaces around a comma (or at the start/end of the line) are stripped;
// interior spaces are preserved as-is.
//
// Examples:
//   ParseLine("INSERT key, value")       -> {"INSERT", "key", "value"}
//   ParseLine("INSERT key value")        -> {"INSERT", "key value"}
//   ParseLine("GET key")                 -> {"GET", "key"}
//
// ParseLine does not enforce a token-count limit; validating the number of
// tokens for a given command is the caller's job (see ProcessCommand).
std::vector<std::string> ParseLine(const std::string& line);

}  // namespace kvstore
