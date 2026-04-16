# Redis Clone C++

A lightweight Redis server clone implemented in C++ that supports key-value operations and sorted sets.

## Overview

This project is a simplified implementation of Redis, featuring:

- In-memory key-value storage
- Binary protocol for client-server communication
- Support for strings, integers, and sorted sets
- Command processing similar to Redis
- Non-blocking I/O with event loop architecture

## Building

### Prerequisites

- C++17 compatible compiler
- `make`
- macOS or Linux

### Compilation

```bash
make
```

This builds:

- `redis-clone` - the server
- `client` - the interactive CLI client

### Tests

Run the unit test suite with:

```bash
make test
```

The test binary is built as `redis-clone-tests`.

## Running

### Server

```bash
./redis-clone
```

The server listens on 0.0.0.0:8080 by default.

You can also pass a custom host and port:

```bash
./redis-clone 127.0.0.1 8080
```

### Client

```bash
./client
```

The client connects to `127.0.0.1:8080` by default.

You can also target a custom host and port:

```bash
./client 127.0.0.1 8080
```

## Supported Commands

The server supports the following Redis-like commands:

### Basic Commands

- `GET key` - Get the value of a key
- `SET key value` - Set key to value
- `DEL key` - Delete key
- `KEYS` - Get all keys
- `PTTL key` - Get the time to live for a key in milliseconds
- `PEXPIRE key milliseconds` - Set a key's time-to-live in milliseconds

### Sorted Set Commands

- `ZADD key score member` - Add member to a sorted set
- `ZREM key member` - Remove member from a sorted set
- `ZSCORE key member` - Get the score of a member in a sorted set
- `ZQUERY key score name offset limit` - Query sorted-set entries starting from a lower bound

### Usage Examples

```
SET mykey hello
GET mykey
PTTL mykey
PEXPIRE mykey 10000
PTTL mykey
DEL mykey
ZADD ranks 10 alice
ZADD ranks 5 bob
ZSCORE ranks alice
ZQUERY ranks 0 "" 0 10
```

## Protocol Details

### Request Format

```
msglen (4 bytes) | nargs (4 bytes) | arglen1 (4 bytes) | arg1 | ... | arglenN (4 bytes) | argN
```

Where:

- msglen: Total message length (excluding the 4 bytes for msglen itself)
- nargs: Number of arguments
- arglenX: Length of argument X
- argX: Byte data for argument X

### Response Format

```
msglen (4 bytes) | tag (1 byte) | payload
```

Where:

- msglen: Total message length (excluding the 4 bytes for msglen itself)
- tag: Type of response (NIL, ERROR, STRING, INTEGER, DOUBLE, ARRAY)
- payload: Type-specific data format

### Example Encoded Request (SET key value)

Arguments: ["SET","mykey","hello"]

```
[msglen= ?] | nargs=3
arglen=3  SET
arglen=5  mykey
arglen=5  hello
```

Hex representation in little-endian:

```
18 00 00 00    # msglen = 24 (bytes after this field)
03 00 00 00    # nargs
03 00 00 00 53 45 54
05 00 00 00 6d 79 6b 65 79
05 00 00 00 68 65 6c 6c 6f
```

## Architecture

### Core Components

- **Server**: Main server implementation handling connections
- **EventLoop**: Non-blocking I/O event loop using `poll()`
- **ConnectionManager**: Manages client connections
- **RequestProcessor**: Processes incoming client requests
- **ProtocolHelper**: Handles the binary protocol encoding/decoding
- **Command System**: Extensible command pattern implementation
- **Database**: In-memory data storage with HashTable and AVL tree

### Data Structures

- **ResizableHashTable**: Main key-value storage
- **AVLTree**: Self-balancing binary tree for sorted sets

### Core Components

1. **Server**
   Initializes the subsystems, owns the main loop, and handles graceful shutdown.
2. **Event Loop**
   Uses `poll()` to multiplex listener and connection sockets without a thread per connection.
3. **Connection Management**
   Tracks connection state, buffers I/O, and handles cleanup for closed or timed-out clients.
4. **Protocol Layer**
   Encodes and decodes the custom binary request and response format.
5. **Command Processing**
   Routes parsed requests through `CommandFactory` to individual command implementations.
6. **Storage Engine**
   Uses `ResizableHashTable` for key-value storage, `AVLTree` for sorted sets, and a TTL heap for expirations.

### Data Flow

1. The server accepts a client connection.
2. `EventLoop` monitors sockets for read and write readiness.
3. `Connection::handleRead()` appends incoming bytes to the input buffer.
4. `RequestProcessor` asks `ProtocolHelper` to parse complete requests.
5. `CommandFactory` creates the matching command handler.
6. The command executes against the database and returns a `Response`.
7. `ProtocolHelper` serializes the response into the output buffer.
8. `Connection::handleWrite()` flushes buffered bytes back to the client.

### Design Patterns

- **Command Pattern**: All operations encapsulated as Command objects with uniform interface
- **Factory Pattern**: CommandFactory creates appropriate Command instances
- **Repository Pattern**: IDatabase interface abstracts storage operations
- **Strategy Pattern**: Different command implementations for various operations

## Platform Notes

- The networking layer now uses `poll()`, which makes the project build on both macOS and Linux without `epoll`.
- `SIGPIPE` is ignored for socket writes so disconnects do not terminate the process on macOS.
- TTL commands use millisecond precision:
  - `PEXPIRE key milliseconds`
  - `PTTL key`

## Attribution

This implementation is based on (and adapted from) the concepts and structure presented in the book:
Build Your Own Redis with C/C++  
Network programming, data structures, and low-level C.  
Author: James Smith  
[https://build-your-own.org/redis/](https://build-your-own.org/redis/)

The code here is an independent reimplementation inspired by that guide.

## License

This project is open source and available under the MIT License. See LICENSE file.
