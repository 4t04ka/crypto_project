//
// Created by Александр Бондарь on 29.01.2025.
//

#ifndef EXAMPLE_CONSTANTS_H
#define EXAMPLE_CONSTANTS_H

#include <unordered_set>

namespace CONSTANTS{
    const std::string ACC_NUMBER = "7"; // нужен
    const std::unordered_set<std::string> NOT_TRADING_LIST = {
            "BTC-USDT", "ETH-USDT", "TON-USDT", "SOL-USDT", "TRUMPSOL-USDT", "ETHW-USDT", "XRP-USDT", "LTC-USDT"
    };
}
namespace CHATS{

    const nlohmann::json TARGET_CHANNELS = {{"-1002499486830", "FAKE_ROSA"}, {"-1001288238074", "INVEST_ZONE"}};
//-1002098041238 - scammer
    // test_sanii -1002499486830
    // {"-1001217702004", "FAKE_ROSA"}
//{{"-1002233859472", "Rose"}, {"-1001288238074", "INVEST_ZONE"}, {"-1002098041238", "SCAMMER"}};
//Легче проверять

    const std::int64_t OUTPUT_CHAT_ID = -1002089771268;//Scams
    const std::int32_t OUTPUT_TOPIC_ID = 50083;

    const std::int64_t COMMAND_CHAT_ID = -1002275205457;//DDos -1002275205457
}
#endif //EXAMPLE_CONSTANTS_H
