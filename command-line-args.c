#include "command-line-args.h"
#include "xattr.h"

#define CMDLINE_ARG_NAMEVALUE 1
#define CMDLINE_FROM_LISTFILE 2
#define CMDLINE_XATTR     4
#define CMDLINE_TXATTR     8
#define CMDLINE_MEMCACHED 64



void AddIncludeExclude(HashratCtx *Ctx, int Type, const char *Item)
{
ListNode *Node;
char *Token=NULL;
const char *ptr;

//if we get given an include with no previous excludes, then 
//set CTX_EXCLUDE as the default
if (! IncludeExclude)
{
  IncludeExclude=ListCreate();
  if (Type==CTX_INCLUDE) Ctx->Flags |= CTX_EXCLUDE;
}

ptr=GetToken(Item, ",",&Token, GETTOKEN_QUOTES);
while (ptr)
{
	Node=ListAddItem(IncludeExclude, CopyStr(NULL, Token));
	Node->ItemType=Type;
	ptr=GetToken(ptr, ",",&Token, GETTOKEN_QUOTES);
}

DestroyString(Token);
}



void HMACSetup(HashratCtx *Ctx)
{
char *Tempstr=NULL, *ptr;
STREAM *S;

  ptr=GetVar(Ctx->Vars,"EncryptionKey");
  if (StrLen(ptr)==0)
  {
    if (isatty(0))
    {
      write(1, "Enter HMAC Key: ",16);

      S=STREAMFromFD(0);
      Tempstr=STREAMReadLine(Tempstr,S);
      StripTrailingWhitespace(Tempstr);
      SetVar(Ctx->Vars,"EncryptionKey",Tempstr);
      ptr=Tempstr;
      STREAMDisassociateFromFD(S);
    }

    //By now we must have an encryption key!
    if (! StrLen(ptr))
    {
      write(1,"ERROR: No HMAC Key given!\n",27);
      exit(2);
    }
  }
DestroyString(Tempstr);
}


//this function is called when a command-line switch has been recognized. It's told which flags to set
//and whether the switch takes a string argument, which it then reads and stores in the variable list
//with the name supplied in 'VarName'. It blanks out anything it reads, so that only unrecognized 
//arguments remain, which are treated as filenames for processing

int CommandLineHandleArg(int argc, char *argv[], int pos, int ParseFlags, int SetFlags, char *VarName, char *VarValue, ListNode *Vars)
{
	Flags |= SetFlags;
	
	if (ParseFlags & CMDLINE_ARG_NAMEVALUE)
	{
	if (argv[pos + 1] ==NULL)
	{
		printf("ERROR: The %s option requires an argument.\n",argv[pos]);
		exit(1);
	}
	else
	{
		strcpy(argv[pos],"");
		pos++;
		SetVar(Vars,VarName,argv[pos]);
	}
	}
	else if (StrLen(VarName)) SetVar(Vars,VarName,VarValue);

	if (argc > 0) strcpy(argv[pos],"");

return(ParseFlags);
}



void CommandLineSetCtx(int argc, char *argv[], int pos, HashratCtx *Ctx, int Flag, int Encoding)
{
if (Encoding > 0) Ctx->Encoding=Encoding;
Ctx->Flags |= Flag;
strcpy(argv[pos],"");

if (Flag == CTX_INCLUDE) 
{
	pos++;
	AddIncludeExclude(Ctx,CTX_INCLUDE, argv[pos]);
	strcpy(argv[pos],"");
}
else if (Flag == CTX_EXCLUDE) 
{
	pos++;
	AddIncludeExclude(Ctx,CTX_EXCLUDE, argv[pos]);
	strcpy(argv[pos],"");
}

}




void CommandLineHandleUpdate(int argc, char *argv[], int i, HashratCtx *Ctx)
{
char *Token=NULL, *ptr;

Flags |= FLAG_UPDATE;
strcpy(argv[i],"");
i++;
ptr=GetToken(argv[i],",",&Token,0);
while (ptr)
{
	if (strcasecmp(Token, "stderr")==0) Ctx->Aux=STREAMFromFD(2);
	else if (strcasecmp(Token, "xattr")==0) Ctx->Flags |= CTX_STORE_XATTR;
	else if (strcasecmp(Token, "txattr")==0) Ctx->Flags |= CTX_STORE_XATTR | CTX_XATTR_ROOT;
	else if (strcasecmp(Token, "memcached")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (strcasecmp(Token, "mcd")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (! Ctx->Aux) Ctx->Aux=STREAMOpenFile(Token,SF_WRONLY | SF_CREAT | SF_TRUNC);

ptr=GetToken(ptr,",",&Token,0);
}
strcpy(argv[i],"");

DestroyString(Token);
}


//this is the main parsing function that goes through the command-line args
HashratCtx *CommandLineParseArgs(int argc,char *argv[])
{
int i=0;
char *ptr, *Tempstr=NULL;
HashratCtx *Ctx;
int ParseFlags=0;

//You never know when you're going to be run with  no args at all (say, out of inetd)

//Setup default context
Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
Ctx->Action=ACT_HASH;
Ctx->ListPath=CopyStr(Ctx->ListPath,"-");

if (argc < 1) 
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}

Ctx->Vars=ListCreate();
Ctx->Out=STREAMFromFD(1);
SetVar(Ctx->Vars,"HashType","md5");

//argv[0] might be full path to the program, or just its name
ptr=strrchr(argv[0],'/');
if (! ptr) ptr=argv[0];
else ptr++;


//if the program name is something other than 'hashrat', then we're being used as a drop-in
//replacement for another program. Change flags/behavior accordingly
if (strcmp(ptr,"md5sum")==0) 
ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "md5",Ctx->Vars);
if (
		(strcmp(ptr,"sha1sum")==0) ||
		(strcmp(ptr,"shasum")==0) 
	) 
ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha1",Ctx->Vars);
if (strcmp(ptr,"sha256sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha256",Ctx->Vars);
if (strcmp(ptr,"sha512sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha512",Ctx->Vars);
if (strcmp(ptr,"whirlpoolsum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "whirlpool",Ctx->Vars);
if (strcmp(ptr,"jh224sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-224",Ctx->Vars);
if (strcmp(ptr,"jh256sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-256",Ctx->Vars);
if (strcmp(ptr,"jh384sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-385",Ctx->Vars);
if (strcmp(ptr,"jh512sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-512",Ctx->Vars);


if (strcmp(ptr,"hashrat.cgi")==0) 
{
	Ctx->Action=ACT_CGI;
	return(Ctx);
}



//here we got through the command-line args, and set things up whenever we find one that we
//recognize. We blank the args we use so that any 'unrecognized' ones still left after this
//process can be treated as filenames for hashing
for (i=1; i < argc; i++)
{
if (
		(strcmp(argv[i],"-V")==0) ||
		(strcmp(argv[i],"--version")==0) ||
		(strcmp(argv[i],"-version")==0)
	)
{
	printf("version: %s\n",VERSION);
	return(NULL);
}
else if (
		(strcmp(argv[i],"--help")==0) ||
		(strcmp(argv[i],"-help")==0) ||
		(strcmp(argv[i],"-?")==0)
	)
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}
else if (strcmp(argv[i],"-C")==0)
{
	Ctx->Action = ACT_CHECK;
	CommandLineSetCtx(argc, argv, i, Ctx, CTX_RECURSE,0);
}
else if (strcmp(argv[i],"-Cf")==0)
{
	Ctx->Action = ACT_CHECK;
	CommandLineSetCtx(argc, argv, i, Ctx, CTX_RECURSE,0);
	ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-c")==0)
{
	Ctx->Action = ACT_CHECK_LIST;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cf")==0)
{
	Ctx->Action = ACT_CHECK_LIST;
	ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
}
else if (strcmp(argv[i],"-s")==0)
{
	Ctx->Action = ACT_SIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-sign")==0)
{
	Ctx->Action = ACT_SIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cs")==0)
{
	Ctx->Action = ACT_CHECKSIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-checksign")==0)
{
	Ctx->Action = ACT_CHECKSIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-m")==0)
{
	Ctx->Action = ACT_FINDMATCHES;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-lm")==0)
{
	Ctx->Action = ACT_LOADMATCHES;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-dups")==0)
{
	Ctx->Action = ACT_FINDDUPLICATES;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-B")==0)
{
	Ctx->Action = ACT_BACKUP;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cB")==0)
{
	Ctx->Action = ACT_CHECKBACKUP;
	strcpy(argv[i],"");
}
else if ((strcmp(argv[i],"-dir")==0) || (strcmp(argv[i],"-dirmode")==0))
{
	Ctx->Action = ACT_HASHDIR;
	Ctx->Flags |= CTX_RECURSE;
	strcpy(argv[i],"");
}
else if ((strcmp(argv[i],"-hook")==0) || (strcmp(argv[i],"-h")==0))
{
	strcpy(argv[i],"");
	i++;
	if ((i < argc) && StrLen(argv[i]))
	{
	DiffHook=CopyStr(DiffHook,argv[i]);
	strcpy(argv[i],"");
	}
	else 
	{
		printf("ERROR: No hook function supplied to -h/-hook switch (are you looking for help? Try --help or -?)\n");
		exit(1);
	}
}
else if (strcmp(argv[i],"-cgi")==0)
{
	Ctx->Action = ACT_CGI;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-md5")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "md5",Ctx->Vars);
else if (strcmp(argv[i],"-sha")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha1")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha256",Ctx->Vars);
else if (strcmp(argv[i],"-sha512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha512",Ctx->Vars);
else if (strcmp(argv[i],"-whirl")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-whirlpool")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-jh-224")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh-256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh-384")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh-512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh224")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh384")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-type")==0) 
{
	strcpy(argv[i],"");
	i++;
	ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", argv[i],Ctx->Vars);
}
//else if (strcmp(argv[i],"-crc32")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "crc32",Ctx->Vars);
else if (strcmp(argv[i],"-8")==0)  CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_OCTAL);
else if (strcmp(argv[i],"-10")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_DECIMAL);
else if (strcmp(argv[i],"-16")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_HEX);
else if (strcmp(argv[i],"-H")==0)  CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_HEXUPPER);
else if (strcmp(argv[i],"-HEX")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_HEXUPPER);
else if (strcmp(argv[i],"-64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_BASE64);
else if (strcmp(argv[i],"-base64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_BASE64);
else if (strcmp(argv[i],"-i64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_IBASE64);
else if (strcmp(argv[i],"-p64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_PBASE64);
else if (strcmp(argv[i],"-x64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_XXENC);
else if (strcmp(argv[i],"-u64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_UUENC);
else if (strcmp(argv[i],"-g64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_CRYPT);
else if (strcmp(argv[i],"-a85")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_ASCII85);
else if (strcmp(argv[i],"-z85")==0) CommandLineSetCtx(argc, argv, i, Ctx,  0, ENCODE_Z85);
else if (strcmp(argv[i],"-d")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_DEREFERENCE,0);
else if (strcmp(argv[i],"-X")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_EXES,0);
else if (strcmp(argv[i],"-exe")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_EXES,0);
else if (strcmp(argv[i],"-r")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_RECURSE,0);
else if (strcmp(argv[i],"-fs")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_ONE_FS,0);
else if (strcmp(argv[i],"-n")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, 0, "Output:Length", "",Ctx->Vars);
else if (strcmp(argv[i],"-hmac")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, FLAG_HMAC, "EncryptionKey", "",Ctx->Vars);
else if (strcmp(argv[i],"-idfile")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, 0,  "SshIdFile", "",Ctx->Vars);
else if (strcmp(argv[i],"-f")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_FROM_LISTFILE, 0, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-i")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_INCLUDE,0);
else if (strcmp(argv[i],"-x")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_EXCLUDE,0);
else if (strcmp(argv[i],"-devmode")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_DEVMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-lines")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rawlines")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-hide-input")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_HIDE_INPUT, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-star-input")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_STAR_INPUT, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rl")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-xattr")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_XATTR, 0, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-txattr")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_TXATTR, 0, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-cache")==0) CommandLineSetCtx(argc, argv, i, Ctx,   CTX_XATTR_CACHE,0);
else if (strcmp(argv[i],"-strict")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-color")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_COLOR, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-S")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-net")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_NET, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-memcached")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE|CMDLINE_MEMCACHED, 0, "Memcached:Server", "",Ctx->Vars);
else if (strcmp(argv[i],"-mcd")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE| CMDLINE_MEMCACHED, 0, "Memcached:Server", "",Ctx->Vars);
else if (strcmp(argv[i],"-xsel")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_XSELECT, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-v")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_VERBOSE, "", "",Ctx->Vars);
else if (
					(strcmp(argv[i],"-t")==0) ||
					(strcmp(argv[i],"-trad")==0)
				) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_TRAD_OUTPUT, "", "",Ctx->Vars);
else if (
					(strcmp(argv[i],"-bsd")==0) ||
					(strcmp(argv[i],"-tag")==0) ||
					(strcmp(argv[i],"--tag")==0)
				) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_BSD_OUTPUT, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-u")==0) CommandLineHandleUpdate(argc, argv, i, Ctx);
else if (strcmp(argv[i],"-attrs")==0) 
{
	strcpy(argv[i],"");
	i++;
	if ((i < argc) && StrLen(argv[i]))
	{
		SetupXAttrList(argv[i]);
		strcpy(argv[i],"");
	}
	else printf("ERROR: No list of attribute names given to -attrs argument\n");
}


}

//The rest of this function finalizes setup based on what we parsed over all the command line


//if we're reading from a list file, then...
if ((ParseFlags & CMDLINE_FROM_LISTFILE))
{
	//... set appropriate action type
	if (Ctx->Action==ACT_HASH) Ctx->Action=ACT_HASH_LISTFILE;

	//find list file name on command line (will still be untouched as it wasn't
	//recognized as a flag in the above processing
	for (i=1; i < argc; i++)
	{
		if (StrLen(argv[i]))
		{
			Ctx->ListPath=CopyStr(Ctx->ListPath, argv[i]);
			strcpy(argv[i],"");
			break;
		}
	}
}




//if -hmac set, then upgrade hash type to the hmac version
if (Flags & FLAG_HMAC)
{
  Ctx->HashType=MCopyStr(Ctx->HashType,"hmac-",GetVar(Ctx->Vars,"HashType"),NULL);
  HMACSetup(Ctx);
}
else Ctx->HashType=CopyStr(Ctx->HashType,GetVar(Ctx->Vars,"HashType"));

switch (Ctx->Action)
{
case ACT_CHECK:
case ACT_CHECK_LIST:
	if (ParseFlags & CMDLINE_XATTR) Ctx->Action=ACT_CHECK_XATTR;
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Action=ACT_CHECK_MEMCACHED;
break;

case ACT_HASH:
case ACT_HASH_LISTFILE:
	if (ParseFlags & CMDLINE_XATTR) Ctx->Flags |= CTX_STORE_XATTR;
	if (ParseFlags & CMDLINE_TXATTR) Ctx->Flags |= CTX_STORE_XATTR | CTX_XATTR_ROOT;
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Flags |= CTX_STORE_MEMCACHED;
break;

case ACT_FINDMATCHES:
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Action=ACT_FINDMATCHES_MEMCACHED;
break;


}

if (Ctx->Encoding==0) Ctx->Encoding=ENCODE_HEX;

//if no path given, then force to '-' for 'standard in'
ptr=GetVar(Ctx->Vars,"Path");
if (! StrLen(ptr)) SetVar(Ctx->Vars,"Path","-");

DestroyString(Tempstr);
return(Ctx);
}



void CommandLinePrintUsage()
{
printf("Hashrat: version %s\n",VERSION);
printf("Author: Colum Paget\n");
printf("Email: colums.projects@gmail.com\n");
printf("Blog:  http://idratherhack.blogspot.com\n");
printf("Credits:\n");
printf("	Thanks for bug reports/advice to: Stephan Hegel, Michael Shigorin <mike@altlinux.org> and Joao Eriberto Mota Filho <eriberto@debian.org>\n");
printf("	Thanks to the people who invented the hash functions!\n	MD5: Ronald Rivest\n	Whirlpool: Vincent Rijmen, Paulo S. L. M. Barreto\n	JH: Hongjun Wu\n	SHA: The NSA (thanks, but please stop reading my email. It's kinda creepy.).\n\n"); 

printf("	Special thanks to Professor Hongjun Wu for taking the time to confirm that his JH algorithm is free for use in GPL programs.\n");

printf("	Special, special thanks to Joao Eriberto Mota Filho for doing a LOT of work to make hashrat debian ready!\n");

printf("\n");
printf("Usage:\n    hashrat [options] [path to hash]...\n");
printf("\n    hashrat -c [options] [input file of hashes]...\n\n");

printf("Options:\n");
printf("  %-15s %s\n","--help", "Print this help");
printf("  %-15s %s\n","-help", "Print this help");
printf("  %-15s %s\n","-?", "Print this help");
printf("  %-15s %s\n","--version", "Print program version");
printf("  %-15s %s\n","-version", "Print program version");
printf("  %-15s %s\n","-md5", "Use md5 hash algorithmn");
printf("  %-15s %s\n","-sha1", "Use sha1 hash algorithmn");
printf("  %-15s %s\n","-sha256", "Use sha256 hash algorithmn");
printf("  %-15s %s\n","-sha512", "Use sha512 hash algorithmn");
printf("  %-15s %s\n","-whirl", "Use whirlpool hash algorithmn");
printf("  %-15s %s\n","-whirlpool", "Use whirlpool hash algorithmn");
printf("  %-15s %s\n","-jh224", "Use jh-224 hash algorithmn");
printf("  %-15s %s\n","-jh256", "Use jh-256 hash algorithmn");
printf("  %-15s %s\n","-jh384", "Use jh-384 hash algorithmn");
printf("  %-15s %s\n","-jh512", "Use jh-512 hash algorithmn");
printf("  %-15s %s\n","-hmac", "HMAC using specified hash algorithm");
printf("  %-15s %s\n","-8", "Encode with octal instead of hex");
printf("  %-15s %s\n","-10", "Encode with decimal instead of hex");
printf("  %-15s %s\n","-H", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-HEX", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-64", "Encode with base64 instead of hex");
printf("  %-15s %s\n","-base64", "Encode with base64 instead of hex");
printf("  %-15s %s\n","-i64", "Encode with base64 with rearranged characters");
printf("  %-15s %s\n","-p64", "Encode with base64 with a-z,A-Z and _-, for best compatibility with 'allowed characters' in websites.");
printf("  %-15s %s\n","-x64", "Encode with XXencode style base64.");
printf("  %-15s %s\n","-u64", "Encode with UUencode style base64.");
printf("  %-15s %s\n","-g64", "Encode with GEDCOM style base64.");
printf("  %-15s %s\n","-a85", "Encode with ASCII85.");
printf("  %-15s %s\n","-z85", "Encode with ZEROMQ variant of ASCII85.");
printf("  %-15s %s\n","-t", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-trad", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-bsd", "Output hashes in bsdsum format");
printf("  %-15s %s\n","-tag", "Output hashes in bsdsum format");
printf("  %-15s %s\n","--tag", "Output hashes in bsdsum format");
printf("  %-15s %s\n","-r", "Recurse into directories when hashing files");
printf("  %-15s %s\n","-f <listfile>", "Hash files listed in <listfile>");
printf("  %-15s %s\n","-i <pattern>", "Only hash items matching <pattern>");
printf("  %-15s %s\n","-x <pattern>", "Exclude items matching <pattern>");
printf("  %-15s %s\n","-n <length>", "Truncate hashes to <length> bytes");
printf("  %-15s %s\n","-c", "CHECK hashes against list from file (or stdin)");
printf("  %-15s %s\n","-cf", "CHECK hashes against list but only show failures");
printf("  %-15s %s\n","-C <dir>", "Recursively CHECK directory against list of files on stdin ");
printf("  %-15s %s\n","-Cf <dir>", "Recursively CHECK directory against list but only show failures");
printf("  %-15s %s\n","-m", "MATCH files from a list read from stdin.");
printf("  %-15s %s\n","-lm", "Read hashes from stdin, upload them to a memcached server (requires the -memcached option).");
printf("  %-15s %s\n","-X", "In CHECK or MATCH mode only examine executable files.");
printf("  %-15s %s\n","-exec", "In CHECK or MATCH mode only examine executable files.");
printf("  %-15s %s\n","-dups", "Search for duplicate files.");
printf("  %-15s %s\n","-memcached <server>", "Specify memcached server. (Overrides reading list from stdin if used with -m, -c or -cf).");
printf("  %-15s %s\n","-mcd <server>", "Specify memcached server. (Overrides reading list from stdin if used with -m, -c or -cf).");
printf("  %-15s %s\n","-h <script>", "Script to run when a file fails CHECK mode, or is found in MATCH mode.");
printf("  %-15s %s\n","-hook <script>", "Script to run when a file fails CHECK mode, or is found in FIND mode");
printf("  %-15s %s\n","-color", "Use ANSI color codes on output when checking hashes.");
printf("  %-15s %s\n","-strict", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-S", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-d","dereference (follow) symlinks"); 
printf("  %-15s %s\n","-fs", "Stay on one file system");
printf("  %-15s %s\n","-dir", "DirMode: Read all files in directory and create one hash for them!");
printf("  %-15s %s\n","-dirmode", "DirMode: Read all files in directory and create one hash for them!");
printf("  %-15s %s\n","-devmode", "DevMode: read from a file EVEN OF IT'S A DEVNODE");
printf("  %-15s %s\n","-lines", "Read lines from stdin and hash each line independantly.");
printf("  %-15s %s\n","-rawlines", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-rl", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-cgi", "Run in HTTP CGI mode");
printf("  %-15s %s\n","-net", "Treat 'file' arguments as either ssh or http URLs, and pull files over the network and then hash them (Allows hashing of files on remote machines).");
printf("  %-15s %s\n","", "URLs are in the format ssh://[username]:[password]@[host]:[port] or http://[username]:[password]@[host]:[port]..");
printf("  %-15s %s\n","-idfile <path>", "Path to an ssh private key file to use to authenticate INSTEAD OF A PASSWORD when pulling files via ssh.");
printf("  %-15s %s\n","-xattr", "Use eXtended file ATTRibutes. In hash mode, store hashes in the file attributes, in check mode compare against hashes stored in file attributes.");
printf("  %-15s %s\n","-txattr", "Use TRUSTED eXtended file ATTRibutes. In hash mode, store hashes in 'trusted' file attributes. 'trusted' attributes can only be read and written by root. Under freebsd this menas SYSTEM attributes.");
printf("  %-15s %s\n","-attrs", "comma-separated list of filesystem attribute names to be set to the value of the hash.");
printf("  %-15s %s\n","-cache", "Use hashes stored in 'user' xattr if they're younger than the mtime of the file. This speeds up outputting hashes.");
printf("  %-15s %s\n","-u <types>", "Update. In checking mode, update hashes for the files as you go. <types> is a comma-separated list of things to update, which can be 'xattr' 'memcached' or a file name. This will update these targets with the hash that was found at the time of checking.");
printf("  %-15s %s\n","-hide-input", "When reading data from stdin in linemode, set the terminal to not echo characters, thus hiding typed input.");
printf("  %-15s %s\n","-star-input", "When reading data from stdin in linemode replace characters with stars.");
printf("  %-15s %s\n","-xsel", "Update X11 clipboard and primary selections to the current hash. This works using Xterm command sequences. The xterm resource 'allowWindowOps' must be set to 'true' for this to work.");

printf("\n\nHashrat can also detect if it's being run under any of the following names (e.g., via symlinks)\n\n");
printf("  %-15s %s\n","md5sum","run with '-trad -md5'");
printf("  %-15s %s\n","shasum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha1sum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha256sum","run with '-trad -sha256'");
printf("  %-15s %s\n","sha512sum","run with '-trad -sha512'");
printf("  %-15s %s\n","jh224sum","run with '-trad -jh224'");
printf("  %-15s %s\n","jh256sum","run with '-trad -jh256'");
printf("  %-15s %s\n","jh384sum","run with '-trad -jh384'");
printf("  %-15s %s\n","jh512sum","run with '-trad -jh512'");
printf("  %-15s %s\n","whirlpoolsum","run with '-trad -whirl'");
printf("  %-15s %s\n","hashrat.cgi","run in web-enabled 'cgi mode'");
}


