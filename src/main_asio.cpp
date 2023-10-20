#include <boost/asio/buffer.hpp>
#include <deque>
#include <iostream>
#include <chrono>
#include <system_error>
#include <boost/predef.h>
#if defined(BOOST_COMP_CLANG)
#  include <experimental/coroutine>
#else
#  include <coroutine>
#endif
#include <sstream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/as_single.hpp>
#include <boost/system.hpp>
#include <boost/outcome.hpp>
#include "computer.hpp"

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::deferred;
using boost::asio::use_awaitable;
using boost::asio::as_tuple;
using boost::asio::io_context;
using boost::asio::signal_set;
using boost::asio::buffer;
using boost::asio::dynamic_buffer;
using boost::asio::async_read_until;
using boost::asio::steady_timer;
namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
using outcome::result;
namespace this_coro = boost::asio::this_coro;
using namespace std::chrono_literals;
using namespace boost::asio::experimental::awaitable_operators;

using DefaultToken = boost::asio::use_awaitable_t<>;
using Local = boost::asio::local::stream_protocol;
using Acceptor = DefaultToken::as_default_on_t<Local::acceptor>;
using Socket = DefaultToken::as_default_on_t<Local::socket>;
using Timer = DefaultToken::as_default_on_t<steady_timer>;
using Endpoint = Local::endpoint;

/// ==============================================================================================
/// Utility functions to send/receive messages separated by newlines.
/// ==============================================================================================
template<typename T, typename S>
awaitable<void> send(S& socket, T value)
{
    auto message = to_string(value);
    co_await socket.async_send(buffer(message + "\n"));
}

template<typename Socket>
struct Receive {
    Socket& socket;
    std::string buffer;

    template<typename T>
    awaitable<result<T>> next()
    {
        auto size =
            co_await async_read_until(
                socket,
                dynamic_buffer(buffer, 256),
                "\n"
            );
        auto message = buffer.substr(0, size - 1); // -1 to trim '\n'
        buffer.erase(0, size);
        co_return T::from_string(message);
    }
};

template<typename Socket>
Receive<Socket> receive(Socket& socket) {
    return Receive<Socket>{ socket };
}

/// ==============================================================================================
/// Client
/// ==============================================================================================
awaitable<result<void>>
    client(
        io_context& ctx,
        unsigned int id,
        Endpoint ep,
        std::chrono::milliseconds delay,
        std::vector<Command> commands
    )
{
    Socket socket(ctx);
    co_await socket.async_connect(ep);

    Timer timer(ctx, delay);
    for (auto& command: commands)
    {
        co_await timer.async_wait();
        timer.expires_at(timer.expiry() + delay);
        co_await send(socket, command);
        std::cout << "client " << id << ": sent command " << command << std::endl;
    }
    co_await send<Command>(socket, Compute{});

    BOOST_OUTCOME_CO_TRY(auto value, co_await receive(socket).next<Answer>());
    std::cout << "client " << id << ": got answer " << value << std::endl;

    co_return outcome::success();
}


/// ==============================================================================================
/// Service
/// ==============================================================================================
awaitable<result<int>> compute(Socket socket)
{
    int value = 0;
    auto recv = receive(socket);
    while (true) {
        BOOST_OUTCOME_CO_TRY(auto command, co_await recv.next<Command>());
        if (auto result = command.run(value)) {
            auto value = *result;
            co_await send(socket, Answer{ value });
            co_return value;
        }
    }
}


/// ==============================================================================================
/// Server
/// ==============================================================================================
awaitable<void> server(Endpoint ep) {
    auto executor = co_await this_coro::executor;
    Acceptor acceptor(executor, ep);

    while (true) {
        auto socket = co_await acceptor.async_accept();
        co_spawn(
            executor,
            [](auto socket) -> awaitable<void> {
                auto result = co_await compute(std::move(socket));
                result.value();
            }(std::move(socket)),
            detached
        );
    }
}

/// ==============================================================================================
/// Main
/// ==============================================================================================
awaitable<void> run(io_context& ctx, Endpoint ep)
{
    auto clients =
           client(ctx, 1, ep, 3s,    {Init{12}, Add{-2}, Mul{8}, Div{10}}) // 8
        && client(ctx, 2, ep, 500ms, {Init{-5}, Add{3}, Mul{7}, Add{-1}, Div{5}}) // -3
        && client(ctx, 3, ep, 2s,    {Init{0}, Add{2}, Mul{-4}, Add{9}});
    co_await (server(ep) || std::move(clients));
}

int main(int argc, char** argv)
{
    try
    {
        auto endpoint = "cxx-async";
        ::unlink(endpoint);

        io_context io_ctx;
        signal_set signals(io_ctx, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_ctx.stop(); });

        co_spawn(
            io_ctx,
            [&io_ctx, endpoint]() -> awaitable<void> {
                // Once the main task is finish, stop the IO loop.
                co_await run(io_ctx, endpoint);
                io_ctx.stop();
            },
            detached
        );
        io_ctx.run();
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << "fatal exception: " << e.what() << "\n";
        return -1;
    }
}
