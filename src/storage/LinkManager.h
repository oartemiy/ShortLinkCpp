#ifndef LINK_MANAGER_H_
#define LINK_MANAGER_H_

#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>
#include <drogon/drogon.h>
// #include "IStorage.h"
#include <cstddef>
#include <shared_mutex>
#include <stdexcept>
#include <string>

using nlohmann::json;
using drogon::Task;

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

class LinkManager
{
public:
    /*
     * NOTE: removed the const value from the return type, which kills the move
     * semantics
     *
     * +TODO: implement json returning, which will elide copping from stl
     * containers
     */

    LinkManager() = default;

    // returns code
    Task<std::string> addUrl(const std::string& original_url);

    // May throw CodeNotFoundException exception
    Task<json> getCodeInfo(const std::string& code);

    // HACK: using (json)std::vector to save order between NON-POST requests
    Task<json> getInfo(std::size_t limit, std::size_t offset);

    // May throw CodeNotFoundException exception
    Task<std::string> redirect(const std::string& code);

    // clean expired links
    Task<void> cleanExpiredLinks();

protected:
    mutable std::shared_mutex _storageMutex;  // to avoid init in cpp file
                                              // std::mutex _storageMutex;

private:
    // requires mutex lock_guard
    Task<bool> isCodeAvailable(const std::string& code);

    // requires mutex lock_guard and isCodeAvailable == true
    Task<bool> isCodeExpired(const std::string& code);
};

#endif