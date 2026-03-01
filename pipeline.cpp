#include "llmpool.h"
#include <cpr/cpr.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Base64 encoding helper
std::string base64_encode(const std::string &in) {
  static const std::string base64_chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  int val = 0, valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

// Notice this now returns a std::string instead of returning void
std::string analyze_image_with_ollama(const std::string &image_path) {
  if (!fs::exists(image_path)) {
    return "Error: " + image_path + " not found";
  }

  std::ifstream file(image_path, std::ios::binary);
  std::string image_data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  std::string base64_image = base64_encode(image_data);

  // Ergonomic JSON construction mapping perfectly to Python dictionaries
  json payload = {{"model", "qwen3-vl:latest"},
                  {"prompt", "What is in this image?"},
                  {"stream", false},
                  {"images", {base64_image}}};

  // cpr automatically handles the connection, sockets, and memory bounds
  cpr::Response r =
      cpr::Post(cpr::Url{"http://localhost:11434/api/generate"},
                cpr::Body{payload.dump()},
                cpr::Header{{"Content-Type", "application/json"}});

  if (r.status_code == 200) {
    try {
      json result = json::parse(r.text);
      return result.value("response", "No response field in JSON");
    } catch (const json::parse_error &e) {
      return "JSON Parse Error: " + std::string(e.what());
    }
  } else {
    return "HTTP Error " + std::to_string(r.status_code) + ": " +
           r.error.message;
  }
}

int main() {
  // 1 thread to avoid VRAM exhaustion for Vision Models
  LLMThreadPool llm_pool(1);

  std::string image_dir = "images";
  if (!fs::exists(image_dir) || !fs::is_directory(image_dir)) {
    std::cerr << "Directory 'images' does not exist. Please create it and add "
                 "some images.\n";
    return 1;
  }

  std::cout << "Submitting image analysis tasks...\n";

  // A struct to pair the original path with its future result
  struct TaskTracker {
    std::string path;
    std::future<std::string> result_future;
  };
  std::vector<TaskTracker> tasks;

  for (const auto &entry : fs::directory_iterator(image_dir)) {
    if (entry.is_regular_file()) {
      std::string path = entry.path().string();

      // submit() now returns a std::future<std::string>!
      tasks.push_back({path, llm_pool.submit([path]() {
                         return analyze_image_with_ollama(path);
                       })});
    }
  }

  std::cout << "All tasks submitted. Waiting for results...\n\n";

  // Block and wait for each future to complete. The liveness hack is gone.
  for (auto &task : tasks) {
    // .get() will block the main thread until THIS specific task finishes
    // returning its string.
    std::string result = task.result_future.get();
    std::cout << "=== Results for '" << task.path << "' ===\n"
              << result << "\n\n";
  }

  std::cout << "Analysis pipeline complete. Exiting cleanly.\n";
  return 0;
}