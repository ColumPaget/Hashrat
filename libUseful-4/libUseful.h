/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIB_USEFUL_H
#define LIB_USEFUL_H

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define __LIBUSEFUL_VERSION__ VERSION
#define __LIBUSEFUL_BUILD_DATE__ __DATE__
#define __LIBUSEFUL_BUILD_TIME__ __TIME__

//__TIME__

#include "includes.h"
#include "memory.h"
#include "IPAddress.h"
#include "Socket.h"
#include "Server.h"
#include "UnixSocket.h"
#include "String.h"
#include "Expect.h"
#include "List.h"
#include "Stream.h"
#include "base64.h"
#include "Tokenizer.h"
#include "FileSystem.h"
#include "GeneralFunctions.h"
#include "DataProcessing.h"
#include "Encodings.h"
#include "Hash.h"
#include "Compression.h"
#include "Time.h"
#include "Vars.h"
#include "Markup.h"
#include "PatternMatch.h"
#include "ConnectionChain.h"
#include "SpawnPrograms.h"
#include "DataParser.h"
#include "CommandLineParser.h"
#include "URL.h"
#include "Pty.h"
#include "Log.h"
#include "Http.h"
#include "Gemini.h"
#include "OAuth.h"
#include "Ssh.h"
#include "Smtp.h"
#include "Terminal.h"
#include "TerminalMenu.h"
#include "TerminalChoice.h"
#include "TerminalBar.h"
#include "Process.h"
#include "SecureMem.h"
#include "SysInfo.h"

#endif
