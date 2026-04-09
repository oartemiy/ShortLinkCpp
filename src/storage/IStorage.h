#ifndef ISTORAGE_H_
#define ISTORAGE_H_

#include "../utils/json.hpp"
#include <string>

using nlohmann::json;

class IStorage
{
public:
    // returns code
    virtual std::string addUrl(const std::string& original_url) noexcept = 0;

    // May throw CodeNotFoundException exception
    virtual json getCodeInfo(const std::string& code) = 0;

    // HACK: using (json)std::vector to save order between NON-POST requests
    virtual json getInfo(std::size_t limit, std::size_t offset) noexcept = 0;

    // May throw CodeNotFoundException exception
    virtual std::string redirect(const std::string& code) = 0;

    // Saving data to file
    virtual void saveToFile() noexcept = 0;

    // Reading data from file
    virtual void readFromFile() noexcept = 0;

    // clean expired links
    virtual void cleanExpiredLinks() noexcept = 0;

private:
    // IStorage data various

    // requires mutex lock_guard
    virtual bool isCodeAvailable(const std::string& code) noexcept = 0;

    // requires mutex lock_guard and isCodeAvailable == true
    virtual bool isCodeExpired(const std::string& code) noexcept = 0;
};

#endif