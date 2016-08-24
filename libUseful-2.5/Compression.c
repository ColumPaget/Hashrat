#include "DataProcessing.h"

int CompressBytes(char **Out, char *Alg, char *In, int Len, int Level)
{
TProcessingModule *Mod=NULL;
char *Tempstr=NULL;
int result, val;

Tempstr=FormatStr(Tempstr,"CompressionLevel=%d",Level);
Mod=StandardDataProcessorCreate("compression",Alg,Tempstr);
if (! Mod) return(-1);

val=Len *2;
*Out=SetStrLen(*Out,val);
result=Mod->Write(Mod,In,Len,Out,&Len,TRUE);

DestroyString(Tempstr);
DataProcessorDestroy(Mod);

return(result);
}
