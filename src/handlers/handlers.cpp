#include "handlers.h"
#include <cstddef>
#include <ctime>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <exception>
#include <json/value.h>
#include <string>
#include <thread>

Task<HttpResponsePtr> postOriginalLinkHandler(HttpRequestPtr req)
{
    try
    {
        json requestJSON = json::parse(req->getBody());
        std::string original_url = requestJSON.at("url");
        if(!original_url.starts_with("http://") &&
           !original_url.starts_with("https://"))
        {
            json error;
            error["error"] = "url: " + original_url + " is not valid!";
            auto res = drogon::HttpResponse::newHttpResponse();
            res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            req->setBody(error.dump());
            res->setStatusCode(drogon::k400BadRequest);
            co_return res;
        }
        std::string code = co_await storage.addUrl(original_url);
        json response;
        response["short_url"] = "http://localhost:8080/" + code;
        response["code"] = code;
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(response.dump());
        res->setStatusCode(drogon::k200OK);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = "Invalid JSON";
        error["details"] = err.what();
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(error.dump());
        res->setStatusCode(drogon::k400BadRequest);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
}

// TODO: implement json
Task<HttpResponsePtr> getAllStatisticHandler(HttpRequestPtr req)
{
    try
    {
        // TODO: implement postgress full support
        // size_t ~ int
        // +TODO: remove KOLHOZ!!!
        std::size_t limit = 100;
        std::size_t offset = 0;
        if(req->parameters().contains("limit"))
            std::size_t limit = std::stoull(req->getParameter("limit"));
        if(req->parameters().contains("offset"))
            std::size_t offset = std::stoull(req->getParameter("offset"));
        json response = std::move(co_await storage.getInfo(limit, offset));
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(response.dump());
        res->setStatusCode(drogon::k200OK);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
    catch(const std::exception& err)
    {
        json error;
        error["error"] = err.what();
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(error.dump());
        res->setStatusCode(drogon::k400BadRequest);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
}

// TODO: impl json
Task<HttpResponsePtr> getCodeStatisticsHandler(HttpRequestPtr req,
                                               std::string code)
{
    try
    {
        json response = co_await storage.getCodeInfo(code);
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(response.dump());
        res->setStatusCode(drogon::k200OK);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(error.dump());
        res->setStatusCode(drogon::k400BadRequest);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
}

Task<HttpResponsePtr> redirectHandler(HttpRequestPtr req, std::string code)
{
    try
    {
        std::string direction = co_await storage.redirect(code);
        auto res = drogon::HttpResponse::newRedirectionResponse(direction);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
    catch(const StorageException& err)
    {
        json error;
        error["error"] = err.what();
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        res->setBody(error.dump());
        res->setStatusCode(drogon::k400BadRequest);
#ifndef NDEBUG
        std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
        co_return res;
    }
}

void signalHandler(int signal)
{
    std::cout << "Stop server with code: " << signal << std::endl;
    std::cout << "Wait for 1 second while stopping cleaning up db..."
              << std::endl;
    stopClean.store(true);
    app().getLoop()->queueInLoop([] { app().quit(); });
#ifndef NDEBUG
    std::cout << "Done by: " << std::this_thread::get_id() << std::endl;
#endif
}