// modify from asio-1.30.2\src\examples\cpp11\chat
// BELOW IS THE ORIGINAL COPYRIGHT INFOMATION
// ///////////////////////////////////////////////
//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <asio.hpp>
#include "protocol.h"

typedef std::deque<message> chat_message_queue;

class client {
public:
    enum class Status {
        LOGGING_IN,
        WAIT_OTHERS,
        PLAYING,
    };

    client(const std::string &host, unsigned short port);

    ~client();

    Status currentStatus() const;

    std::string currentStatusString() const;

    int getPlayerIndex() const {
        return playerIndex;
    }

    void write(const nlohmann::json &j);

    std::vector<nlohmann::json> retrieve();

    void close();

private:
    void do_connect();

    void do_read_header();

    void do_read_body();

    void do_write();

    void onError();

private:
    Status status;
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver::results_type endpoints;
    message read_msg_;
    std::mutex read_msgs_lock_;
    std::vector<nlohmann::json> read_msgs_;
    std::vector<std::string> debug_msgs_;
    std::deque<nlohmann::json> write_msgs_;
    std::thread t;

    int playerIndex = -1;
};
