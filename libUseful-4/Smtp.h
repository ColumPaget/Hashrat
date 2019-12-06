/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SMTP_H
#define LIBUSEFUL_SMTP_H

#include "includes.h"
#include "defines.h"

#define SMTP_NOSSL 1
#define SMTP_NOHEADER 2

#ifdef __cplusplus
extern "C" {
#endif

int SMTPSendMail(const char *Sender, const char *Recipient, const char *Subject, const char *Body, int Flags);
int SMTPSendMailFile(const char *Sender, const char *Recipient, const char *Path, int Flags);

#ifdef __cplusplus
}
#endif


#endif
