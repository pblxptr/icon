# iCon
iCon allows to quickly establish communication between clients and endpoints. It uses:
- ZeroMQ (zmq sockets) as a transport layer
- Boost.Asio to provide proactor based messaging
- Protobuf as a serialization mechanism

## Build status (tests are passing but in local env. Github actions will be adjusted)

[![CMake](https://github.com/pblxptr/icon/actions/workflows/cmake.yml/badge.svg?branch=master)](https://github.com/pblxptr/icon/actions/workflows/cmake.yml)
