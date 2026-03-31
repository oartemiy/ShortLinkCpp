#include "gen_code/gen_code.h"
#include "server/httplib.h"
#include "storage/LinkManager.h"
#include "utils/json.hpp"
#include <exception>

using httplib::Request;
using httplib::Response;

using nlohmann::json;

LinkManager db;

int main()
{
    httplib::Server srv;

    srv.Post("/shorten",
             [](const Request& req, Response& res)
             {
                 json request = json::parse(req.body);
                 std::string shortLink;
                 do
                 {
                     shortLink = gen_code();
                 }
                 while(!db.isShortLinkAvailable(shortLink));
                 db.addShortLink(request["url"], shortLink);
                 json response;
                 response["short_url"] = "http://localhost:8080/" + shortLink;
                 response["code"] = shortLink;
                 res.set_content(response.dump(), "application/json");
                 res.status = 200;
             });

    srv.Get("/stats/:code",
            [](const Request& req, Response& res)
            {
                std::string code = req.path_params.at("code");
                try
                {
                    if(db.isShortLinkAvailable(code))
                        throw LinkNotFoundException("No such link");
                    decltype(auto) info = db.getLinkInfo(code);
                    json response;
                    response["code"] = info.short_link;
                    response["original_url"] = info.original_url;
                    response["created_at"] = info.created_at;
                    response["clicks"] = info.clicks;
                    res.set_content(response.dump(), "application/json");
                    res.status = 200;
                }
                catch(const std::exception& err)
                {
                    json error;
                    error["Error"] = err.what();
                    res.set_content(error.dump(), "application/json");
                    res.status = 404;
                }
            });

    std::cout << "Server is running on localhost:8080" << std::endl;
    srv.listen("localhost", 8080);
    return 0;
}