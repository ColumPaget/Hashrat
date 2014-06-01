#include "command-line-args.h"



//this function is called when a command-line switch has been recognized. It's told which flags to set
//and whether the switch takes a string argument, which it then reads and stores in the variable list
//with the name supplied in 'VarName'. It blanks out anything it reads, so that only unrecognized 
//arguments remain, which are treated as filenames for processing

int CommandLineHandleArg(int argc, char *argv[], int pos, int SetFlags, char *VarName, char *VarValue, ListNode *Vars)
{
	Flags |= SetFlags;
	
	if (SetFlags & FLAG_ARG_NAMEVALUE)
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
		if (SetFlags & FLAG_INCLUDE) AddIncludeExclude(FLAG_INCLUDE, argv[pos]);
		else if (SetFlags & FLAG_EXCLUDE) AddIncludeExclude(FLAG_EXCLUDE, argv[pos]);
		else SetVar(Vars,VarName,argv[pos]);
	}
	}
	else if (StrLen(VarName)) SetVar(Vars,VarName,VarValue);

	if (argc > 0) strcpy(argv[pos],"");
return(pos);
}



//this is the main parsing function that goes through the command-line args
HashratCtx *CommandLineParseArgs(int argc,char *argv[])
{
int i;
char *ptr, *Tempstr=NULL;
HashratCtx *Ctx;

//You never know when you're going to be run with  no args at all (say, out of inetd)

//Setup default context
Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
Ctx->Encoding=ENCODE_HEX;
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
CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "md5",Ctx->Vars);
if (
		(strcmp(ptr,"sha1sum")==0) ||
		(strcmp(ptr,"shasum")==0) 
	) 
CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha1",Ctx->Vars);
if (strcmp(ptr,"sha256sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha256",Ctx->Vars);
if (strcmp(ptr,"sha512sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha512",Ctx->Vars);
if (strcmp(ptr,"whirlpoolsum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "whirlpool",Ctx->Vars);
if (strcmp(ptr,"jh224sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "jh-224",Ctx->Vars);
if (strcmp(ptr,"jh256sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "jh-256",Ctx->Vars);
if (strcmp(ptr,"jh384sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "jh-385",Ctx->Vars);
if (strcmp(ptr,"jh512sum")==0) CommandLineHandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "jh-512",Ctx->Vars);


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
else if (strcmp(argv[i],"-c")==0)
{
	Ctx->Action = ACT_CHECK;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cf")==0)
{
	Ctx->Action = ACT_CHECK;
	CommandLineHandleArg(argc, argv, i, FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
}
else if (strcmp(argv[i],"-diff-hook")==0) 
{
	strcpy(argv[i],"");
	i++;
	DiffHook=CopyStr(DiffHook,argv[i]);
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cgi")==0)
{
	Ctx->Action = ACT_CGI;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-md5")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "md5",Ctx->Vars);
else if (strcmp(argv[i],"-sha")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha1")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha256")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "sha256",Ctx->Vars);
else if (strcmp(argv[i],"-sha512")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "sha512",Ctx->Vars);
else if (strcmp(argv[i],"-whirl")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-whirlpool")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-jh-224")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh-256")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh-384")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh-512")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh224")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh256")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh384")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh512")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "jh-512",Ctx->Vars);
//else if (strcmp(argv[i],"-crc32")==0) CommandLineHandleArg(argc, argv, i, 0, "HashType", "crc32",Ctx->Vars);
else if (strcmp(argv[i],"-8")==0) CommandLineHandleArg(argc, argv, i, FLAG_BASE8, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-10")==0) CommandLineHandleArg(argc, argv, i, FLAG_BASE10, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-H")==0) CommandLineHandleArg(argc, argv, i, FLAG_HEXUPPER, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-HEX")==0) CommandLineHandleArg(argc, argv, i, FLAG_HEXUPPER, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-64")==0) CommandLineHandleArg(argc, argv, i, FLAG_BASE64, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-base64")==0) CommandLineHandleArg(argc, argv, i, FLAG_BASE64, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-hmac")==0) CommandLineHandleArg(argc, argv, i, FLAG_HMAC | FLAG_ARG_NAMEVALUE, "EncryptionKey", "",Ctx->Vars);
else if (strcmp(argv[i],"-idfile")==0) CommandLineHandleArg(argc, argv, i, FLAG_ARG_NAMEVALUE, "SshIdFile", "",Ctx->Vars);
else if (strcmp(argv[i],"-r")==0) CommandLineHandleArg(argc, argv, i, FLAG_RECURSE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-f")==0) CommandLineHandleArg(argc, argv, i, FLAG_FROM_LISTFILE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-i")==0) CommandLineHandleArg(argc, argv, i, FLAG_INCLUDE | FLAG_ARG_NAMEVALUE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-x")==0) CommandLineHandleArg(argc, argv, i, FLAG_EXCLUDE | FLAG_ARG_NAMEVALUE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-d")==0) CommandLineHandleArg(argc, argv, i, FLAG_DEREFERENCE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-dirmode")==0) CommandLineHandleArg(argc, argv, i, FLAG_DIRMODE | FLAG_RECURSE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-devmode")==0) CommandLineHandleArg(argc, argv, i, FLAG_DIRMODE | FLAG_DEVMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-lines")==0) CommandLineHandleArg(argc, argv, i, FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-fs")==0) CommandLineHandleArg(argc, argv, i, FLAG_ONE_FS, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-xattr")==0) CommandLineHandleArg(argc, argv, i, FLAG_XATTR, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-strict")==0) CommandLineHandleArg(argc, argv, i, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-S")==0) CommandLineHandleArg(argc, argv, i, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-net")==0) CommandLineHandleArg(argc, argv, i, FLAG_NET, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rl")==0) CommandLineHandleArg(argc, argv, i, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rawlines")==0) CommandLineHandleArg(argc, argv, i, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (
					(strcmp(argv[i],"-t")==0) ||
					(strcmp(argv[i],"-trad")==0)
				) CommandLineHandleArg(argc, argv, i, FLAG_TRAD_OUTPUT, "", "",Ctx->Vars);

else if (strcmp(argv[i],"-attrs")==0) 
{
	strcpy(argv[i],"");
	i++;
	SetupXAttrList(argv[i]);
	strcpy(argv[i],"");
}


}

//The rest of this function finalizes setup based on what we parsed over all the command line


//Set encoding from args
if (Flags & FLAG_BASE8) Ctx->Encoding = ENCODE_OCTAL;
if (Flags & FLAG_BASE10) Ctx->Encoding = ENCODE_DECIMAL;
if (Flags & FLAG_HEXUPPER) Ctx->Encoding = ENCODE_HEXUPPER;
if (Flags & FLAG_BASE64) Ctx->Encoding = ENCODE_BASE64;


//if we're reading from a list file, then...
if ((Flags & FLAG_FROM_LISTFILE))
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
  HMACSetup();
}
else Ctx->HashType=CopyStr(Ctx->HashType,GetVar(Ctx->Vars,"HashType"));


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
printf("Blog:  http://idratherhack.blogspot.com\n\n");
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
printf("  %-15s %s\n","-jh244", "Use jh-244 hash algorithmn");
printf("  %-15s %s\n","-jh256", "Use jh-256 hash algorithmn");
printf("  %-15s %s\n","-jh384", "Use jh-384 hash algorithmn");
printf("  %-15s %s\n","-jh512", "Use jh-512 hash algorithmn");
printf("  %-15s %s\n","-hmac", "HMAC using specified hash algorithm");
printf("  %-15s %s\n","-8", "Encode with octal instead of hex");
printf("  %-15s %s\n","-10", "Encode with decimal instead of hex");
printf("  %-15s %s\n","-H", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-HEX", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-base64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-t", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-trad", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-r", "Recurse into directories when hashing files");
printf("  %-15s %s\n","-f <listfile>", "Hash files listed in <listfile>");
printf("  %-15s %s\n","-i <pattern>", "Only hash items matching <pattern>");
printf("  %-15s %s\n","-x <pattern>", "Exclude items matching <pattern>");
printf("  %-15s %s\n","-c", "Check hashes against list from file (or stdin)");
printf("  %-15s %s\n","-cf", "Check hashes but only show failures");
printf("  %-15s %s\n","-strict", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-S", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-d","dereference (follow) symlinks"); 
printf("  %-15s %s\n","-dirmode", "DirMode: Read all files in directory and create one hash for them!");
printf("  %-15s %s\n","-devmode", "DevMode: read from a file EVEN OF IT'S A DEVNODE");
printf("  %-15s %s\n","-fs", "Stay one one file system");
printf("  %-15s %s\n","-lines", "Read lines from stdin and hash each line independantly.");
printf("  %-15s %s\n","-rawlines", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-rl", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-cgi", "Run in HTTP CGI mode");
printf("  %-15s %s\n","-net", "Treat 'file' arguments as either ssh or http URLs, and pull files over the network and then hash them (Allows hashing of files on remote machines).");
printf("  %-15s %s\n","", "URLs are in the format ssh://[username]:[password]@[host]:[port] or http://[username]:[password]@[host]:[port]..");
printf("  %-15s %s\n","-idfile <path>", "Path to an ssh private key file to use to authenticate INSTEAD OF A PASSWORD when pulling files via ssh.");


/*
else if (strcmp(argv[i],"-xattr")==0) CommandLineHandleArg(argc, argv, i, FLAG_XATTR, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-attrs")==0) 
*/


printf("\n\nHashrat can also detect if it's being run under any of the following names (e.g., via symlinks)\n\n");
printf("  %-15s %s\n","md5sum","run with '-trad -md5'");
printf("  %-15s %s\n","shasum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha1sum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha256sum","run with '-trad -sha256'");
printf("  %-15s %s\n","sha512sum","run with '-trad -sha512'");
printf("  %-15s %s\n","jh244sum","run with '-trad -jh244'");
printf("  %-15s %s\n","jh256sum","run with '-trad -jh256'");
printf("  %-15s %s\n","jh384sum","run with '-trad -jh384'");
printf("  %-15s %s\n","jh512sum","run with '-trad -jh512'");
printf("  %-15s %s\n","whirlpoolsum","run with '-trad -whirl'");
printf("  %-15s %s\n","hashrat.cgi","run in web-enabled 'cgi mode'");
}


