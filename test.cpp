#include "command.h"
#include "parse.h"
#include "storage.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

int testsRun = 0;
int testsFailed = 0;
int tempFileCounter = 0;

std::string makeTempStorageFile() {
  std::string fileName = "test_tmp_" + std::to_string(tempFileCounter++) + ".log";
  std::remove(fileName.c_str());
  return fileName;
}

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
  {
    Storage storage(makeTempStorageFile());
    expectEq("insert with 3 tokens stores the key=value pair",
             processTokens({"INSERT", "key", "value"}, storage),
             "OK: stored key=value");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("insert command name is case-insensitive",
             processTokens({"InSeRt", "key", "value"}, storage),
             "OK: stored key=value");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("3 tokens with a command other than insert is rejected",
             processTokens({"GET", "key", "extra"}, storage),
             "ERR: invalid command");
  }

  {
    Storage storage(makeTempStorageFile());
    processTokens({"INSERT", "key", "value"}, storage);
    expectEq("get with 2 tokens returns the stored value",
             processTokens({"GET", "key"}, storage), "OK: key=value");
  }

  {
    Storage storage(makeTempStorageFile());
    processTokens({"INSERT", "key", "value"}, storage);
    expectEq("get command name is case-insensitive",
             processTokens({"gEt", "key"}, storage), "OK: key=value");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("get on a missing key is rejected",
             processTokens({"GET", "key"}, storage), "ERR: key not found");
  }

  {
    Storage storage(makeTempStorageFile());
    processTokens({"INSERT", "key", "value"}, storage);
    expectEq("delete with 2 tokens removes the key",
             processTokens({"DELETE", "key"}, storage), "OK: deleted key");
  }

  {
    Storage storage(makeTempStorageFile());
    processTokens({"INSERT", "key", "value"}, storage);
    expectEq("delete command name is case-insensitive",
             processTokens({"delete", "key"}, storage), "OK: deleted key");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("delete on a missing key is rejected",
             processTokens({"DELETE", "key"}, storage),
             "ERR: invalid delete key");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("2 tokens with an unrecognized command is rejected",
             processTokens({"FOO", "key"}, storage), "ERR: invalid command");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("scan with no stored data reports empty",
             processTokens({"SCAN"}, storage), "OK: scanning ... (empty)");
  }

  {
    Storage storage(makeTempStorageFile());
    processTokens({"INSERT", "a", "1"}, storage);
    processTokens({"INSERT", "b", "2"}, storage);
    expectEq("scan with stored data returns every key=value pair",
             processTokens({"SCAN"}, storage), "OK: scanning ... a=1 b=2");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("scan command name is case-insensitive",
             processTokens({"scan"}, storage), "OK: scanning ... (empty)");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("1 token that isn't scan is rejected",
             processTokens({"FOO"}, storage), "ERR: invalid command");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("empty token list is rejected as invalid length",
             processTokens({}, storage), "ERR: invalid input length");
  }

  {
    Storage storage(makeTempStorageFile());
    expectEq("more than 3 tokens is rejected as invalid length",
             processTokens({"INSERT", "key", "value", "extra"}, storage),
             "ERR: invalid input length");
  }
}

int main() {
  runTests();
  runProcessTokensTests();

  std::cout << std::endl;
  std::cout << (testsRun - testsFailed) << "/" << testsRun << " tests passed"
            << std::endl;

  for (int i = 0; i < tempFileCounter; i++) {
    std::remove(("test_tmp_" + std::to_string(i) + ".log").c_str());
  }

  return testsFailed == 0 ? 0 : 1;
}
