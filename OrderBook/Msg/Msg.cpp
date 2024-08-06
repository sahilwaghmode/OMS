//
//  Msg.cpp
//  MarketDataSimulator
//
//  Created by Sahil Waghmode on 27/07/24.
//

#include "Msg.hpp"
#include "iostream"

namespace Msg 
{

std::unique_ptr<char> create_txt_msg(const std::string& txt, int& length)
{
    char* msg = new char(sizeof(MsgHeader) + txt.length());
    memset(msg, 0, sizeof(MsgHeader) + txt.length());
    TxtMsg* header = reinterpret_cast<TxtMsg*>(msg);
    header->length = sizeof(MsgHeader) + (int)txt.length();
    length = header->length;
    
    strcpy(reinterpret_cast<char*>(&header->msg_start), txt.c_str());
    std::string_view sv(reinterpret_cast<char*>(&header->msg_start));
    return std::unique_ptr<char>(msg);
}

std::string read_txt_msg(char* msgHeader)
{
    TxtMsg* header = reinterpret_cast<TxtMsg*>(msgHeader);
    std::string_view sv(reinterpret_cast<char*>(&header->msg_start));
    return {reinterpret_cast<char*>(&header->msg_start)};
}

}
