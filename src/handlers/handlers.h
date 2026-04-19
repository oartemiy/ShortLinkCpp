#ifndef HANDLERS_H_
#define HANDLERS_H_

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include "../storage/LinkManager.h"
#include <drogon/utils/coroutine.h>
#include <nlohmann/json.hpp>
#include <string>
#include <atomic>

using nlohmann::json;
using drogon::HttpRequestPtr;
using drogon::Task;
using drogon::app;
using drogon::HttpResponsePtr;

extern LinkManager storage;
extern std::atomic<bool> stopClean;

Task<HttpResponsePtr> postOriginalLinkHandler(HttpRequestPtr req);

Task<HttpResponsePtr> getAllStatisticHandler(HttpRequestPtr req);

Task<HttpResponsePtr> getCodeStatisticsHandler(HttpRequestPtr req);

void signalHandler(int signal);

Task<HttpResponsePtr> redirectHandler(HttpRequestPtr req);

#endif