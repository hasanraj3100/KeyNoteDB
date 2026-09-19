#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "kvstore/command_processor.h"
#include "kvstore/parser.h"
#include "kvstore/storage.h"

namespace kvstore {
namespace {

int g_tests_run = 0;
int g_tests_failed = 0;
int g_temp_file_counter = 0;

// Returns a fresh base path for a Storage instance, e.g. "test_tmp_0".
// Storage derives "<base>.log" and "<base>.snapshot" from it;
// RemoveAllTempFiles() cleans up both.
std::string MakeTempStorageBasePath() {
  return "test_tmp_" + std::to_string(g_temp_file_counter++);
}

void RemoveAllTempFiles() {
  for (int i = 0; i < g_temp_file_counter; i++) {
    const std::string base = "test_tmp_" + std::to_string(i);
    std::remove((base + ".log").c_str());
    std::remove((base + ".snapshot").c_str());
  }
}

std::string ToString(const std::vector<std::string>& tokens) {
  std::string out = "{";
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i) out += ", ";
    out += "\"" + tokens[i] + "\"";
  }
  out += "}";
  return out;
}

void ExpectEq(const std::string& test_name,
              const std::vector<std::string>& actual,
              const std::vector<std::string>& expected) {
  g_tests_run++;
  if (actual == expected) {
    std::cout << "[PASS] " << test_name << std::endl;
    return;
  }

  g_tests_failed++;
  std::cout << "[FAIL] " << test_name << std::endl;
  std::cout << "       expected: " << ToString(expected) << std::endl;
  std::cout << "       actual:   " << ToString(actual) << std::endl;
}

void ExpectEq(const std::string& test_name, const std::string& actual,
              const std::string& expected) {
  g_tests_run++;
  if (actual == expected) {
    std::cout << "[PASS] " << test_name << std::endl;
    return;
  }

  g_tests_failed++;
  std::cout << "[FAIL] " << test_name << std::endl;
  std::cout << "       expected: \"" << expected << "\"" << std::endl;
  std::cout << "       actual:   \"" << actual << "\"" << std::endl;
}

void RunParserTests() {
  // Only the very first space (splitting the command from the rest of the
  // line) acts as a delimiter. After that, commas are the only delimiter,
  // which is what lets a key/value contain spaces. Leading spaces (at the
  // very start of the line, right after the command, or right after a
  // comma) and trailing spaces (right before a comma or the end of the
  // line) are both stripped -- only spaces in the interior of a token are
  // kept.
  ExpectEq("insert with comma and space separates into 3 tokens; the space "
           "after the comma is a leading space and gets stripped",
           ParseLine("INSERT key, value"), {"INSERT", "key", "value"});

  ExpectEq("insert with comma only, no space", ParseLine("INSERT key,value"),
            {"INSERT", "key", "value"});

  ExpectEq("space no longer splits tokens once the command has been split "
           "off, so a key/value may contain spaces",
           ParseLine("INSERT key value"), {"INSERT", "key value"});

  ExpectEq("get command produces 2 tokens", ParseLine("GET key"),
            {"GET", "key"});

  ExpectEq("delete command produces 2 tokens", ParseLine("DELETE key"),
            {"DELETE", "key"});

  ExpectEq("scan command produces 1 token", ParseLine("SCAN"), {"SCAN"});

  ExpectEq("empty string produces no tokens", ParseLine(""), {});

  ExpectEq("string of only spaces produces no tokens", ParseLine("   "), {});

  ExpectEq("string of only commas produces no tokens", ParseLine(",,,"), {});

  ExpectEq("leading spaces after the command are stripped, but interior "
           "spaces are preserved literally, not collapsed",
           ParseLine("INSERT   key    value"), {"INSERT", "key    value"});

  ExpectEq("repeated commas between tokens collapse to one split",
           ParseLine("INSERT key,,,value"), {"INSERT", "key", "value"});

  ExpectEq("leading whitespace before the command is ignored",
           ParseLine("   INSERT key value"), {"INSERT", "key value"});

  ExpectEq("leading spaces right after the command are stripped",
           ParseLine("GET  key"), {"GET", "key"});

  ExpectEq("trailing whitespace at the end of the line is ignored",
           ParseLine("INSERT key value   "), {"INSERT", "key value"});

  ExpectEq("trailing comma is ignored", ParseLine("INSERT key,value,"),
            {"INSERT", "key", "value"});

  ExpectEq("space before a comma is trailing and gets stripped, and space "
           "after the comma is leading and also gets stripped",
           ParseLine("INSERT key , value"), {"INSERT", "key", "value"});

  ExpectEq("leading spaces right after a comma are stripped even when "
           "there is no space before the comma",
           ParseLine("INSERT  , value"), {"INSERT", "value"});

  ExpectEq("ParseLine preserves the original case of every token",
           ParseLine("insert Key Value"), {"insert", "Key Value"});

  ExpectEq("single-character tokens with a space between them merge into "
           "one token",
           ParseLine("a b c"), {"a", "b c"});

  ExpectEq("ParseLine does not enforce a token-count limit itself; that's "
           "ProcessCommand's job, not the parser's -- and without a comma, "
           "everything after the command is one token",
           ParseLine("INSERT key value extra"),
           {"INSERT", "key value extra"});

  ExpectEq("spaces inside a key/value are kept, but leading and trailing "
           "spaces around a comma are stripped",
           ParseLine("INSERT key with spaces, value with trailing   ,another"),
           {"INSERT", "key with spaces", "value with trailing", "another"});
}

void RunProcessCommandTests() {
  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("insert with 3 tokens stores the key=value pair",
             ProcessCommand({"INSERT", "key", "value"}, storage),
             "OK: stored key=value");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("insert command name is case-insensitive",
             ProcessCommand({"InSeRt", "key", "value"}, storage),
             "OK: stored key=value");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("3 tokens with a command other than insert is rejected",
             ProcessCommand({"GET", "key", "extra"}, storage),
             "ERR: invalid command");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ProcessCommand({"INSERT", "key", "value"}, storage);
    ExpectEq("get with 2 tokens returns the stored value",
             ProcessCommand({"GET", "key"}, storage), "OK: key=value");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ProcessCommand({"INSERT", "key", "value"}, storage);
    ExpectEq("get command name is case-insensitive",
             ProcessCommand({"gEt", "key"}, storage), "OK: key=value");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("get on a missing key is rejected",
             ProcessCommand({"GET", "key"}, storage), "ERR: key not found");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ProcessCommand({"INSERT", "key", "value"}, storage);
    ExpectEq("delete with 2 tokens removes the key",
             ProcessCommand({"DELETE", "key"}, storage), "OK: deleted key");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ProcessCommand({"INSERT", "key", "value"}, storage);
    ExpectEq("delete command name is case-insensitive",
             ProcessCommand({"delete", "key"}, storage), "OK: deleted key");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("delete on a missing key is rejected",
             ProcessCommand({"DELETE", "key"}, storage),
             "ERR: invalid delete key");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("2 tokens with an unrecognized command is rejected",
             ProcessCommand({"FOO", "key"}, storage), "ERR: invalid command");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("scan with no stored data reports empty",
             ProcessCommand({"SCAN"}, storage), "OK: scanning ... (empty)");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ProcessCommand({"INSERT", "a", "1"}, storage);
    ProcessCommand({"INSERT", "b", "2"}, storage);
    ExpectEq("scan with stored data returns every key=value pair",
             ProcessCommand({"SCAN"}, storage),
             "OK: scanning ...\n a=1\n b=2\n");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("scan command name is case-insensitive",
             ProcessCommand({"scan"}, storage), "OK: scanning ... (empty)");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("1 token that isn't scan is rejected",
             ProcessCommand({"FOO"}, storage), "ERR: invalid command");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("empty token list is rejected as invalid length",
             ProcessCommand({}, storage), "ERR: invalid input length");
  }

  {
    Storage storage(MakeTempStorageBasePath());
    ExpectEq("more than 3 tokens is rejected as invalid length",
             ProcessCommand({"INSERT", "key", "value", "extra"}, storage),
             "ERR: invalid input length");
  }
}

void RunStoragePersistenceTests() {
  {
    const std::string base_path = MakeTempStorageBasePath();
    {
      Storage storage(base_path);
      storage.Put("key", "value");
      storage.Put("other", "thing");
      storage.Delete("other");
    }
    Storage reopened(base_path);
    ExpectEq("data written before restart is recovered by replaying the log",
             reopened.Get("key").value_or("<missing>"), "value");
    ExpectEq("a deleted key stays deleted after restart",
             reopened.Get("other").value_or("<missing>"), "<missing>");
  }

  {
    const std::string base_path = MakeTempStorageBasePath();
    {
      Storage storage(base_path);
      for (int i = 0; i < 12; i++) {
        storage.Put("key" + std::to_string(i), std::to_string(i));
      }
    }
    Storage reopened(base_path);
    ExpectEq("data survives restart after a snapshot has been taken",
             reopened.Get("key11").value_or("<missing>"), "11");
  }

  {
    // A base path inside a nonexistent directory can never have its log
    // file opened, so construction should fail loudly instead of silently
    // continuing with a dead stream.
    bool threw = false;
    try {
      Storage storage("nonexistent_dir/base");
    } catch (const std::runtime_error&) {
      threw = true;
    }
    ExpectEq("constructing Storage with an unopenable log file throws "
             "std::runtime_error",
             threw ? "threw" : "did not throw", "threw");
  }
}

}  // namespace
}  // namespace kvstore

int main() {
  kvstore::RunParserTests();
  kvstore::RunProcessCommandTests();
  kvstore::RunStoragePersistenceTests();

  std::cout << std::endl;
  std::cout << (kvstore::g_tests_run - kvstore::g_tests_failed) << "/"
            << kvstore::g_tests_run << " tests passed" << std::endl;

  kvstore::RemoveAllTempFiles();

  return kvstore::g_tests_failed == 0 ? 0 : 1;
}
