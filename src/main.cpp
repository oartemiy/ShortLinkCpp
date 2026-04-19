#include "handlers/handlers.h"
#include "storage/LinkManager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbConfig.h>
#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>

using drogon::app;
using drogon::Get;
using drogon::HttpRequestPtr;
using drogon::Post;

using drogon::async_run;
using drogon::HttpResponsePtr;
using nlohmann::json;

LinkManager storage;
std::atomic<bool> stopClean{false};

Task<void> cleanupLinks()
{
    while(!stopClean.load())
    {
        co_await drogon::sleepCoro(app().getLoop(), std::chrono::minutes(1));
        if(!stopClean.load())
            co_await storage.cleanExpiredLinks();
    }
}

int main()
{
    // ERROR: redirection errors
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // app().loadConfigFile("/Users/oartemiy/code/BackendCpp/config.json");
    app().addListener("localhost", 8080);

    // 2. СОЗДАЁМ клиента ДО старта фреймворка
    app().createDbClient("postgresql", "localhost", 5432, "links", "oartemiy", "");

    app().registerHandler(
        "/shorten",
        [](HttpRequestPtr req) -> Task<HttpResponsePtr>
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
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
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
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
#endif
                co_return res;
            }
        },
        {Post});

    app().registerHandler(
        "/stats",
        [](HttpRequestPtr req) -> Task<HttpResponsePtr>
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
                    std::size_t offset =
                        std::stoull(req->getParameter("offset"));
                json response =
                    std::move(co_await storage.getInfo(limit, offset));
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                res->setBody(response.dump());
                res->setStatusCode(drogon::k200OK);
#ifndef NDEBUG
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
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
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
#endif
                co_return res;
            }
        },
        {Get});

    app().registerHandler(
        "/stats/{code}",
        [](HttpRequestPtr req) -> Task<HttpResponsePtr>
        {
            std::string code = req->getParameter("code");
            try
            {
                json response = co_await storage.getCodeInfo(code);
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                res->setBody(response.dump());
                res->setStatusCode(drogon::k200OK);
#ifndef NDEBUG
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
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
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
#endif
                co_return res;
            }
        },
        {Get});

    app().registerHandler(
        "/{code}",
        [](HttpRequestPtr req) -> Task<HttpResponsePtr>
        {
            std::string code = req->getParameter("code");
            try
            {
                std::string direction = co_await storage.redirect(code);
                auto res =
                    drogon::HttpResponse::newRedirectionResponse(direction);
#ifndef NDEBUG
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
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
                std::cout << "Done by: " << std::this_thread::get_id()
                          << std::endl;
#endif
                co_return res;
            }
        },
        {Get});

    async_run(
        []() -> Task<void>
        {
            while(!stopClean.load())
            {
                co_await drogon::sleepCoro(app().getLoop(),
                                           std::chrono::minutes(1));
                if(!stopClean.load())
                    co_await storage.cleanExpiredLinks();
            }
        });

    std::cout << "Server is running on localhost:8080" << std::endl;

    app().run();

    return 0;
}