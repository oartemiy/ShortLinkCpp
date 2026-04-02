#ifndef LINK_MANAGER_H_
#define LINK_MANAGER_H_

#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

static std::string current_iso8601_time();

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

struct LinkInfo
{
    std::string original_url;
    std::string created_at;
    uint64_t clicks = 0;
};

class LinkManager
{
private:
    // code -> LinkInfo
    std::unordered_map<std::string, LinkInfo> _storage;
    std::mutex _storageMutex;  // to avoid init in cpp file

    bool isCodeAvailable(const std::string& code) noexcept;

public:
    // returns code
    std::string addUrl(const std::string& original_url) noexcept;

    // May throw CodeNotFoundException exception
    const LinkInfo& getCodeInfo(const std::string& code) noexcept(false);

    const std::unordered_map<std::string, LinkInfo>& getAllInfo() noexcept;

    // May throw CodeNotFoundException exception
    void redirect(const std::string& code) noexcept(false);

    // Saving data to file
    void saveToFile() noexcept;

    // Reading data from file
    void readFromFile() noexcept;
};

#endif