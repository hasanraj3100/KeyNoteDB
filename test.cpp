#include "command.h"
#include "parse.h"
#include <iostream>
#include <string>
#include <vector>

namespace {

int testsRun = 0;
int testsFailed = 0;

std::string toString(const std::vector<std::string> &tokens) {
  std::string out = "{";
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i)
      out += ", ";
    out += "\"" + tokens[i] + "\"";
  }
  out += "}";
  return out;
}

void expectEq(const std::string &testName,
              const std::vector<std::string> &actual,
              const std::vector<std::string> &expected) {
  testsRun++;
  if (actual == expected) {
    std::cout << "[PASS] " << testName << std::endl;
    return;
  }

  testsFailed++;
  std::cout << "[FAIL] " << testName << std::endl;
  std::cout << "       expected: " << toString(expected) << std::endl;
  std::cout << "       actual:   " << toString(actual) << std::endl;
}

void expectEq(const std::string &testName, const std::string &actual,
              const std::string &expected) {
  testsRun++;
  if (actual == expected) {
    std::cout << "[PASS] " << testName << std::endl;
    return;
  }

  testsFailed++;
  std::cout << "[FAIL] " << testName << std::endl;
  std::cout << "       expected: \"" << expected << "\"" << std::endl;
  std::cout << "       actual:   \"" << actual << "\"" << std::endl;
}

} // namespace

void runTests() {
  expectEq("insert with comma and space separates into 3 tokens",
           parseLine("INSERT key, value"), {"INSERT", "key", "value"});

  expectEq("insert with comma only, no space", parseLine("INSERT key,value"),
           {"INSERT", "key", "value"});

  expectEq("insert with space only, no comma", parseLine("INSERT key value"),
           {"INSERT", "key", "value"});

  expectEq("get command produces 2 tokens", parseLine("GET key"),
           {"GET", "key"});

  expectEq("delete command produces 2 tokens", parseLine("DELETE key"),
           {"DELETE", "key"});

  expectEq("scan command produces 1 token", parseLine("SCAN"), {"SCAN"});

  expectEq("empty string produces no tokens", parseLine(""), {});

  expectEq("string of only spaces produces no tokens", parseLine("   "), {});

  expectEq("string of only commas produces no tokens", parseLine(",,,"), {});

  expectEq("repeated spaces between tokens collapse to one split",
           parseLine("INSERT   key    value"), {"INSERT", "key", "value"});

  expectEq("repeated commas between tokens collapse to one split",
           parseLine("INSERT key,,,value"), {"INSERT", "key", "value"});

  expectEq("leading whitespace is ignored", parseLine("   INSERT key value"),
           {"INSERT", "key", "value"});

  expectEq("trailing whitespace is ignored", parseLine("INSERT key value   "),
           {"INSERT", "key", "value"});

  expectEq("trailing comma is ignored", parseLine("INSERT key,value,"),
           {"INSERT", "key", "value"});

  expectEq("comma surrounded by spaces still separates tokens",
           parseLine("INSERT key , value"), {"INSERT", "key", "value"});

  expectEq("parseLine preserves the original case of every token",
           parseLine("insert Key Value"), {"insert", "Key", "Value"});

  expectEq("single-character tokens are kept", parseLine("a b c"),
           {"a", "b", "c"});

  expectEq("parseLine does not enforce a token-count limit itself; "
           "that's handleInput's job, not the parser's",
           parseLine("INSERT key value extra"),
           {"INSERT", "key", "value", "extra"});
}

void runProcessTokensTests() {
  expectEq("insert with 3 tokens stores the key=value pair",
           processTokens({"INSERT", "key", "value"}), "OK: stored key=value");

  expectEq("insert command name is case-insensitive",
           processTokens({"InSeRt", "key", "value"}), "OK: stored key=value");

  expectEq("3 tokens with a command other than insert is rejected",
           processTokens({"GET", "key", "extra"}), "ERR: invalid command");

  expectEq("get with 2 tokens returns the key", processTokens({"GET", "key"}),
           "OK: getting key");

  expectEq("get command name is case-insensitive",
           processTokens({"gEt", "key"}), "OK: getting key");

  expectEq("delete with 2 tokens returns the key",
           processTokens({"DELETE", "key"}), "OK: deleted key");

  expectEq("delete command name is case-insensitive",
           processTokens({"delete", "key"}), "OK: deleted key");

  expectEq("2 tokens with an unrecognized command is rejected",
           processTokens({"FOO", "key"}), "ERR: invalid command");

  expectEq("scan with 1 token is accepted", processTokens({"SCAN"}),
           "OK: scanning ...");

  expectEq("scan command name is case-insensitive", processTokens({"scan"}),
           "OK: scanning ...");

  expectEq("1 token that isn't scan is rejected", processTokens({"FOO"}),
           "ERR: invalid command");

  expectEq("empty token list is rejected as invalid length", processTokens({}),
           "ERR: invalid input length");

  expectEq("more than 3 tokens is rejected as invalid length",
           processTokens({"INSERT", "key", "value", "extra"}),
           "ERR: invalid input length");
}

int main() {
  runTests();
  runProcessTokensTests();

  std::cout << std::endl;
  std::cout << (testsRun - testsFailed) << "/" << testsRun << " tests passed"
            << std::endl;

  return testsFailed == 0 ? 0 : 1;
}
