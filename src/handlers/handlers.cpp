#include "handlers.h"
#include "../server/httplib.h"
#include "../utils/json.hpp"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <limits>
#include <string>
#include <thread>
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
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = "Invalid JSON";
        error["details"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 400;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
}

// TODO: implement json
void getAllStatisticHandler(const Request& req, Response& res)
{
    json response;

    try
    {
        // TODO: implement postgress full support
        // size_t ~ int
        // TODO: remove KOLHOZ!!!
        std::size_t limit = req.get_param_value("limit") == ""
                                ? std::numeric_limits<std::int32_t>::max()
                                : std::stoull(req.get_param_value("limit"));
        std::size_t offset = req.get_param_value("offset") == ""
                                 ? 0
                                 : std::stoull(req.get_param_value("offset"));
        response = std::move(db.getInfo(limit, offset));
        res.set_content(response.dump(), "application/json");
        res.status = 200;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 400;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
}

// TODO: impl json
void getCodeStatisticsHandler(const Request& req, Response& res)
{
    try
    {
        std::string code = req.path_params.at("code");
        json response = db.getCodeInfo(code);
        res.set_content(response.dump(), "application/json");
        res.status = 200;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 404;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
}

void redirectHandler(const Request& req, Response& res)
{
    try
    {
        std::string code = req.path_params.at("code");
        std::string direction = db.redirect(code);
        res.set_redirect(direction, httplib::StatusCode::SeeOther_303);
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        res.set_content(error.dump(), "application/json");
        res.status = 404;
        #ifndef NDEBUG
            std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
        #endif
    }
}

void signalHandler(int signal)
{
    std::cout << "Stop server with code: " << signal << std::endl;
    std::cout << "Wait for 1 second while cleaning up db..." << std::endl;
    stopClean.store(true);
    // if(cleanupThread_ptr)
    //     cleanupThread_ptr->join();
    if(srv_ptr)
        srv_ptr->stop();
    #ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
    #endif
}