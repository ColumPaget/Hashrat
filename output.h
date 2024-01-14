#ifndef HASHRAT_OUTPUT_H
#define HASHRAT_OUTPUT_H

#include "common.h"

int HashratOutputFileInfo(HashratCtx *Ctx, STREAM *Out, const char *Path, struct stat *Stat, const char *iHash);
void OutputHash(HashratCtx *Ctx, const char *Hash, const char *Comment);

#endif

