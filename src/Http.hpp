#pragma once
// A single HTTPS GET. Windows uses the built-in WinHTTP; macOS and Linux use libcurl.
#include <string>

struct HttpResult {
    int status = 0;     // HTTP status code, 0 if the request never completed
    std::string body;
    std::string error;  // set when status is 0
    bool ok() const { return status >= 200 && status < 300; }
};

// FORTUNE_OFFLINE=1 makes every request fail without touching the network.
HttpResult httpGet(const std::string& url, int timeoutSeconds = 10);
