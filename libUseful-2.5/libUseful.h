#ifndef LIB_USEFUL_H
#define LIB_USEFUL_H

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define __LIBUSEFUL_VERSION__ "2.0"
#define __LIBUSEFUL_BUILD_DATE__ __DATE__ 
#define __LIBUSEFUL_BUILD_TIME__ __TIME__ 

//__TIME__

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "memory.h"
#include "socket.h"
#include "unix_socket.h"
#include "string.h"
#include "expect.h"
#include "list.h"
#include "file.h"
#include "base64.h"
#include "Tokenizer.h"
#include "FileSystem.h"
#include "GeneralFunctions.h"
#include "DataProcessing.h"
#include "EncryptedFiles.h"
#include "ConnectManager.h"
#include "Hash.h"
#include "Compression.h"
#include "Time.h"
#include "Vars.h"
#include "Markup.h"
#include "MathExpr.h"
#include "PatternMatch.h"
#include "SpawnPrograms.h"
#include "MessageBus.h"
#include "ParseURL.h"
#include "sound.h"
#include "pty.h"
#include "Log.h"
#include "http.h"
#include "oauth.h"
#include "tar.h"
#include "ansi.h"
#include "proctitle.h"
#include "securemem.h"

#endif
