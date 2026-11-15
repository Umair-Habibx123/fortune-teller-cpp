#include "Api.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

#include "Http.hpp"

namespace fs = std::filesystem;

namespace api {
namespace {

bool retryable(const HttpResult& r) { return r.status == 0 || r.status == 502 || r.status == 504; }

bool offline() {
    const char* v = std::getenv("FORTUNE_OFFLINE");
    return v && *v && std::string(v) != "0";
}

}  // namespace

Response get(const std::string& pathAndQuery) {
    Response out;
    HttpResult http;
    const int attempts = offline() ? 1 : 3;
    for (int attempt = 1; attempt <= attempts; ++attempt) {
        // horoscopes answer within 10 s, calculations within 30 s
        http = httpGet(std::string(kBase) + pathAndQuery, 30);
        if (http.ok() || !retryable(http) || attempt == attempts) break;
        std::this_thread::sleep_for(std::chrono::seconds(attempt));  // 1 s, then 2 s
    }

    if (http.status == 0) {
        out.problem = "no internet connection (" + http.error + ")";
        return out;
    }
    auto doc = Json::parse(http.body);
    if (!http.ok()) {
        std::string message = doc ? (*doc)["error"].str() : "";
        if (message.empty() && doc) {
            // validation errors (HTTP 422) put their messages in a "detail" array
            const Json& detail = (*doc)["detail"];
            if (detail.isArray() && detail.size() > 0) message = detail[0]["msg"].str();
        }
        out.problem = "the service answered HTTP " + std::to_string(http.status) + (message.empty() ? "" : ": " + message);
        return out;
    }
    if (!doc) {
        out.problem = "the service sent something that isn't valid JSON";
        return out;
    }
    out.json = std::move(doc);
    out.body = http.body;
    return out;
}

// Cache file: first line = date fetched, the rest = the raw JSON body.
Result cachedGet(const fs::path& cacheDir, const std::string& name, const std::string& pathAndQuery, const Date& today,
                 const std::function<bool(const Date&)>& isFresh) {
    Result result;
    const fs::path file = cacheDir / (name + ".json");

    std::optional<Cached> cached;
    if (std::ifstream in{file, std::ios::binary}) {
        std::string first;
        std::getline(in, first);
        if (!first.empty() && first.back() == '\r') first.pop_back();
        std::stringstream rest;
        rest << in.rdbuf();
        auto date = parseDate(first);
        auto doc = Json::parse(rest.str());
        if (date && doc) cached = Cached{std::move(*doc), *date, true, false};
    }
    if (cached && isFresh(cached->fetchedOn)) {
        result.data = std::move(cached);
        return result;
    }

    Response fresh = get(pathAndQuery);
    if (fresh.json) {
        std::error_code ec;
        fs::create_directories(cacheDir, ec);
        const fs::path temp = file.string() + ".tmp";
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (out) out << today.iso() << '\n' << fresh.body;
        }
        fs::rename(temp, file, ec);
        if (ec) fs::remove(temp, ec);
        result.data = Cached{std::move(*fresh.json), today, false, false};
        return result;
    }

    result.problem = fresh.problem;
    if (cached) {
        cached->stale = true;
        result.data = std::move(cached);
    }
    return result;
}

}  // namespace api
