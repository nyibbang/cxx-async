cxx-async
=========

This project is an experiment for writing C++ asynchronous programming.

The goal of the implementation is to implement a "computer" service across local (aka Unix/POSIX)
sockets. It must start a server that will wait for connections. Once a client connects to the
server, it starts sending commands and waits for a response. The server must create a task for
each client that will interpret the commands and send the result back.

In the implementations, multiple clients are created and are executed in parallel, and they send
their commands at different intervals.

The implementation must be mono-threaded.

- [Command types and functions](src/computer.hpp)
- [Boost.Asio implementation](src/main_asio.cpp)
- [Boost.Cobalt implementation](src/main_cobalt.cpp)
