#ifndef LIBUSEFUL_MARKUP_H
#define LIBUSEFUL_MARKUP_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


char *XMLGetTag(char *Input, char **Namespace, char **TagType, char **TagData);
char *HtmlGetTag(char *Input, char **TagType, char **TagData);
char *HtmlDeQuote(char *RetStr, char *Data);




#ifdef __cplusplus
}
#endif



#endif
