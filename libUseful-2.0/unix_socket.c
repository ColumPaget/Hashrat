#include "unix_socket.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int InitUnixServerSocket(const char *Path, int ConType)
{
  int result;
  struct sockaddr_un addr;
	int sockfd=-1, SockType;

	result=1;
	if (ConType==STREAM_TYPE_UNIX) SockType=SOCK_STREAM;
	else SockType=SOCK_DGRAM;

	memset(&addr,0,sizeof(struct sockaddr_un));
 	addr.sun_family=AF_UNIX;
 	strcpy(addr.sun_path,Path);

 	sockfd=socket(AF_UNIX, SockType, 0);

	if (sockfd > -1)
	{
	unlink(Path);
 	result=bind(sockfd,(struct sockaddr *) &addr, SUN_LEN(&addr));
	chmod(Path,0666);
  if (result != 0)
  {	
		close(sockfd);
		return(-1);
  }
  else
  {
		if (SockType==SOCK_STREAM) listen(sockfd,5);
 	}
	}

return(sockfd);
}



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
