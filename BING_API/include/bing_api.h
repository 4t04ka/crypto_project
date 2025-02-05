#ifndef EXAMPLE_BING_API_H
#define EXAMPLE_BING_API_H

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <nlohmann/json.hpp>
#include <fstream>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <fmt/core.h>
#include <boost/optional.hpp>
#include "../../CONFIG/config_constants.h"
using json = nlohmann::json;

class BING_API_ClIENT{
private:
    std::string API_KEY;
    std::string API_SECRET;
    std::string API_HOST;
    bool test;


    std::string urlEncode(const std::string& value);
    std::string computeHMACSHA256(const std::string& key, const std::string& data);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string buildParams(const std::string& payloadStr, int64_t timestamp);
    std::string buildParamsEncode(const std::string& payloadStr, int64_t timestamp);
    void restart(bool test);

public:
    BING_API_ClIENT(bool test);

    json Read_Config();
    void Overwrite_json_file(const json& new_data);
    json Get_Private_Request(const std::string& uri, const std::string& method);
    json Get_Public_Request(const std::string& uri, const std::string& method);
    json Post_Request(const std::string& api, std::string payload);
    json Get_Ticker_Leverage(const std::string& ticker);
    json Set_Ticker_Leverage(const std::string& ticker, int leverage, std::string side);
    float Get_Balance();
    json Get_Market_Prices();
    json Post_Malone(std::string side, std::string ticker, float stopLoss, float quantity, float takeProfit);
    json Trailing_Stop(std::string side, std::string ticker, float quantity, float priceRate);
    float Get_Ticker_Price(std::string ticker);
    json Get_Order_Info(std::string ticker, int64_t order);
    json Get_Info_();
    json Make_Deal(std::string ticker, std::string channel, std::string action);
};


#endif //EXAMPLE_BING_API_H