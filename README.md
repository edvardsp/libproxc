# ProXC - A CSP-inspired concurrency library for C

This is a library for C which enables Communicating Sequential Processes (CSP) like structuring of your code.
Programming languages such as Occam and XC utilizes and builds around this very paradigm, but sadly is hardware locked.
Many good CSP libraries for general purpose programming languages has been made, including both for Java (JCSP) and C++ (C++CSP2), but not for C.
There does exists plenty of libraries for C which implements many features from CSP, such as coroutines, channels, etc., but not all in a single library.

ProXC aims to do this, and with no macro magic!

## Supported Platforms

Only requirement is support for C++14 compiler, as well as `boost.context` version 1.61 or higher installed.

Only tested on 64-bit Linux.

## Prerequisites

You will need

* C++14 supported compiler
* Meson - build system front-end
* ninja - build system back-end

### Meson

Can be installed via pip for python3.

    $ pip3 install meson

### Ninja

Can be installed through any package manager, e.g. Ubuntu,

    $ sudo apt-get install ninja-build

## Compiling and Installing

First, clone the repository and create a build directory.

    $ git clone https://github.com/edvardsp/libproxc.git
    $ mkdir libproxc/build && cd libproxc/build

Next, configure the project. The compiler can be set by setting the `CXX` variable.

    $ CXX=*compiler-name* meson ..

Project options can be viewed by typing `mesonconf`. A standard release configuration can be done by setting

    $ mesonconf -Dbuildtype=release -Db_ndebug=true

If your linker supports Link Time Optimizations (lto), then the configuration `-Db_lto=true` is recommended, as ProXC is a template heavy libray.

Lastly, build and install.

    $ ninja
    $ sudo ninja install
    $ sudo ldconfig

To check that the compilation was succesfull, run `ninja test`. This should run all unit tests, which should all complete with success.

