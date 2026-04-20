#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "../storage/LinkManager.h"
#include <atomic>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>
#include <string>

using drogon::app;
using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::Task;
using nlohmann::json;

extern LinkManager storage;
extern std::atomic<bool> stopClean;

class PostOriginalLinkHandler
{
public:
    Task<HttpResponsePtr> operator()(HttpRequestPtr req);
};

class GetAllStatisticHandler
{
public:
    Task<HttpResponsePtr> operator()(HttpRequestPtr req);
};

class GetCodeStatisticsHandler
{
public:
    Task<HttpResponsePtr> operator()(HttpRequestPtr req, std::string code);
};

class RedirectHandler
{
public:
    Task<HttpResponsePtr> operator()(HttpRequestPtr req, std::string code);
};

void signalHandler(int signal);


#endif