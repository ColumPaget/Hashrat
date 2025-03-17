#include "ContentType.h"

const char *ContentTypeForFile(const char *File)
{
    const char *ptr;

    ptr=strrchr(File, '.');
    if (StrValid(ptr))
    {
        if (strcasecmp(ptr, ".pdf")==0) return("application/pdf");
        if (strcasecmp(ptr, ".rtf")==0) return("application/rtf");
        if (strcasecmp(ptr, ".xml")==0) return("application/xml");
        if (strcasecmp(ptr, ".zip")==0) return("application/zip");
        if (strcasecmp(ptr, ".csv")==0) return("text/csv");
        if (strcasecmp(ptr, ".txt")==0) return("text/plain");
        if (strcasecmp(ptr, ".htm")==0) return("text/html");
        if (strcasecmp(ptr, ".html")==0) return("text/html");
        if (strcasecmp(ptr, ".md")==0) return("text/markdown");
        if (strcasecmp(ptr, ".ics")==0) return("text/calendar");
        if (strcasecmp(ptr, ".ical")==0) return("text/calendar");
        if (strcasecmp(ptr, ".gif")==0) return("image/gif");
        if (strcasecmp(ptr, ".jpg")==0) return("image/jpeg");
        if (strcasecmp(ptr, ".png")==0) return("image/png");
        if (strcasecmp(ptr, ".svg")==0) return("image/svg");
        if (strcasecmp(ptr, ".jp2")==0) return("image/jp2");
        if (strcasecmp(ptr, ".jpx")==0) return("image/jpx");
        if (strcasecmp(ptr, ".webm")==0) return("image/webm");
        if (strcasecmp(ptr, ".wav")==0) return("audio/wav");
        if (strcasecmp(ptr, ".mp3")==0) return("audio/mpeg");
        if (strcasecmp(ptr, ".ogg")==0) return("audio/ogg");
        if (strcasecmp(ptr, ".oga")==0) return("audio/ogg");
        if (strcasecmp(ptr, ".flac")==0) return("audio/flac");
        if (strcasecmp(ptr, ".mka")==0) return("audio/x-matroska");
        if (strcasecmp(ptr, ".mp4")==0) return("video/mp4");
        if (strcasecmp(ptr, ".ogv")==0) return("video/theora");
        if (strcasecmp(ptr, ".mkv")==0) return("video/x-matroska");
    }

    return("octet-stream");
}
