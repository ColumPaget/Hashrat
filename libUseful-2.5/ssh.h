#ifndef LIBUSEFUL_SSH
#define LIBUSEFUL_SSH

#include "file.h"

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command);

#endif
