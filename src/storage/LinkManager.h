#ifndef LINK_MANAGER_H_
#define LINK_MANAGER_H_

#include "../utils/json.hpp"
#include "IStorage.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

using nlohmann::json;

// Errors

// Base
class StorageException: public std::runtime_error
{
public:
    explicit StorageException(const std::string& msg): std::runtime_error(msg)
    {
    }
};

class CodeNotFoundException: public StorageException
{
public:
    explicit CodeNotFoundException(const std::string& code)
        : StorageException("Link with code: " + code + " not found")
    {
    }
};

class CodeTLLError: public StorageException
{
public:
    explicit CodeTLLError(const std::string& code)
        : StorageException("Code: " + code + " has expired its time") {};
};

class LinkManager : public IStorage
{
public:
    /*
     * NOTE: removed the const value from the return type, which kills the move
     * semantics
     *
     * TODO: implement json returning, which will elide copping from stl
     * containers
     */

    // returns code
    std::string addUrl(const std::string& original_url) noexcept override;

    // May throw CodeNotFoundException exception
    json getCodeInfo(const std::string& code) override;

    // HACK: using (json)std::vector to save order between NON-POST requests
    json getInfo(std::size_t limit, std::size_t offset) noexcept override;

    // May throw CodeNotFoundException exception
    std::string redirect(const std::string& code) override;

    // Saving data to file
    void saveToFile() noexcept override;

    // Reading data from file
    void readFromFile() noexcept override;

    // clean expired links
    void cleanExpiredLinks() noexcept override;

private:
    struct LinkInfo
    {
        std::string original_url;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expires_at;
        uint64_t clicks = 0;
    };

    // code -> LinkInfo
    std::unordered_map<std::string, LinkInfo> _storage;
    std::mutex _storageMutex;  // to avoid init in cpp file

    // requires mutex lock_guard
    bool isCodeAvailable(const std::string& code) noexcept override;

    // requires mutex lock_guard and isCodeAvailable == true
    bool isCodeExpired(const std::string& code) noexcept override;
};

#endif