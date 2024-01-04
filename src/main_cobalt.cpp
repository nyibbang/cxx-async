#include <iostream>
#include <chrono>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <boost/outcome.hpp>
#include "computer.hpp"

namespace cobalt = boost::cobalt;
namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
using namespace std::chrono_literals;

using outcome::result;
using boost::asio::buffer;
using boost::asio::dynamic_buffer;
namespace this_coro = cobalt::this_coro;
using Local = boost::asio::local::stream_protocol;
using Acceptor = cobalt::use_op_t::as_default_on_t<Local::acceptor>;
using Socket = cobalt::use_op_t::as_default_on_t<Local::socket>;
using Timer = cobalt::use_op_t::as_default_on_t<boost::asio::steady_timer>;
using Endpoint = Local::endpoint;

/// ==============================================================================================
/// Utility functions to send/receive messages separated by newlines.
/// ==============================================================================================
template<typename T, typename S>
cobalt::promise<void> send(S& socket, const T& value) {
    auto message = to_string(value);
    co_await socket.async_send(buffer(message + "\n"));
}

template<typename T, typename Socket>
cobalt::generator<T> receive(Socket& socket) {
    std::string buffer;
    while (true) {
        auto size =
            co_await async_read_until(
                socket,
                dynamic_buffer(buffer),
                "\n"
            );
        auto message = buffer.substr(0, size - 1); // -1 to trim '\n'
        buffer.erase(0, size);
        co_yield T::from_string(message).value();
    }
    // unreachable
    assert(false);
}

/// ==============================================================================================
/// Client
/// ==============================================================================================
cobalt::promise<void>
    client(
        unsigned int id,
        Endpoint ep,
        std::chrono::milliseconds delay,
        std::vector<Command> commands
    )
{
    auto executor = co_await this_coro::executor;
    Socket socket(executor);
    co_await socket.async_connect(ep);

    Timer timer({executor}, delay);
    for (auto& command: commands)
    {
        co_await timer.async_wait();
        timer.expires_at(timer.expiry() + delay);
        co_await send(socket, command);
        std::cout << "client " << id << ": sent command " << command << std::endl;
    }
    co_await send<Command>(socket, Compute{});

    auto answer = co_await receive<Answer>(socket);
    std::cout << "client " << id << ": got answer " << answer << std::endl;
}

/// ==============================================================================================
/// Service
/// ==============================================================================================
cobalt::promise<void> compute(Socket socket) {
    int value = 0;
    auto commands = receive<Command>(socket);
    while (commands) {
        if (auto result = (co_await commands).run(value)) {
            auto value = *result;
            co_await send(socket, Answer{ value });
            co_return;
        }
    }
}

/// ==============================================================================================
/// Server
/// ==============================================================================================
cobalt::generator<Socket> accept_sockets(Endpoint ep) {
    auto executor = co_await this_coro::executor;
    Acceptor acceptor(executor, ep);
    while (true) {
        co_yield co_await acceptor.async_accept();
    }
    co_return Socket(executor);
}

cobalt::promise<void> run_server(cobalt::wait_group& workers, Endpoint ep) {
    auto sockets = accept_sockets(ep);
    while (sockets) {
        workers.push_back(compute(co_await sockets));
    }
}

cobalt::promise<void> server(Endpoint ep) {
    co_await cobalt::with(
        cobalt::wait_group(),
        std::bind(&run_server, std::placeholders::_1, ep)
    );
}

/// ==============================================================================================
/// Main
/// ==============================================================================================
cobalt::main co_main(int argc, char** argv) {
    auto endpoint = "cxx-async";
    ::unlink(endpoint);
    try {
        auto clients = [endpoint]() -> cobalt::promise<void> {
            co_await cobalt::join(
                client(1, endpoint, 3s,    {Init{12}, Add{-2}, Mul{8}, Div{10}}), // 8
                client(2, endpoint, 500ms, {Init{-5}, Add{3}, Mul{7}, Add{-1}, Div{5}}), // -3
                client(3, endpoint, 2s,    {Init{0}, Add{2}, Mul{-4}, Add{9}})
            );
        };
        co_await cobalt::race(
            server(endpoint),
            clients()
        );
        co_return 0;
    } catch (std::exception& e) {
        std::cerr << "fatal exception: " << e.what() << "\n";
        co_return -1;
    }
}
