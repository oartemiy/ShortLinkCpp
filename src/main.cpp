#include "handlers/handlers.h"
#include "server/httplib.h"
#include "storage/LinkManager.h"
#include "utils/json.hpp"
#include <csignal>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

LinkManager db;
httplib::Server* srv_ptr = nullptr;

int main()
{
    httplib::Server srv;
    srv.new_task_queue = [] { return new httplib::ThreadPool(4); };
    srv_ptr = &srv;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    db.readFromFile();

    srv.Post("/shorten", postOriginalLinkHandler);

    srv.Get("/stats", getAllStatisticHandler);

    srv.Get("/stats/:code", getCodeStatisticsHandler);

    srv.Get("/:code", redirectHandler);

    std::cout << "Server is running on localhost:8080" << std::endl;
    srv.listen("localhost", 8080);

    db.saveToFile();
    return 0;
}