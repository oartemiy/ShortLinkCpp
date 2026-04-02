#include "gen_code/gen_code.h"
#include "server/httplib.h"
#include "storage/LinkManager.h"
#include "utils/json.hpp"
#include <atomic>
#include <csignal>
#include <exception>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

LinkManager db;
httplib::Server* srv_ptr = nullptr;

void signalHandler(int signal)
{
    std::cout << "Stop server with code: " << signal << std::endl;
    if (srv_ptr) srv_ptr->stop();
}

int main()
{
    httplib::Server srv;
    srv.new_task_queue = [] { return new httplib::ThreadPool(4); };
    srv_ptr = &srv;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);


    db.readFromFile();

    srv.Post("/shorten",
             [](const Request& req, Response& res)
             {
                 try
                 {
                     json request = json::parse(req.body);
                     std::string original_url = request.at("url");
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
             });

    srv.Get("/stats",
            [](const Request& req, Response& res)
            {
                auto allInfo = db.getAllInfo();
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
            });

    srv.Get("/stats/:code",
            [](const Request& req, Response& res)
            {
                try
                {
                    std::string code = req.path_params.at("code");
                    decltype(auto) codeInfo = db.getCodeInfo(code);
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
            });

    std::cout << "Server is running on localhost:8080" << std::endl;
    srv.listen("localhost", 8080);

    db.saveToFile();
    return 0;
}