# ProXC - A CSP-inspired concurrency library for C

This is a library for C which enables Communicating Sequential Processes (CSP) like structuring of your code.
Programming languages such as Occam and XC utilizes and builds around this very paradigm, but sadly is hardware locked.
Many good CSP libraries for general purpose programming languages has been made, including both for Java (JCSP) and C++ (C++CSP2), but not for C.
There does exists plenty of libraries for C which implements many features from CSP, such as coroutines, channels, etc., but not all in a single library.

ProXC aims to do this, and with no macro magic!

## Features

* Lightweight stackful coroutines, called PROC
    * supports arbitrary number of args, acquired through ARGN
* Lightweight runtime environment
* Nestable PROC execution sequence with
    * PAR - parallel execution of given PROC, PAR and SEQ
    * SEQ - sequential execution of given PROC, PAR and SEQ
* Two methods of dispatching execution sequences
    * RUN - fork & join, given a tree of PROC, PAR and SEQ
    * GO - fire & forget, given a tree of PROC, PAR and SEQ
* Any to any, pseudo-type safe, channels
* ALT - wait on multiple guarded commands, which are guarded by a boolean condition
* Guarded commands consist of
    * Skip Guard - always available
    * Time Guard - timeout on a given relative time, with a granularity of microseconds
    * Chan Guard - wait on a channel READ (Note! only channel reads are supported)
* YIELD - give up running time for another PROC, if available
* SLEEP - suspend PROC for a given time, with a granularity of microseconds

## Supports

Currently only supports 32- and 64-bit x86 Linux.

## Compile and Install

Requires CMake 2.8+ for compiling.

Run the following in your terminal. This installs both the static and shared library in /usr/local/lib. ldconfig must be called to register the shared library, as cmake does not do this automatically (for some reason).

    $ git clone https://github.com/edvardsp/libproxc.git
    $ mkdir libproxc/build && cd libproxc/build
    $ cmake ..
    $ make
    $ sudo make install
    $ sudo ldconfig

## Example

```c
#include <stdio.h>
#include <stdlib.h>

#include <proxc.h>

void printer() {
    for (;;) {
        printf("printer: Hello!\n");
        SLEEP(MSEC(500));
    }
}

void fxn() {
    int id = *(int *)ARGN(0);
    int val = *(int *)ARGN(1);
    printf("fxn %d: start %d\n", id, val);
    SLEEP(SEC(3));
    printf("fxn %d: stop %d\n", id, val);
}

void foobar() {
    GO(PROC(printer));

    int ids[] = { 1, 2, 3 };
    int one = 1, two = 2;
    printf("foobar: start\n");
    RUN(PAR(
            PAR(
                SEQ( PROC(fxn, &ids[0], &one), PROC(fxn, &ids[1], &one) ),
                PROC(fxn, &ids[2], &one)
            ),
            SEQ(
                PROC(fxn, &ids[0], &two),
                PAR( PROC(fxn, &ids[1], &two), PROC(fxn, &ids[2], &two) )
            )
        )
    );
    printf("foobar: stop\n");
}

int main() {
    proxc_start(foobar);
    return 0;
}
```
