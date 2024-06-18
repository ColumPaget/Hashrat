[![Build Status](https://travis-ci.com/ColumPaget/libUseful.svg?branch=master)](https://travis-ci.com/ColumPaget/libUseful)

libUseful - a 'C' library of useful functions. 
-----------------------------------------------

libUseful provides a range of functions that simplify common programming tasks in 'C', particularly networking and communications. It hides the complexities of sockets, openssl, zlib, pseudoterminals, http, etc and provides commonly needed functionality like resizeable strings, linked lists and maps. 

libUseful has been compiled and used on various linux and freebsd systems.

Install
-------

should be as simple as:

./configure --prefix=/usr/local --enable-ssl --enable-ip6 
make
make install

.h files will be copied to <prefix>/include/libUseful-4

On linux a few linux-specific functions can be activated via configure:

--enable-sendfile   enable kernel-level 'fastcopy' within the STREAMSendFile function

On intel systems some MMX/SSE/SSE2 features can be turned on by

--enable-simd=<level>    where level is one of 'mmx', 'sse' or 'sse2'

the major use of --enable-simd is SSE2 cache hinting in the list/map system, which can prevent large lists from poisoning the cache and so improve program performance.

Compiler Flags
--------------

If libUseful is compiled with openssl support then you need to pass the following library flags to gcc when you compile your programs

gcc -o myProg myProg.c -lUseful -lssl -lcrypto -lz

If openssl is not used (libUseful compiled with --disable-ssl), but zlib is used then the flags are

gcc -o myProg myProg.c -lUseful -lssl -lcrypto -lz

If neither zlib or openssl functionality is used in libUseful, then the flags are just

gcc -o myProg myProg.c -lUseful 


IPv6 support
------------

Please note that, due to the author's lack of access to IPv6 networks the IPv6 functionality has not been extensively tested, though it has been seen to work.


Documentation
-------------

Please read Quickstart.md for an overview of core paradigms of using libUseful. Individual functions are documented in the .h header files.


Author
------
libUseful is (C) 2009 Colum Paget. It is released under the GPLv3 so you may do anything with them that the GPL allows.

Email: colums.projects@gmail.com


DISCLAIMER
----------
This is free software. It comes with no guarentees and I take no responsiblity if it makes your computer explode or opens a portal to the demon dimensions, or does anything.

