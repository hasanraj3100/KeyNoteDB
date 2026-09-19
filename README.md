# Key Value Store 

This is a simple key value data storage system built with C++. I am building this just to see if I like Database Development. I don't know how long I will maintain it, but I will at least create the version 0 of it then keep working on it as long as I like it.


# Roadmap
This tells me what I am going to do, the plan.

## Version - 0 (done)
A basic Key Value Store using C++. Supports query: 

```
INSERT key, value 
GET key 
DELETE key 
SCAN
```

The data stays in memory, using STL's map data structure.
The parser should handle invalid queries.

## Version - 1 (done)
Adds persistence so data survives a restart:

- Every INSERT/DELETE is appended to a log file.
- On startup, the log is replayed to rebuild the in-memory map.
- The parser now allows keys/values to contain spaces (e.g. `INSERT key, a value with spaces`), trimming leading/trailing spaces around the comma.
- Once enough operations have accumulated, the in-memory map is snapshotted to disk and the log is truncated back to empty, so the next startup only has to replay the (now short) log on top of the snapshot instead of the whole history.

# Project Layout

```
include/kvstore/   public headers (parser, storage, command_processor)
src/               implementation + main.cpp (the CLI entry point)
tests/             a small hand-rolled test runner, no external framework
```

Everything lives under the `kvstore` namespace.

# Building & Running

```
make            # builds ./build/kvstore
make test       # builds and runs ./build/kvstore_tests
make clean
```

Running `./build/kvstore` starts a REPL reading commands from stdin. It
persists to `kvstore_data.log` (plus `kvstore_data.snapshot` once a
snapshot has been taken) in the current directory.
