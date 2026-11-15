#pragma once
// Calls to FreeHoroscopeAPI (https://freehoroscopeapi.com) with retries, plus a
// small on-disk cache so data keeps working offline.
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "Date.hpp"
#include "Json.hpp"

namespace api {

constexpr const char* kBase = "https://freehoroscopeapi.com";

struct Response {
    std::optional<Json> json;  // parsed body when the call succeeded
    std::string body;          // raw body (saved to the cache)
    std::string problem;       // empty on success
};

// GET kBase + path. Retries a limited number of times with growing delays on
// 502/504 and network failures, as the API documentation recommends.
Response get(const std::string& pathAndQuery);

struct Cached {
    Json json;
    Date fetchedOn;
    bool fromCache = false;
    bool stale = false;       // older than wanted because the refresh failed
};

struct Result {
    std::optional<Cached> data;
    std::string problem;      // why fresh data couldn't be fetched
};

// Uses the cached copy `name` while isFresh(fetchedOn) says it's still good;
// otherwise downloads it again. Falls back to the old copy (marked stale) when offline.
Result cachedGet(const std::filesystem::path& cacheDir, const std::string& name, const std::string& pathAndQuery,
                 const Date& today, const std::function<bool(const Date& fetchedOn)>& isFresh);

}  // namespace api
