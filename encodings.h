#ifndef HASHRAT_ENCODING_H
#define HASHRAT_ENCODING_H

#include "common.h"

//provides certain values for use in frontends like .cgi and xdialog

extern const char *EncodingNames[];
extern const char *EncodingDescriptions[];
extern int Encodings[];

char *EncodingNameFromID(int id);
char *EncodingDescriptionFromID(int id);

#endif
