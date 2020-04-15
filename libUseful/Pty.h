/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_PTY_H
#define LIBUSEFUL_PTY_H

#include "defines.h"

//These are passed to TTYConfig
#define TTYFLAG_PTY 1
#define TTYFLAG_CANON 4096
#define TTYFLAG_HARDWARE_FLOW 8192  //enable hardware flow control
#define TTYFLAG_SOFTWARE_FLOW 16324 //enable software flow control
#define TTYFLAG_CRLF_KEEP 32768
#define TTYFLAG_IGNSIG 65536
#define TTYFLAG_ECHO 131072        //enable input echo
#define TTYFLAG_IN_CRLF 262144     //change CarriageReturn to LineFeed on input
#define TTYFLAG_IN_LFCR 524288     //change LineFeed to CarriageReturn on input
#define TTYFLAG_OUT_CRLF 1048576   //change LineFeed to CarriageReturn-Linefeed on output
#define TTYFLAG_NONBLOCK 2097152
#define TTYFLAG_SAVE     4194304   //save attributes for later use with TTYReset
#define COMMS_COMBINE_STDERR 8388608   //save attributes for later use with TTYReset

#define STREAMConfigTTY(S,speed,flags) ((S && istty(S->in_fd)) ? TTYConfig(S->in_fd,speed,flags))
#define STREAMResetTTY(S) ((S && istty(S->in_fd)) ? TTYReset(S->in_fd))
#define STREAMHangUpTTY(S) ((S && istty(S->in_fd)) ? TTYHangUp(S->in_fd))

#ifdef __cplusplus
extern "C" {
#endif

//Reset a tty config to the values that were set before TTYOpen or TTYConfig were called
int TTYReset(int tty);

//shutdown TTY (hangup line if phone)
int TTYHangUp(int tty);

void PTYSetGeometry(int pty, int wid, int high);

//open a tty device (like /dev/ttyS0). Flags are as 'TTYFLAG_' #defines above
int TTYOpen(const char *devname, int LineSpeed, int Flags);

//change config of an already opened tty device. Flags are as 'TTYFLAG_' #defines above
void TTYConfig(int tty, int LineSpeed, int Flags);

//TTYOpen except accepting text config, not flags. Intended for bindings to languages like lua that don't handle
//bit-flags well. possible config text options are:

// 'pty'  this is a noop in these functions, but is used in the Spawn functions
// 'canon' turn on canonical processing
// 'echo' turn on tty echo
// 'xon' software flow control
// 'sw' software flow control
// 'hw' hardware flow control
// 'nonblock'  nonblocking
// 'nb'  nonblocking
// 'ilfcr' input: change linefeed to carriage return
// 'icrlf' input: change carriage return to linefeed
// 'ocrlf' output: change linefeed to carriagereturn-linefeed
// 'nosig' no signals
// 'save'  save current attributes (saved attributes can be restored by using TTYReset)
// numeric values are taken as tty linespeeds

int TTYConfigOpen(const char *Dev, const char *Config);

//Parse 'text config' into flags and speed
int TTYParseConfig(const char *Config, int *Speed);

//master and slave are two ends of a PseudoTTY 'pipe'. This is a two-way pipe with all the properties (like line-speed, tty modes etc) of
// a tty. I normally attach 'slave' to stdin and stdout of some process, and use 'master' as the end I read/write from/to
int PseudoTTYGrab(int *master, int *slave, int Flags);

#ifdef __cplusplus
}
#endif


#endif
