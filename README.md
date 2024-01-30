cxx-async
=========

This project is an experiment of writing C++ asynchronous code.

The goal of the implementations is to expose then use a "computer" service across local (Unix)
sockets. Each must start a server that will wait for connections. Once a client connects, it
starts sending commands and awaits for a response. The server must create a task for
each client that will interpret the commands and send the result back.

In the implementations, multiple clients are created and are executed in parallel, and they send
their commands at different intervals.

The implementations are all mono-threaded.

- [Command types and functions](src/computer.hpp)
- [Boost.Asio implementation](src/main_asio.cpp)
- [Boost.Cobalt implementation](src/main_cobalt.cpp)
