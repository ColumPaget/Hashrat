#ifndef HASHRAT_OTP_H
#define HASHRAT_OTP_H

#include "common.h"

void OTPParse(HashratCtx *Ctx, const char *URL);
void OTPGoogle(HashratCtx *Ctx, const char *Secret);
void OTPProcess(HashratCtx *Ctx);

#endif
