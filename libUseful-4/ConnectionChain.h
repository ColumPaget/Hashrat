/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_CONNECTCHAIN
#define LIBUSEFUL_CONNECTCHAIN

#include "includes.h"

/*

-------------------------------------- PLEASE NOTE ------------------------------------
 Separation character has had to be changed from ',' to '|' because commas can occur validly in URLS
---------------------------------------------------------------------------------------


The only function a user should be concerned with here is 'SetGlobalConnectionChain'. This allows setting
a PIPE-seperated list of 'connection hops' (proxy servers) that are to be used for all connections in
the program. e.g.


if (! SetGlobalConnectionChain("socks5:127.0.0.1|https:89.40.24.6:8080|socks5:16.22.22.1:1080"))
{
printf("ERROR! Failed to SetGlobalConnectionChain\n");
}
else
{
	S=STREAMOpen("tcp:mail.test.com:25","");
	if (S)
	{
		Tempstr=STREAMReadLine(Tempstr, S);
		printf("READ: %s\n",Tempstr);
		STREMClose(S);
	}

	S=HTTPGet("http://www.test.com/index.html","","");
	if (S)
	{
	Tempstr=STREAMReadDocument(Tempstr, S);
	printf("READ: %s\n",Tempstr);
	STREAMClose(S);
	}
}

Will build a connection chain through the listed socks and http proxies every for connections to
mail.test.com and www.test.com.

Note, to use this feature you must make tcp connections using either the STREAMOpen or STREAMConnect
function. Lower-level connection functions do not use connection chains. All the HTTP functions in
Http.h also use connection chains.


If you only want to use a proxy chain on a particular connection instead of setting a global connection chain,
then open the connection with 'STREAMOpen' or 'STREAMConnect' and list all the hops before the connection URL. e.g.

S=STREAMOpen("socks5:user:password@127.0.0.1|https:89.40.24.6:1080|tcp:mail.test.com:25");

or

S=STREAMCreate();
S=STREAMConnect(S,"socks5:user:password@127.0.0.1|https:89.40.24.6:1080|tcp:mail.test.com:25", 0);

these will first connect to a socks5 proxy at 127.0.0.1, then an https CONNECT proxy at 89.40.24.6, and finally
to the destiation host 'mail.test.com'.

the URL scheme for all proxy types is user:password@host:port. user, password and port are optional,
most proxy types have default ports and public proxies have no authentication.

Known proxy-server types are: tcp, ssh, sshtunnel, socks4, socks5, https

tcp:  this is just a host/port that we connect to and are instantly bounced through to the next hop.
For example, ssh connection forwarding using -L creates such a port.

ssh:  this launches an ssh connection using the -W host:port proxy method. SSH proxy connections can only
appear as the FIRST hop in a chain. Command-line ssh must be available on your machine for this to work,
as libUseful spawns off an ssh process to achieve this.

SSH supports a named connection configuration method where user+password+keyfile+host+port combinations
can be specified in the ~/.ssh/config file. In this situation the URL is replaced with the name of the
entry in the config file.

sshtunnel: this launches an ssh connection using the -L localport:remotehost:remoteport method. This ssh
tunnel will be available to all users on the local machine, so the previous ssh -W method is generally
preferred. Everything else about this connection is the same as for 'ssh' connection hops.

sshproxy: this launches an ssh connection using the -D 127.0.0.1:localport method. This creates a socks5
proxy on a randomly generated local port on 127.0.0.1 (ip4 local interface). 

socks4:  connect via a SOCKS version 4 proxy. These can usually be concatenated into long connection chains.
SOCKS version 4 requires DNS lookups to happen locally, which means there's some leakage of information
about which hosts you're connecting to via socks 4, as libUseful has to look the hostnames up from servers
on the local side of the connection.

socks5:  connect via a SOCKS version 5 proxy. These can usually be concatenated into long connection chains.
SOCKS version 5 supports remote DNS lookups, solving the 'information leakage' issues of version 4.

https:   connect via an HTTPS CONNECT proxy. These can usually be concatenated into long connection chains.
Please note that although HTTPS servers encrypt most communications, they do not encrypt CONNECT style
proxy connections. Such a connection is just a pure tcp forwarding link.


Please note that, due to lack of access to IPv6 networks, the Connection chains feature has not been much
tested with IPv6.

*/


#ifdef __cplusplus
extern "C" {
#endif

int SetGlobalConnectionChain(const char *Chain);
int STREAMProcessConnectHops(STREAM *S, const char *Value);

#ifdef __cplusplus
}
#endif

#endif



