#ifndef HANDLERS_H_
#define HANDLERS_H_
#include "../server/httplib.h"
#include "../storage/LinkManager.h"
#include "../utils/json.hpp"

using httplib::Request;
using httplib::Response;
using nlohmann::json;

extern LinkManager db;
extern httplib::Server* srv_ptr;

// class UrlError: public std::runtime_error
// {
//     explicit UrlError(const std::string& err)
//         : std::runtime_error("Url: " + err + " is invalid!")
//     {
//     }
// };

void postOriginalLinkHandler(const Request& req, Response& res);

void getAllStatisticHandler(const Request& req, Response& res);

void getCodeStatisticsHandler(const Request& req, Response& res);

void signalHandler(int signal);

void redirectHandler(const Request& req, Response& res);

#endif