//
//  Msg.hpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 27/07/24.
//

#ifndef Msg_hpp
#define Msg_hpp

#include <string>
#include <iostream>

namespace Msg
{

struct MsgHeader
{
    double length = 0;
};

struct TxtMsg : MsgHeader
{
    double msg_start;
};

enum class MsgType : uint32_t
{
    NEW_ORDER = 1,
    CXL_ORDER,
    DELETE_ORDER,
    EXEC_VISIBLE,
    EXEC_HIDDEN,
    QUIT
};

struct OrderInfo : MsgHeader
{
    double time_stamp;
    double price;
    MsgType msg_type;
    int a;
    double quantity;
    double side;
};

std::unique_ptr<char> create_txt_msg(const std::string& txt, int& length);

}

#endif /* Msg_hpp */
