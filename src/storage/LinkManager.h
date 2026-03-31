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

class LinkNotFoundException: public StorageException
{
public:
    explicit LinkNotFoundException(const std::string& shortLink)
        : StorageException("Link with short link " + shortLink + "not found")
    {
    }
};

struct LinkInfo
{
    std::string short_link;
    std::string original_url;
    std::string created_at;
    uint64_t clicks = 0;
};

class LinkManager
{
private:
    // code -> LinkInfo
    std::unordered_map<std::string, LinkInfo> _storage;
    inline static std::mutex _storage_mutex;  // to avoid init in cpp file

public:
    bool isShortLinkAvailable(const std::string& code) const;
    
    void addShortLink(const std::string& link, const std::string& shortLink);

    const LinkInfo& getLinkInfo(const std::string& shortLink) const;

    std::vector<std::reference_wrapper<const LinkInfo>> getAllLinks() const;

    void redirect(const std::string& shortLink);
};

#endif