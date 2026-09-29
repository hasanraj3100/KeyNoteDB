# KeyNoteDB

A persistent key-value store written from scratch in C++17, with no external
dependencies. It keeps data in an in-memory hash map, records every write
in a write-ahead log, and compacts that log into crash-safe snapshots.

The project is a hands-on study of how storage engines work: write-ahead
logging, recovery, snapshotting, and the syscall-level details (`fsync`,
atomic `rename`) that decide whether data survives a crash.

## Features

- **Simple query language:** `INSERT`, `GET`, `DELETE` and `SCAN` over an
  interactive REPL. Commands are case-insensitive.
- **Write-ahead log:** every mutation is appended to a log before it is
  acknowledged, so data survives a process restart.
- **Log compaction via snapshots:** once enough operations build up, the full
  state is written to a snapshot and the log is truncated. Startup only
  replays the operations since the last snapshot.
- **Crash-safe snapshots:** a snapshot is written to a temp file, `fsync`ed,
  atomically `rename`d into place, and the directory is `fsync`ed. The log is
  truncated only after the new snapshot is durable, so a crash at any point
  leaves either the old snapshot or the new one, never a partial file.
- **Fails loudly:** the store throws on startup if its log file can't be
  opened, instead of running with a dead stream.
- **Tested:** a dependency-free test suite covers parsing, command handling,
  persistence, and recovery across restarts.

## Quick start

Requirements: a C++17 compiler (GCC or Clang), `make`, and a POSIX system
(Linux or macOS).

```sh
git clone https://github.com/hasanraj3100/KeyNoteDB.git
cd KeyNoteDB
make          # builds ./build/kvstore
make test     # builds and runs the test suite
```

## Usage

`./build/kvstore` starts a REPL that reads one command per line from stdin.

```text
$ ./build/kvstore
INSERT name, Ada Lovelace
OK: stored name=Ada Lovelace
GET name
OK: name=Ada Lovelace
SCAN
OK: scanning ...
 name=Ada Lovelace
DELETE name
OK: deleted name
GET name
ERR: key not found
```

| Command                | Description                             |
| ---------------------- | --------------------------------------- |
| `INSERT <key>, <value>` | Insert or overwrite a key               |
| `GET <key>`            | Return the value stored for a key       |
| `DELETE <key>`         | Remove a key                            |
| `SCAN`                 | List all entries (in unspecified order) |

Keys and values may contain spaces. Whitespace around the comma is trimmed.
Successful commands reply with `OK: ...` and errors with `ERR: ...`.

Data is stored in the current working directory as `kvstore_data.log` and
`kvstore_data.snapshot`.

## How it works

### Write path

```
INSERT / DELETE
      │
      ├──► update in-memory std::unordered_map
      ├──► append operation to kvstore_data.log
      └──► if ops since last snapshot ≥ threshold → take snapshot
```

### Snapshot (log compaction)

1. Write every entry to `kvstore_data.snapshot.temp`.
2. `fflush` + `fsync` the temp file.
3. `rename` it over `kvstore_data.snapshot`. The rename is atomic on POSIX,
   so readers only ever see the complete old snapshot or the complete new one.
4. `fsync` the parent directory so the rename itself is durable.
5. Truncate and `fsync` the log, which now only needs to hold future operations.

If any step fails, the temp file is removed, the previous snapshot and log
stay intact, and the snapshot is retried on the next write.

### Recovery

On startup the store loads `kvstore_data.snapshot` (if present) and then
replays `kvstore_data.log` on top of it. Log entries are absolute operations
(`INSERT k, v` / `DELETE k`), so replaying them is idempotent. This holds even
when a crash happens between installing a snapshot and truncating the log.

## Project structure

```
include/kvstore/   public headers
  parser.h             tokenizes a command line
  command_processor.h  executes parsed commands against Storage
  storage.h            in-memory map + WAL + snapshot engine
src/               implementations and main.cpp (REPL entry point)
tests/             self-contained test runner (no external framework)
Makefile
```

All code lives in the `kvstore` namespace.

## Current limitations

These are known and tracked. Most are next on the roadmap.

- **Log writes are not `fsync`ed.** Log appends are flushed to the OS but
  not forced to disk, so a power loss can drop the most recent operations.
  Snapshots are fully durable.
- **Write errors aren't propagated.** `Put` updates memory before appending to
  the log and doesn't check whether the append succeeded.
- **Commas are delimiters.** Keys and values can't contain `,`.
- **Single process, single thread.** No concurrent access and no network
  interface yet.
- **POSIX-only.** The snapshot code uses `fsync`, `open` and `ftruncate`.

## Roadmap

- [x] **v0: in-memory store.** Parser, command processor, `std::map` backend.
- [x] **v1: persistence.** Write-ahead log, replay on startup, snapshot-based
      log compaction.
- [x] **v1.1: crash safety.** Atomic snapshots (temp file + `fsync` + `rename`
      + directory `fsync`), startup failure on an unopenable log.
- [ ] **v1.2: durable writes.** `fsync` log appends, write to the log before
      updating memory, and surface write failures to the caller.
- [ ] **Next:** a checksummed record format to detect torn or corrupt log
      entries, escaping for delimiter characters, and tests for crash and
      failure paths.

## Testing

```sh
make test
```

The suite covers tokenizing edge cases, every command (including
case-insensitivity and invalid input), persistence across restarts (with and
without a snapshot), and constructor failure on an unopenable log file.
