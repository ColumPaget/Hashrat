#include "libUseful.h"


main(int argc, char *argv[])
{
STREAM *S, *Out;
ListNode *Vars;
char *Path=NULL, *Tempstr=NULL, *ptr;
int bytes_read, bytes_total, val, result;


S=STREAMOpenFile(argv[1],O_RDONLY);

Out=STREAMFromFD(1);
Vars=ListCreate();
while (TarReadHeader(S, Vars))
{
  Path=CopyStr(Path,GetVar(Vars,"Path"));
printf("PATH: %s %s %s\n",Path,GetVar(Vars,"Size"),GetVar(Vars,"Type"));
  if (StrLen(Path))
  {
    ptr=GetVar(Vars,"Type");
    if (ptr && (strcmp(ptr,"file")==0))
    {
      bytes_read=0;
      bytes_total=atoi(GetVar(Vars,"Size"));
      Tempstr=SetStrLen(Tempstr,BUFSIZ);
      while (bytes_read < bytes_total)
      {
        val=bytes_total - bytes_read;
        if (val > BUFSIZ) val=BUFSIZ;
				if ((val % 512)==0) result=val;
				else result=((val / 512) + 1) * 512;
        result=STREAMReadBytes(S,Tempstr,result);
				if (result > val) result=val;
				printf("READ: %d\n",val); fflush(NULL);
        if (Out) STREAMWriteBytes(Out,Tempstr,result);
        bytes_read+=result;
      }
    }
  }
  ListClear(Vars,DestroyString);
}

ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Path);
}
