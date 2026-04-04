#include "handlers.h"
#include "../server/httplib.h"
#include "../utils/json.hpp"
#include <cstddef>
#include <exception>
#include <string>
#include <unordered_map>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

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
    std::unordered_map<std::string, LinkInfo> allInfo;
    if(req.has_param("limit"))
    {
        // size_t ~ ull
        try
        {
            std::size_t limit = std::stoull(req.get_param_value("limit"));
            allInfo = db.getLimitInfo(limit);
        }
        catch(const std::exception& err)
        {
            json error;
            error["error"] = err.what();
            res.set_content(error.dump(), "application/json");
            res.status = 400;
            return;
        }
    }
    else
    {
        allInfo = db.getAllInfo();
    }
    json response = json::array();
    response.get_ptr<json::array_t*>()->reserve(allInfo.size());
    for(const auto& [code, info]: allInfo)
    {
        json current;
        current["code"] = code;
        current["original_url"] = info.original_url;
        current["created_at"] = info.created_at;
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
        response["created_at"] = codeInfo.created_at;
        response["clicks"] = codeInfo.clicks;
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    }
    catch(const CodeNotFoundException& err)
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
    catch(const CodeNotFoundException& err)
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
    if(srv_ptr)
        srv_ptr->stop();
}