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
