#include "find.h"
#include "files.h"
#include "memcached.h"
#include "fingerprint.h"
#include "check-hash.h"
#include <search.h>


/****************************************************

This file contains the implementation of an in-memory storage/search
system for hashes->paths. It's used in finding duplicate files, files
matching a hash, and changed files.

The libc 'tree' implementation is used for speed, but it can only
store one item per key, so the node items (TFingerprint) have
a 'next' pointer, so that the system is a tree with linked lists
hanging off each node.

In 'check' mode items are deleted from this list, so that we can detect
items that have been deleted from the filesystem (as they are still
left in the tree at the end of a check)

Because the head node is a real data node, and the head of the list
it cannot be deleted if there are other items hanging off it, and so
its path is set to 'blank' instead

****************************************************/



void *Tree=NULL;



int MatchCompareFunc(const void *i1, const void *i2)
{
    TFingerprint *M1, *M2;

    M1=(TFingerprint *) i1;
    M2=(TFingerprint *) i2;


    return(strcmp(M1->Hash, M2->Hash));
}


int MatchAdd(TFingerprint *FP, const char *Path, int Flags)
{
    TFingerprint *Item;
    int result=FALSE;
    const char *ptr;
    void *vptr;

    if (Flags & FLAG_MEMCACHED)
    {
        ptr=FP->Data;
        if (StrEnd(ptr)) ptr=FP->Path;
        if (StrEnd(ptr)) ptr=Path;
        if (MemcachedSet(FP->Hash, 0, ptr)) result=TRUE;
    }
    else
    {
        vptr=tsearch(FP, &Tree, MatchCompareFunc);
        Item=*(TFingerprint **) vptr;
        if (strcmp(Item->Path, FP->Path) !=0)
        {
            while (Item->Next !=NULL) Item=(TFingerprint *) Item->Next;
            Item->Next=(void *) FP;
        }
        result=TRUE;
    }

    return(result);
}


TFingerprint *FindPathMatches(HashratCtx *Ctx, TFingerprint *Head, const char *Path)
{
    TFingerprint *Item=NULL, *Prev=NULL;

    Item=Head;
    Prev=Head;
    while (Item->Next && (strcmp(Path,Item->Path) !=0))
    {
        Prev=Item;
        Item=Item->Next;
    }

    if (strcmp(Path, Item->Path)==0)
    {
        if (Ctx->Action==ACT_CHECK)
        {
            //unclip item from list
            if (Item != Head) Prev->Next=Item->Next;
        }
        return(Item);
    }

    return(NULL);
}



TFingerprint *CheckForMatch(HashratCtx *Ctx, const char *Path, struct stat *FStat, const char *HashStr)
{
    TFingerprint *Lookup, *Head=NULL, *Prev=NULL, *Item=NULL, *Result=NULL;
    const char *p_Path;
    void *ptr;

    if (! StrValid(Path)) return(NULL);

    p_Path=Path;
    if (strncmp(p_Path, "./", 2)==0) p_Path++;
    Lookup=TFingerprintCreate(HashStr,"","",p_Path);
    switch (Ctx->Action)
    {
    case ACT_FINDMATCHES_MEMCACHED:
        Lookup->Data=MemcachedGet(Lookup->Data, Lookup->Hash);
        if (StrValid(Lookup->Data)) Result=TFingerprintCreate(Lookup->Hash, Lookup->HashType, Lookup->Data, "");
        break;

    case ACT_FINDDUPLICATES:
    case ACT_FINDMATCHES:
        ptr=tfind(Lookup, &Tree, MatchCompareFunc);
        if (ptr)
        {
            Item=*(TFingerprint **) ptr;

            //we have to make a copy because 'Result' is destroyed by parent function
            Result=TFingerprintCreate(Item->Hash, Item->HashType, Item->Data, Item->Path);
        }
        break;

    default:
        ptr=tfind(Lookup, &Tree, MatchCompareFunc);
        if (ptr)
        {
            Item=FindPathMatches(Ctx, *(TFingerprint **) ptr, p_Path);
            if (Item)
            {
                Result=TFingerprintCreate(Item->Hash, Item->HashType, Item->Data, Item->Path);
                if (Ctx->Action==ACT_CHECK)
                {
                    if (Item==Head)
                    {
                        if (Item->Next==NULL)
                        {
                            // tree functions take a copy of the 'head' item, so we cannot
                            // destroy it. No idea how they do this, it's magic
                            // however we can destroy non-head items that we hang off
                            // the tree
                            tdelete(Lookup, &Tree, MatchCompareFunc);
                        }
                        else Item->Path=CopyStr(Item->Path, "");
                    }
                    //else TFingerprintDestroy(Item);
                }
            }
        }
        break;
    }

    TFingerprintDestroy(Lookup);

    return(Result);
}

void OutputUnmatchedItem(const void *p_Item, const VISIT which, const int depth)
{
    TFingerprint *Item;
    char *Tempstr=NULL;

    if ((which==preorder) || (which==leaf))
    {
        Item=*(TFingerprint **) p_Item;

        while (Item)
        {
            //if a root node of the linked list has been deleted, its path is
            //set blank, rather than actually deleting it, as we need it to
            //continue acting as the head node, so we check StrValid rather
            //than checking for NULL
            if (StrValid(Item->Path))
            {
                if (access(Item->Path, F_OK) !=0) HandleCheckFail(Item->Path, "Missing");
                //else HashSingleFile(&Tempstr, Ctx, Ctx->HashType, Item->Path);


            }
            Item=Item->Next;
        }
    }

    Destroy(Tempstr);
}


void OutputUnmatched(HashratCtx *Ctx)
{
    twalk(Tree, OutputUnmatchedItem);
}


int LoadFromIOC(const char *XML, int Flags)
{
    char *TagType=NULL, *TagData=NULL;
    const char *ptr;
    char *ID=NULL;
    int count=0;
    TFingerprint *FP;

    ptr=XMLGetTag(XML,NULL,&TagType,&TagData);
    while (ptr)
    {
        if (strcasecmp(TagType,"short_description")==0) ptr=XMLGetTag(ptr,NULL,&TagType,&ID);
        if (
            (strcasecmp(TagType,"content")==0) &&
            (strcasecmp(TagData,"type=\"md5\"")==0)
        )
        {
            ptr=XMLGetTag(ptr,NULL,&TagType,&TagData);
            FP=TFingerprintCreate(TagData, "md5", ID, "");
            if (MatchAdd(FP, ID, Flags)) count++;
        }
        ptr=XMLGetTag(ptr,NULL,&TagType,&TagData);
    }

    Destroy(ID);
    Destroy(TagType);
    Destroy(TagData);

    return(count);
}




void *MatchesLoad(HashratCtx *Ctx, int Flags)
{
    char *Line=NULL, *Tempstr=NULL, *Type=NULL, *ptr;
    TFingerprint *FP;
    STREAM *S;
    int count=0;


    S=STREAMFromFD(0);
    STREAMSetTimeout(S,100);
    Line=STREAMReadLine(Line,S);
    if (! StrValid(Line)) return(NULL);

    if (strncasecmp(Line,"<?xml ",6)==0)
    {
        //xml document. Must be an OpenIOC fileq
        while (Line)
        {
            StripTrailingWhitespace(Line);
            Tempstr=CatStr(Tempstr,Line);
            Line=STREAMReadLine(Line,S);
        }
        count=LoadFromIOC(Tempstr,Flags);
    }
    else
    {
        while (Line)
        {
            StripTrailingWhitespace(Line);
            FP=TFingerprintParse(Line);
            if (FP)
            {
                if (MatchAdd(FP, "", Flags)) count++;
                //native format can specify the type of hash that it is supplying
                if (StrValid(FP->HashType)) Ctx->HashType=CopyStr(Ctx->HashType, FP->HashType);
            }
            Line=STREAMReadLine(Line, S);
        }
    }

    if (Flags & FLAG_MEMCACHED) printf("Stored %d hashes in memcached server\n", count);

    Destroy(Tempstr);
    Destroy(Line);
    Destroy(Type);

    return(Tree);
}

