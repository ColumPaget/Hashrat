#include "unix_socket.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


int OpenUnixSocket(const char *Path, int Type)
{
int sock;
struct sockaddr_un sa;
int val;

if (Type==STREAM_TYPE_UNIX) val=SOCK_STREAM;
else val=SOCK_DGRAM;

sock=socket(AF_UNIX, val, FALSE);
if (sock==-1) return(-1);

memset(&sa,0,sizeof(struct sockaddr_un));
sa.sun_family=AF_UNIX;
strcpy(sa.sun_path,Path);
val=sizeof(sa);
if (connect(sock,(struct sockaddr *) &sa,val)==0) return(sock);

close(sock);
return(-1);
}



int STREAMConnectUnixSocket(STREAM *S, const char *Path, int ConType)
{

	S->in_fd=OpenUnixSocket(Path, ConType);
	if (S->in_fd==-1) return(FALSE);
	S->out_fd=S->in_fd;
	S->Type=ConType;

	return(TRUE);
}
