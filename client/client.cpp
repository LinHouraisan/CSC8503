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

#include "client.h"

using asio::ip::tcp;

client::client(const std::string &host, unsigned short port) : status(Status::LOGGING_IN), socket_(io_context_) {

    tcp::resolver resolver(io_context_);
    endpoints = resolver.resolve(host, std::to_string(port));
    for (auto &ret : endpoints) {
        std::cout << "connect to " << ret.endpoint() << std::endl;
    }
    do_connect();

    t = std::thread([this]() {
        io_context_.run();
    });
}

client::~client() {
    close();
}

client::Status client::currentStatus() const {
    return status;
}

std::string client::currentStatusString() const {
    switch (status) {
    case Status::LOGGING_IN:
        return "loging in...";
    case Status::WAIT_OTHERS:
        return "your index is: " + std::to_string(playerIndex) + ", wait others...";
    case Status::PLAYING:
        return "playing";
    }
    return "unknown";
}

void client::write(const nlohmann::json &j) {
    asio::post(io_context_, [this, j]() {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(j);
        if (!write_in_progress) {
            do_write();
        }
    });
}

std::vector<nlohmann::json> client::retrieve() {
    std::vector<nlohmann::json> ret;
    {
        std::unique_lock<std::mutex> ul(read_msgs_lock_);
        ret.swap(read_msgs_);
        debug_msgs_.clear();
    }
    return ret;
}

void client::close() {
    asio::post(io_context_, [this]() {
        socket_.close();
    });
    t.join();
}

void client::do_connect() {
    asio::async_connect(socket_, endpoints, [this](std::error_code ec, tcp::endpoint) {
        if (ec) {
            std::cout << "connect error: " << ec.message() << std::endl;
            do_connect();
            return;
        }
        std::cout << "connected. login..." << std::endl;

        LoginMessage login;
        nlohmann::json j;
        nlohmann::to_json(j, login);

        write(j);

        do_read_header();
    });
}

void client::do_read_header() {
    asio::async_read(socket_, asio::buffer(read_msg_.data(), message::header_length),
                     [this](std::error_code ec, std::size_t /*length*/) {
                         if (ec) {
                             onError();
                         }

                         read_msg_.decode_header();
                         do_read_body();
                     });
}

void client::do_read_body() {
    asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()),
                     [this](std::error_code ec, std::size_t /*length*/) {
                         if (ec) {
                             onError();
                             return;
                         }
                         // std::cout << "recv: " << read_msg_.body() << std::endl;

                         nlohmann::json j = nlohmann::json::parse(read_msg_.body());
                         CID cid = j["cid"];
                         switch (status) {
                         case Status::LOGGING_IN: {
                             switch (cid) {
                             case CID::LOGIN_RESULT: {
                                 LoginResultMessage loginResult;
                                 nlohmann::from_json(j, loginResult);
                                 if (!loginResult.success) {
                                     throw std::runtime_error("login fail");
                                 }
                                 std::cout << "login success. playerIndex = " << loginResult.playerIndex << std::endl;
                                 playerIndex = loginResult.playerIndex;
                                 status = Status::WAIT_OTHERS;
                                 break;
                             }
                             default:
                                 assert(0);
                             }
                             break;
                         }
                         case Status::WAIT_OTHERS: {
                             if (cid != CID::START_GAME) {
                                 throw std::runtime_error("error cid");
                             }
                             status = Status::PLAYING;
                             break;
                         }
                         case Status::PLAYING: {
                             std::unique_lock<std::mutex> ul(read_msgs_lock_);
                             read_msgs_.push_back(j);
                             debug_msgs_.push_back(read_msg_.body());
                             break;
                         }
                         default: {
                             assert(0);
                         }
                         }

                         do_read_header();
                     });
}

void client::do_write() {
    message msg(nlohmann::to_string(write_msgs_.front()));

    asio::async_write(socket_, asio::buffer(msg.data(), msg.length()),
                      [this](std::error_code ec, std::size_t /*length*/) {
                          if (ec) {
                              onError();
                              return;
                          }

                          write_msgs_.pop_front();
                          if (!write_msgs_.empty()) {
                              do_write();
                          }
                      });
}

void client::onError() {
    std::cerr << "error. close connection." << std::endl;
    socket_.close();
    throw std::runtime_error("net error");
}
