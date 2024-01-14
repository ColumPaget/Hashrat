/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_OTP_H
#define LIBUSEFUL_OTP_H

/* Generate a Time based One Time Password. 
For compatiblity with Google authenticator you should use:

HashType=sha1
Digits=6
Interval=30
SecretEncoding=ENCODE_BASE32

This function does not parse totp urls, you will have to extract the secret from those yourself.
*/
char *TOTP(char *RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval);


//Google authenticator compatible TOTP
char *GoogleTOTP(char *RetStr, const char *EncodedSecret);

#endif
