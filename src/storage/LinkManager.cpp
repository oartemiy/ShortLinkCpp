#include "LinkManager.h"
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

static std::string current_iso8601_time()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

bool LinkManager::isShortLinkAvailable(const std::string& code) const
{
    std::lock_guard<std::mutex> lock(_storage_mutex);
    return !_storage.contains(code);
}

void LinkManager::addShortLink(const std::string& link,
                               const std::string& shortLink)
{
    // guaranteed that shortLink is available
    std::lock_guard<std::mutex> lock(_storage_mutex);
    _storage.insert({shortLink, {shortLink, link, current_iso8601_time(), 0}});
}

const LinkInfo& LinkManager::getLinkInfo(const std::string& shortLink) const
{
    std::lock_guard<std::mutex> lock(_storage_mutex);
    return _storage.at(shortLink);
}

inline void LinkManager::redirect(const std::string& shortLink)
{
    std::lock_guard<std::mutex> lock(_storage_mutex);
    _storage[shortLink].clicks++;
}

std::vector<std::reference_wrapper<const LinkInfo>>
LinkManager::getAllLinks() const
{
    std::lock_guard<std::mutex> lock(_storage_mutex);
    std::vector<std::reference_wrapper<const LinkInfo>> result;
    result.reserve(_storage.size());
    for(const auto& [shortLink, info]: _storage)
        result.push_back(info);
    return result;
}