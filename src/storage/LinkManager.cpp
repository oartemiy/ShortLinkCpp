#include "LinkManager.h"
#include "../gen_code/gen_code.h"
#include "../utils/json.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

using nlohmann::json;

static std::string current_iso8601_time()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// required mutex lock_guard
inline bool LinkManager::isCodeAvailable(const std::string& code) noexcept
{
    return !_storage.contains(code);
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
    _storage.insert({code, {original_url, current_iso8601_time(), 0}});
    return code;
}

const LinkInfo
LinkManager::getCodeInfo(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    return _storage[code];
}

inline void LinkManager::redirect(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    _storage[code].clicks++;
}

const std::unordered_map<std::string, LinkInfo>
LinkManager::getAllInfo() noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    return _storage;
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
        curJSON["created_at"] = info.created_at;
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
        _storage.insert({curJSON["code"],
                         {curJSON["original_url"], curJSON["created_at"],
                          curJSON["clicks"]}});
    }
    file.close();
    std::cout << "Readed db.json data to LinkManager" << std::endl;
}