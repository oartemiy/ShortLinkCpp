#include "LinkManager.h"
#include "../gen_code/gen_code.h"
#include "../utils/json.hpp"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>

using nlohmann::json;

static std::string to_string(std::time_t time)
{
    std::tm tm = *std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

inline std::time_t current_time()
{
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

// required mutex lock_guard
inline bool LinkManager::isCodeAvailable(const std::string& code) noexcept
{
    return !_storage.get_optional<LinkInfo>(code).has_value();
}

inline bool LinkManager::isCodeExpired(const std::string& code) noexcept
{
    auto curTime = current_time();
    return _storage.get<LinkInfo>(code).expires_at <= curTime;
}

std::string LinkManager::addUrl(const std::string& original_url) noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    std::string code;
    do
    {
        code = gen_code();
    }
    while(!isCodeAvailable(code));
    auto createdAt = current_time();
    auto expiresAt = createdAt + 120; // 120 == std::chorono::minutes(2)
    _storage.replace<LinkInfo>({code, original_url, createdAt, expiresAt, 0ull});
    return code;
}

json LinkManager::getCodeInfo(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(isCodeExpired(code))
        throw CodeTLLError(code);
    auto codeInfo = _storage.get<LinkInfo>(code);
    json response;
    response["code"] = code;
    response["original_url"] = codeInfo.original_url;
    response["created_at"] = to_string(codeInfo.created_at);
    response["expires_at"] = to_string(codeInfo.expires_at);
    response["clicks"] = codeInfo.clicks;
    return response;
}

std::string LinkManager::redirect(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(isCodeExpired(code))
        throw CodeTLLError(code);
    auto codeInfo = _storage.get<LinkInfo>(code);
    codeInfo.clicks += 1;
    _storage.update(codeInfo);
    return codeInfo.original_url;
}

json LinkManager::getInfo(std::size_t limitURL, std::size_t offsetURL) noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    json result = json::array();
    std::size_t size = _storage.count<LinkInfo>();
    result.get_ptr<json::array_t*>()->reserve(std::min(limitURL, size));

    using sqlite_orm::c;
    using sqlite_orm::limit;
    using sqlite_orm::where;
    auto curTime = current_time();
    auto storageVector =
        _storage.get_all<LinkInfo>(where(c(&LinkInfo::expires_at) > curTime),
                                   limit(offsetURL, limitURL));
    for(const auto& curLink: storageVector)
    {

        json curJSON;
        curJSON["code"] = curLink.code;
        curJSON["original_url"] = curLink.original_url;
        curJSON["created_at"] = to_string(curLink.created_at);
        curJSON["expires_at"] = to_string(curLink.expires_at);
        curJSON["clicks"] = curLink.clicks;
        result.push_back(std::move(curJSON));
    }
    return result;
}

void LinkManager::saveToFile() noexcept
{
    // Yes. it's too easy
    std::cout << "Saved LinkManager to links.db" << std::endl;
}

void LinkManager::readFromFile() noexcept
{
    // Yes. it's too easy
    std::cout << "Read links.db data to LinkManager" << std::endl;
}

void LinkManager::cleanExpiredLinks() noexcept
{
    using sqlite_orm::c;
    using sqlite_orm::where;
    std::lock_guard<std::mutex> lock(_storageMutex);
    auto curTime = current_time();
    _storage.remove_all<LinkInfo>(where(c(&LinkInfo::expires_at) <= curTime));
}