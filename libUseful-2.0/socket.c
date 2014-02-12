#include "includes.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#endif

#include "ConnectionChain.h"

int IsAddress(char *Str)
{
int len,count;
len=StrLen(Str);
if (len <1) return(FALSE);
for (count=0; count < len; count++)
{
 if ((! isdigit(Str[count])) && (Str[count] !='.')) 
 {
	return(FALSE);
 }
}

return(TRUE);
}


int IsSockConnected(int sock)
{
struct sockaddr_in sa;
int salen, result;

if (sock==-1) return(FALSE);
salen=sizeof(sa);
result=getpeername(sock,(struct sockaddr *) &sa, &salen);
if (result==0) return(TRUE);
if (errno==ENOTCONN) return(SOCK_CONNECTING);
return(FALSE);
}


const char *GetInterfaceIP(const char *Interface)
{
 int fd, result;
 struct ifreq ifr;

 fd = socket(AF_INET, SOCK_DGRAM, 0);
 if (fd==-1) return("");

 ifr.ifr_addr.sa_family = AF_INET;
 strncpy(ifr.ifr_name, Interface, IFNAMSIZ-1);
 result=ioctl(fd, SIOCGIFADDR, &ifr);
 if (result==-1) return("");
 close(fd);

 return(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}


int InitServerSock(char *Address, int Port)
{
int sock;
struct sockaddr_in sa;
int salen;
int result;

sock=socket(AF_INET,SOCK_STREAM,0);
if (sock <0) return(-1);

result=1;
salen=sizeof(result);
setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&result,salen);

sa.sin_port=htons(Port);
sa.sin_family=AF_INET;
if (StrLen(Address) > 0) 
{
	if (! IsAddress(Address)) sa.sin_addr.s_addr=StrtoIP(GetInterfaceIP(Address));
	else sa.sin_addr.s_addr=StrtoIP(Address);
}
else sa.sin_addr.s_addr=INADDR_ANY;

salen=sizeof(struct sockaddr_in);
result=bind(sock,(struct sockaddr *) &sa, salen);

if (result==0)
{
 result=listen(sock,10);
}

if (result==0) return(sock);
else 
{
close(sock);
return(-1);
}
}



int InitUnixServerSock(char *Path)
{
int sock;
struct sockaddr_un sa;
int salen;
int result;

sock=socket(AF_UNIX,SOCK_STREAM,0);
if (sock <0) return(-1);
result=1;
salen=sizeof(result);
strcpy(sa.sun_path,Path);
sa.sun_family=AF_UNIX;
salen=sizeof(struct sockaddr_un);
result=bind(sock,(struct sockaddr *) &sa, salen);

if (result==0)
{
 result=listen(sock,10);
}

if (result==0) return(sock);
else 
{
close(sock);
return(-1);
}
}


int TCPServerSockAccept(int ServerSock, int *Addr)
{
struct sockaddr_in sa;
int salen;
int sock;

salen=sizeof(sa);
sock=accept(ServerSock,(struct sockaddr *) &sa,&salen);
if (Addr) *Addr=sa.sin_addr.s_addr;
return(sock);
}

int UnixServerSockAccept(int ServerSock)
{
struct sockaddr_un sa;
int salen;
int sock;

salen=sizeof(sa);
sock=accept(ServerSock,(struct sockaddr *) &sa,&salen);
return(sock);
}






int GetSockDetails(int sock, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort)
{
int salen, result;
struct sockaddr_in sa;

*LocalPort=0;
*RemotePort=0;
*LocalAddress=CopyStr(*LocalAddress,"");
*RemoteAddress=CopyStr(*RemoteAddress,"");

salen=sizeof(struct sockaddr_in);
result=getsockname(sock, (struct sockaddr *) &sa, &salen);

if (result==0)
{
	*LocalAddress=CopyStr(*LocalAddress,IPtoStr(sa.sin_addr.s_addr));
	*LocalPort=ntohs(sa.sin_port);

	//Set Address to be the same as control sock, as it might not be INADDR_ANY
	result=getpeername(sock, (struct sockaddr *) &sa, &salen);

	if (result==0)
	{
		*RemoteAddress=CopyStr(*RemoteAddress,IPtoStr(sa.sin_addr.s_addr));
		*RemotePort=sa.sin_port;
	}

	//We've got the local sock, so lets still call it a success
	result=0;
}

if (result==0) return(TRUE);
return(FALSE);
}






/* Users will probably only use this function if they want to reconnect   */
/* a broken connection, or reuse a socket for multiple connections, hence */
/* the name... */
int ReconnectSock(int sock, char *Host, int Port, int Flags)
{
int salen, result;
struct sockaddr_in sa;
struct hostent *hostdata;

sa.sin_family=AF_INET;
sa.sin_port=htons(Port);

if (IsAddress(Host))
{
inet_aton(Host, (struct in_addr *) &sa.sin_addr);
}
else 
{ 
   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(-1);
   }
sa.sin_addr=*(struct in_addr *) *hostdata->h_addr_list;
}

salen=sizeof(sa);
if (Flags & CONNECT_NONBLOCK) 
{
fcntl(sock,F_SETFL,O_NONBLOCK);
}

result=connect(sock,(struct sockaddr *)&sa, salen);
if (result==0) result=TRUE;

if ((result==-1) && (Flags & CONNECT_NONBLOCK) && (errno == EINPROGRESS)) result=FALSE;
return(result);
}


int ConnectToHost(char *Host, int Port,int Flags)
{
int sock, result;

sock=socket(AF_INET,SOCK_STREAM,0);
if (sock <0) return(-1);
result=ReconnectSock(sock,Host,Port,Flags);
if (result==-1)
{
close(sock);
return(-1);
}

return(sock);

}




int CheckForTerm(DownloadContext *CTX, char inchar)
{

    if (inchar == CTX->TermStr[CTX->TermPos]) 
    {
        CTX->TermPos++;
        if (CTX->TermPos >=strlen(CTX->TermStr)) 
        {
           CTX->TermPos=0;
           return(TRUE);
        }
    }
    else
    {
      if (CTX->TermPos >0) 
      {
         STREAMWriteBytes(CTX->Output, CTX->TermStr,CTX->TermPos);
         CTX->TermPos=0;
         return(CheckForTerm(CTX,inchar));
      }
    }
return(FALSE); 
}



int  ProcessIncommingBytes(DownloadContext *CTX)
{
int inchar, FoundTerm=FALSE, err;

 inchar=STREAMReadChar(CTX->Input);

 if (inchar==EOF) return(TRUE);

 FoundTerm=CheckForTerm(CTX,(char) inchar);
 while ((inchar !=EOF) && (! FoundTerm))
 {
     if (CTX->TermPos==0) STREAMWriteChar(CTX->Output, (char) inchar);
     inchar=STREAMReadChar(CTX->Input);
     err=errno;
     FoundTerm=CheckForTerm(CTX, (char) inchar);
 }
if (inchar==EOF) return(TRUE);
if (FoundTerm) return(TRUE);
return(FALSE);
}


int DownloadToTermStr2(STREAM *Connection, STREAM *SaveFile, char *TermStr)
{
DownloadContext CTX;

CTX.TermStr=CopyStr(NULL,TermStr);
CTX.Input=Connection;
CTX.Output=SaveFile;
CTX.TermPos=0;

while(ProcessIncommingBytes(&CTX) !=TRUE);

DestroyString(CTX.TermStr);
return(TRUE);
}

int DownloadToDot(STREAM *Connection, STREAM *SaveFile)
{
DownloadToTermStr2(Connection,SaveFile,"\r\n.\r\n");
}


int DownloadToTermStr(STREAM *Connection, STREAM *SaveFile, char *TermStr)
{
char *Tempstr=NULL;

Tempstr=STREAMReadLine(Tempstr,Connection);
while (Tempstr)
{
  if (strcmp(Tempstr,TermStr)==0)
{
 break;
}
  STREAMWriteLine(Tempstr,SaveFile);
  Tempstr=STREAMReadLine(Tempstr,Connection);
}
return(TRUE);
}

char *LookupHostIP(char *Host)
{
struct hostent *hostdata;

   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(NULL);
   }

//inet_ntoa shouldn't need this cast to 'char *', but it emitts a warning
//without it
return((char *) inet_ntoa(*(struct in_addr *) *hostdata->h_addr_list));
}


char *GetRemoteIP(int sock)
{
struct sockaddr_in sa;
int salen, result;

salen=sizeof(struct sockaddr_in);
result=getpeername(sock,(struct sockaddr *) &sa, &salen);
if  (result==-1)  
{
if (errno==ENOTSOCK)  return("127.0.0.1");
else return("0.0.0.0");
}

return((char *) inet_ntoa(sa.sin_addr));
}


char *IPStrToHostName(char *IPAddr)
{
struct sockaddr_in sa;
struct hostent *hostdata=NULL;

inet_aton(IPAddr,& sa.sin_addr);
hostdata=gethostbyaddr(&sa.sin_addr.s_addr,sizeof((sa.sin_addr.s_addr)),AF_INET);
if (hostdata) return(hostdata->h_name);
else return("");
}




char *IPtoStr(unsigned long Address)
{
struct sockaddr_in sa;
sa.sin_addr.s_addr=Address;
return((char *) inet_ntoa(sa.sin_addr));

}

unsigned long StrtoIP(char *Str)
{
struct sockaddr_in sa;
if (inet_aton(Str,&sa.sin_addr)) return(sa.sin_addr.s_addr);
return(0);
}


int STREAMIsConnected(STREAM *S)
{
int result=FALSE;

if (! S) return(FALSE);
result=IsSockConnected(S->in_fd);
if (result==TRUE)
{
	if (S->State & SS_CONNECTING)
	{
		S->State |= SS_CONNECTED;
		S->State &= (~SS_CONNECTING);
	}
}
if ((result==SOCK_CONNECTING) && (! (S->State & SS_CONNECTING))) result=FALSE;
return(result);
}



int STREAMDoPostConnect(STREAM *S, int Flags)
{
int result=FALSE;
char *ptr;
struct timeval tv;

if (! S) return(FALSE);
if ((S->in_fd > -1) && (S->Timeout > 0) )
{
  tv.tv_sec=S->Timeout;
  tv.tv_usec=0;
  if (FDSelect(S->in_fd, SELECT_WRITE, &tv) != SELECT_WRITE)
  {
    close(S->in_fd);
    S->in_fd=-1;
    S->out_fd=-1;
  }
  else if (! (Flags & CONNECT_NONBLOCK))  STREAMSetNonBlock(S, FALSE);
}

if (S->in_fd > -1)
{
S->Type=STREAM_TYPE_TCP;
result=TRUE;
STREAMSetFlushType(S,FLUSH_LINE,0);
//if (Flags & CONNECT_SOCKS_PROXY) result=DoSocksProxyTunnel(S);
if (Flags & CONNECT_SSL) DoSSLClientNegotiation(S, Flags);

ptr=GetRemoteIP(S->in_fd);
if (ptr) STREAMSetValue(S,"PeerIP",ptr);
}

return(result);
}



int STREAMConnectToHost(STREAM *S, char *DesiredHost, int DesiredPort,int Flags)
{
ListNode *Curr;
char *Token=NULL, *ptr;
int result=FALSE;
int HopNo=0, val=0;
ListNode *LastHop=NULL;

S->Path=FormatStr(S->Path,"tcp:%s:%d",DesiredHost,DesiredPort);
//Find the last hop, used to decide what ssh command to use
Curr=ListGetNext(S->Values);
while (Curr)
{
ptr=GetToken(Curr->Tag,":",&Token,0);
if (strcasecmp(Token,"ConnectHop")==0) LastHop=Curr;
Curr=ListGetNext(Curr);
}

STREAMSetFlushType(S,FLUSH_LINE,0);
Curr=ListGetNext(S->Values);
while (Curr)
{
ptr=GetToken(Curr->Tag,":",&Token,0);

if (strcasecmp(Token,"ConnectHop")==0) result=STREAMProcessConnectHop(S, (char *) Curr->Item,Curr==LastHop);

HopNo++;
if (! result) break;
Curr=ListGetNext(Curr);
}

//If we're not handling the connection through 'Connect hops' then
//just connect to host
if ((HopNo==0) && StrLen(DesiredHost))
{
	if (Flags & CONNECT_NONBLOCK) S->Flags |= SF_NONBLOCK;
	val=Flags;

	//STREAMDoPostConnect handles this	
	if (S->Timeout > 0) val |= CONNECT_NONBLOCK;

	S->in_fd=ConnectToHost(DesiredHost,DesiredPort,val);

	S->out_fd=S->in_fd;
	if (S->in_fd > -1) result=TRUE;
}

if (result==TRUE)
{
	if (Flags & CONNECT_NONBLOCK) 
	{
		S->State |=SS_CONNECTING;
		S->Flags |=SF_NONBLOCK;
	}
	else
	{
		S->State |=SS_CONNECTED;
		result=STREAMDoPostConnect(S, Flags);
	}
}


return(result);
}




#ifdef HAVE_LIBSSL
void STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(STREAM *S, SSL_CTX *ctx)
{
ListNode *Curr;
char *VerifyFile=NULL, *VerifyPath=NULL;

Curr=ListGetNext(LibUsefulValuesGetHead());
while (Curr)
{
  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_CERT_FILE:",14)==0))
  {
	  SSL_CTX_use_certificate_chain_file(ctx,(char *) Curr->Item);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_KEY_FILE:",13)==0))
  {
	  SSL_CTX_use_PrivateKey_file(ctx,(char *) Curr->Item,SSL_FILETYPE_PEM);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTDIR",18)==0))
  {
	  VerifyPath=CopyStr(VerifyPath,(char *) Curr->Item);
  }

	if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTFILE",19)==0))
	{
	  VerifyFile=CopyStr(VerifyFile,(char *) Curr->Item);
	}

  Curr=ListGetNext(Curr);
}



Curr=ListGetNext(S->Values);
while (Curr)
{
  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_CERT_FILE:",14)==0))
  {
	  SSL_CTX_use_certificate_chain_file(ctx,(char *) Curr->Item);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_KEY_FILE:",13)==0))
  {
	  SSL_CTX_use_PrivateKey_file(ctx,(char *) Curr->Item,SSL_FILETYPE_PEM);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTDIR",18)==0))
  {
	  VerifyPath=CopyStr(VerifyPath,(char *) Curr->Item);
  }

	if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTFILE",19)==0))
	{
	  VerifyFile=CopyStr(VerifyFile,(char *) Curr->Item);
	}


  Curr=ListGetNext(Curr);
}


SSL_CTX_load_verify_locations(ctx,VerifyFile,VerifyPath);

DestroyString(VerifyFile);
DestroyString(VerifyPath);

}
#endif


void HandleSSLError()
{
int val;

#ifdef HAVE_LIBSSL
	  val=ERR_get_error();
	  fprintf(stderr,"Failed to create SSL_CTX: %s\n",ERR_error_string(val,NULL));
	  fflush(NULL);
#endif
}


int INTERNAL_SSL_INIT()
{
static int InitDone=FALSE;

#ifdef HAVE_LIBSSL
if (InitDone) return(TRUE);
  SSL_library_init();
#ifdef USE_OPENSSL_ADD_ALL_ALGORITHMS
  OpenSSL_add_all_algorithms();
#endif
  SSL_load_error_strings();

  InitDone=TRUE;
  return(TRUE);
#endif

return(FALSE);
}


int SSLAvailable()
{
	return(INTERNAL_SSL_INIT());
}



int DoSSLClientNegotiation(STREAM *S, int Flags)
{
int result=FALSE, val;
#ifdef HAVE_LIBSSL
SSL_METHOD *Method;
SSL_CTX *ctx;
SSL *ssl;
//struct x509 *cert=NULL;
X509 *cert=NULL;

if (S)
{
INTERNAL_SSL_INIT();
//  SSL_load_ciphers();
  Method=SSLv23_client_method();
  if (! Method) Method=SSLv2_client_method();
  ctx=SSL_CTX_new(Method);
  if (! ctx) HandleSSLError();
  else
  {
  STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(S,ctx);
  ssl=SSL_new(ctx);
  SSL_set_fd(ssl,S->in_fd);
  STREAMSetItem(S,"LIBUSEFUL-SSL-CTX",ssl);
  result=SSL_connect(ssl);
  S->Flags|=SF_SSL;

	val=SSL_get_verify_result(ssl);

	switch(val)
	{
		case X509_V_OK: STREAMSetValue(S,"SSL-Certificate-Verify","OK"); break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT: STREAMSetValue(S,"SSL-Certificate-Verify","unable to get issuer"); break;
		case X509_V_ERR_UNABLE_TO_GET_CRL: STREAMSetValue(S,"SSL-Certificate-Verify","unable to get certificate CRL"); break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decrypt certificate's signature"); break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decrypt CRL's signature"); break;
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decode issuer public key"); break;
		case X509_V_ERR_CERT_SIGNATURE_FAILURE: STREAMSetValue(S,"SSL-Certificate-Verify","certificate signature invalid"); break;
		case X509_V_ERR_CRL_SIGNATURE_FAILURE: STREAMSetValue(S,"SSL-Certificate-Verify","CRL signature invalid"); break;
		case X509_V_ERR_CERT_NOT_YET_VALID: STREAMSetValue(S,"SSL-Certificate-Verify","certificate is not yet valid"); break;
		case X509_V_ERR_CERT_HAS_EXPIRED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate has expired"); break;
		case X509_V_ERR_CRL_NOT_YET_VALID: STREAMSetValue(S,"SSL-Certificate-Verify","CRL is not yet valid the CRL is not yet valid."); break;
		case X509_V_ERR_CRL_HAS_EXPIRED: STREAMSetValue(S,"SSL-Certificate-Verify","CRL has expired the CRL has expired."); break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid notBefore value"); break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid notAfter value"); break;
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CRL lastUpdate value"); break;
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CRL nextUpdate value"); break;
		case X509_V_ERR_OUT_OF_MEM: STREAMSetValue(S,"SSL-Certificate-Verify","out of memory"); break;
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT: STREAMSetValue(S,"SSL-Certificate-Verify","self signed certificate"); break;
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN: STREAMSetValue(S,"SSL-Certificate-Verify","self signed certificate in certificate chain"); break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY: STREAMSetValue(S,"SSL-Certificate-Verify","cant find root certificate in local database"); break;
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","ERROR: unable to verify the first certificate"); break;
		case X509_V_ERR_CERT_CHAIN_TOO_LONG: STREAMSetValue(S,"SSL-Certificate-Verify","certificate chain too long"); break;
		case X509_V_ERR_CERT_REVOKED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate revoked"); break;
		case X509_V_ERR_INVALID_CA: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CA certificate"); break;
		case X509_V_ERR_PATH_LENGTH_EXCEEDED: STREAMSetValue(S,"SSL-Certificate-Verify","path length constraint exceeded"); break;
		case X509_V_ERR_INVALID_PURPOSE: STREAMSetValue(S,"SSL-Certificate-Verify","unsupported certificate purpose"); break;
		case X509_V_ERR_CERT_UNTRUSTED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate not trusted"); break;
		case X509_V_ERR_CERT_REJECTED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate rejected"); break;
		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","subject issuer mismatch"); break;
		case X509_V_ERR_AKID_SKID_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","authority and subject key identifier mismatch"); break;
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","authority and issuer serial number mismatch"); break;
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN: STREAMSetValue(S,"SSL-Certificate-Verify","key usage does not include certificate signing"); break;
		case X509_V_ERR_APPLICATION_VERIFICATION: STREAMSetValue(S,"SSL-Certificate-Verify","application verification failure"); break;
	}
  }

cert=SSL_get_peer_certificate(ssl);
if (cert)
{
 STREAMSetValue(S,"SSL-Certificate-Issuer",X509_NAME_oneline( X509_get_issuer_name(cert),NULL, 0));
}
}

STREAMSetValue(S,"SSL-Cipher",STREAMQuerySSLCipher(S));

#endif
return(result);
}


int DoSSLServerNegotiation(STREAM *S, int Flags)
{
int result=FALSE;
#ifdef HAVE_LIBSSL
SSL_METHOD *Method;
SSL_CTX *ctx;
SSL *ssl;


if (S)
{
INTERNAL_SSL_INIT();
  Method=SSLv23_server_method();
  if (! Method) Method=SSLv2_server_method();
  if (Method)
  {
  ctx=SSL_CTX_new(Method);
	  
  if (ctx)
 {
  STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(S,ctx);
  ssl=SSL_new(ctx);
  SSL_set_fd(ssl,S->in_fd);
  STREAMSetItem(S,"LIBUSEFUL-SSL-CTX",ssl);
  SSL_set_verify(ssl,SSL_VERIFY_NONE,NULL);
  SSL_set_accept_state(ssl);
  result=SSL_accept(ssl);
  if (result != TRUE)
  {
	 result=SSL_get_error(ssl,result);
	 result=ERR_get_error();
	 fprintf(stderr,"error: %s\n",ERR_error_string(result,NULL));
	 result=FALSE;
  }
  S->Flags|=SF_SSL;
  }
  }
}

#endif
return(result);
}


const char *STREAMQuerySSLCipher(STREAM *S)
{
void *ptr;

if (! S) return(NULL);
ptr=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");
if (! ptr) return(NULL);

#ifdef HAVE_LIBSSL

return(SSL_get_cipher((SSL *) ptr));
#else
return(NULL);
#endif
}


int STREAMIsPeerAuth(STREAM *S)
{
void *ptr;

#ifdef HAVE_LIBSSL
ptr=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");
if (! ptr) return(FALSE);

if (SSL_get_verify_result((SSL *) ptr)==X509_V_OK)
{
  if (SSL_get_peer_certificate((SSL *) ptr) !=NULL) return(TRUE);
}
#endif
return(FALSE);
}



int OpenUDPSock(int Port)
{
	int result;
	struct sockaddr_in addr;
	int fd;

	addr.sin_family=AF_INET;
//	addr.sin_addr.s_addr=Interface;
	addr.sin_addr.s_addr=INADDR_ANY;
	addr.sin_port=htons(Port);

	fd=socket(AF_INET, SOCK_DGRAM,0);
	result=bind(fd,(struct sockaddr *) &addr, sizeof(addr));
        if (result !=0)
        {
		close(fd);
		return(-1);
        }
   return(fd);
}


int STREAMSendDgram(STREAM *S, char *Host, int Port, char *Bytes, int len)
{
struct sockaddr_in sa;
int salen;
struct hostent *hostdata;

sa.sin_port=htons(Port);
sa.sin_family=AF_INET;
inet_aton(Host,& sa.sin_addr);
salen=sizeof(sa);

if (IsAddress(Host))
{
   inet_aton(Host, (struct in_addr *) &sa.sin_addr);
}
else 
{ 
   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(-1);
   }
sa.sin_addr=*(struct in_addr *) *hostdata->h_addr_list;
}


return(sendto(S->out_fd,Bytes,len,0,(struct sockaddr *) &sa,salen));
}



STREAM *STREAMOpenUDP(int Port,int NonBlock)
{
int fd;
STREAM *Stream;

fd=OpenUDPSock(Port);
if (fd <0) return(NULL);
Stream=STREAMFromFD(fd);
Stream->Path=FormatStr(Stream->Path,"udp::%d",Port);
Stream->Type=STREAM_TYPE_UDP;
return(Stream);
}



/* This is a simple function to decide if a string is an IP address as   */
/* opposed to a host/domain name.                                        */

int IsIPAddress(char *Str)
{
int len,count;
len=strlen(Str);
if (len <1) return(FALSE);
for (count=0; count < len; count++)
   if ((! isdigit(Str[count])) && (Str[count] !='.')) return(FALSE);
 return(TRUE);
}


