#include "proctitle.h"
#define __GNU_SOURCE
#include "errno.h"

/*This is code to change the command-line of a program as visible in ps */

extern char **environ;
char *TitleBuffer=NULL;
int TitleLen=0;


void CopyEnv(const char *Env)
{
char *tmp, *ptr;

	tmp=strdup(Env);
	if (tmp)
	{
		ptr=strchr(tmp,'=');
		if (ptr)
		{
			*ptr='\0';
			ptr++;
		}
		else ptr=NULL;
		setenv(tmp,ptr,1);
		free(tmp);
	}
}

//The command-line args that we've been passed (argv) will occupy a block of contiguous memory that
//contains these args and the environment strings. In order to change the command-line args we isolate
//this block of memory by iterating through all the strings in it, and making copies of them. The
//pointers in 'argv' and 'environ' are then redirected to these copies. Now we can overwrite the whole
//block of memory with our new command-line arguments.
void ProcessTitleCaptureBuffer(char **argv)
{
char **arg, *end=NULL, *tmp;

#ifdef __GNU_LIBRARY__
extern char *program_invocation_name;
extern char *program_invocation_short_name;

program_invocation_name=strdup(program_invocation_name);
program_invocation_short_name=strdup(program_invocation_short_name);
#endif

TitleBuffer=argv[0];
arg=argv;
while (*arg)
{
for (end=*arg; *end != '\0'; end++) ;
*arg=strdup(*arg);
arg++;
}

arg=environ;
//clearenv();  //clearenv is not portable
environ[0]=NULL;
while (*arg)
{
	for (end=*arg; *end != '\0'; end++);
	CopyEnv(*arg);
	arg++;
}

TitleLen=end-TitleBuffer;
}


void ProcessSetTitle(char **argv, char *FmtStr, ...)
{
va_list args;

		if (! TitleBuffer) ProcessTitleCaptureBuffer(argv);
		memset(TitleBuffer,0,TitleLen);

    va_start(args,FmtStr);
		vsnprintf(TitleBuffer,TitleLen,FmtStr,args);
    va_end(args);
}

