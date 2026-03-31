#include "gen_code.h"
#include <cstddef>
#include <mutex>
#include <random>
#include <string>

std::string gen_code(std::size_t len)
{
    const char* base64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string code;
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<std::size_t> dist(0, 63);
    static std::mutex rng_mutex; // rng mutex
    code.reserve(len);
    for(std::size_t i = 0; i < len; ++i) {
        std::lock_guard<std::mutex> lock(rng_mutex);
        code.push_back(base64[dist(rng)]);
    }
    return code;
}