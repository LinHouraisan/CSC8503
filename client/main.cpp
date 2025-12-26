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

int main(int argc, char *argv[]) {
    try {

        client cli("localhost", PORT);

        char line[message::max_body_length + 1];
        while (std::cin.getline(line, message::max_body_length + 1)) {
            message msg(line);
            cli.write(msg);
        }

        cli.close();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
