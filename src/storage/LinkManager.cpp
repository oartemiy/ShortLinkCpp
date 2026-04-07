#include "LinkManager.h"
#include "../gen_code/gen_code.h"
#include "../utils/json.hpp"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

using nlohmann::json;

static std::string to_string(std::chrono::system_clock::time_point chrono_time)
{
    auto time = std::chrono::system_clock::to_time_t(chrono_time);
    std::tm tm = *std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

inline std::chrono::system_clock::time_point current_time()
{
    return std::chrono::system_clock::now();
}

// required mutex lock_guard
inline bool LinkManager::isCodeAvailable(const std::string& code) noexcept
{
    return !_storage.contains(code);
}

inline bool LinkManager::isCodeExpired(const std::string& code) noexcept
{
    auto curTime = current_time();
    return _storage[code].expires_at <= curTime;
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
    auto expiresAt = createdAt + std::chrono::minutes(2);
    _storage.insert({code, {original_url, createdAt, expiresAt, 0ull}});
    return code;
}

json LinkManager::getCodeInfo(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(isCodeExpired(code))
        throw CodeTLLError(code);
    const LinkInfo& codeInfo = _storage[code];
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
    _storage[code].clicks++;
    return _storage[code].original_url;
}

// std::unordered_map<std::string, LinkInfo> LinkManager::getAllInfo() noexcept
// {
//     std::lock_guard<std::mutex> lock(_storageMutex);
//     std::unordered_map<std::string, LinkInfo> storageCopy;
//     auto curTime = current_time();
//     for(const auto& [code, info]: _storage)
//         if(info.expires_at > curTime)
//             storageCopy.insert({code, info});
//     return storageCopy;
//     // return _storage;
// }

json LinkManager::getInfo(std::size_t limit, std::size_t offset) noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    json result = json::array();
    result.get_ptr<json::array_t*>()->reserve(std::min(limit, _storage.size()));
    auto it = _storage.cbegin();
    auto curTime = current_time();
    while(result.size() < limit && it != _storage.cend())
    {
        if(offset == 0 && it->second.expires_at > curTime)
        {
            json curJSON;
            curJSON["code"] = it->first;
            curJSON["original_url"] = it->second.original_url;
            curJSON["created_at"] = to_string(it->second.created_at);
            curJSON["expires_at"] = to_string(it->second.expires_at);
            curJSON["clicks"] = it->second.clicks;
            result.push_back(std::move(curJSON));
        }
        ++it;

        if(offset != 0)
            --offset;
    }
    return result;
}

void LinkManager::saveToFile() noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    std::ofstream file("db.json");
    if(!file.is_open())
    {
        std::cerr << "No db.json file. Stop saving" << std::endl;
        return;
    }
    json currentSaveJSON = json::array();
    currentSaveJSON.get_ptr<json::array_t*>()->reserve(_storage.size());
    for(const auto& [code, info]: _storage)
    {
        json curJSON;
        curJSON["code"] = code;
        curJSON["original_url"] = info.original_url;
        curJSON["created_at"] =
            std::chrono::system_clock::to_time_t(info.created_at);
        curJSON["expires_at"] =
            std::chrono::system_clock::to_time_t(info.expires_at);
        curJSON["clicks"] = info.clicks;
        currentSaveJSON.push_back(curJSON);
        // std::cout << curJSON.dump(4) << std::endl;
    }
    file << currentSaveJSON.dump(4);
    file.close();
    std::cout << "Saved LinkManager to db.json" << std::endl;
}

void LinkManager::readFromFile() noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    std::ifstream file("db.json");
    if(!file.is_open())
    {
        std::cerr << "No db.json file. Stop reading" << std::endl;
        return;
    }
    json currentSaveJSON;
    file >> currentSaveJSON;
    for(const auto& curJSON: currentSaveJSON)
    {
        _storage.insert(
            {curJSON["code"],
             {curJSON["original_url"],
              std::chrono::system_clock::from_time_t(curJSON["created_at"]),
              std::chrono::system_clock::from_time_t(curJSON["expires_at"]),
              curJSON["clicks"]}});
    }
    file.close();
    std::cout << "Read db.json data to LinkManager" << std::endl;
}

void LinkManager::cleanExpiredLinks() noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    std::unordered_map<std::string, LinkInfo> newStorage;
    auto curTime = current_time();
    for(auto& [code, info]: _storage)
        if(info.expires_at > curTime)
            newStorage.insert({code, std::move(info)});
    _storage = std::move(newStorage);
}