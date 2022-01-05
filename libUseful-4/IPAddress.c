#include "IPAddress.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <unistd.h>




int IsIP4Address(const char *Str)
{
    const char *ptr;
    int dot_count=0;
    int AllowDot=FALSE;

    if (! StrValid(Str)) return(FALSE);

    for (ptr=Str; *ptr != '\0'; ptr++)
    {
        if (*ptr == '.')
        {
            if (! AllowDot) return(FALSE);
            dot_count++;
            AllowDot=FALSE;
        }
        else
        {
            if (! isdigit(*ptr)) return(FALSE);
            AllowDot=TRUE;
        }
    }

    if (dot_count != 3) return(FALSE);

    return(TRUE);
}


int IsIP6Address(const char *Str)
{
    const char *ptr;
    const char *IP6CHARS="0123456789abcdefABCDEF:%";
    int result=FALSE;

    if (! StrValid(Str)) return(FALSE);

    ptr=Str;
    if (*ptr == '[') ptr++;

    for (; *ptr != '\0'; ptr++)
    {
//dont check after a '%', as this can be a netdev postfix
        if (*ptr=='%') break;
        if (*ptr==']') break;

//require at least one ':'
        if (*ptr==':') result=TRUE;

        if (! strchr(IP6CHARS,*ptr)) return(FALSE);
    }

    return(result);
}




/* This is a simple function to decide if a string is an IP address as   */
/* opposed to a host/domain name.                                        */

int IsIPAddress(const char *Str)
{
    if (IsIP4Address(Str) || IsIP6Address(Str)) return(TRUE);
    return(FALSE);
}


const char *LookupHostIP(const char *Host)
{
    struct hostent *hostdata;

    if (! Host) return("");

    hostdata=gethostbyname(Host);
    if (!hostdata)
    {
        return(NULL);
    }

//inet_ntoa shouldn't need this cast to 'char *', but it emits a warning
//without it
    return(inet_ntoa(*(struct in_addr *) *hostdata->h_addr_list));
}


ListNode *LookupHostIPList(const char *Host)
{
    struct hostent *hostdata;
    ListNode *List;
    char **ptr;

    hostdata=gethostbyname(Host);
    if (!hostdata)
    {
        return(NULL);
    }

    List=ListCreate();
//inet_ntoa shouldn't need this cast to 'char *', but it emitts a warning
//without it
    for (ptr=hostdata->h_addr_list; *ptr !=NULL; ptr++)
    {
        ListAddItem(List, CopyStr(NULL,  (char *) inet_ntoa(*(struct in_addr *) *ptr)));
    }

    return(List);
}



const char *IPStrToHostName(const char *IPAddr)
{
    struct sockaddr_in sa;
    struct hostent *hostdata=NULL;

    inet_aton(IPAddr,& sa.sin_addr);
    hostdata=gethostbyaddr(&sa.sin_addr.s_addr,sizeof((sa.sin_addr.s_addr)),AF_INET);
    if (hostdata) return(hostdata->h_name);
    else return("");
}


const char *IPtoStr(unsigned long Address)
{
    struct sockaddr_in sa;
    sa.sin_addr.s_addr=Address;
    return(inet_ntoa(sa.sin_addr));

}

unsigned long StrtoIP(const char *Str)
{
    struct sockaddr_in sa;
    if (inet_aton(Str,&sa.sin_addr)) return(sa.sin_addr.s_addr);
    return(0);
}


