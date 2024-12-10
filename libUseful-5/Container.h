/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_CONTAINER_H
#define LIBUSEFUL_CONTAINER_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>

//this module relates to namespaces/containers. Much of this is pretty linux specific, and would be called
//via 'ProcessApplyConfig' rather than calling this function directly.

int ContainerApplyConfig(const char *Config);

#ifdef __cplusplus
}
#endif

#endif
