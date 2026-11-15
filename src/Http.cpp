#include "Http.hpp"

#include <cstdlib>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>
#else
#include <curl/curl.h>
#endif

namespace {

const char* kUserAgent = "fortune-teller-cpp/2.0 (+https://github.com/Umair-Habibx123/fortune-teller-cpp)";

bool offline() {
    const char* v = std::getenv("FORTUNE_OFFLINE");
    return v && *v && std::string(v) != "0";
}

}  // namespace

#ifdef _WIN32

namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

struct Handle {
    HINTERNET h = nullptr;
    explicit Handle(HINTERNET handle) : h(handle) {}
    ~Handle() {
        if (h) WinHttpCloseHandle(h);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

std::string lastError(const char* what) { return std::string(what) + " failed (Windows error " + std::to_string(GetLastError()) + ")"; }

}  // namespace

HttpResult httpGet(const std::string& url, int timeoutSeconds) {
    HttpResult result;
    if (offline()) {
        result.error = "offline mode (FORTUNE_OFFLINE is set)";
        return result;
    }

    std::wstring wurl = widen(url);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof parts;
    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    parts.lpszHostName = host;
    parts.dwHostNameLength = 256;
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = 2048;
    wchar_t extra[2048] = {};
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts)) {
        result.error = lastError("Reading the URL");
        return result;
    }

    Handle session(WinHttpOpen(widen(kUserAgent).c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.h) {
        result.error = lastError("Opening an internet session");
        return result;
    }
    const int ms = timeoutSeconds * 1000;
    WinHttpSetTimeouts(session.h, ms, ms, ms, ms);

    Handle connection(WinHttpConnect(session.h, host, parts.nPort, 0));
    if (!connection.h) {
        result.error = lastError("Connecting");
        return result;
    }
    const std::wstring target = std::wstring(path) + extra;
    Handle request(WinHttpOpenRequest(connection.h, L"GET", target.c_str(), nullptr, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0));
    if (!request.h) {
        result.error = lastError("Creating the request");
        return result;
    }
    // the API sometimes answers with a redirect; WinHTTP follows it by default
    if (!WinHttpSendRequest(request.h, L"Accept: application/json\r\n", static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA,
                            0, 0, 0) ||
        !WinHttpReceiveResponse(request.h, nullptr)) {
        result.error = lastError("Sending the request");
        return result;
    }

    DWORD status = 0, size = sizeof status;
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &status, &size, WINHTTP_NO_HEADER_INDEX);
    result.status = static_cast<int>(status);

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available) || available == 0) break;
        std::string chunk(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request.h, chunk.data(), available, &read) || read == 0) break;
        chunk.resize(read);
        result.body += chunk;
        if (result.body.size() > (1u << 20)) break;  // the API caps responses at 1 MiB
    }
    return result;
}

#else  // macOS, Linux: libcurl

namespace {

std::size_t collect(char* data, std::size_t size, std::size_t count, void* user) {
    auto* body = static_cast<std::string*>(user);
    if (body->size() > (1u << 20)) return 0;
    body->append(data, size * count);
    return size * count;
}

}  // namespace

HttpResult httpGet(const std::string& url, int timeoutSeconds) {
    HttpResult result;
    if (offline()) {
        result.error = "offline mode (FORTUNE_OFFLINE is set)";
        return result;
    }
    static const bool initialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    CURL* curl = initialized ? curl_easy_init() : nullptr;
    if (!curl) {
        result.error = "could not start libcurl";
        return result;
    }
    struct curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    const CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_OK) {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        result.status = static_cast<int>(status);
    } else {
        result.error = curl_easy_strerror(code);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

#endif
