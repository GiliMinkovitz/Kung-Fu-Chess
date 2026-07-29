#include "server/game/loopback_game_host.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

namespace kfc {

namespace {

std::string build_create_command(const GameCreationRequest& request) {
    std::ostringstream command;
    command << "CREATE " << request.white_user_id << ' ' << request.black_user_id;
    if (request.db_game_id.has_value()) {
        command << ' ' << *request.db_game_id;
    }
    command << '\n';
    return command.str();
}

GameCreationResponse parse_create_response(const std::string& line, const std::string& game_server_id,
                                           const std::string& game_endpoint) {
    std::istringstream input(line);
    std::string status;
    input >> status;
    if (status != "OK") {
        std::string reason;
        std::getline(input, reason);
        if (!reason.empty() && reason.front() == ' ') {
            reason.erase(reason.begin());
        }
        throw std::runtime_error(reason.empty() ? "game creation failed" : reason);
    }

    RoomId room_id = 0;
    input >> room_id;
    if (room_id == 0) {
        throw std::runtime_error("invalid game creation response");
    }

    return GameCreationResponse{room_id, game_server_id, game_endpoint};
}

}  // namespace

LoopbackGameHost::LoopbackGameHost(std::string bind_address, const unsigned short internal_port,
                                   std::string game_server_id, std::string game_endpoint)
    : bind_address_(std::move(bind_address)),
      internal_port_(internal_port),
      game_server_id_(std::move(game_server_id)),
      game_endpoint_(std::move(game_endpoint)) {}

GameCreationResponse LoopbackGameHost::create_room(const GameCreationRequest& request) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    const auto endpoints =
        resolver.resolve(bind_address_, std::to_string(internal_port_));
    boost::asio::connect(socket, endpoints);

    const std::string command = build_create_command(request);
    boost::asio::write(socket, boost::asio::buffer(command));

    boost::asio::streambuf response_buffer;
    boost::asio::read_until(socket, response_buffer, '\n');
    std::istream response_stream(&response_buffer);
    std::string response_line;
    std::getline(response_stream, response_line);
    if (!response_line.empty() && response_line.back() == '\r') {
        response_line.pop_back();
    }

    return parse_create_response(response_line, game_server_id_, game_endpoint_);
}

}  // namespace kfc
