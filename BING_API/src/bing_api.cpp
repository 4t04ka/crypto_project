#include "../include/bing_api.h"


json BING_API_ClIENT::Read_Config(){
    std::ifstream file("/Users/aleksandrbondar/CLionProjects/example/CONFIG/ACCOUNTS_CONFIGS/config_acc_"+CONSTANTS::ACC_NUMBER +".json");

    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл!" << std::endl;
        file.close();
        restart(this->test);
    }
    // Загружаем JSON из файла
    json conf_data;
    try {
        file >> conf_data;
        file.close();
    } catch (const json::parse_error& e) {
        std::cerr << "Ошибка при разборе JSON: " << e.what() << std::endl;
        file.close();
        restart(this->test);
    }

    return conf_data;
}
void BING_API_ClIENT::Overwrite_json_file(const json& new_data) {
    // Открываем файл для записи, очищая его содержимое
    std::ofstream file("/Users/aleksandrbondar/CLionProjects/example/CONFIG/ACCOUNTS_CONFIGS/config_acc_"+CONSTANTS::ACC_NUMBER +".json", std::ios::trunc); // ios::trunc очищает файл перед записью
    if (!file.is_open()) {
        throw std::runtime_error("OPEN_CONFIG_TO_REWRITE_ERROR");

    }
    // Записываем новое содержимое в JSON-формате
    file << new_data.dump(4);  // Отформатированный вывод с отступами
    file.close();
//    std::cout << "Файл успешно перезаписан." << std::endl;
}

void BING_API_ClIENT::restart(bool test){
    *this = BING_API_ClIENT(test);
}

std::string BING_API_ClIENT::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (const char& c : value) {
        // Разрешенные символы
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // Все остальные символы кодируем
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}

std::string BING_API_ClIENT::computeHMACSHA256(const std::string& key, const std::string& data) {
    unsigned char* result;
    unsigned int len = SHA256_DIGEST_LENGTH;

    // Вычисляем HMAC
    result = HMAC(EVP_sha256(), key.c_str(), key.length(), (unsigned char*)data.c_str(), data.length(), nullptr, &len);
    std::cout << data <<std::endl;
    // Преобразуем результат в строку (hex)
    std::ostringstream hexResult;
    for (unsigned int i = 0; i < len; ++i) {
        hexResult << std::setw(2) << std::setfill('0') << std::hex << (int)result[i];
    }

    return hexResult.str();
}

size_t BING_API_ClIENT::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), realsize);
    return realsize;
}

std::string BING_API_ClIENT::buildParams(const std::string& payloadStr, int64_t timestamp) {
    std::ostringstream params;

    // Парсим строку JSON в объект
    nlohmann::json payload = nlohmann::json::parse(payloadStr);

    // Собираем параметры в правильном порядке
    if (payload.contains("symbol")) {
        params << "symbol=" << payload["symbol"].get<std::string>();
    }
    if (payload.contains("orderId")) {
        params << "&orderId=" << payload["orderId"];
    }
    if (payload.contains("side")) {
        params << "&side=" << payload["side"].get<std::string>();
    }
    if (payload.contains("positionSide")) {
        params << "&positionSide=" << payload["positionSide"].get<std::string>();
    }
    if (payload.contains("priceRate")) {
        params << "&priceRate=" << payload["priceRate"].get<std::string>();
    }
    if (payload.contains("type")) {
        params << "&type=" << payload["type"].get<std::string>();
    }
    if (payload.contains("quantity")) {
        params << "&quantity=" << payload["quantity"];
    }

    if (payload.contains("leverage")) {
        params << "&leverage=" << payload["leverage"].get<std::string>();
    }

    if (payload.contains("stopLoss")) {
        params << "&stopLoss=" << urlEncode(payload["stopLoss"].dump());
    }
    if (payload.contains("takeProfit")) {
        params << "&takeProfit=" << urlEncode(payload["takeProfit"].dump());
    }
    // Добавляем timestamp
    params << "&timestamp=" << timestamp;

    return params.str();
}

std::string BING_API_ClIENT::buildParamsEncode(const std::string& payloadStr, int64_t timestamp) {
    std::ostringstream params;

    // Парсим строку JSON в объект
    nlohmann::json payload = nlohmann::json::parse(payloadStr);

    // Собираем параметры в правильном порядке
    if (payload.contains("symbol")) {
        params << "symbol=" << payload["symbol"].get<std::string>();
    }
    if (payload.contains("orderId")) {
        params << "&orderId=" << payload["orderId"];
    }
    if (payload.contains("side")) {
        params << "&side=" << payload["side"].get<std::string>();
    }
    if (payload.contains("positionSide")) {
        params << "&positionSide=" << payload["positionSide"].get<std::string>();
    }
    if (payload.contains("priceRate")) {
        params << "&priceRate=" << payload["priceRate"].get<std::string>();
    }
    if (payload.contains("type")) {
        params << "&type=" << payload["type"].get<std::string>();
    }
    if (payload.contains("quantity")) {
        params << "&quantity=" << payload["quantity"];
    }
    if (payload.contains("leverage")) {
        params << "&leverage=" << payload["leverage"].get<std::string>();
    }
    if (payload.contains("stopLoss")) {
        params << "&stopLoss=" << (payload["stopLoss"].dump());
    }
    if (payload.contains("takeProfit")) {
        params << "&takeProfit=" << (payload["takeProfit"].dump());
    }
    // Добавляем timestamp
    params << "&timestamp=" << timestamp;

    return params.str();
}

//public realisations
BING_API_ClIENT::BING_API_ClIENT(bool test):test(test){

    if (test) API_HOST = "https://open-api-vst.bingx.com";
    else API_HOST = "https://open-api.bingx.com";

    json conf_data;
    try {
        conf_data = Read_Config();
    }
    catch (std::exception & e){
        std::cerr << "Общая ошибка: " << e.what() << std::endl;
        throw std::runtime_error(e.what());
    }


    API_KEY = conf_data["Bingx_API_KEY"];
    API_SECRET = conf_data["Bingx_API_SECRET"];
}

json BING_API_ClIENT::Get_Private_Request(const std::string& uri, const std::string& method) {
    // Получение текущего времени в миллисекундах
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Формирование строки параметров
    std::ostringstream parameters;
    parameters << "timestamp=" << timestamp;
    // Вычисление подписи
    std::string sign = computeHMACSHA256(API_SECRET, parameters.str());
    std::ostringstream url;
    url << API_HOST << uri << "?" << parameters.str() << "&signature=" << sign;
//        std::cout << host;

    // Настройка cURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL." << std::endl;
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL");
    }
    json j;
    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    std::cout<< url.str() << std::endl;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Выполнение запроса
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL");
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP Response Code: " << http_code << std::endl;
        std::cout << "Response body: " << responseBuffer << std::endl;
        j = json::parse(responseBuffer);
    }

    // Очистка ресурсов
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return j;
};

json BING_API_ClIENT::Get_Public_Request(const std::string& uri, const std::string& method) {
    // Получение текущего времени в миллисекундах
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Формирование строки параметров
    std::ostringstream parameters;
    // Вычисление подписи
    std::string sign = computeHMACSHA256(API_SECRET, parameters.str());
    std::ostringstream url;
    url << API_HOST<< uri << "?" << parameters.str() << "&signature=" << sign;
//        std::cout << host;

    // Настройка cURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL." << std::endl;
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL.");
    }
    json j;
    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    std::cout<< url.str() << std::endl;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Выполнение запроса
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::logic_error("СURL error: ");
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
//        std::cout << "HTTP Response Code: " << http_code << std::endl;
//        std::cout << "Response body: " << responseBuffer << std::endl;
        j = json::parse(responseBuffer);
    }

    // Очистка ресурсов
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return j;
};

json BING_API_ClIENT::Post_Request(const std::string& api, std::string payload) {
    // Получаем текущий таймстемп
    std::string method = "POST";
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    nlohmann::json jsonPayload = nlohmann::json::parse(payload);
    // Кодируем только значения stopLoss и takeProfit
    if (jsonPayload.contains("stopLoss")) {
        jsonPayload["stopLoss"] = urlEncode(jsonPayload["stopLoss"].dump());
    }

    if (jsonPayload.contains("takeProfit")) {
        jsonPayload["takeProfit"] = urlEncode(jsonPayload["takeProfit"].dump());
    }

    // Преобразуем обратно в строку
    std::string encodedPayload = jsonPayload.dump();
    std::string params = buildParams(payload, timestamp);
//    std::cout<<params<< std::endl;
    // Вычисляем HMAC-SHA256

//    std::cout << "Payl" << encodedPayload << std::endl;
    std::string signature =  computeHMACSHA256(API_SECRET,buildParamsEncode(payload, timestamp));

    // Формируем полный URL
    std::string url = API_HOST + api + "?" + (params) + ("&signature=") + (signature);

//    std::cout << "Final URL: " << url << std::endl;
    nlohmann::json responseJson;
    // Выполняем HTTP-запрос
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
        headers = curl_slist_append(headers, "Content-Length: 0");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }

        std::string responseBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);


        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L); // Тайм-аут в секундах
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L); // Тайм-аут подключения
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        CURLcode res = curl_easy_perform(curl);
        std::string response;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);



        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::logic_error("СURL error: ");
        } else {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode == 400 && response.empty()) {
                std::cerr << "Error 400: Server returned no response body." << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw std::logic_error("Error 400: Server returned no response body.");
            }
            try {
                responseJson = nlohmann::json::parse(responseBuffer);
                if (responseJson.contains("data")) {
                    std::cout << "Data: " << responseJson["data"] << std::endl;
                } else {
                    std::cout << "No data found in response." << std::endl;
                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                    throw std::logic_error("No data found in response.");
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw std::logic_error(e.what());
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    }
    else {
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL.");
    }
    return responseJson;
}

json BING_API_ClIENT::Get_Ticker_Leverage(const std::string& ticker) {
    const std::string api = "/openApi/swap/v2/trade/leverage";
    const std::string method = "GET";

    // Получение текущего времени в миллисекундах
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Формирование строки параметров
    std::ostringstream parameters;
    std::string payload = fmt::format(
            R"({{"symbol": "{}"}})", ticker);
    parameters << buildParamsEncode(payload, timestamp);
    // Вычисление подписи
    std::string sign = computeHMACSHA256(API_SECRET, parameters.str());
    std::ostringstream url;
    url << API_HOST << api << "?" << parameters.str() << "&signature=" << sign;
//        std::cout << host;

    // Настройка cURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL." << std::endl;
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL for Get Ticker Leverage");
//        return boost::none;
    }
    json j;
    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    std::cout<< url.str() << std::endl;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Выполнение запроса
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::logic_error(curl_easy_strerror(res));
//        return boost::none;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP Response Code: " << http_code << std::endl;
        std::cout << "Response body: " << responseBuffer << std::endl;
        j = json::parse(responseBuffer);
    }

    // Очистка ресурсов
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return j;
};

json BING_API_ClIENT::Set_Ticker_Leverage(const std::string& ticker, int leverage, std::string side){

    const std::string api = "/openApi/swap/v2/trade/leverage";
    const std::string method = "POST";

    // Получение текущего времени в миллисекундах
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Формирование строки параметров
    std::ostringstream parameters;
    std::string payload = fmt::format(
            R"({{
                    "symbol": "{}",
                    "side": "{}",
                    "leverage": "{}"
                    }})", ticker, side, leverage);
    parameters << buildParamsEncode(payload, timestamp);
    // Вычисление подписи
    std::string sign = computeHMACSHA256(API_SECRET, parameters.str());
//        std::ostringstream url;
//        url << API_HOST << api << "?" << parameters.str() << "&signature=" << sign;

    std::string url = API_HOST + api + "?" + (parameters.str()) + ("&signature=") + (sign);

//    std::cout << "Final URL: " << url << std::endl;
    nlohmann::json responseJson;
    // Выполняем HTTP-запрос
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
        headers = curl_slist_append(headers, "Content-Length: 0");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }

        std::string responseBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);


        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L); // Тайм-аут в секундах
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L); // Тайм-аут подключения
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        CURLcode res = curl_easy_perform(curl);
        std::string response;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);



        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::logic_error("СURL error: ");
        } else {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode == 400 && response.empty()) {
                std::cerr << "Error 400: Server returned no response body." << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw std::logic_error("Error 400: Server returned no response body.");
            }
            try {
                responseJson = nlohmann::json::parse(responseBuffer);
                if (responseJson.contains("data")) {
                    std::cout << "Data: " << responseJson["data"] << std::endl;
                } else {
                    std::cout << "No data found in response." << std::endl;
                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                    throw std::logic_error("No data found in response.");
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw std::logic_error(e.what());
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    }
    else{
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL.");
    }
    return responseJson;
}

float BING_API_ClIENT::Get_Balance(){
    json res = Get_Private_Request("/openApi/swap/v3/user/balance", "GET");
    return std::stof(res["data"][0]["balance"].get<std::string>());;
};

json BING_API_ClIENT::Get_Market_Prices(){
    return Get_Public_Request("/openApi/swap/v2/quote/price", "GET");
};

json BING_API_ClIENT::Post_Malone(std::string side, std::string ticker, float stopLoss, float quantity, float takeProfit){

    std::string action;
    if (side == "LONG") action = "BUY";
    else action = "SELL";

    std::string payload = fmt::format(R"({{
        "symbol": "{}",
        "side": "{}",
        "positionSide": "{}",
        "type": "{}",
        "quantity": {},
        "stopLoss": {{
            "type": "{}",
            "stopPrice": {},
            "price": {},
            "workingType": "{}"
        }},
        "takeProfit": {{
            "type": "{}",
            "stopPrice": {},
            "price": {},
            "workingType": "{}"
        }}
        }})",ticker, action, side, "MARKET", quantity,"STOP_MARKET", stopLoss, stopLoss, "MARK_PRICE","TAKE_PROFIT_MARKET", takeProfit, takeProfit, "MARK_PRICE");
    return Post_Request("/openApi/swap/v2/trade/order",  payload);
}

json BING_API_ClIENT::Trailing_Stop(std::string side, std::string ticker, float quantity, float priceRate){
    std::string action;
    if (side != "LONG") action = "BUY";
    else action = "SELL";

    std::string payload = fmt::format(R"({{
        "symbol": "{}",
        "side": "{}",
        "positionSide": "{}",
        "priceRate": "{}",
        "type": "{}",
        "quantity": {}
        }})",ticker, action, side, priceRate,"TRAILING_STOP_MARKET", quantity);
    return Post_Request("/openApi/swap/v2/trade/order",  payload);
}

float BING_API_ClIENT::Get_Ticker_Price(std::string ticker){
    json json_data = Get_Market_Prices();
    if (json_data.contains("data")) {
        for (const auto& item : json_data["data"]) {
            if (item.contains("symbol") && item["symbol"] == ticker) {
                if (item.contains("price")) {
                    return stof(item["price"].get<std::string>());
                }
            }
        }
    }
    throw std::logic_error("NOT_FOUND_TICKER_ERROR");
}

json BING_API_ClIENT::Get_Order_Info(std::string ticker, int64_t order){
    const std::string api = "/openApi/swap/v2/trade/order";
    const std::string method = "GET";

    // Получение текущего времени в миллисекундах
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Формирование строки параметров
    std::ostringstream parameters;
    std::string payload = fmt::format(
            R"({{"symbol": "{}", "orderId": {} }})", ticker, order);
    parameters << buildParamsEncode(payload, timestamp);
    // Вычисление подписи
    std::string sign = computeHMACSHA256(API_SECRET, parameters.str());
    std::ostringstream url;
    url << API_HOST << api << "?" << parameters.str() << "&signature=" << sign;
//        std::cout << host;

    // Настройка cURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL." << std::endl;
        curl_easy_cleanup(curl);
        throw std::logic_error("Failed to initialize CURL for Get Ticker Leverage");
//        return boost::none;
    }
    json j;
    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    std::cout<< url.str() << std::endl;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-BX-APIKEY: " + API_KEY).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Выполнение запроса
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::logic_error(curl_easy_strerror(res));
//        return boost::none;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP Response Code: " << http_code << std::endl;
        std::cout << "Response body: " << responseBuffer << std::endl;
        j = json::parse(responseBuffer);
    }

    // Очистка ресурсов
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return j;
}

json BING_API_ClIENT::Get_Info_(){
    json conf = Read_Config();
    conf["balance"] = Get_Balance();
    return conf;
}

json BING_API_ClIENT::Make_Deal(std::string ticker, std::string channel, std::string action){
    try {
        if (CONSTANTS::NOT_TRADING_LIST.find(ticker) != CONSTANTS::NOT_TRADING_LIST.end()){
            throw std::logic_error("NOT_TRADING_TICKER");
        }
        //Получение данных из config
        json data_config = Read_Config();

        int max_leverage = data_config["channels"][channel]["MAX_LEVERAGE"];
        int take_profit = data_config["channels"][channel]["TAKE_PROFIT"];
        int stop_loss = data_config["channels"][channel]["STOP_LOSS"];
        int trailing_stop = data_config["channels"][channel]["TRAILING_STOP"];


        float ticker_price = Get_Ticker_Price(ticker);
//        std::cout << ticker_price << std::endl;

        //Получение выставленного плеча
        json leverage_data = Get_Ticker_Leverage(ticker);


        int leverage;
        float availableVol;

        if (action == "LONG") {
            leverage = leverage_data["data"]["longLeverage"];
            availableVol = std::stof(leverage_data["data"]["availableLongVol"].get<std::string>());
        } else {
            leverage = leverage_data["data"]["shortLeverage"];
            availableVol = std::stof(leverage_data["data"]["availableShortVol"].get<std::string>());
        }

        //Непосредственно вычисления
        float quantity = availableVol * 0.85;
        float takeProfit;
        float stopLoss;
        float trailingStop;

        if (action == "LONG") { takeProfit = ticker_price * (1 + (((((leverage / 1.0) / max_leverage) * take_profit) /
                                                                   100.0) / max_leverage));
        }
        else { takeProfit = ticker_price *
                            (1 - (((((leverage / 1.0) / max_leverage) * take_profit) / 100.0) / max_leverage));
        }

        if (action == "LONG") { stopLoss = ticker_price * (1.0 -
                                                           (((((leverage / 1.0) / max_leverage) * stop_loss) / 100.0) /
                                                            max_leverage));
        }
        else { stopLoss = ticker_price *
                          (1.0 + (((((leverage / 1.0) / max_leverage) * stop_loss) / 100.0) / max_leverage));
        }


        trailingStop = ((((leverage / 1.0) / max_leverage) * trailing_stop) / 100.0) / max_leverage;

        //Открытие позиции
        json request = Post_Malone(action, ticker, stopLoss, quantity, takeProfit);
        Trailing_Stop(action, ticker, quantity, trailingStop);



//        std::cout << req.dump(4) << std::endl;
        if (!request.contains("data")){
            json responce;
            responce["error"] = "Пустой ответ сервера";
            return responce;
        }
        std::cout << request.dump(4) << std::endl;
        json order_info = Get_Order_Info(ticker, request["data"]["order"]["orderId"].get<int64_t>());
        std::cout << order_info.dump(4) << std::endl;

        request["avgPrice"] = order_info["data"]["order"]["avgPrice"].get<std::string>();
        request["time"] = order_info["data"]["order"]["time"].get<long long>();
        request["commission"] = order_info["data"]["order"]["commission"].get<std::string>();
        request["leverage"] = order_info["data"]["order"]["leverage"].get<std::string>();

        return request;
    }
    catch (std::exception &e){
        json responce;
        responce["error"] = e.what();
        return responce;
    }


}
//int main() {
//    // Ваш API-секрет
//
//
////    clock_t now = clock();
//    BING_API_ClIENT client = BING_API_ClIENT();
//    json a = client.Post_Request("https", "open-api-vst.bingx.com", "/openApi/swap/v2/trade/order", "POST");
//    json b = client.Get_Request("https","open-api-vst.bingx.com", "/openApi/swap/v2/quote/price", "GET");
////    std::cout << a.dump(4) << std::endl;
//    std::cout << b.dump(4) << std::endl;
////    clock_t end = clock();
////    double time_taken = double(end - now) / CLOCKS_PER_SEC;
////    std::cout <<std::endl<< time_taken<< std::endl;
//
////    get_config();
//    return 0;
//}
