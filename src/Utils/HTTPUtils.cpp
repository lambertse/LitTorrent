#include "HTTPUtils.h"
#include <iostream>
#include <sstream>

// Include the third-party HTTP library here
#include "cpp-httplib/httplib.h"

namespace LitTorrent {
HTTPUtils::HTTPUtils() : timeoutSeconds(30) {}

HTTPUtils::~HTTPUtils() {}

void HTTPUtils::SetTimeout(int seconds) { timeoutSeconds = seconds; }

HTTPResponse HTTPUtils::Get(const std::string &url) {
  HTTPResponse response;
  lastError.clear();

  try {
    std::string protocol, host, path;
    int port;

    if (!ParseURL(url, protocol, host, port, path)) {
      lastError = "Invalid URL format";
      response.success = false;
      return response;
    }

    // Create HTTP client using httplib
    std::string hostWithPort = host;
    if ((protocol == "http" && port != 80) ||
        (protocol == "https" && port != 443)) {
      hostWithPort = host + ":" + std::to_string(port);
    }

    httplib::Client client(host, port);
    client.set_connection_timeout(timeoutSeconds);
    client.set_read_timeout(timeoutSeconds);
    client.set_write_timeout(timeoutSeconds);

    // Perform GET request
    auto res = client.Get(path.c_str());

    if (!res) {
      lastError = "Connection failed";
      response.success = false;
      return response;
    }

    // Fill response
    response.statusCode = res->status;
    response.statusMessage = res->reason;
    response.body = std::vector<uint8_t>(res->body.begin(), res->body.end());
    response.success = (res->status == 200);

    if (!response.success) {
      lastError =
          "HTTP error: " + std::to_string(res->status) + " " + res->reason;
    }
  } catch (const std::exception &e) {
    lastError = std::string("Exception: ") + e.what();
    response.success = false;
  }

  return response;
}

std::string HTTPUtils::GetLastError() const { return lastError; }

bool HTTPUtils::ParseURL(const std::string &url, std::string &protocol,
                         std::string &host, int &port, std::string &path) {
  // Find protocol
  size_t protocolEnd = url.find("://");
  if (protocolEnd == std::string::npos) {
    return false;
  }

  protocol = url.substr(0, protocolEnd);

  // Find host start
  size_t hostStart = protocolEnd + 3;

  // Find path start
  size_t pathStart = url.find('/', hostStart);

  if (pathStart != std::string::npos) {
    std::string hostPort = url.substr(hostStart, pathStart - hostStart);
    path = url.substr(pathStart);

    // Check for port
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
      host = hostPort.substr(0, colonPos);
      try {
        port = std::stoi(hostPort.substr(colonPos + 1));
      } catch (...) {
        return false;
      }
    } else {
      host = hostPort;
      port = (protocol == "https") ? 443 : 80;
    }
  } else {
    std::string hostPort = url.substr(hostStart);
    path = "/";

    // Check for port
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
      host = hostPort.substr(0, colonPos);
      try {
        port = std::stoi(hostPort.substr(colonPos + 1));
      } catch (...) {
        return false;
      }
    } else {
      host = hostPort;
      port = (protocol == "https") ? 443 : 80;
    }
  }

  return true;
}

} // namespace LitTorrent
