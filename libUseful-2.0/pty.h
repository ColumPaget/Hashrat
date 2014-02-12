#ifndef LIBUSEFUL_PTY_H
#define LIBUSEFUL_PTY_H

#include "defines.h"

#define TTYFLAG_ECHO 256
#define TTYFLAG_CRLF 512
#define TTYFLAG_LFCR 1024
#define TTYFLAG_NONBLOCK 2048
#define TTYFLAG_CANON 4096
#define TTYFLAG_HARDWARE_FLOW 8192
#define TTYFLAG_SOFTWARE_FLOW 16324
#define TTYFLAG_CRLF_KEEP 32768
#define TTYFLAG_IGNSIG 65536

#ifdef __cplusplus
extern "C" {
#endif

void HangUpLine(int tty);
int OpenTTY(char *devname, int LineSpeed, int Flags);
void InitTTY(int tty, int LineSpeed, int Flags);
void ResetTTY(int tty);
int GrabPseudoTTY(int *pty, int *tty, int Flags);

#ifdef __cplusplus
}
#endif


#endif
