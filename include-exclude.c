#include "include-exclude.h"

void IncludeExcludeAdd(HashratCtx *Ctx, int Type, const char *Item)
{
    ListNode *Node;
    char *Token=NULL;
    const char *ptr;
    static int PathRuleAdded=FALSE;

//if we get given an include with no previous excludes, then
//set CTX_EXCLUDE as the default
    if (! IncludeExclude)
    {
        IncludeExclude=ListCreate();
    }


//if the first rule added for file paths/names is 'include, then set the default to be exclude
//if the first such rule is an exlcude, then set the default to be include
    if (! PathRuleAdded)
    {
        if (Type==CTX_INCLUDE)
        {
            Ctx->Flags |= CTX_EXCLUDE;
            PathRuleAdded=TRUE;
        }
        else if (Type==CTX_EXCLUDE)
        {
            Ctx->Flags |= CTX_INCLUDE;
            PathRuleAdded=TRUE;
        }
    }

    ptr=GetToken(Item, ",",&Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        Node=ListAddItem(IncludeExclude, CopyStr(NULL, Token));
        Node->ItemType=Type;
        ptr=GetToken(ptr, ",",&Token, GETTOKEN_QUOTES);
    }

    Destroy(Token);
}


void IncludeExcludeLoadExcludesFromFile(const char *Path, HashratCtx *Ctx)
{
    STREAM *S;
    char *Tempstr=NULL;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            IncludeExcludeAdd(Ctx, CTX_EXCLUDE, Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    DestroyString(Tempstr);
}


//this checks include and exclude by name. These can override each other, so you can exclude a pattern,
//but include things that match that pattern that are members of an 'include' subpattern
static int IncludeExcludeCheckFilenames(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
    ListNode *Curr;
    const char *mptr, *dptr;
    int result=CTX_INCLUDE;

    if (Ctx->Flags & CTX_EXCLUDE) result=CTX_EXCLUDE;
    if (FStat && S_ISDIR(FStat->st_mode)) result=CTX_INCLUDE;

    Curr=ListGetNext(IncludeExclude);
    while (Curr)
    {
        mptr=(char *) Curr->Item;
        dptr=Path;

        //if match pattern doesn't start with '/' then we want to strip that off the current path
        //so that we can match against it. However if the match pattern contains no '/' at all, then
        //it's a file name rather than a path, in which case we should use basename on both it and
        //the current path
        if (*mptr != '/')
        {
            if (strchr(mptr,'/'))
            {
                if (*dptr=='/') dptr++;
            }
            else
            {
                mptr=GetBasename(mptr);
                dptr=GetBasename(Path);
            }
        }

        switch (Curr->ItemType)
        {
        case CTX_INCLUDE:
            if (fnmatch(mptr,dptr,0)==0) result=CTX_INCLUDE;
            break;

        case CTX_EXCLUDE:
            if (fnmatch(mptr,dptr,0)==0) result=CTX_EXCLUDE;
            break;

            /*
            	case INEX_INCLUDE_DIR:
            	if (strncmp(mptr,dptr,StrLen(mptr))==0) result=TRUE;
            	break;

            	case INEX_EXCLUDE_DIR:
            	if (strncmp(mptr,dptr,StrLen(mptr))==0) result=FALSE;
            	printf("FNMD: [%s] [%s] %d\n",mptr,dptr,result);
            	break;
            */
        }

        Curr=ListGetNext(Curr);
    }

    return(result);
}


int IncludeExcludeCheck(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
    ListNode *Curr;
    const char *mptr, *dptr;
    int result, mult;
    time_t When;

    if (FStat)
    {
        if (Ctx->Flags & CTX_ONE_FS)
        {
            if (Ctx->StartingFS==0) Ctx->StartingFS=FStat->st_dev;
            else if (FStat->st_dev != Ctx->StartingFS) return(CTX_ONE_FS);
        }
        if ((Ctx->Flags & CTX_EXES) && (! (FStat->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))) return(CTX_EXCLUDE);
    }



//'CheckIncludeExclude' checks 'normal' includes and excludes (by filename). The rest of this function
//checks properties of included files, like their mtime
    result=IncludeExcludeCheckFilenames(Ctx, Path, FStat);
    if (result == CTX_INCLUDE)
    {
        Curr=ListGetNext(IncludeExclude);
        while (Curr)
        {
            mptr=(char *) Curr->Item;
            mult=60;
            switch (Curr->ItemType)
            {
            case CTX_MYEAR:
                mult *= 365;

            case CTX_MTIME:
                mult *= 24 * 60;
            //fall through

            case CTX_MMIN:
                //don't even consider mtime if we don't have a File Stat for the item, or if it's a directory
                if (FStat && (! S_ISDIR(FStat->st_mode)))
                {
                    if (*mptr=='-')
                    {
                        When=Now - atol(mptr+1) * mult;
                        //'since' a time will mean that the mtime for the file will be greater than that time
                        if (FStat->st_mtime < When) result=CTX_EXCLUDE;
                    }
                    else if (*mptr=='+')
                    {
                        When=Now - atol(mptr+1) * mult;
                        //'before' a time will mean that the mtime for the file will be less than that time
                        if (FStat->st_mtime > When) result=CTX_EXCLUDE;
                    }
                    else
                    {
                        When=Now - atol(mptr) * mult;
                        if ( (FStat->st_mtime < When) || (FStat->st_mtime > (When + mult)) ) result=CTX_EXCLUDE;
                    }
                }
                break;

            }
            Curr=ListGetNext(Curr);
        }
    }

    return(result);
}

