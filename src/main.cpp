#include "handlers/handlers.h"
#include "storage/LinkManager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpTypes.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>

using drogon::app;
using drogon::Get;
using drogon::Post;

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

    app().loadConfigFile("config.json");
    app().addListener("localhost", 8080);

    app().registerHandler("/shorten", postOriginalLinkHandler, {Post});

    app().registerHandler("/stats", getAllStatisticHandler, {Get});

    app().registerHandler("/stats/{code}", getCodeStatisticsHandler, {Get});

    app().registerHandler("/{code}", redirectHandler, {Get});

    drogon::async_run(cleanupLinks());

    std::cout << "Server is running on localhost:8080" << std::endl;

    app().run();

    return 0;
}