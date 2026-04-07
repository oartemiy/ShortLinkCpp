#include "handlers.h"
#include "../server/httplib.h"
#include "../utils/json.hpp"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

static std::string to_string(std::chrono::system_clock::time_point chrono_time)
{
    auto time = std::chrono::system_clock::to_time_t(chrono_time);
    std::tm tm = *std::gmtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void postOriginalLinkHandler(const Request& req, Response& res)
{
    try
    {
        json request = json::parse(req.body);
        std::string original_url = request.at("url");
        if(!original_url.starts_with("http://") &&
           !original_url.starts_with("https://"))
        {
            json error;
            error["error"] = "url: " + original_url + " is not valid!";
            res.set_content(error.dump(), "application/json");
            return;
        }
        std::string code = db.addUrl(original_url);
        json response;
        response["short_url"] = "http://localhost:8080/" + code;
        response["code"] = code;
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = "Invalid JSON";
        error["details"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 400;
    }
}

void getAllStatisticHandler(const Request& req, Response& res)
{
    std::vector<std::pair<std::string, LinkInfo>> allInfo;

    try
    {
        // TODO: remove KOLHOZ!!!
        // size_t ~ ull
        std::size_t limit = req.get_param_value("limit") == ""
                                ? std::numeric_limits<std::size_t>::max()
                                : std::stoull(req.get_param_value("limit"));
        std::size_t offset = req.get_param_value("offset") == ""
                                 ? 0ull
                                 : std::stoull(req.get_param_value("offset"));
        allInfo = db.getInfo(limit, offset);
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 400;
        return;
    }

    json response = json::array();
    response.get_ptr<json::array_t*>()->reserve(allInfo.size());
    for(const auto& [code, info]: allInfo)
    {
        json current;
        current["code"] = code;
        current["original_url"] = info.original_url;
        current["created_at"] = to_string(info.created_at);
        current["expires_at"] = to_string(info.expires_at);
        current["clicks"] = info.clicks;
        response.push_back(current);
    }
    res.set_content(response.dump(), "application/json");
}

void getCodeStatisticsHandler(const Request& req, Response& res)
{
    try
    {
        std::string code = req.path_params.at("code");
        auto codeInfo = db.getCodeInfo(code);
        json response;
        response["code"] = code;
        response["original_url"] = codeInfo.original_url;
        response["created_at"] = to_string(codeInfo.created_at);
        response["expires_at"] = to_string(codeInfo.expires_at);
        response["clicks"] = codeInfo.clicks;
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 404;
    }
}

void redirectHandler(const Request& req, Response& res)
{
    try
    {
        std::string code = req.path_params.at("code");
        std::string direction = db.redirect(code);
        res.set_redirect(direction, httplib::StatusCode::SeeOther_303);
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 404;
    }
}

void signalHandler(int signal)
{
    std::cout << "Stop server with code: " << signal << std::endl;
    std::cout << "Wait for 1 minute while cleaning up db..." << std::endl;
    stopClean.store(true);
    // if(cleanupThread_ptr)
    //     cleanupThread_ptr->join();
    if(srv_ptr)
        srv_ptr->stop();
}