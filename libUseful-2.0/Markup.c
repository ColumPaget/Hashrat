#include "Markup.h"

char *XMLGetTag(char *Input, char **Namespace, char **TagType, char **TagData)
{
char *ptr, *tptr;
int len=0, InTag=FALSE, TagHasName=FALSE;

if (! Input) return(NULL);
if (*Input=='\0') return(NULL);
ptr=Input;

//This ensures we are never dealing with nulls
if (! *TagType) *TagType=CopyStr(*TagType,"");
if (! *TagData) *TagData=CopyStr(*TagData,"");

if (*ptr=='<') 
{
	ptr++;
	while (isspace(*ptr)) ptr++;

	len=0;
	InTag=TRUE;
	TagHasName=TRUE;

	//if we start with a slash tag, then add that to the tag name
	if (*ptr=='/') 
	{
		*TagType=AddCharToBuffer(*TagType,len,*ptr);
		len++;
		ptr++;
	}

	while (InTag)
	{
		switch (*ptr)
		{
		//These all cause us to end. NONE OF THEM REQUIRE ptr++
		//ptr++ will happen when we read tag data
		case '>':
		case '\0':
		case ' ':
		case '	':
		case '\n':
			InTag=FALSE;
		break;

		//If a namespace return value is supplied, break the name up into namespace and
		//tag. Otherwise don't
		case ':':
			if (Namespace)
			{
			tptr=*TagType;
			if (*tptr=='/')
			{
			  tptr++;
			  len=1;
			}
			else len=0;
			*Namespace=CopyStr(*Namespace,tptr);
			}
			else
			{
			*TagType=AddCharToBuffer(*TagType,len,*ptr);
			len++;
			}
			ptr++;
			
		break;

		

		case '\r':
			ptr++;
		break;

		default:
			*TagType=AddCharToBuffer(*TagType,len,*ptr);
			len++;
			ptr++;
		break;
		}
		
	}
}

//End of Parse TagName. Strip any '/'
tptr=*TagType;
if ((len > 0) && (tptr[len-1]=='/')) tptr[len-1]='\0'; 
tptr[len]='\0'; 

while (isspace(*ptr)) ptr++;


len=0;
InTag=TRUE;
while (InTag) 
{
	switch (*ptr)
	{
		//End of tag, skip '>' and fall through
		case '>':
		ptr++;

		//Start of next tag or end of text
		case '<':
		case '\0':
		 InTag=FALSE;
		break;

		//Quotes!
		case '\'':
		case '\"':

		//Somewhat ugly code. If we're dealing with an actual tag, then TagHasName
		//will be set. This means we're dealing with data within a tag and quotes mean something. 
		//Otherwise we are dealing with text outside of tags and we just add the char to
		//TagData and continue
		if (TagHasName)
		{
		tptr=ptr;
		while (*tptr != *ptr)
		{
			*TagData=AddCharToBuffer(*TagData,len,*ptr);
			len++;
			ptr++;
		}
		}
		*TagData=AddCharToBuffer(*TagData,len,*ptr);
		len++;
		ptr++;
		break;

		default:
		*TagData=AddCharToBuffer(*TagData,len,*ptr);
		len++;
		ptr++;
		break;
	}

}

//End of Parse TagData. Strip any '/'
tptr=*TagData;
if ((len > 0) && (tptr[len-1]=='/')) tptr[len-1]='\0'; 
tptr[len]='\0'; 

strlwr(*TagType);
while (isspace(*ptr)) ptr++;

return(ptr);
}




char *HtmlGetTag(char *Input, char **TagType, char **TagData)
{
char *ptr;
int len=0;

if (! Input) return(NULL);
if (*Input=='\0') return(NULL);

return(XMLGetTag(Input, NULL, TagType, TagData));
}



char *HtmlDeQuote(char *RetStr, char *Data)
{
char *Output=NULL, *Token=NULL, *ptr;
int len=0;

Output=CopyStr(RetStr,Output);
ptr=Data;

while (ptr && (*ptr != '\0'))
{
	if (*ptr=='&')
	{
		ptr++;
		ptr=GetToken(ptr,";",&Token,0);

		if (*Token=='#')
		{
			Output=AddCharToBuffer(Output,len,strtol(Token+1,NULL,16));
			len++;
		}
		else if (strcmp(Token,"amp")==0)
		{
			Output=AddCharToBuffer(Output,len,'&');
			len++;
		}
		else if (strcmp(Token,"lt")==0)
		{
			Output=AddCharToBuffer(Output,len,'<');
			len++;
		}
		else if (strcmp(Token,"gt")==0)
		{
			Output=AddCharToBuffer(Output,len,'>');
			len++;
		}
		else if (strcmp(Token,"quot")==0)
		{
			Output=AddCharToBuffer(Output,len,'"');
			len++;
		}
		else if (strcmp(Token,"apos")==0)
		{
			Output=AddCharToBuffer(Output,len,'\'');
			len++;
		}
		else if (strcmp(Token,"tilde")==0)
		{
			Output=AddCharToBuffer(Output,len,'~');
			len++;
		}
		else if (strcmp(Token,"circ")==0)
		{
			Output=AddCharToBuffer(Output,len,'^');
			len++;
		}




	}
	else
	{
		Output=AddCharToBuffer(Output,len,*ptr);
		len++;
		ptr++;
	}
}

DestroyString(Token);

return(Output);
}


