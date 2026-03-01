# LLM Pool

A C++17 based thread pool implementation designed for concurrent LLM (Large Language Model) task execution, specifically tailored for image analysis using Ollama.

## Features

- **Concurrent Execution**: Efficiently manage multiple LLM tasks using a pool of worker threads.
- **Vision Integration**: Built-in support for image analysis using Ollama's vision models.
- **Future-Based Results**: Uses `std::future` for easy retrieval of results from asynchronous tasks.
- **C++17 Standards**: Utilizes modern C++ features like `std::filesystem`, `std::variant`, and variadic templates.
- **Robust Networking**: Uses [CPR (C++ Requests)](https://github.com/libcpr/cpr) for high-level HTTP communication.

## Architecture

### LLMThreadPool Implementation

The core of the project is the `LLMThreadPool`, located in `llmpool.h`. It follows the "Principal-level Thread Pool" paradigm:

1.  **Task Queue**: A thread-safe queue managed by `std::mutex` and `std::condition_variable`.
2.  **Worker Threads**: A vector of threads that continuously pop tasks from the queue and execute them.
3.  **Task Submission**: The `submit()` function uses variadic templates and `std::packaged_task` to wrap functions and return a `std::future`. This allows the main thread to wait for specific results without blocking the entire pool.

```cpp
// Example Submission
auto future = pool.submit([](int x) { return x * x; }, 42);
int result = future.get(); // Blocks until task completes
```

### Image Analysis Pipeline

The `pipeline.cpp` application demonstrates the pool's power:
1.  **Base64 Encoding**: Converts image files into a format suitable for JSON payloads.
2.  **JSON Construction**: Uses `nlohmann/json` for ergonomic payload creation.
3.  **HTTP Request**: Sends POST requests to the Ollama API using `cpr`.
4.  **Parallel Processing**: Images in the `images/` directory are processed concurrently, regulated by the thread pool (defaulting to 1 thread for vision models to avoid VRAM over-saturation).

## Prerequisites

- C++17 Compiler (GCC/Clang)
- [libcurl](https://curl.se/libcurl/)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [libcpr](https://github.com/libcpr/cpr)
- [nlohmann/json](https://github.com/nlohmann/json)
- [Ollama](https://ollama.com/) (running locally)

## Building

```bash
make
```

The Makefile is configured to link all necessary dependencies, including `ssl` and `crypto` for the static `libcpr` library.

## Usage

1.  Start your local Ollama server.
2.  Ensure you have images in the `images/` directory.
3.  Run the pipeline:
    ```bash
    ./pipeline
    ```

## License

MIT
