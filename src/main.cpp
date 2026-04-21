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
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    app().addListener("localhost", 8080);

    app().addDbClient(drogon::orm::PostgresConfig{
        "localhost", 5432, "links", "oartemiy", "", 4, "default", true});
    // app().createDbClient("postgresql", "localhost", 5432, "links", "oartemiy", "");

    app().registerHandler("/shorten", PostOriginalLinkHandler{}, {Post});

    app().registerHandler("/stats", GetAllStatisticHandler{}, {Get});

    app().registerHandler("/stats/{code}", GetCodeStatisticsHandler{}, {Get});

    app().registerHandler("/{code}", RedirectHandler{}, {Get});

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