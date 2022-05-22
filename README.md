# Lock-free Key-Value Web Server [![Code license](https://img.shields.io/github/license/work-examples/key-value-web-server-lock-free)](LICENSE)

**Example project:** An HTTP web server with a REST API for key-value storage.
Internal data structure is lock-free. Server collects request statistics.

**Language**: `C++20`  
**Dependencies**: `Boost v1.78.0`, `RapidJSON v1.1.0-b557259-p0`, `CrowCpp v1.0+2`, `PyInstaller` (optional)  
**Software requirements**: `CMake 3.19+`, C++20 compatible compiler, `Python 3.7+`  
**Operation systems**: `Windows`, `Linux`, `macOS`

| Branch      | CI Build Status                                                                                                                                                                                                                                                      | CodeQL Code Analysis                                                                                                                                                                                                                                                                         | Microsoft C++ Code Analysis                                                                                                                                                                                                                                                   |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **master**  | [![CI status](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/build_cmake.yml/badge.svg?branch=master)](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/build_cmake.yml?query=branch%3Amaster)   | [![CodeQL Code Analysis Status](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/codeql-analysis.yml/badge.svg?branch=master)](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/codeql-analysis.yml?query=branch%3Amaster) | [![Microsoft C++ Code Analysis Status](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/msvc.yml/badge.svg?branch=master)](https://github.com/work-examples/key-value-web-server-lock-free/actions/workflows/msvc.yml?query=branch%3Amaster) |

<a name="content"></a>

## Contents

- [C++ Programmer's Test Task Description](#task_description)
  - [Server](#description_server)
  - [Client Application](#description_client)
- [Task Implementation Remarks](#implementation_remarks)
  - [Choosing Web Server Implementation](#choosing_web_server)
  - [Other Implementation Features](#other_implementation_features)
- [Compile and Run](#compile_and_run)
- [Web API](#web_api)
  - [Get Value](#api_get_value)
  - [Set Value](#api_set_value)
  - [Get Statistics](#api_get_statistics)
- [Benchmark](#benchmark)
  - [Testing Environment](#benchmark_environment)
  - [Results](#benchmark_results)

<a name="task_description"></a>

## C++ Programmer's Test Task Description

You need to write two applications, a client and a server, that communicate with each other.

<a name="description_server"></a>

### Server

Applications will use a simple dictionary access protocol, defined bellow.
The server must be implemented using TCP.

Any serialization method may be used: `FlatBuffers`, `Protocol Buffers`, `Json`, etc.

The implementation should be non-blocking. Libraries such as `libuv` or `boost` may be used.

The following operations should be supported:

```python
def get(key: string) -> GetResponse
```

Search the specified key and return `GetResponse` structure back.
The `GetResponse` type contains the value found in the dictionary for the specified key in case of success.
In case of error it will contain the reason behind it.

```python
def set(key: string, value: string) -> SetResponse
```

Set values in the dictionary for the specified key.
In case of error the `SetResponse` will contain the reason behind it.

```python
def stats() -> StatsResponse
```

Get server statistics. The `StatsResponse` type should include:

- total number of `get` operations
- number of successful `get` operations
- number of failed `get` operations

The following features are optional for the task implementation:

| Optional feature | Description                                                                                                                   |
|------------------|-------------------------------------------------------------------------------------------------------------------------------|
| Bloom filter     | Minimize number of accesses to the dictionary during `get` operations. This feature should be possible to disable in startup. |
| Benchmark        | Design a benchmark to measure the latency and throughput of the server.                                                       |

<a name="description_client"></a>

### Client Application

A client application should also be implemented in order to test the server.
The client should be a basic library that allows concurrent requests to the server.

<a name="implementation_remarks"></a>

## Task Implementation Remarks

<a name="choosing_web_server"></a>

### Choosing Web Server Implementation

I decided to use well-known HTTP protocol for client-server communication.
Thus, we can easily debug the solution using any browser, programs like `Postman`.
This way I could reuse existing solutions to implement HTTP server in C++.

Choosing a C++ HTTP server library was quite difficult.
I had the following requirements:

- the library should be implemented with a performance in mind
- the library must be alive. I.e. it must have fresh commits and releases.
- the library should be widely used. I decided its repository should have at least 1000 stars in GitHub.
- library must be multithreaded
- library must support HTTP keep-alive
- library must support Linux and Windows
- I would prefer the library to be object-oriented and not to have C-style API.
- I would like the library to be easily integrated. I.e. it should be easy to compile (maybe header-only).
  It would be better if the library is supported by Hunter or Conan package managers.
- It should have complete HTTP server implementation, not just set of tools to create one.

I was choosing among many libraries:
`Beast`, `civetweb`, `libevhtp`, `mongoose`, `PocoCpp`, `cppserver`, `seasocks`,
`cpp-httplib`, `restinio`, `cpprestsdk`, `pistache`, `crow`, `CrowCpp`, `drogon`.

Finally, I settled on `CrowCpp`.

<a name="other_implementation_features"></a>

### Other Implementation Features

The heart of the server engine uses fixed-size bucket array
and single-linked list inside each bucket.
This structure allows very simple implementation of searching and adding new elements.
We don't need to remove keys from the storage so we can implement these operations lock-free without traditional loops.

The **maximum** complexity for operations would be:

- search time: `O(1 + length(corresponding bucket list))`
- insertion time: `O(1 + length(corresponding bucket list))`

Web server statistics values are only protected by `std::atomic`,
which allows small inconsistency between different values.
This is a reasonable tradeoff for speed.

Solution is cross-platform.

It uses CMake to generate a C++ project.
All external dependencies are under source control
or downloaded automatically by embedded Hunter package manager (`Boost`, `RapidJSON`).

The implementation is divided into several components that are completely independent of each other.
The **Inversion of Control** pattern is used to combine them into a working solution.

I have used the **pImpl** pattern to hide implementation details in the `.cpp` file.
It also speeds up compilation by limiting the number of `.cpp` files
compiled with included complex C++ templates.

The solution also makes heavy use of move semantics.

#### Memory allocation

Lock-free requirement for memory allocation is partially implemented.
I.e. storage engine uses custom memory allocators. Different threads are getting different instances of allocators.
But using different memory heaps is not implemented now.

Threads also may block each other during key-value replacement while deallocating memory allocated from another thread.

HTTP server implementation, JSON parser do not use custom memory allocators and may block threads.

#### Known implementation disadvantages

- usage of `std::atomic<std::shared_ptr>` which is not lock-free in MSVC 2019
  and is not implemented in stable version of GCC
- potential blocking in memory allocations

<a name="compile_and_run"></a>

## Compile and Run

1. Run CMake.
2. Compile project. You will get `WebServer` executable
3. Execute `WebServer`. It will listen on `127.0.0.1:8000`
   and it will use `database.json` file from current directory for persistence.
4. Run HTTP client script: `python3 client.py`

Database file example:
[database.example.json](database.example.json)

Client application is run the following way. It needs python3 and has no external dependencies.
CI is also preparing `Client.exe` executable which is a compiled version of `client.py` with Python interpreter inside.

```bash
python3 client.py
```

<a name="web_api"></a>

## Web API

Two API methods are supported.

Here is the prepared API request collection for Postman:
[WebServer.postman_collection.json](WebServer.postman_collection.json)

<a name="api_get_value"></a>

### Get Value

`GET` <http://127.0.0.1:8000/api/records/{key-name}>

Reply body example:

```json
{
    "name": "name 1",
    "value": "my value"
}
```

<a name="api_set_value"></a>

### Set Value

`POST` <http://127.0.0.1:8000/api/records/{key-name}>

Request body example:

```json
{
    "value": "my value"
}
```

Reply body example:

```json
{
    "name": "name 1"
}
```

<a name="api_get_statistics"></a>

### Get Statistics Value

`GET` <http://127.0.0.1:8000/api/statistics/reads>

Reply body example:

```json
{
    "total": 123,
    "succeeded": 100,
    "failed": 23
}
```

In case of error all API endpoints return HTTP error code 4xx or 5xx and the special reply format in the body:

```json
{
    "name": "name 1",
    "error": "Item not found"
}
```

<a name="benchmark"></a>

## Benchmark

<a name="benchmark_environment"></a>

### Testing Environment

CPU Intel Core i5 (8th gen), mobile version, 8 logical cores.  
Visual Studio 2019 (v16.11.13), Release build  
Windows 10 Version 21H1 (Build 19043.1645)  
Number of HTTP server threads: 8  
Logging of every request is disabled for both server and client side.

Starting applications without logging messages for every HTTP request:

```bash
WebServer.exe --no-logs
Client.exe --no-logs
```

<a name="benchmark_results"></a>

### Results

| Number of <br/>request threads | 10K requests <br/>per thread, req/sec | 100K requests <br/>per thread, req/sec |
|-------------------------------:|--------------------------------------:|---------------------------------------:|
|                              1 |                                 4 400 |                                  4 500 |
|                              2 |                                 6 400 |                                  7 600 |
|                              3 |                                 9 100 |                                 11 000 |
|                              4 |                                10 400 |                                 12 000 |
|                              6 |                                11 800 |                                 14 300 |
|                              8 |                                12 300 |                                 14 600 |
