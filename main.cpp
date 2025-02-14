#include "BING_API/include/bing_api.h"
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <iomanip>


// overloaded
namespace detail {
    template <class... Fs>
    struct overload;

    template <class F>
    struct overload<F> : public F {
        explicit overload(F f) : F(f) {
        }
    };
    template <class F, class... Fs>
    struct overload<F, Fs...>
            : public overload<F>
                    , public overload<Fs...> {
        overload(F f, Fs... fs) : overload<F>(f), overload<Fs...>(fs...) {
        }
        using overload<F>::operator();
        using overload<Fs...>::operator();
    };
}  // namespace detail

template <class... F>
auto overloaded(F... f) {
    return detail::overload<F...>(f...);
}

namespace td_api = td::td_api;


class Client{
public:
    Client(BING_API_ClIENT &cl):client(cl){

        glob_api_id = client.conf_data["TG_ID"].get<int64_t>();
        glob_output_chat_id = CHATS::OUTPUT_CHAT_ID;
        glob_api_hash = client.conf_data["TG_SECRET"].get<std::string>();
        td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));
        client_manager_ = std::make_unique<td::ClientManager>();
        client_id_ = client_manager_->create_client_id();
        send_query(td_api::make_object<td_api::getOption>("version"), {});



    }

    void send_text(std::int64_t chat_id, std::string text){
        std::cout << "Sending message to chat " << chat_id << "..." << std::endl;
        auto send_message = td_api::make_object<td_api::sendMessage>();
        send_message->chat_id_ = chat_id;
        auto message_content = td_api::make_object<td_api::inputMessageText>();
        message_content->text_ = td_api::make_object<td_api::formattedText>();
        message_content->text_->text_ = std::move(text);
        send_message->input_message_content_ = std::move(message_content);

        send_query(std::move(send_message), {});
    }

    std::string format_time_from_unix(int64_t unix_timestamp) {
        std::time_t timestamp = static_cast<std::time_t>(unix_timestamp);  // Преобразуем миллисекунды в секунды
        std::tm* time_info = std::localtime(&timestamp);  // Преобразуем в локальное время
        std::ostringstream oss;
        oss << std::put_time(time_info, "%H:%M:%S") << '.' << unix_timestamp % 1000<<" ";

        return oss.str();
    }

    void loop() {
        while (true) {
            if (need_restart_) {
                restart();
            } else if (!are_authorized_) {
                process_response(client_manager_->receive(10));
            } else {

                std::string line;
                std::getline(std::cin, line);
                std::istringstream ss(line);
                std::string action;

                if (!(ss >> action)) {
                    continue;
                }
                if (action == "q") {
                    return;
                }
                if (action == "u") {
                    std::cout << "Checking for updates..." << std::endl;
                    while (true) {
                        auto response = client_manager_->receive(0);
                        if (response.object) {
                            process_response(std::move(response));
                        } else {
                            break;
                        }
                    }

                } else if (action == "close") {
                    std::cout << "Closing..." << std::endl;
                    send_query(td_api::make_object<td_api::close>(), {});
                } else if (action == "me") {
                    send_query(td_api::make_object<td_api::getMe>(),
                               [this](Object object) { std::cout << to_string(object) << std::endl; });
                } else if (action == "l") {
                    std::cout << "Logging out..." << std::endl;
                    send_query(td_api::make_object<td_api::logOut>(), {});
                } else if (action == "m") {
                    std::int64_t chat_id;
                    ss >> chat_id;
                    ss.get();
                    std::string text;
                    std::getline(ss, text);

                    send_text(glob_output_chat_id, text);
                } else if (action == "c") {
                    std::cout << "Loading chat list..." << std::endl;
                    send_query(td_api::make_object<td_api::getChats>(nullptr, 20), [this](Object object) {
                        if (object->get_id() == td_api::error::ID) {
                            return;
                        }
                        auto chats = td::move_tl_object_as<td_api::chats>(object);
                        for (auto chat_id : chats->chat_ids_) {
                            std::cout << "[chat_id:" << chat_id << "] [title:" << chat_title_[chat_id] << "]" << std::endl;
                        }
                    });
                }

            }
        }
    }

    using Object = td_api::object_ptr<td_api::Object>;
    std::unique_ptr<td::ClientManager> client_manager_;
    std::int32_t client_id_{0};

    td_api::object_ptr<td_api::AuthorizationState> authorization_state_;
    bool are_authorized_{false};
    bool need_restart_{false};
    std::uint64_t current_query_id_{0};
    std::uint64_t authentication_query_id_{0};

    std::map<std::uint64_t, std::function<void(Object)>> handlers_;

    std::map<std::int64_t, td_api::object_ptr<td_api::user>> users_;

    std::map<std::int64_t, std::string> chat_title_;
    BING_API_ClIENT& client;
    std::int64_t glob_api_id ;
    std::string glob_api_hash;
    std::int64_t glob_output_chat_id;


    void restart() {
        client_manager_.reset();

        glob_api_id = client.conf_data["TG_ID"].get<int64_t>();
        glob_output_chat_id = CHATS::OUTPUT_CHAT_ID;
        glob_api_hash = client.conf_data["TG_SECRET"].get<std::string>();
        td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));
        client_manager_ = std::make_unique<td::ClientManager>();
        client_id_ = client_manager_->create_client_id();
        send_query(td_api::make_object<td_api::getOption>("version"), {});


    }

    void send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
        auto query_id = next_query_id();
        if (handler) {
            handlers_.emplace(query_id, std::move(handler));
        }
        client_manager_->send(client_id_, query_id, std::move(f));
    }

    void process_response(td::ClientManager::Response response) {
        if (!response.object) {
            return;
        }
        //std::cout << response.request_id << " " << to_string(response.object) << std::endl;
        if (response.request_id == 0) {
            return process_update(std::move(response.object));
        }
        auto it = handlers_.find(response.request_id);
        if (it != handlers_.end()) {
            it->second(std::move(response.object));
            handlers_.erase(it);
        }
    }

    bool can_convert_to_int64(const std::string& str) {
        try {
            // Проверяем пустую строку
            if (str.empty()) return false;

            // Создаем поток для парсинга строки
            std::istringstream iss(str);
            int64_t result;
            iss >> result;

            // Проверяем наличие ошибок парсинга и отсутствие лишних символов
            return !iss.fail() && iss.eof();
        } catch (...) {
            return false;
        }
    }

    std::string get_user_name(std::int64_t user_id) const {
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return "unknown user";
        }
        return it->second->first_name_ + " " + it->second->last_name_;
    }

    std::string get_chat_title(std::int64_t chat_id) const {
        auto it = chat_title_.find(chat_id);
        if (it == chat_title_.end()) {
            return "unknown chat";
        }
        return it->second;
    }


    void info_check(std::string text){
       std::istringstream text_str(text);
       std::string word;
       int count = 1;
       while(text_str >> word){
            for (char &c : word) {
                c = toupper(c);
            }
            if (count == 1 and word != "INFO"){
                break;
            }

            if (count == 2 and CONSTANTS::ACC_NUMBER == word){
                std::cout << "yes"<<std::endl;
                json information = client.Get_Info_();
                std::ostringstream text_stream;
                text_stream << "$ACC"<<CONSTANTS::ACC_NUMBER<<"\n\n"<< "BingX: "<<information["balance"].get<float>()<<"$"<<"\n\n";
                for (auto it = information["channels"].begin();
                     it != information["channels"].end();
                     ++it)
                {
                    const std::string& channel_name = it.key();        // Доступ к имени канала
                    const auto& channel_info = it.value();

                    text_stream << "| " << channel_name << std::endl;
                    text_stream << "|-- MARGIN: " << channel_info["MARGIN"].get<int64_t>()<<"$"<< std::endl;
                    text_stream << "|-- DOLYA: " << channel_info["DOLYA"].get<int64_t>()<<"%"<< std::endl;
                    text_stream << "|-- LEV: " << channel_info["MAX_LEVERAGE"].get<int64_t>()<<"X"<< std::endl;
                    text_stream << "|-- TP: " << channel_info["TAKE_PROFIT"].get<int64_t>()<<"%"<< std::endl;
                    text_stream << "|-- SL: " << channel_info["STOP_LOSS"].get<int64_t>()<<"%"<< std::endl;
                    text_stream << "|-- TS: " << channel_info["TRAILING_STOP"].get<int64_t>()<<"%"<< std::endl;
                    text_stream << "\n\n";
                }
                text = text_stream.str();
                send_text(CHATS::OUTPUT_CHAT_ID, text);
                break;
            }
            count++;
       }
        return;
    }

    void update_check(std::string text){
        std::istringstream text_str(text);
        std::string word, chat, param;
        int count = 1;
        bool flag = false;
        json data;
        json extra_data;

        while(text_str >> word){
            if (count < 3 || count == 4){
                for (char &c : word) {
                    c = toupper(c);
                }
            }
            if (count == 1 and word != "UPDATE"){
                break;
            }
            if (count == 2 and word != CONSTANTS::ACC_NUMBER){
                break;
            }
            if (count == 3) {
                data = client.conf_data;
                extra_data = data;
                if (data["channels"].contains(word)){
                    chat = word;
                } else{
                    throw std::logic_error("UNKNOWN_CHANNEL_ERROR");
                }
            }
            if (count == 4){
                if (!data["channels"][chat].contains(word)) throw std::logic_error("UNKNOWN_PARAMETR_ERROR");
                else param = word;
            }
            if (count == 5){
                try{
                    int num = std::stoi(word);
                }
                catch(...){
                    throw std::logic_error("IMPOSSIBLE_VALUE_ERROR");
                }
                data["channels"][chat][param] = std::stoi(word);
                try {
                    client.Overwrite_json_file(data);
                    client.conf_data = data;
                }
                catch(...) {
                    client.Overwrite_json_file(extra_data);
                    client.conf_data = extra_data;
                    throw std::logic_error("IMPOSSIBLE_WRITE_ERROR");
                }
            }
            count++;
        }
    }

    void open_positions(std::string text){
        std::istringstream text_str(text);
        std::string word;
        int count = 1;

        while(text_str >> word){
            for (char &c : word) {
                c = toupper(c);
            }
            if (count == 1 and word != "OPEN"){
                break;
            }

            if (count == 2 and word.find("POSITION") != std::string::npos){

                json information = client.Get_Open_Deals();
                std::cout << information.dump(4) << std::endl;
                std::ostringstream text_stream;
                text_stream << "$ACC" << CONSTANTS::ACC_NUMBER << "\n\n";
                if (!(information.contains("data") && information["data"].is_array() && information["data"].empty())) {
                    for (const auto &deal: information["data"]) {
                        std::string symbol = deal["symbol"].get<std::string>();
                        symbol = symbol.substr(0, symbol.find("-USDT"));  // Удаление "-USDT"

                        std::string position_side = deal["positionSide"].get<std::string>();
                        std::string position_id = deal["positionId"].get<std::string>();
                        double margin = std::stod(deal["margin"].get<std::string>());
                        int leverage = deal["leverage"].get<int>();
                        double avg_price = std::stod(deal["avgPrice"].get<std::string>());
                        double unrealized_profit = std::stod(deal["unrealizedProfit"].get<std::string>());
                        double realised_profit = std::stod(deal["realisedProfit"].get<std::string>());
                        double pnl_ratio = std::stod(deal["pnlRatio"].get<std::string>());

                        double total_profit = unrealized_profit + realised_profit;

                        // Форматированный вывод
                        text_stream << "┃" << symbol << " " << position_side << '\n';
                        text_stream << "┣─────────────\n";
                        text_stream << "┣─ ID: " << position_id << '\n';
                        text_stream << "┣─ Margin: " << margin << " USDT\n";
                        text_stream << "┣─ Leverage: x" << leverage << '\n';
                        text_stream << "┣─ avgPrice: " << avg_price << '\n';
                        text_stream << "┣─────────────\n";
                        text_stream << "┣─ Profit: " << total_profit << " USDT\n";
                        text_stream << "┗─ PnL: " << pnl_ratio * 100 << "%\n\n";
                    }
                } else text_stream << "NONE\n";
                text = text_stream.str();
                send_text(CHATS::OUTPUT_CHAT_ID, text);
                break;
            }
            count++;
        }
        return;


    }

    void close_positions_all(std::string text){
        std::istringstream text_str(text);
        std::ostringstream mes;
        std::string word;

        while(text_str >> word){
            for (char &c : word) {
                c = toupper(c);
            }
            if (word == "CLOSE_ALL"){
                json response = client.Close_All_Deals();
                std::ostringstream message;
                if (response["data"].contains("success")) {
                    message << "🟢 Успешно закрытые сделки:\n";
                    for (const auto& deal_id : response["data"]["success"]) {
                        message << "┣─ ID: " << deal_id << "\n";
                    }
                }
                else if (response["data"].contains("failed") && !response["data"]["failed"].is_null()) {
                    message << "❌ Неуспешно закрытые сделки:\n";
                    for (const auto& deal_id : response["data"]["failed"]) {
                        message << "┣─ ID: " << deal_id << "\n";
                    }
                } else {
                    message << "✅ Все сделки успешно закрыты.\n";
                }
//                std::cout << responce.dump(4) << std::endl;
                send_text(CHATS::OUTPUT_CHAT_ID, message.str());

            } else break;

        }
        return;
    }

    void close_position_by_name(std::string text){
        std::istringstream text_str(text);
        std::string word;
        int count = 1;

        while(text_str >> word){
            for (char &c : word) {
                c = toupper(c);
            }
            if (count == 1 and word != "CLOSE"){
                break;
            }

            if (count == 2) {

                json information = client.Close_Deal(word + "-USDT");
                std::ostringstream message;
                if (information["data"].contains("success")) {
                    message << "🟢 Успешно закрытые сделки:\n";
                    for (const auto& deal_id : information["data"]["success"]) {
                        message << "┣─ ID: " << deal_id << "\n";
                    }
                }
                else if (information["data"].contains("failed") && !information["data"]["failed"].is_null()) {
                    message << "❌ Неуспешно закрытые сделки:\n";
                    for (const auto& deal_id : information["data"]["failed"]) {
                        message << "┣─ ID: " << deal_id << "\n";
                    }
                } else {
                    message << "✅ Все сделки успешно закрыты.\n";
                }
                send_text(CHATS::OUTPUT_CHAT_ID, message.str());

            }
            count++;
        }
        return;

    }

    std::string timestampToDate(long long timestamp) {
        time_t time = timestamp / 1000;
        tm *ltm = localtime(&time);

        std::ostringstream dateStream;
        dateStream << std::setw(2) << std::setfill('0') << ltm->tm_mday << "."
                   << std::setw(2) << std::setfill('0') << ltm->tm_mon + 1 << "."
                   << ltm->tm_year + 1900;

        return dateStream.str();
    }

    void get_acc_histrory(std::string text){
        std::istringstream stream(text);
        std::string word;
        int count = 1;
        while (stream >> word && count < 4){
            for (char &c : word) {
                c = toupper(c);
            }
            if (count == 1 && word != "HISTORY"){
                break;
            }
            if (count == 2 && word != CONSTANTS::ACC_NUMBER){
                break;
            }
            if (count == 3){
                int num;
                try{
                    num = std::stoi(word);
                    if (!(( 0 < num) && (num < 8))) num = 3;
                }
                catch(...){
                    throw std::logic_error("IMPOSSIBLE_VALUE_ERROR");
                }
                json response = client.Get_Deals_History(num);
                std::time_t num_days_ago = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count() - (num * 24 * 60 * 60 * 1000);

                std::map<std::string, std::map<std::string, std::pair<double, double>>> report;

                for (const auto &elem : response["data"]) {
                    std::string date = timestampToDate(elem["time"].get<long long>());
                    std::string ticker = elem["symbol"].get<std::string>();
                    double income = std::stod(elem["income"].get<std::string>());
                    std::string incomeType = elem["incomeType"].get<std::string>();


                    if (elem["time"].get<long long>() < num_days_ago) {
                        continue;
                    }

                    // Убираем суффикс "-USDT"
                    if (ticker.size() > 5 && ticker.substr(ticker.size() - 5) == "-USDT") {
                        ticker = ticker.substr(0, ticker.size() - 5);
                    }

                    if (incomeType == "TRADING_FEE" || incomeType == "FUNDING_FEE" || incomeType == "INSURANCE_CLEAR") {
                        report[date][ticker].first += income;
                    } else if (incomeType == "REALIZED_PNL") {
                        report[date][ticker].second += income;
                    }
                }

                // Формирование вывода в ostringstream
                std::ostringstream information;
                double totalSum = 0.0;

                for (const auto &dateEntry : report) {
                    information << "\u2503" << dateEntry.first << "\n"; // \u2503 — символ боковой линии "┃"

                    for (const auto &tickerEntry : dateEntry.second) {
                        const std::string &ticker = tickerEntry.first;
                        double fee = tickerEntry.second.first;
                        double realised = tickerEntry.second.second;
                        double total = fee + realised;

                        information << "\u2503" << ticker << "\n"
                                    << "\u2523\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
                                    << "\u2523\u2500 Fee: " << fee << " USDT\n"
                                    << "\u2523\u2500 Realised: " << realised << " USDT\n"
                                    << "\u2523\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
                                    << "\u2523\u2500 Total: " << total << " USDT\n"
                                    << "\u2523\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
                        totalSum += total;
                    }
                }
                information << "┣─ Total (" << num << " days): " << std::fixed <<  std::setprecision(2) << totalSum << " USDT\n";
                information << "┗───────────────\n";


                send_text(glob_output_chat_id, information.str());
//                std::cout << response.dump(4) << std::endl;
            }
            count++;

        }
    }


    void Rose_post_processing(std::string text, int64_t message_time, int64_t chat_id){
        bool flag = false;
        std::istringstream stream(text);
        std::string first, second,word;
        int count = 0;

        while (stream >> word && count < 2) {
            // Преобразуем слово к верхнему регистру
            for (char &c : word) {
                c = toupper(c);
            }

            if (count == 0) {
                if (word[0] == '#') {
                    first = word.substr(1);
                    count++;
                } else {
                    break;  // Если первое слово не начинается с '#', выходим
                }
            } else if (count == 1) {
                if (word == "BUY" || word == "SHORT") {
                    second = word;  // Сохраняем второе слово
                    flag = true;
                    count++;
                } else {
                    break;  // Если второе слово не BUY или SHORT, выходим
                }
            }
        }
        if(flag){
            json answ;

            if (second == "BUY") {
                std::cout << "Long" << std::endl;
                answ = client.Make_Deal(first+"-USDT",CHATS::TARGET_CHANNELS[std::to_string(chat_id)],"LONG");
                if (answ.contains("data")) {
                    std::string symbol = answ["data"]["order"]["symbol"].get<std::string>();
                    std::cout << answ["time"] << " " <<message_time << std::endl;
                    std::string answ_text = fmt::format("${} LONG\n\n| --Open Price: {}\n| --LV: {}\n| --TP: {}\n| --SL: {}\n| --Fee: {}\n|\n| --Time Received: {}\n| --Time Opened:  {}", symbol.substr(0, symbol.size() - 5),answ["avgPrice"].get<std::string>(), answ["leverage"].get<std::string>(),nlohmann::json::parse(answ["data"]["order"]["takeProfit"].get<std::string>())["price"].get<double>(), nlohmann::json::parse(answ["data"]["order"]["stopLoss"].get<std::string>())["price"].get<double>(),
                            answ["commission"].get<std::string>(),format_time_from_unix(message_time), format_time_from_unix(answ["time"].get<int64_t>()/1000));
//
                    send_text(glob_output_chat_id, answ_text);
                }
                else {send_text(glob_output_chat_id, "Error: " + answ["error"].get<std::string>());};

            } else if (second == "SHORT") {
                std::cout << "short" << std::endl;
                std::cout<< answ["data"]["activationPrice"]<<std::endl;
                answ = client.Make_Deal(first+"-USDT",CHATS::TARGET_CHANNELS[std::to_string(chat_id)],"SHORT");
                if (answ.contains("data")){
                    std::string symbol = answ["data"]["order"]["symbol"].get<std::string>();
                    std::string answ_text = fmt::format("${} SHORT\n\n| --Open Price: {}\n| --LV: {}\n| --TP: {}\n| --SL: {}\n| --Fee: {}\n|\n| --Time Received: {}\n| --Time Opened:  {}", symbol.substr(0, symbol.size() - 5),answ["avgPrice"].get<std::string>(), answ["leverage"].get<std::string>(),nlohmann::json::parse(answ["data"]["order"]["takeProfit"].get<std::string>())["price"].get<double>(), nlohmann::json::parse(answ["data"]["order"]["stopLoss"].get<std::string>())["price"].get<double>(),
                                                        answ["commission"].get<std::string>(),format_time_from_unix(message_time), format_time_from_unix(answ["time"].get<int64_t>()/1000));

                    send_text(glob_output_chat_id, answ_text);
                }
                else {send_text(glob_output_chat_id, "Error: " + answ["error"].get<std::string>());};

            }
            flag = false;
            std::cout<<answ.dump(4)<<std::endl;
        }
        return;
    }

    void Rusik_post_processing(std::string text, int64_t message_time, int64_t chat_id)
    {
        std::istringstream stream(text);
        std::string word, ticker_name, action_name;
        bool flag = false;
        int count = 1;

        while(stream >> word){
            for (char &c : word) {
                c = toupper(c);
            }
            for (char &c : word) {
                if (!std::isalnum(c)) {
                    c = ' ';
                } else {
                    c = std::toupper(c);
                }
            }

            std::istringstream clean_stream(word);
            std::string clean_word;
            while (clean_stream >> clean_word) {
                if (count == 1) {
                    ticker_name = clean_word;
                }

                if (clean_word == "LONG" || clean_word == "SHORT") {
                    action_name = clean_word;
                    flag = true;
                    break;
                }

                count++;
            }
        }
        if(flag){
            json answ;
            std::cout << action_name << std::endl;
            answ = client.Make_Deal(ticker_name+"-USDT",CHATS::TARGET_CHANNELS[std::to_string(chat_id)],action_name);
            if (answ.contains("data")) {
                std::string symbol = answ["data"]["order"]["symbol"].get<std::string>();
                std::cout << answ["time"] << " " <<message_time << std::endl;
                std::string answ_text = fmt::format("${} {}\n\n| --Open Price: {}\n| --LV: {}\n| --TP: {}\n| --SL: {}\n| --Fee: {}\n|\n| --Time Received: {}\n| --Time Opened:  {}", symbol.substr(0, symbol.size() - 5), action_name ,answ["avgPrice"].get<std::string>(), answ["leverage"].get<std::string>(),nlohmann::json::parse(answ["data"]["order"]["takeProfit"].get<std::string>())["price"].get<double>(), nlohmann::json::parse(answ["data"]["order"]["stopLoss"].get<std::string>())["price"].get<double>(),
                                                    answ["commission"].get<std::string>(),format_time_from_unix(message_time), format_time_from_unix(answ["time"].get<int64_t>()/1000));

                send_text(glob_output_chat_id, answ_text);
            }
            else {send_text(glob_output_chat_id, "Error: " + answ["error"].get<std::string>());};
            std::cout<<answ.dump(4)<<std::endl;
        }

        return;
    }

    void Scammer_post_processing(std::string text, int64_t message_time, int64_t chat_id)
    {
        std::istringstream stream(text);
        std::string word, ticker_name, action_name;
        bool flag = false;


        while(stream >> word){
            for (char &c : word) {
                c = toupper(c);
            }

            if (word.size() >= 5 && word.rfind("-USDT", word.size() - 5) != std::string::npos) {
//                std::cout << "Слово заканчивается на -USDT" << std::endl;
                ticker_name = word.substr(0, word.size() - 5);
            }

            if (word == "LONG" || word == "SHORT") {
                action_name = word;
                flag = true;
                break;
            }


        }
        if(flag){
            json answ;
            std::cout << action_name << std::endl;
            answ = client.Make_Deal(ticker_name+"-USDT",CHATS::TARGET_CHANNELS[std::to_string(chat_id)],action_name);
            if (answ.contains("data")) {
                std::string symbol = answ["data"]["order"]["symbol"].get<std::string>();
                std::cout << answ["time"] << " " <<message_time << std::endl;
                std::string answ_text = fmt::format("${} {}\n\n| --Open Price: {}\n| --LV: {}\n| --TP: {}\n| --SL: {}\n| --Fee: {}\n|\n| --Time Received: {}\n| --Time Opened:  {}", symbol.substr(0, symbol.size() - 5), action_name ,answ["avgPrice"].get<std::string>(), answ["leverage"].get<std::string>(),nlohmann::json::parse(answ["data"]["order"]["takeProfit"].get<std::string>())["price"].get<double>(), nlohmann::json::parse(answ["data"]["order"]["stopLoss"].get<std::string>())["price"].get<double>(),
                                                    answ["commission"].get<std::string>(),format_time_from_unix(message_time), format_time_from_unix(answ["time"].get<int64_t>()/1000));

                send_text(glob_output_chat_id, answ_text);
            }
            else {send_text(glob_output_chat_id, "Error: " + answ["error"].get<std::string>());};
            std::cout<<answ.dump(4)<<std::endl;
        }

        return;
    }

    boost::optional<json> text_processing(std::string text, int64_t message_time, int64_t chat_id){
        //Обработка сообщений из target чатов
//        if (CHATS::TARGET_CHANNELS.contains(std::to_string(chat_id))){
            if (chat_id == -1001217702004){ //ROSE
                Rose_post_processing(text, message_time, chat_id);
                std::cout << "Receive message from target chat: [" << text << "]" << std::endl;
                send_text(CHATS::OUTPUT_CHAT_ID, text);
            }
            if (chat_id == -1001288238074){//INVEST_ZONE
                Rusik_post_processing(text, message_time, chat_id);
                std::cout << "Receive message from target chat: [" << text << "]" << std::endl;
//                    send_text(CHATS::OUTPUT_CHAT_ID, text);

            }
//            if (chat_id == -1002098041238){//SCAMMER
//                Scammer_post_processing(text, message_time, chat_id);
//                std::cout << "Receive message from target chat: [" << text << "]" << std::endl;
//                send_text(CHATS::OUTPUT_CHAT_ID, text);
//            }
        //Обработка сообщений из командного чата
        try {
            if (CHATS::COMMAND_CHAT_ID == chat_id) {
                info_check(text);
                update_check(text);
                open_positions(text);
                close_positions_all(text);
                close_position_by_name(text);
                get_acc_histrory(text);
            }
        }
        catch(std::exception &e) {
            json responce;
            responce["error"] = e.what();
            return responce;
        }
        return boost::none;
    }

    void process_update(td_api::object_ptr<td_api::Object> update) {
        td_api::downcast_call(
                *update,
                overloaded(
                        [this](td_api::updateAuthorizationState &update_authorization_state) {
                            authorization_state_ = std::move(update_authorization_state.authorization_state_);
                            on_authorization_state_update();
                        },
                        [this](td_api::updateNewChat &update_new_chat) {
                            // Можно игнорировать новые чаты, если обрабатываем только один
//                            if (update_new_chat.chat_->id_ == glob_target_chat_id) {
                                chat_title_[update_new_chat.chat_->id_] = update_new_chat.chat_->title_;
//                            }
                        },
                        [this](td_api::updateChatTitle &update_chat_title) {
//                            if (update_chat_title.chat_id_ == glob_target_chat_id) {
                                chat_title_[update_chat_title.chat_id_] = update_chat_title.title_;
//                            }
                        },
                        [this](td_api::updateUser &update_user) {
                            auto user_id = update_user.user_->id_;
                            users_[user_id] = std::move(update_user.user_);
                        },
                        [this](td_api::updateNewMessage &update_new_message) {
                            auto chat_id = update_new_message.message_->chat_id_;
//                            if (chat_id == glob_target_chat_id) { // Обрабатываем только сообщения из целевого чата
                                std::string sender_name;
                                td_api::downcast_call(
                                        *update_new_message.message_->sender_id_,
                                        overloaded(
                                                [this, &sender_name](td_api::messageSenderUser &user) {
                                                    sender_name = get_user_name(user.user_id_);
                                                },
                                                [this, &sender_name](td_api::messageSenderChat &chat) {
                                                    sender_name = get_chat_title(chat.chat_id_);
                                                }
                                        )
                                );
                                std::string text;



                                if (update_new_message.message_->content_->get_id() == td::td_api::messageText::ID) {
                                    text = static_cast<td::td_api::messageText &>(*update_new_message.message_->content_).text_->text_;
                                }
                                    // Проверка, является ли сообщение с фото (или другим вложением) с подписью
                                else if (update_new_message.message_->content_->get_id() == td::td_api::messagePhoto::ID) {
                                    const auto &message_photo = static_cast<td::td_api::messagePhoto &>(*update_new_message.message_->content_);
                                    if (message_photo.caption_) {  // Проверяем, есть ли подпись
                                        text = message_photo.caption_->text_;
                                    }
                                }

                                int64_t message_time = update_new_message.message_->date_;
//                                std::cout << "Receive message from target chat: [from:" << sender_name << "] [" << text << "]" << std::endl;
                                //Обработка сообщений
                                boost::optional<json> text_processing_responce = text_processing(text, message_time, chat_id);
                                if (text_processing_responce.has_value()){
                                    send_text(CHATS::OUTPUT_CHAT_ID, (*text_processing_responce)["error"].get<std::string>());
                                }

                        },
                        [](auto &update) {}
                )
        );
    }

    auto create_authentication_query_handler() {
        return [this, id = authentication_query_id_](Object object) {
            if (id == authentication_query_id_) {
                check_authentication_error(std::move(object));
            }
        };
    }

    void on_authorization_state_update() {
        authentication_query_id_++;
        td_api::downcast_call(*authorization_state_,
                              overloaded(
                                      [this](td_api::authorizationStateReady &) {
                                          are_authorized_ = true;
                                          std::cout << "Authorization is completed" << std::endl;
                                      },
                                      [this](td_api::authorizationStateLoggingOut &) {
                                          are_authorized_ = false;
                                          std::cout << "Logging out" << std::endl;
                                      },
                                      [this](td_api::authorizationStateClosing &) { std::cout << "Closing" << std::endl; },
                                      [this](td_api::authorizationStateClosed &) {
                                          are_authorized_ = false;
                                          need_restart_ = true;
                                          std::cout << "Terminated" << std::endl;
                                      },
                                      [this](td_api::authorizationStateWaitPhoneNumber &) {
                                          std::cout << "Enter phone number: " << std::flush;
                                          std::string phone_number;
                                          std::cin >> phone_number;
                                          send_query(
                                                  td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone_number, nullptr),
                                                  create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitEmailAddress &) {
                                          std::cout << "Enter email address: " << std::flush;
                                          std::string email_address;
                                          std::cin >> email_address;
                                          send_query(td_api::make_object<td_api::setAuthenticationEmailAddress>(email_address),
                                                     create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitEmailCode &) {
                                          std::cout << "Enter email authentication code: " << std::flush;
                                          std::string code;
                                          std::cin >> code;
                                          send_query(td_api::make_object<td_api::checkAuthenticationEmailCode>(
                                                             td_api::make_object<td_api::emailAddressAuthenticationCode>(code)),
                                                     create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitCode &) {
                                          std::cout << "Enter authentication code: " << std::flush;
                                          std::string code;
                                          std::cin >> code;
                                          send_query(td_api::make_object<td_api::checkAuthenticationCode>(code),
                                                     create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitRegistration &) {
                                          std::string first_name;
                                          std::string last_name;
                                          std::cout << "Enter your first name: " << std::flush;
                                          std::cin >> first_name;
                                          std::cout << "Enter your last name: " << std::flush;
                                          std::cin >> last_name;
                                          send_query(td_api::make_object<td_api::registerUser>(first_name, last_name, false),
                                                     create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitPassword &) {
                                          std::cout << "Enter authentication password: " << std::flush;
                                          std::string password;
                                          std::getline(std::cin, password);
                                          send_query(td_api::make_object<td_api::checkAuthenticationPassword>(password),
                                                     create_authentication_query_handler());
                                      },
                                      [this](td_api::authorizationStateWaitOtherDeviceConfirmation &state) {
                                          std::cout << "Confirm this login link on another device: " << state.link_ << std::endl;
                                      },
                                      [this](td_api::authorizationStateWaitTdlibParameters &) {
                                          auto request = td_api::make_object<td_api::setTdlibParameters>();
                                          request->database_directory_ = "tdlib";
                                          request->use_message_database_ = true;
                                          request->use_secret_chats_ = true;
                                          request->api_id_ = glob_api_id;
                                          request->api_hash_ = glob_api_hash;
                                          request->system_language_code_ = "ru";
                                          request->device_model_ = "Desktop";
                                          request->application_version_ = "1.0";
                                          send_query(std::move(request), create_authentication_query_handler());
                                      }));
    }

    void check_authentication_error(Object object) {
        if (object->get_id() == td_api::error::ID) {
            auto error = td::move_tl_object_as<td_api::error>(object);
            std::cout << "Error: " << to_string(error) << std::flush;
            on_authorization_state_update();
        }
    }

    std::uint64_t next_query_id() {
        return ++current_query_id_;
    }
};

int main() {


    BING_API_ClIENT client(true);
    Client example(client);


    while (true) {
        if (example.need_restart_) {
            example.restart();
            continue;

        } else if (!example.are_authorized_) {
            example.process_response(example.client_manager_->receive(0));
            continue;
        }

        auto response = example.client_manager_->receive(0);
        if (response.object) {
            example.process_response(std::move(response));
        }

    }


}

    //        std::this_thread::sleep_for(std::chrono::milliseconds (100));
//    example.loop();
//    auto send_message = td_api::make_object<td_api::sendMessage>();
//    send_message->chat_id_ = -1002089771268; // Указание идентификатора чата
//
//    auto message_content = td_api::make_object<td_api::inputMessageText>();
//    message_content->text_ = td_api::make_object<td_api::formattedText>();
//    message_content->text_->text_ = std::move("тест 1");
//
//    send_message->input_message_content_ = std::move(message_content);
//    example.send_query(std::move(send_message), {});

