/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SYSINFO
#define LIBUSEFUL_SYSINFO


#include "defines.h"
#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {OSINFO_TYPE, OSINFO_ARCH, OSINFO_RELEASE, OSINFO_HOSTNAME, OSINFO_UPTIME, OSINFO_TOTALMEM, OSINFO_FREEMEM, OSINFO_BUFFERMEM, OSINFO_TOTALSWAP, OSINFO_FREESWAP, OSINFO_LOAD1MIN, OSINFO_LOAD5MIN, OSINFO_LOAD15MIN, OSINFO_HOMEDIR, OSINFO_TMPDIR, OSINFO_PROCS, OSINFO_USERINFO, OSINFO_DOMAINNAME, OSINFO_INTERFACES} EOSInfo;

// pass in one of the OSINFO_ defines above to get out a string answer
//OSINFO_TYPE   system type, e.g. 'linux'
//OSINFO_ARCH   system architecture, e.g. 'arm' 'i386'
//OSINFO_RELEASE  system kernel version
//OSINFO_HOSTNAME system hostname
//OSINFO_DOMAINNAME system domainname
//OSINFO_INTERFACES list of network interfaces
//OSINFO_HOMEDIR    user home directory
//OSINFO_TMPDIR     user temporary directory
const char *OSSysInfoString(int Info);

// pass in one of the OSINFO_ defines to get a numeric answer
// OSINFO_UPTIME  seconds since boot
// OSINFO_TOTALMEM  visible system memory
// OSINFO_FREEMEM   available system memory
// OSINFO_BUFFERMEM memory used in buffers
// OSINFO_TOTALSWAP total swap space
// OSINFO_FREESWAP  free swap space
// OSINFO_LOAD1MIN  system load in last minute
// OSINFO_LOAD5MIN  system load in last five minutes
// OSINFO_LOAD15MIN  system load in last fifteen minutes
// OSINFO_PROCS      number of processes running
size_t OSSysInfoLong(int Info);

#ifdef __cplusplus
}
#endif


#endif

