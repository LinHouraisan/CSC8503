// modify from asio-1.30.2\src\examples\cpp11\chat
// BELOW IS THE ORIGINAL COPYRIGHT INFOMATION
// ///////////////////////////////////////////////
//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>

constexpr unsigned short PORT = 12345;
constexpr int PLAYER_LIMIT = 2;

class message {
public:
    static constexpr std::size_t header_length = 4;
    static constexpr std::size_t max_body_length = 512;

    message() : data_(header_length, '\0'), body_length_(0) {}

    message(const std::string &s) : message() {
        body_length_ = s.size();
        encode_header();
        data_ += s;
    }

    const std::string &data() const {
        return data_;
    }

    char *data() {
        return data_.data();
    }

    std::size_t length() const {
        return data_.length();
    }

    const char *body() const {
        return data_.data() + header_length;
    }

    char *body() {
        return data_.data() + header_length;
    }

    std::size_t body_length() const {
        return body_length_;
    }

    bool decode_header() {
        body_length_ = *reinterpret_cast<uint32_t *>(data_.data());
        data_.resize(header_length + body_length_);
        return true;
    }

    void encode_header() {
        memcpy(data_.data(), reinterpret_cast<void *>(&body_length_), header_length);
    }

private:
    std::string data_;
    uint32_t body_length_;
};

enum class CID {
    LOGIN,
    LOGIN_RESULT,
    START_GAME,
    UPDATE_POS,
};

struct LoginMessage {
    CID cid = CID::LOGIN;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginMessage, cid);

struct LoginResultMessage {
    CID cid = CID::LOGIN_RESULT;
    int playerIndex;
    bool success;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginResultMessage, cid, playerIndex, success);

struct StartaGameMessage {
    CID cid = CID::START_GAME;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StartaGameMessage, cid);

struct UpdatePosMessage {
    CID cid = CID::UPDATE_POS;
    int playerIndex;
    float pos[3];
    float quat[4];
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UpdatePosMessage, cid, playerIndex, pos, quat);