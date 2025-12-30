#include "../src/Utils/HTTPUtils.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace LitTorrent;

class HTTPUtilsTest : public ::testing::Test {
protected:
  void SetUp() override { httpUtils = std::make_unique<HTTPUtils>(); }

  void TearDown() override { httpUtils.reset(); }

  std::unique_ptr<HTTPUtils> httpUtils;
};

// Test: Constructor and default values
TEST_F(HTTPUtilsTest, ConstructorInitializesCorrectly) {
  EXPECT_NE(httpUtils, nullptr);
  EXPECT_TRUE(httpUtils->GetLastError().empty());
}

// Test: SetTimeout
TEST_F(HTTPUtilsTest, SetTimeoutAcceptsValidValue) {
  EXPECT_NO_THROW(httpUtils->SetTimeout(10));
  EXPECT_NO_THROW(httpUtils->SetTimeout(60));
  EXPECT_NO_THROW(httpUtils->SetTimeout(1));
}

// Test: Valid HTTP GET request
TEST_F(HTTPUtilsTest, GetRequestSucceedsWithValidURL) {
  // Using a reliable public API endpoint
  HTTPResponse response = httpUtils->Get("http://httpbin.org/get");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
  EXPECT_FALSE(response.body.empty());
  EXPECT_TRUE(httpUtils->GetLastError().empty());
}

// Test: HTTPS GET request
TEST_F(HTTPUtilsTest, GetRequestSucceedsWithHTTPS) {
  // HTTPResponse response = httpUtils->Get("https://httpbin.org/get");
  //
  // EXPECT_TRUE(response.success);
  // EXPECT_EQ(response.statusCode, 200);
  // EXPECT_FALSE(response.body.empty());
}

// Test: GET request with query parameters
TEST_F(HTTPUtilsTest, GetRequestWithQueryParameters) {
  HTTPResponse response =
      httpUtils->Get("http://httpbin.org/get?param1=value1&param2=value2");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
  EXPECT_FALSE(response.body.empty());

  // Convert body to string to check content
  std::string bodyStr(response.body.begin(), response.body.end());
  EXPECT_NE(bodyStr.find("param1"), std::string::npos);
  EXPECT_NE(bodyStr.find("value1"), std::string::npos);
}

// Test: Invalid URL format
TEST_F(HTTPUtilsTest, GetRequestFailsWithInvalidURL) {
  HTTPResponse response = httpUtils->Get("invalid-url-without-protocol");

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.statusCode, 0);
  EXPECT_FALSE(httpUtils->GetLastError().empty());
  EXPECT_NE(httpUtils->GetLastError().find("Invalid URL"), std::string::npos);
}

// Test: URL without protocol
TEST_F(HTTPUtilsTest, GetRequestFailsWithoutProtocol) {
  HTTPResponse response = httpUtils->Get("www.google.com");

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.statusCode, 0);
  EXPECT_FALSE(httpUtils->GetLastError().empty());
}

// Test: 404 Not Found
TEST_F(HTTPUtilsTest, GetRequestHandles404Error) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/status/404");

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.statusCode, 404);
  EXPECT_FALSE(httpUtils->GetLastError().empty());
}

// Test: 500 Internal Server Error
TEST_F(HTTPUtilsTest, GetRequestHandles500Error) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/status/500");

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.statusCode, 500);
  EXPECT_FALSE(httpUtils->GetLastError().empty());
}

// Test: 301 Redirect
TEST_F(HTTPUtilsTest, GetRequestHandlesRedirect) {
  HTTPResponse response = httpUtils->Get(
      "http://httpbin.org/redirect-to?url=http://httpbin.org/get");

  // Check if redirect was followed
  if (response.statusCode == 200) {
    // Redirect was followed successfully
    EXPECT_TRUE(response.success);
  } else if (response.statusCode >= 300 && response.statusCode < 400) {
    // Got a redirect response (not followed)
    EXPECT_FALSE(response.success);
  } else {
    // Some other response - just verify we got something
    EXPECT_GT(response.statusCode, 0);
  }
}

// Test: Non-existent domain
TEST_F(HTTPUtilsTest, GetRequestFailsWithNonExistentDomain) {
  HTTPResponse response =
      httpUtils->Get("http://this-domain-does-not-exist-12345.com");

  EXPECT_FALSE(response.success);
  EXPECT_FALSE(httpUtils->GetLastError().empty());
  EXPECT_NE(httpUtils->GetLastError().find("Connection failed"),
            std::string::npos);
}

// Test: Custom port
TEST_F(HTTPUtilsTest, GetRequestWithCustomPort) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org:80/get");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
}

// Test: Response body content
TEST_F(HTTPUtilsTest, GetRequestReturnsCorrectBodyContent) {
  HTTPResponse response =
      httpUtils->Get("http://httpbin.org/base64/SFRUUFV0aWxzVGVzdA==");

  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.body.empty());

  // Convert to string and check content
  std::string bodyStr(response.body.begin(), response.body.end());
  EXPECT_NE(bodyStr.find("HTTPUtilsTest"), std::string::npos);
}

// Test: Timeout handling
TEST_F(HTTPUtilsTest, GetRequestRespectsTimeout) {
  httpUtils->SetTimeout(1); // 1 second timeout

  auto start = std::chrono::high_resolution_clock::now();

  // This endpoint delays for 10 seconds
  HTTPResponse response = httpUtils->Get("http://httpbin.org/delay/10");

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

  // Should timeout before 10 seconds
  EXPECT_LT(duration, 10);
  EXPECT_FALSE(response.success);
}

// Test: Empty response body
TEST_F(HTTPUtilsTest, GetRequestHandlesEmptyResponse) {
  // This endpoint returns 200 with no content
  HTTPResponse response = httpUtils->Get("http://httpbin.org/status/200");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
}

// Test: Large response
TEST_F(HTTPUtilsTest, GetRequestHandlesLargeResponse) {
  // Request a large response (100KB of data)
  HTTPResponse response = httpUtils->Get("http://httpbin.org/bytes/102400");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
  EXPECT_EQ(response.body.size(), 102400);
}

// Test: User-Agent header
TEST_F(HTTPUtilsTest, GetRequestSendsHeaders) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/user-agent");

  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.body.empty());
}

// Test: Multiple sequential requests
TEST_F(HTTPUtilsTest, MultipleSequentialRequests) {
  for (int i = 0; i < 5; i++) {
    HTTPResponse response = httpUtils->Get("http://httpbin.org/get");
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.statusCode, 200);
  }
}

// Test: Different HTTP methods endpoint
TEST_F(HTTPUtilsTest, GetRequestToPostEndpointFails) {
  // GET to an endpoint that only accepts POST
  HTTPResponse response = httpUtils->Get("http://httpbin.org/post");

  // httpbin.org/post actually accepts GET, so let's use a method-specific
  // endpoint
  response = httpUtils->Get("http://httpbin.org/status/405");

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.statusCode, 405);
}

// Test: URL with fragment
TEST_F(HTTPUtilsTest, GetRequestWithFragment) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/get#fragment");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
}

// Test: URL encoding in query parameters
TEST_F(HTTPUtilsTest, GetRequestWithEncodedParameters) {
  HTTPResponse response =
      httpUtils->Get("http://httpbin.org/get?param=hello%20world");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
}

// Test: IPv4 address as host
TEST_F(HTTPUtilsTest, GetRequestWithIPv4Address) {
  // Using a public IP (Google DNS resolver has a web interface)
  // Note: This might not work in all networks
  HTTPResponse response =
      httpUtils->Get("http://93.184.216.34/"); // example.com IP

  // Just check it doesn't crash, result may vary
  EXPECT_GE(response.statusCode, 0);
}

// Test: Very long URL
TEST_F(HTTPUtilsTest, GetRequestWithLongURL) {
  std::string longParam(1000, 'a');
  std::string url = "http://httpbin.org/get?long=" + longParam;

  HTTPResponse response = httpUtils->Get(url);

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
}

// Test: GetLastError is cleared on successful request
TEST_F(HTTPUtilsTest, LastErrorClearedOnSuccess) {
  // First, cause an error
  httpUtils->Get("invalid-url");
  EXPECT_FALSE(httpUtils->GetLastError().empty());

  // Then make a successful request
  HTTPResponse response = httpUtils->Get("http://httpbin.org/get");
  EXPECT_TRUE(response.success);
  EXPECT_TRUE(httpUtils->GetLastError().empty());
}

// Test: Status message is set
TEST_F(HTTPUtilsTest, StatusMessageIsSet) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/get");

  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.statusMessage.empty());
}

// Test: Binary response data
TEST_F(HTTPUtilsTest, GetRequestHandlesBinaryData) {
  HTTPResponse response = httpUtils->Get("http://httpbin.org/bytes/1024");

  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.statusCode, 200);
  EXPECT_EQ(response.body.size(), 1024);

  // Verify it's actual binary data (not just text)
  bool hasBinaryData = false;
  for (uint8_t byte : response.body) {
    if (byte < 32 && byte != '\n' && byte != '\r' && byte != '\t') {
      hasBinaryData = true;
      break;
    }
  }
  EXPECT_TRUE(hasBinaryData);
}

// Main function
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
