/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


/* 
This module provides a data processor that uses openssl for encryption and decryption.
The resulting output is openssl compatible 'Salted__' format, with pbkdf2 interrated 10000 times
*/


#ifndef LIBUSEFUL_ENCRYPTION_H
#define LIBUSEFUL_ENCRYPTION_H


#include "includes.h"

#include "DataProcessing.h"


#ifdef __cplusplus
extern "C" {
#endif

TProcessingModule *libCryptoProcessorCreate();


#ifdef __cplusplus
}
#endif



#endif
