#include "output.h"

static char *FindCommand(char *RetStr, const char *Commands)
{
    char *Cmd=NULL, *Token=NULL, *Path=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    ptr=GetToken(Commands, ",", &Cmd, 0);
    while (ptr)
    {
        GetToken(Cmd, "\\S", &Token, 0);
        Path=FindFileInPath(Path, Token, getenv("PATH"));
        if (StrValid(Path))
        {
            RetStr=CopyStr(RetStr, Cmd);
            break;
        }
        ptr=GetToken(ptr, ",", &Cmd, 0);
    }

    Destroy(Token);
    Destroy(Path);
    Destroy(Cmd);

    return(RetStr);
}

static void OutputXtermSelect(HashratCtx *Ctx, const char *Text)
{
    char *Tempstr=NULL, *Base64=NULL;

    Base64=EncodeBytes(Base64, Text, StrLen(Text), ENCODE_BASE64);
    Tempstr=FormatStr(Tempstr,"\x1b]52;cps;%s\x07",Base64);
    STREAMWriteString(Tempstr, Ctx->Out);

    Destroy(Tempstr);
    Destroy(Base64);
}


static void OutputToClipboard(HashratCtx *Ctx, const char *Text)
{
    char *Tempstr=NULL, *Cmd=NULL;
    const char *ptr;
    STREAM *S;

    Cmd=FindCommand(Cmd, GetVar(Ctx->Vars, "clipboard_commands"));

//if we found a matching clipboard command. Otherwise fall back to xterm osc52
    if (StrValid(Cmd))
    {
        Tempstr=MCopyStr(Tempstr, "cmd:", Cmd, NULL);
        S=STREAMOpen(Tempstr, "");
        if (S)
        {
            STREAMWriteLine(Text, S);
            STREAMCommit(S);
            STREAMClose(S);
        }
    }
    else OutputXtermSelect(Ctx, Text);

    Destroy(Tempstr);
    Destroy(Cmd);
}


static void OutputQRCode(HashratCtx *Ctx, const char *Text)
{
    char *Cmd=NULL, *Tempstr=NULL, *PNGPath=NULL;
    STREAM *S;


    waitpid(-1, NULL, WNOHANG);
    if (fork() ==0)
    {
        PNGPath=FormatStr(PNGPath, "/tmp/%d.png", getpid());
        Tempstr=MCopyStr(Tempstr, "cmd:qrencode -o ", PNGPath, NULL);
        S=STREAMOpen(Tempstr, "");
        if (S)
        {
            STREAMWriteLine(Text, S);
            STREAMCommit(S);
            STREAMClose(S);
        }

        Cmd=FindCommand(Cmd, GetVar(Ctx->Vars, "image_viewers"));
        if (StrValid(Cmd))
        {
            Tempstr=MCopyStr(Tempstr, Cmd, " '", PNGPath, "'", NULL);
            Spawn(Tempstr, "");
            sleep(5);
            unlink(PNGPath);
        }

        _exit(0);
    }

    Destroy(Tempstr);
    Destroy(PNGPath);
    Destroy(Cmd);
}



int HashratOutputFileInfo(HashratCtx *Ctx, STREAM *Out, const char *Path, struct stat *Stat, const char *iHash)
{
    char *Line=NULL, *Tempstr=NULL, *Hash=NULL, *ptr;
    const char *p_Type="unknown";
    uint64_t diff;

    if (Ctx->SegmentLength > 0) Hash=ReformatHash(Hash, iHash, Ctx);
    else Hash=CopyStr(Hash, iHash);

    if (Ctx->OutputLength > 0) StrTrunc(Hash, Ctx->OutputLength);

    if (Flags & FLAG_TRAD_OUTPUT) Line=MCopyStr(Line,Hash, "  ", Path,"\n",NULL);
    else if (Flags & FLAG_BSD_OUTPUT)
    {
        Line=CopyStr(Line,Ctx->HashType);
        for (ptr=Line; *ptr != '\0'; ptr++) *ptr=toupper(*ptr);
        Line=MCatStr(Line, " (", Path, ") = ", Hash, "\n",NULL);
    }
    else
    {
        /*
        	struct stat {
        dev_t     st_dev;     // ID of device containing file
        ino_t     st_ino;     // inode number
        mode_t    st_mode;    // protection
        nlink_t   st_nlink;   // number of hard links
        uid_t     st_uid;     // user ID of owner
        gid_t     st_gid;     // group ID of owner
        dev_t     st_rdev;    // device ID (if special file)
        off_t     st_size;    // total size, in bytes
        blksize_t st_blksize; // blocksize for file system I/O
        blkcnt_t  st_blocks;  // number of 512B blocks allocated
        time_t    st_atime;   // time of last access
        time_t    st_mtime;   // time of last modification
        time_t    st_ctime;   // time of last status change
        */

        if (S_ISREG(Stat->st_mode)) p_Type="file";
        else if (S_ISDIR(Stat->st_mode)) p_Type="dir";
        else if (S_ISLNK(Stat->st_mode)) p_Type="link";
        else if (S_ISSOCK(Stat->st_mode)) p_Type="socket";
        else if (S_ISFIFO(Stat->st_mode)) p_Type="fifo";
        else if (S_ISCHR(Stat->st_mode)) p_Type="chrdev";
        else if (S_ISBLK(Stat->st_mode)) p_Type="blkdev";

        Line=FormatStr(Line,"hash='%s:%s' type='%s' mode='%o' uid='%lu' gid='%lu' ", Ctx->HashType,Hash,p_Type,Stat->st_mode,Stat->st_uid,Stat->st_gid);

        //This dance is to handle the fact that on some 32-bit OS, like openbsd, stat with have 64-bit members
        //even if we've not asked for 'largefile' support, while on Linux it will have 32-bit members

        if (sizeof(Stat->st_size)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "size='%llu' ",Stat->st_size);
        else Tempstr=FormatStr(Tempstr, "size='%lu' ",Stat->st_size);

        Line=CatStr(Line, Tempstr);

        if (sizeof(Stat->st_mtime)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "mtime='%llu' ",Stat->st_mtime);
        else Tempstr=FormatStr(Tempstr, "mtime='%lu' ",Stat->st_mtime);
        Line=CatStr(Line, Tempstr);

        if (sizeof(Stat->st_ino)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "inode='%llu' ",Stat->st_ino);
        else Tempstr=FormatStr(Tempstr, "inode='%lu' ",Stat->st_ino);
        Line=CatStr(Line, Tempstr);

        if (Flags & FLAG_VERBOSE)
        {
            diff=GetTime(TIME_MILLISECS) - HashStartTime;
            if (diff > 1000) Tempstr=FormatStr(Tempstr, "hash-time='%0.2fs' ", (double) diff / 1000.0	);
            else Tempstr=FormatStr(Tempstr, "hash-time='%llums' ", (unsigned long long) diff	);
            Line=CatStr(Line, Tempstr);
        }

        //we must quote out apostrophes
        Tempstr=QuoteCharsInStr(Tempstr, Path, "'");
        Line=MCatStr(Line,"path='",Tempstr,"'\n",NULL);
    }

    STREAMWriteString(Line,Out);

    Destroy(Tempstr);
    Destroy(Line);
    Destroy(Hash);

    return(TRUE);
}




void OutputHash(HashratCtx *Ctx, const char *Hash, const char *Comment)
{
    char *Tempstr=NULL, *Reformatted=NULL;
    const char *p_Hash;

    p_Hash=Hash;
    if (Ctx->Flags & CTX_REFORMAT)
    {
        Reformatted=ReformatHash(Reformatted, Hash, Ctx);
        p_Hash=Reformatted;
    }

    Tempstr=MCopyStr(Tempstr, p_Hash, "  ", Comment, "\n", NULL);
    STREAMWriteString(Tempstr, Ctx->Out);

    if (Flags & FLAG_CLIPBOARD) OutputToClipboard(Ctx, Tempstr);
    else if (Flags & FLAG_XSELECT) OutputXtermSelect(Ctx, Tempstr);

    if (Flags & FLAG_QRCODE) OutputQRCode(Ctx, Tempstr);

    Destroy(Reformatted);
    Destroy(Tempstr);
}


