#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace LitTorrent {
struct HTTPResponse {
  int statusCode;
  std::string statusMessage;
  std::vector<uint8_t> body;
  bool success;

  HTTPResponse() : statusCode(0), success(false) {}
};

class HTTPUtils {
public:
  HTTPUtils();
  ~HTTPUtils();

  // Set timeout in seconds
  void SetTimeout(int seconds);

  // Perform GET request
  // Returns HTTPResponse with status and body
  HTTPResponse Get(const std::string &url);

  // Get last error message
  std::string GetLastError() const;

private:
  int timeoutSeconds;
  std::string lastError;

  // Helper methods
  bool ParseURL(const std::string &url, std::string &protocol,
                std::string &host, int &port, std::string &path);
};

} // namespace LitTorrent

