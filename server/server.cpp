// modify from asio-1.30.2\src\examples\cpp11\chat
// BELOW IS THE ORIGINAL COPYRIGHT INFOMATION
// ///////////////////////////////////////////////
//
//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include "protocol.h"

using asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant {
public:
    chat_participant(tcp::socket socket) : socket_(std::move(socket)) {}

    virtual ~chat_participant() {}
    virtual void deliver(const message &msg) = 0;

    tcp::socket socket_;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room {
public:
    bool joined(chat_participant_ptr participant) {
        return participants_.contains(participant);
    }

    void join(chat_participant_ptr participant) {
        participants_.insert(participant);
    }

    void leave(chat_participant_ptr participant) {
        participants_.erase(participant);
    }

    void deliver(chat_participant_ptr src, const message &msg) {
        for (auto participant : participants_) {
            if (participant == src) {
                continue;
            }
            participant->deliver(msg);
        }
    }

    int nums() const {
        return participants_.size();
    }

private:
    std::set<chat_participant_ptr> participants_;
    int nextPlayerIndex = 0;
};

//----------------------------------------------------------------------

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {
public:
    enum class Status {
        UNLOGGED,
        WAIT_OTHERS,
        PLAYING,
    };

    chat_session(tcp::socket socket, chat_room &room)
        : chat_participant(std::move(socket)), status(Status::UNLOGGED), room_(room) {}

    void start() {
        do_read_header();
    }

    void deliver(const message &msg) {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    }

private:
    void onError() {
        std::cout << socket_.remote_endpoint() << " fail. remove it." << std::endl;
        if (room_.joined(shared_from_this())) {
            room_.leave(shared_from_this());
        }
        std::cout << "current users : " << room_.nums() << std::endl;
    }

    void do_read_header() {
        auto self(shared_from_this());
        asio::async_read(socket_, asio::buffer(read_msg_.data(), message::header_length),
                         [this, self](std::error_code ec, std::size_t /*length*/) {
                             if (ec) {
                                 onError();
                                 return;
                             }
                             read_msg_.decode_header();
                             do_read_body();
                         });
    }

    void do_read_body() {
        auto self(shared_from_this());
        asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()),
                         [this, self](std::error_code ec, std::size_t /*length*/) {
                             if (ec) {
                                 onError();
                                 return;
                             }
                             std::cout << "from " << socket_.remote_endpoint() << ": " << read_msg_.body() << std::endl;

                             auto json = nlohmann::json::parse(read_msg_.body());
                             CID cid = json["cid"];

                             switch (status) {
                             case Status::UNLOGGED: {
                                 if (cid != CID::LOGIN) {
                                     std::cerr << "unknown message: " << read_msg_.body() << std::endl;
                                     onError();
                                     return;
                                 }

                                 bool couldJoin = room_.nums() + 1 <= PLAYER_LIMIT;
                                 bool shouldStartGame = room_.nums() + 1 == PLAYER_LIMIT;

                                 int nums = room_.nums();
                                 if (couldJoin) {
                                     room_.join(shared_from_this());
                                 }

                                 {
                                     LoginResultMessage reply;
                                     reply.playerIndex = couldJoin ? nums : -1;
                                     reply.success = couldJoin;
                                     nlohmann::json j;
                                     nlohmann::to_json(j, reply);
                                     std::string replyStr = nlohmann::to_string(j);
                                     std::cout << "to " << socket_.remote_endpoint() << ": " << replyStr << std::endl;

                                     message msg(replyStr);

                                     deliver(msg);
                                 }

                                 if (!couldJoin) {
                                     std::cout << "kick out " << socket_.remote_endpoint() << std::endl;
                                     onError();
                                     return;
                                 }

                                 std::cout << "current users : " << room_.nums() << std::endl;
                                 if (!shouldStartGame) {
                                     status = Status::WAIT_OTHERS;
                                     break;
                                 }

                                 StartaGameMessage startGameMessage;
                                 nlohmann::json j;
                                 nlohmann::to_json(j, startGameMessage);
                                 std::string replyStr = nlohmann::to_string(j);

                                 message msg(replyStr);

                                 // TODO: convert all's status

                                 room_.deliver(nullptr, msg);
                                 status = Status::PLAYING;

                                 break;
                             }
                             case Status::WAIT_OTHERS:
                                 // the first one stay at this status
                                 // onError();
                                 // return;
                                 room_.deliver(self, read_msg_);
                                 break;
                             case Status::PLAYING: {
                                 room_.deliver(self, read_msg_);
                                 break;
                             }
                             }

                             do_read_header();
                         });
    }

    void do_write() {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                          [this, self](std::error_code ec, std::size_t /*length*/) {
                              if (ec) {
                                  onError();
                              }
                              std::cout << "to " << socket_.remote_endpoint() << ": " << write_msgs_.front().body()
                                        << std::endl;

                              write_msgs_.pop_front();
                              if (!write_msgs_.empty()) {
                                  do_write();
                              }
                          });
    }

    Status status;
    chat_room &room_;
    message read_msg_;
    chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class Server {
public:
    Server(asio::io_context &io_context, unsigned short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        std::cout << acceptor_.local_endpoint() << " start listenning..." << std::endl;
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (ec) {
                throw std::runtime_error("failed to accept");
            }

            std::cout << socket.remote_endpoint() << " connected." << std::endl;

            std::make_shared<chat_session>(std::move(socket), room_)->start();

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {

        asio::io_context io_context;

        Server servers(io_context, PORT);

        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
