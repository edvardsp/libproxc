# Introduction

`libproxc` (pronounced '*lib-prok-see*') is a concurrency library for modern C and C++, based on abstractions from Communicating Sequential Processes (CSP).

The C (ProXC) and C++ (ProXC++) libraries both provides the same core of abstractions; lightweight threads called processes (not to be confused with OS processes), which exclusively communicate via message-passing. Message passing is realized through channels, and supports guarded, simultaneous channel operations.

Both ProXC and ProXC++ projects are developed separately, which can respectively be located in the folders `proxc/` and `proxc++/`.

# Motivation

The development of `libproxc` is motivated by providing a concurrency framework for C and C++ programs which expresses correct and expressive abstractions for concurrent systems. The C and C++ languages are inherently not concurrent; C++11 addressed this by introducing some concurrency primitives to the standard library, e.g. `std::thread` and `std::future`. Correct and expressive concurrency abstractions are not a part of the standard languages, and `libproxc` aims to fill this void.

Note that `libproxc` is merely a proof-of-concept. Multiple libraries for C and C++ which provide CSP-based concurrency abstractions. However, none offer a complete feature set which provide the correct and necessary abstractions. These abstractions are necessary for creating CSP-based concurrency models in C and C++. `libproxc` is to show such libraries are possible (and has the added effect of being a fun project to work on).

# Development

ProXC and ProXC++ was developed as a part of my project and master thesis at NTNU, respectively. Both theses are available in the `theses/` folder.

