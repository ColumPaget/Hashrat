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



//Calculate TOTP at a given time, returns number of seconds the code is valid from that time
int TOTPAtTime(char **RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, time_t When, int Digits, int Interval);


//Calculate TOTP at the current moment
char *TOTP(char *RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval);


//Calculate TOTP at Current time, at CurrTime - (Interval / 2 +5) and at CurrTime + (Interval / 2 +5)
//returns number of seconds the 'Curr' code is valid from CurrTime
int TOTPPrevCurrNext(char **Prev, char **Curr, char **Next, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval);


//Calculate GoogleAutenticator code for the current moment
char *GoogleTOTP(char *RetStr, const char *EncodedSecret);


//Google authenticator compatible TOTP
char *GoogleTOTP(char *RetStr, const char *EncodedSecret);

#endif
