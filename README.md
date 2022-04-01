# Introduction
## Test Task
>FEED DELAYER\
\
Using our websockets API - available at https://api.deriv.com, build a small application which calls https://api.deriv.com/api-explorer/#ticks to obtain a real-time tick stream of a given instrument, and then delays that tick stream by 1 minute.\
\
Consider design choices that might help with efficient behaviour.

## Solution

The solution is based on the Boost async websocket.

Every stream message is delayed by a given time and the value of the `"epoch"` field in the message is changed accordingly.

* Stream output to STDOUT.
* Service messages output to STDERR.

# Requirements
* C++ compiler (defaults to gcc)
* [CMake](https://cmake.org/)
* [OpenSSL](https://www.openssl.org/)
```
$ sudo apt-get install libssl-dev
```
* [Boost](http://www.boost.org/)
```
$ sudo apt-get install libboost-all-dev
```

# Usage
```
Usage: ticks_app <delay_in_seconds> <symbol_name>
Example:
    ticks_app 60 R_100
```
* Stream output to STDOUT.
* Service messages output to STDERR.

To disable service messages:
```
$ ticks_<delay_in_seconds> <symbol_name> 2>/dev/null
```
