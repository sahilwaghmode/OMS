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
    char* msg = new char(sizeof(MsgHeader) + txt.length()+10);
    MsgHeader* header = reinterpret_cast<MsgHeader*>(msg);
    header->length = sizeof(MsgHeader) + (int)txt.length();
    length = header->length;
    
    strcpy(&header->msg, txt.c_str());
    std::string_view sv(&header->msg);
    return std::unique_ptr<char>(msg);
}

std::string read_txt_msg(char* msgHeader)
{
    MsgHeader* header = reinterpret_cast<MsgHeader*>(msgHeader);
    std::string_view sv(&header->msg);
    return {header->msg};
}

}
