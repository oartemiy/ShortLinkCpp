#include "LinkManager.h"
#include "../gen_code/gen_code.h"
#include <cstddef>
#include <ctime>
#include <drogon/utils/coroutine.h>
#include <json/json.h>
#include <json/value.h>
#include <mutex>
#include <nlohmann/json.hpp>

#include <shared_mutex>
#include <string>

using drogon::app;
using nlohmann::json;

// required mutex lock_guard
Task<bool> LinkManager::isCodeAvailable(const std::string& code)
{
    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "SELECT code FROM links WHERE code = $1", code);
    co_return res.empty();
}

// TODO: optimize query
// required mutex lock_guard
Task<bool> LinkManager::isCodeExpired(const std::string& code)
{
    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "SELECT code FROM links WHERE code = $1 AND expires_at <= NOW()", code);
    co_return !res.empty();
}

Task<std::string> LinkManager::addUrl(const std::string& original_url)
{
    std::unique_lock<std::shared_mutex> lock(_storageMutex);
    std::string code;
    do
    {
        code = gen_code();
    }
    while(!co_await isCodeAvailable(
        code));  // TODO: improve problem big ammount of with sql queries
    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "INSERT INTO links (code, original_url) VALUES ($1, $2)", code,
        original_url);
    co_return code;
}

Task<json> LinkManager::getCodeInfo(const std::string& code)
{
    std::shared_lock<std::shared_mutex> lock(_storageMutex);
    if(co_await isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(co_await isCodeExpired(code))
        throw CodeTLLError(code);

    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "SELECT COALESCE(row_to_json(t), '{}'::json) FROM "
        "(SELECT code, original_url, "
        "created_at, expires_at, clicks "
        "FROM links WHERE code = $1) AS t",
        code);

    json response = json::parse(res[0][0].as<std::string>());
    co_return response;
}

Task<json> LinkManager::getInfo(std::size_t limit, std::size_t offset)
{
    std::shared_lock<std::shared_mutex> lock(_storageMutex);

    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "SELECT COALESCE(json_agg(t), '[]'::json) FROM (SELECT code, "
        "original_url, created_at, "
        "expires_at, clicks "
        "FROM links WHERE expires_at > NOW() LIMIT $1 OFFSET $2) AS t",
        limit, offset);
    json result = json::parse(res[0][0].as<std::string>());
    co_return result;
}

Task<std::string> LinkManager::redirect(const std::string& code)
{
    std::unique_lock<std::shared_mutex> lock(_storageMutex);
    if(co_await isCodeAvailable(code))
        throw CodeNotFoundException(code);
    if(co_await isCodeExpired(code))
        throw CodeTLLError(code);

    auto db = app().getFastDbClient("default");
    auto res = co_await db->execSqlCoro(
        "UPDATE links SET clicks = clicks + 1 WHERE code = $1 "
        "RETURNING original_url",
        code);

    co_return res[0][0].as<std::string>();
}

Task<void> LinkManager::cleanExpiredLinks()
{
    std::unique_lock<std::shared_mutex> lock(_storageMutex);
    auto db = app().getFastDbClient("default");
    auto res =
        co_await db->execSqlCoro("DELETE FROM links WHERE expires_at <= NOW()");
    co_return;
}