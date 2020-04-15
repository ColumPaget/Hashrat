#include "../libUseful.h"

/*
Pass this program a URL of a host to connect to e.g.

  ssl://www.google.com:443
  ssl://imap.myhost.com:993
  https://www.google.com

if the host supports SSL connections on that port, then this program
will print out certificate and encryption details

*/

main(int argc, char *argv[])
{
STREAM *S;
char *URL=NULL, *Config=NULL, *Tempstr=NULL;
const char *ptr;

URL=CopyStr(URL, argv[1]);
S=STREAMOpen(URL, Config);
if (S)
{
	ptr=STREAMGetValue(S,"SSL:Cipher");
	if (StrValid(ptr))
	{
		printf("Encrypted Connection: %s bit %s\n",STREAMGetValue(S, "SSL:CipherBits"), ptr);
		printf("Certificate Verified: %s\n",STREAMGetValue(S, "SSL:CertificateVerify"));
		printf("Certificate Issuer  : %s\n",STREAMGetValue(S, "SSL:CertificateIssuer"));
		printf("Certificate Subject : %s\n",STREAMGetValue(S, "SSL:CertificateSubject"));
		printf("Certificate NotBefor: %s\n",STREAMGetValue(S, "SSL:CertificateNotBefore"));
		printf("Certificate NotAfter: %s\n",STREAMGetValue(S, "SSL:CertificateNotAfter"));
	}
}
else printf("Connection not encrypted\n");

STREAMClose(S);
}

