#include "command-line-args.h"
#include "include-exclude.h"
#include "xattr.h"

CMDLINE CmdLine;



void HMACSetup(HashratCtx *Ctx)
{
char *Tempstr=NULL;
const char *ptr;
STREAM *S;

  ptr=GetVar(Ctx->Vars,"EncryptionKey");
  if (! StrValid(ptr))
  {
    if (isatty(0))
    {
      write(1, "Enter HMAC Key: ",16);

      S=STREAMFromFD(0);
      Tempstr=STREAMReadLine(Tempstr,S);
      StripTrailingWhitespace(Tempstr);
      SetVar(Ctx->Vars,"EncryptionKey",Tempstr);
      ptr=Tempstr;
      STREAMDestroy(S);
    }

    //By now we must have an encryption key!
    if (! StrValid(ptr))
    {
      write(1,"ERROR: No HMAC Key given!\n",27);
      exit(2);
    }
  }
Destroy(Tempstr);
}


//this function is called when a command-line switch has been recognized. It's told which flags to set
//and whether the switch takes a string argument, which it then reads and stores in the variable list
//with the name supplied in 'VarName'. 

void CommandLineHandleArg(HashratCtx *Ctx, int SetFlags, const char *VarName, const char *VarValue, ListNode *Vars)
{
const char *arg, *next;

	Flags |= SetFlags;
	
	arg=CommandLineCurr(&CmdLine);
	if (SetFlags & FLAG_NEXTARG)
	{
		next=CommandLineNext(&CmdLine);
		if (next==NULL)
		{
			printf("ERROR: The %s option requires an argument.\n",arg);
			exit(1);
		}
		else 
		{
			SetVar(Vars,VarName,next);
			if (strcmp(VarName, "Output:Length")==0) 
			{
				Ctx->OutputLength=atoi(next);
				Ctx->Flags |= CTX_REFORMAT;
			}
			if (strcmp(VarName, "Segment:Length")==0) 
			{
				Ctx->Flags |= CTX_REFORMAT;
				Ctx->SegmentLength=atoi(next);
			}
			if (strcmp(VarName, "OutputPrefix")==0) Ctx->Flags |= CTX_REFORMAT;
		}
	}
	else if (StrValid(VarName)) SetVar(Vars,VarName,VarValue);
}



void CommandLineSetCtx(HashratCtx *Ctx, int Flag, int Encoding)
{
if (Encoding > 0) Ctx->Encoding=Encoding;

switch (Flag)
{
case CTX_INCLUDE:
case CTX_EXCLUDE:
case CTX_MTIME:
case CTX_MMIN:
case CTX_MYEAR:
 	IncludeExcludeAdd(Ctx, Flag, CommandLineNext(&CmdLine));
break;

default:
	Ctx->Flags |= Flag;
break;
}

}




void CommandLineHandleUpdate(HashratCtx *Ctx)
{
char *Token=NULL;
const char *ptr;

Flags |= FLAG_UPDATE;
ptr=CommandLineNext(&CmdLine);
ptr=GetToken(ptr,",",&Token,0);
while (ptr)
{
	if (strcasecmp(Token, "stderr")==0) Ctx->Aux=STREAMFromFD(2);
	else if (strcasecmp(Token, "xattr")==0) Ctx->Flags |= CTX_STORE_XATTR;
	else if (strcasecmp(Token, "txattr")==0) Ctx->Flags |= CTX_STORE_XATTR | CTX_XATTR_ROOT;
	else if (strcasecmp(Token, "memcached")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (strcasecmp(Token, "mcd")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (! Ctx->Aux) Ctx->Aux=STREAMFileOpen(Token, SF_WRONLY | SF_CREAT | SF_TRUNC);

ptr=GetToken(ptr,",",&Token,0);
}

Destroy(Token);
}


HashratCtx *CommandLineParseArg0()
{
HashratCtx *Ctx;
const char *ptr;

//Setup default context
Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
Ctx->Action=ACT_HASH;

//something went wrong with cmdline parsing
ptr=CommandLineCurr(&CmdLine);
if (! ptr) 
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}

Ctx->Vars=ListCreate();
Ctx->Out=STREAMFromFD(1);
SetVar(Ctx->Vars,"HashType","md5");


//argv[0] might be full path to the program, or just its name
ptr=GetBasename(ptr);

//if the program name is something other than 'hashrat', then we're being used as a drop-in
//replacement for another program. Change flags/behavior accordingly
if (strcmp(ptr,"md5sum")==0) 
CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "md5",Ctx->Vars);
if (
		(strcmp(ptr,"sha1sum")==0) ||
		(strcmp(ptr,"shasum")==0) 
	) 
CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "sha1",Ctx->Vars);
if (strcmp(ptr,"sha256sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "sha256",Ctx->Vars);
if (strcmp(ptr,"sha512sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "sha512",Ctx->Vars);
if (strcmp(ptr,"whirlpoolsum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "whirlpool",Ctx->Vars);
if (strcmp(ptr,"jh224sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "jh-224",Ctx->Vars);
if (strcmp(ptr,"jh256sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "jh-256",Ctx->Vars);
if (strcmp(ptr,"jh384sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "jh-385",Ctx->Vars);
if (strcmp(ptr,"jh512sum")==0) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "HashType", "jh-512",Ctx->Vars);


if (strcmp(ptr,"hashrat.cgi")==0) Ctx->Action=ACT_CGI;

return(Ctx);
}


//this is the main parsing function that goes through the command-line args
HashratCtx *CommandLineParseArgs(int argc, char *argv[])
{
char *Tempstr=NULL;
const char *ptr, *arg;
HashratCtx *Ctx;

CommandLineParserInit(&CmdLine, argc, argv);
Ctx=CommandLineParseArg0();
Ctx->SegmentChar='-';

arg=CommandLineNext(&CmdLine);
while (arg)
{
if (
		(strcmp(arg,"-V")==0) ||
		(strcmp(arg,"--version")==0) ||
		(strcmp(arg,"-version")==0)
	)
{
	printf("version: %s\n",VERSION);
	return(NULL);
}
else if (
		(strcmp(arg,"--help")==0) ||
		(strcmp(arg,"-help")==0) ||
		(strcmp(arg,"-?")==0)
	)
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}
else if (strcmp(arg,"-C")==0)
{
	Ctx->Action = ACT_CHECK;
	CommandLineSetCtx(Ctx, CTX_RECURSE,0);
}
else if (strcmp(arg,"-Cf")==0)
{
	Ctx->Action = ACT_CHECK;
	CommandLineSetCtx(Ctx, CTX_RECURSE,0);
	CommandLineHandleArg(Ctx,FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
}
else if (strcmp(arg,"-cf")==0)
{
	Ctx->Action = ACT_CHECK_LIST;
	CommandLineHandleArg(Ctx,FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
}
else if (strcmp(arg,"-c")==0) Ctx->Action = ACT_CHECK_LIST;
else if (strcmp(arg,"-s")==0) Ctx->Action = ACT_SIGN;
else if (strcmp(arg,"-sign")==0) Ctx->Action = ACT_SIGN;
else if (strcmp(arg,"-cs")==0) Ctx->Action = ACT_CHECKSIGN;
else if (strcmp(arg,"-checksign")==0) Ctx->Action = ACT_CHECKSIGN;
else if (strcmp(arg,"-m")==0) Ctx->Action = ACT_FINDMATCHES;
else if (strcmp(arg,"-lm")==0) Ctx->Action = ACT_LOADMATCHES;
else if (strcmp(arg,"-dups")==0) Ctx->Action = ACT_FINDDUPLICATES;
else if (strcmp(arg,"-B")==0) Ctx->Action = ACT_BACKUP;
else if (strcmp(arg,"-cB")==0) Ctx->Action = ACT_CHECKBACKUP;
else if (strcmp(arg,"-cgi")==0) Ctx->Action = ACT_CGI;
else if (strcmp(arg,"-md5")==0) CommandLineHandleArg(Ctx,0, "HashType", "md5",Ctx->Vars);
else if (strcmp(arg,"-sha")==0) CommandLineHandleArg(Ctx,0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(arg,"-sha1")==0) CommandLineHandleArg(Ctx,0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(arg,"-sha256")==0) CommandLineHandleArg(Ctx,0, "HashType", "sha256",Ctx->Vars);
else if (strcmp(arg,"-sha512")==0) CommandLineHandleArg(Ctx,0, "HashType", "sha512",Ctx->Vars);
else if (strcmp(arg,"-whirl")==0) CommandLineHandleArg(Ctx,0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(arg,"-whirlpool")==0) CommandLineHandleArg(Ctx,0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(arg,"-jh-224")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(arg,"-jh-256")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(arg,"-jh-384")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(arg,"-jh-512")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(arg,"-jh224")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(arg,"-jh256")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(arg,"-jh384")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(arg,"-jh512")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(arg,"-jh")==0) CommandLineHandleArg(Ctx,0, "HashType", "jh-512",Ctx->Vars);
//else if (strcmp(argv[i],"-crc32")==0) CommandLineHandleArg(Ctx,0, "HashType", "crc32",Ctx->Vars);
else if (strcmp(arg,"-8")==0)  CommandLineSetCtx(Ctx,  0, ENCODE_OCTAL);
else if (strcmp(arg,"-10")==0) CommandLineSetCtx(Ctx,  0, ENCODE_DECIMAL);
else if (strcmp(arg,"-16")==0) CommandLineSetCtx(Ctx,  0, ENCODE_HEX);
else if (strcmp(arg,"-H")==0)  CommandLineSetCtx(Ctx,  0, ENCODE_HEXUPPER);
else if (strcmp(arg,"-HEX")==0) CommandLineSetCtx(Ctx,  0, ENCODE_HEXUPPER);
else if (strcmp(arg,"-64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_BASE64);
else if (strcmp(arg,"-base64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_BASE64);
else if (strcmp(arg,"-i64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_IBASE64);
else if (strcmp(arg,"-p64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_PBASE64);
else if (strcmp(arg,"-r64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_RBASE64);
else if (strcmp(arg,"-rfc4648")==0) CommandLineSetCtx(Ctx,  0, ENCODE_RBASE64);
else if (strcmp(arg,"-x64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_XXENC);
else if (strcmp(arg,"-u64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_UUENC);
else if (strcmp(arg,"-g64")==0) CommandLineSetCtx(Ctx,  0, ENCODE_CRYPT);
else if (strcmp(arg,"-a85")==0) CommandLineSetCtx(Ctx,  0, ENCODE_ASCII85);
else if (strcmp(arg,"-z85")==0) CommandLineSetCtx(Ctx,  0, ENCODE_Z85);
else if (strcmp(arg,"-d")==0) CommandLineSetCtx(Ctx, CTX_DEREFERENCE,0);
else if (strcmp(arg,"-exe")==0) CommandLineSetCtx(Ctx, CTX_EXES,0);
else if (strcmp(arg,"-exec")==0) CommandLineSetCtx(Ctx, CTX_EXES,0);
else if (strcmp(arg,"-r")==0) CommandLineSetCtx(Ctx, CTX_RECURSE,0);
else if (strcmp(arg,"-hid")==0) CommandLineSetCtx(Ctx, CTX_HIDDEN,0);
else if (strcmp(arg,"-hidden")==0) CommandLineSetCtx(Ctx, CTX_HIDDEN,0);
else if (strcmp(arg,"-fs")==0) CommandLineSetCtx(Ctx, CTX_ONE_FS,0);
else if (strcmp(arg,"-n")==0) CommandLineHandleArg(Ctx,FLAG_NEXTARG, "Output:Length", "",Ctx->Vars);
else if (strcmp(arg,"-segment")==0) CommandLineHandleArg(Ctx,FLAG_NEXTARG, "Segment:Length", "",Ctx->Vars);
else if (strcmp(arg,"-hmac")==0) CommandLineHandleArg(Ctx,FLAG_NEXTARG | FLAG_HMAC, "EncryptionKey", "",Ctx->Vars);
else if (strcmp(arg,"-idfile")==0) CommandLineHandleArg( Ctx,FLAG_NEXTARG,  "SshIdFile", "", Ctx->Vars);
else if (strcmp(arg,"-f")==0) CommandLineHandleArg( Ctx,FLAG_NEXTARG, "ItemsListSource", "", Ctx->Vars);
else if (strcmp(arg,"-iprefix")==0) CommandLineHandleArg( Ctx, FLAG_NEXTARG, "InputPrefix", "", Ctx->Vars);
else if (strcmp(arg,"-oprefix")==0) CommandLineHandleArg( Ctx, FLAG_NEXTARG, "OutputPrefix", "", Ctx->Vars);
else if (strcmp(arg,"-i")==0) CommandLineSetCtx(Ctx, CTX_INCLUDE,0);
else if (strcmp(arg,"-name")==0) CommandLineSetCtx(Ctx, CTX_INCLUDE,0);
else if (strcmp(arg,"-mtime")==0) CommandLineSetCtx(Ctx, CTX_MTIME,0);
else if (strcmp(arg,"-mmin")==0) CommandLineSetCtx(Ctx, CTX_MMIN,0);
else if (strcmp(arg,"-myear")==0) CommandLineSetCtx(Ctx, CTX_MYEAR,0);
else if (strcmp(arg,"-x")==0) CommandLineSetCtx(Ctx, CTX_EXCLUDE,0);
else if (strcmp(arg,"-X")==0) IncludeExcludeLoadExcludesFromFile(Ctx, CommandLineNext(&CmdLine));
else if (strcmp(arg,"-devmode")==0) CommandLineHandleArg(Ctx,FLAG_DEVMODE, "", "",Ctx->Vars);
else if (strcmp(arg,"-lines")==0) CommandLineHandleArg(Ctx,FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(arg,"-rawlines")==0) CommandLineHandleArg(Ctx,FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(arg,"-hide-input")==0) CommandLineHandleArg(Ctx,FLAG_HIDE_INPUT, "", "",Ctx->Vars);
else if (strcmp(arg,"-star-input")==0) CommandLineHandleArg(Ctx,FLAG_STAR_INPUT, "", "",Ctx->Vars);
else if (strcmp(arg,"-rl")==0) CommandLineHandleArg(Ctx,FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(arg,"-xattr")==0) CommandLineHandleArg(Ctx,FLAG_XATTR, "", "",Ctx->Vars);
else if (strcmp(arg,"-txattr")==0) CommandLineHandleArg(Ctx,FLAG_TXATTR, "", "",Ctx->Vars);
else if (strcmp(arg,"-cache")==0) CommandLineSetCtx(Ctx, CTX_XATTR_CACHE,0);
else if (strcmp(arg,"-strict")==0) CommandLineHandleArg(Ctx,FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(arg,"-color")==0) CommandLineHandleArg(Ctx,FLAG_COLOR, "", "",Ctx->Vars);
else if (strcmp(arg,"-S")==0) CommandLineHandleArg(Ctx,FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(arg,"-net")==0) CommandLineHandleArg(Ctx,FLAG_NET, "", "",Ctx->Vars);
else if (strcmp(arg,"-memcached")==0) CommandLineHandleArg(Ctx,FLAG_NEXTARG | FLAG_MEMCACHED, "Memcached:Server", "",Ctx->Vars);
else if (strcmp(arg,"-mcd")==0) CommandLineHandleArg(Ctx,FLAG_NEXTARG | FLAG_MEMCACHED, "Memcached:Server", "",Ctx->Vars);
else if (strcmp(arg,"-xsel")==0) CommandLineHandleArg(Ctx,FLAG_XSELECT, "", "",Ctx->Vars);
else if (strcmp(arg,"-v")==0) CommandLineHandleArg(Ctx,FLAG_VERBOSE, "", "",Ctx->Vars);
else if ((strcmp(arg,"-dir")==0) || (strcmp(arg,"-dirmode")==0))
{
	Ctx->Action = ACT_HASHDIR;
	Ctx->Flags |= CTX_RECURSE;
}
else if ((strcmp(arg,"-hook")==0) || (strcmp(arg,"-h")==0))
{
	arg=CommandLineNext(&CmdLine);
	if (StrValid(arg)) DiffHook=CopyStr(DiffHook,arg);
	else 
	{
		printf("ERROR: No hook function supplied to -h/-hook switch (are you looking for help? Try --help or -?)\n");
		exit(1);
	}
}
else if (strcmp(arg,"-type")==0) 
{
	arg=CommandLineNext(&CmdLine);
	CommandLineHandleArg(Ctx,0, "HashType", arg, Ctx->Vars);
}
else if (
					(strcmp(arg,"-t")==0) ||
					(strcmp(arg,"-trad")==0)
				) CommandLineHandleArg(Ctx,FLAG_TRAD_OUTPUT, "", "",Ctx->Vars);
else if (
					(strcmp(arg,"-bsd")==0) ||
					(strcmp(arg,"-tag")==0) ||
					(strcmp(arg,"--tag")==0)
				) CommandLineHandleArg(Ctx,FLAG_BSD_OUTPUT, "", "",Ctx->Vars);
else if (strcmp(arg,"-u")==0) CommandLineHandleUpdate(Ctx);
else if (strcmp(arg,"-attrs")==0) 
{
	arg=CommandLineNext(&CmdLine);
	if (StrValid(arg)) SetupXAttrList(arg);
	else printf("ERROR: No list of attribute names given to -attrs argument\n");
}
else 
{
Tempstr=QuoteCharsInStr(Tempstr, arg, ",");
Ctx->Targets=MCatStr(Ctx->Targets, Tempstr, ",", NULL);
}

arg=CommandLineNext(&CmdLine);
}

//The rest of this function finalizes setup based on what we parsed over all the command line


//if we're reading from a list file, then set appropriate action type
if (StrValid(GetVar(Ctx->Vars,"ItemsListSource"))) if (Ctx->Action==ACT_HASH) Ctx->Action=ACT_HASH_LISTFILE;


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
	if (Flags & FLAG_XATTR) Ctx->Action=ACT_CHECK_XATTR;
	if (Flags & FLAG_MEMCACHED) Ctx->Action=ACT_CHECK_MEMCACHED;
break;

case ACT_HASH:
case ACT_HASH_LISTFILE:
	if (Flags & FLAG_XATTR) Ctx->Flags |= CTX_STORE_XATTR;
	if (Flags & FLAG_TXATTR) Ctx->Flags |= CTX_STORE_XATTR | CTX_XATTR_ROOT;
	if (Flags & FLAG_MEMCACHED) Ctx->Flags |= CTX_STORE_MEMCACHED;
break;

case ACT_FINDMATCHES:
	if (Flags & FLAG_MEMCACHED) Ctx->Action=ACT_FINDMATCHES_MEMCACHED;
break;


}

if (Ctx->Encoding==0) Ctx->Encoding=ENCODE_HEX;

//if no path given, then force to '-' for 'standard in'
ptr=GetVar(Ctx->Vars,"Path");
if (! StrValid(ptr)) SetVar(Ctx->Vars,"Path","-");

Destroy(Tempstr);
return(Ctx);
}



void CommandLinePrintUsage()
{
printf("Hashrat: version %s\n",VERSION);
printf("Author: Colum Paget\n");
printf("Email: colums.projects@gmail.com\n");
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
printf("  %-15s %s\n","-r64", "Encode with base64 with a-z,A-Z and _-, rfc4648 compatible.");
printf("  %-15s %s\n","-rfc4648", "Encode with base64 with a-z,A-Z and _-, rfc4648 compatible.");
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
printf("  %-15s %s\n","-hid", "Show hidden (starting with .) files");
printf("  %-15s %s\n","-hidden", "Show hidden (starting with .) files");
printf("  %-15s %s\n","-f <listfile>", "Hash files listed in <listfile>");
printf("  %-15s %s\n","-i <patterns>", "Only hash items matching a comma-seperated list of shell patterns");
printf("  %-15s %s\n","-x <patterns>", "Exclude items matching a comma-sepearted list of shell patterns");
printf("  %-15s %s\n","-X <file>", "Exclude items matching shell patters stored in <file>");
printf("  %-15s %s\n","-name  <patterns>", "Only hash items matching a comma-seperated list of shell patterns (-name aka 'find')");
printf("  %-15s %s\n","-mtime <days>", "Only hash items <days> old. Has the same format as the find command, e.g. -10 is younger than ten days, +10 is older than ten, and 10 is ten days old");
printf("  %-15s %s\n","-mmin  <mins>", "Only hash items <min> minutes old. Has the same format as the find command, e.g. -10 is younger than ten mins, +10 is older than ten, and 10 is ten mins old");
printf("  %-15s %s\n","-myear <years>", "Only hash items <years> old. Has the same format as the find command, e.g. -10 is younger than ten years, +10 is older than ten, and 10 is ten years old");
printf("  %-15s %s\n","-exec", "In CHECK or MATCH mode only examine executable files.");
printf("  %-15s %s\n","-dups", "Search for duplicate files.");
printf("  %-15s %s\n","-n <length>", "Truncate hashes to <length> bytes");
printf("  %-15s %s\n","-segment <length>", "Break hash up into segments of <length> chars seperated by '-'");
printf("  %-15s %s\n","-c", "CHECK hashes against list from file (or stdin)");
printf("  %-15s %s\n","-cf", "CHECK hashes against list but only show failures");
printf("  %-15s %s\n","-C <dir>", "Recursively CHECK directory against list of files on stdin ");
printf("  %-15s %s\n","-Cf <dir>", "Recursively CHECK directory against list but only show failures");
printf("  %-15s %s\n","-m", "MATCH files from a list read from stdin.");
printf("  %-15s %s\n","-lm", "Read hashes from stdin, upload them to a memcached server (requires the -memcached option).");
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


