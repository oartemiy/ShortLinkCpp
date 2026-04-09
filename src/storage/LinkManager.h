#ifndef LINK_MANAGER_H_
#define LINK_MANAGER_H_

#include "../utils/json.hpp"
#include "../utils/sqlite_orm.h"
#include "IStorage.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <stdexcept>
#include <string>

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

struct LinkInfo
{
    std::string code;
    std::string original_url;
    // unsopporte for std::chrono::system_clock::time_point
    std::time_t created_at;
    std::time_t expires_at;
    uint64_t clicks = 0;
};

inline auto createStorage(const std::string& path)
{
    using sqlite_orm::make_column;
    using sqlite_orm::make_table;

    auto storage = sqlite_orm::make_storage(
        path, make_table("links",
                         make_column("code", &LinkInfo::code,
                                     sqlite_orm::primary_key()),
                         make_column("original_url", &LinkInfo::original_url),
                         make_column("created_at", &LinkInfo::created_at),
                         make_column("expires_at", &LinkInfo::expires_at),
                         make_column("clicks", &LinkInfo::clicks)));
    storage.sync_schema();
    return storage;
}

class LinkManager: public IStorage
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

    LinkManager(): _storage(createStorage("links.db")) {}

private:
    // code -> LinkInfo
    using db_type = decltype(createStorage("links.db"));  // may insert ""

    db_type _storage;
    std::mutex _storageMutex;  // to avoid init in cpp file

    // requires mutex lock_guard
    bool isCodeAvailable(const std::string& code) noexcept override;

    // requires mutex lock_guard and isCodeAvailable == true
    bool isCodeExpired(const std::string& code) noexcept override;
};

#endif