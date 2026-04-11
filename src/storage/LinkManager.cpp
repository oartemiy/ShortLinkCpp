#include "LinkManager.h"
#include "../gen_code/gen_code.h"
#include "../utils/json.hpp"
#include <cstddef>
#include <ctime>
#include <mutex>
#include <pqxx/pqxx>
#include <string>

using nlohmann::json;

// static std::string to_string(std::chrono::system_clock::time_point chrono_time)
// {
//     auto time = std::chrono::system_clock::to_time_t(chrono_time);
//     std::tm tm = *std::gmtime(&time);
//     std::stringstream ss;
//     ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
//     return ss.str();
// }

// inline std::chrono::system_clock::time_point current_time()
// {
//     return std::chrono::system_clock::now();
// }

// required mutex lock_guard
inline bool LinkManager::isCodeAvailable(const std::string& code) noexcept
{
    pqxx::work find(_cx);
    auto res =
        find.exec("SELECT code FROM links WHERE code = $1", pqxx::params(code));
    find.commit();
    return res.empty();
}

// TODO: optimize query
// required mutex lock_guard
inline bool LinkManager::isCodeExpired(const std::string& code) noexcept
{
    pqxx::work find(_cx);
    auto res = find.exec(
        "SELECT code FROM links WHERE code = $1 AND expires_at <= NOW()",
        pqxx::params(code));
    find.commit();
    return !res.empty();
}

std::string LinkManager::addUrl(const std::string& original_url) noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    std::string code;
    do
    {
        code = gen_code();
    }
    while(!isCodeAvailable(
        code));  // TODO: improve problem big ammount of with sql queries
    pqxx::work insert(_cx);
    insert.exec("INSERT INTO links (code, original_url) VALUES ($1, $2)",
                pqxx::params(code, original_url));
    insert.commit();
    return code;
}

json LinkManager::getCodeInfo(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(isCodeExpired(code))
        throw CodeTLLError(code);

    pqxx::work select(_cx);
    // null protection
    auto res =
        select.exec("SELECT COALESCE(row_to_json(t), '{}'::json) FROM (SELECT code, original_url, "
                    "created_at, expires_at, clicks "
                    "FROM links WHERE code = $1) AS t",
                    pqxx::params(code));
    select.commit();

    json response = json::parse(res[0][0].as<std::string>());
    return response;
}

json LinkManager::getInfo(std::size_t limit, std::size_t offset) noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);

    pqxx::work select(_cx);
    auto res = select.exec(
        "SELECT COALESCE(json_agg(t), '[]'::json) FROM (SELECT code, original_url, created_at, "
        "expires_at, clicks "
        "FROM links WHERE expires_at > NOW() LIMIT $1 OFFSET $2) AS t",
        pqxx::params(limit, offset));
    select.commit();
    json result = json::parse(res[0][0].as<std::string>());
    return result;
}

std::string LinkManager::redirect(const std::string& code)
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    if(isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(isCodeExpired(code))
        throw CodeTLLError(code);

    pqxx::work update(_cx);
    auto res =
        update.exec("UPDATE links SET clicks = clicks + 1 WHERE code = $1 "
                    "RETURNING original_url",
                    pqxx::params(code));
    update.commit();
    return res[0][0].as<std::string>();
}

void LinkManager::cleanExpiredLinks() noexcept
{
    std::lock_guard<std::mutex> lock(_storageMutex);
    pqxx::work clean(_cx);
    auto res = clean.exec("DELETE FROM links WHERE expires_at <= NOW()");
    clean.commit();
}