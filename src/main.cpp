#include "handlers/handlers.h"
#include "server/httplib.h"
#include "storage/LinkManager.h"
#include "utils/json.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <thread>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

LinkManager db;
httplib::Server* srv_ptr = nullptr;
std::thread* cleanupThread_ptr = nullptr;
std::atomic<bool> stopClean{false};

void cleanupLinks(LinkManager& dbRef)
{
    while(!stopClean.load())
    {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        if(!stopClean.load())
            db.cleanExpiredLinks();
    }
}
int main()
{
    httplib::Server srv;
    srv.new_task_queue = [] { return new httplib::ThreadPool(4); };
    srv_ptr = &srv;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // NOTE: tested all is working
    std::thread cleanupThread(cleanupLinks, std::ref(db));
    cleanupThread_ptr = &cleanupThread;

    db.readFromFile();

    srv.Post("/shorten", postOriginalLinkHandler);

    srv.Get("/stats", getAllStatisticHandler);

    srv.Get("/stats/:code", getCodeStatisticsHandler);

    srv.Get("/:code", redirectHandler);

    std::cout << "Server is running on localhost:8080" << std::endl;
    srv.listen("localhost", 8080);

    stopClean.store(true);
    // signal handler can join thread
    if(cleanupThread.joinable())
        cleanupThread.join();
    db.saveToFile();

    return 0;
}