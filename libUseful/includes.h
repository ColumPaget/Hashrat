/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_INCLUDES_H
#define LIBUSEFUL_INCLUDES_H

#define _GNU_SOURCE
#define _USE_POSIX

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <limits.h>
#include <pwd.h>  //for uid_t
#include <grp.h>  //for gid_t
#include <math.h> //for math defines like PI



#include "defines.h"
#include "String.h"
#include "List.h"
#include "Time.h"
#include "Stream.h"
#include "Socket.h"
#include "OpenSSL.h"
#include "Vars.h"
#include "Errors.h"
#include "LibSettings.h"
#include "GeneralFunctions.h"
#include "Tokenizer.h"
#include "Process.h"

#endif
